#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"

bool AICPlayerController::CanSubmitCreation() const
{
    IC_TRACE_FUNCTION();
    const UICSaveSubsystem* Saves = GetSaves();
    return Menu == TEXT("Create") && !bCreated && !bItemTransactionInProgress && Saves && !Saves->bSaving;
}

bool AICPlayerController::ValidateCreationRequest(FString& OutReason) const
{
    IC_TRACE_FUNCTION();
    OutReason.Empty();
    if (!GetWorldState() || !GetSaves() || !GetTime() || !GetInventory() || !GetPawn())
        OutReason = TEXT("플레이어 또는 저장 시스템이 준비되지 않았습니다.");
    else if (bCreated)
        OutReason = TEXT("이미 생성된 캐릭터입니다.");
    else if (GetSaves()->bSaving)
        OutReason = TEXT("저장 중입니다. 완료될 때까지 기다려 주세요.");
    else if (bItemTransactionInProgress)
        OutReason = TEXT("아이템 효과 처리 완료 후 다시 시도하세요.");
    else if (Job < 0 || Job > 3 || Preset < 0 || Preset > 1)
        OutReason = TEXT("성별과 시작 무기를 다시 선택하세요.");
    else
    {
        const FString Clean = PlayerName.TrimStartAndEnd();
        bool bInvalid = Clean.IsEmpty() || Clean.Len() > 12;
        for (TCHAR Ch : PlayerName)
            if (Ch < TEXT(' ') || Ch == 0x2028 || Ch == 0x2029) bInvalid = true;
        if (bInvalid) OutReason = TEXT("이름은 공백을 제외한 1~12자이며 줄바꿈을 사용할 수 없습니다.");
    }
    return OutReason.IsEmpty();
}

void AICPlayerController::ReportCreationFailure(const FString& Reason)
{
    IC_TRACE_FUNCTION();
    Show(Reason);
    if(GetGameInstance())if(auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>())if(Flow->bPendingNewGame)Flow->Fail(Reason);
    UE_LOG(LogICCore, Warning, TEXT("[Player.Create.Rejected] Preset=%d Job=%d Rule=%s Reason=%s"),Preset,Job,*NewGameDefinitionID.ToString(),*Reason);
}

bool AICPlayerController::DebugCreatePlayer()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    if (bCreated)
    {
        ReportCreationFailure(TEXT("이미 생성된 캐릭터입니다."));
        return false;
    }
    if (PlayerName.TrimStartAndEnd().IsEmpty())
    {
        PlayerName = TEXT("NPC_Test");
    }
    CreatePlayer();
    return bCreated || (bInitialSavePending && GetSaves() && GetSaves()->bSaving);
#else
    return false;
#endif
}

void AICPlayerController::CreatePlayer()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICCore, Log, TEXT("[Player.Create.Request] Preset=%d Job=%d NameLength=%d Rule=%s RetrySave=%d"),Preset,Job,PlayerName.Len(),*NewGameDefinitionID.ToString(),bInitialSavePending);
    FString Reason;
    if (!ValidateCreationRequest(Reason)) { ReportCreationFailure(Reason); return; }
    UICWorldStateSubsystem* State = GetWorldState();
    UICSaveSubsystem* Saves = GetSaves();
    UICTimeSubsystem* Time = GetTime();
    UICInventoryComponent* Inventory = GetInventory();
    if (!bInitialSavePending)
    {
        FICDefinitionResult Lookup;
        const auto* Rules=Inventory->GetDefinitions()?Inventory->GetDefinitions()->Find<FICNewGameRow>(NewGameDefinitionID,Lookup):nullptr;
        if(!Rules || !Rules->StartingWeapons.IsValidIndex(Job) || !Rules->StartingSkills.IsValidIndex(Job)
            || Rules->StartingQuickSlots.Num()!=4 || Rules->InitialGold<0 || !FMath::IsFinite(Rules->InitialGameMinute) || Rules->InitialGameMinute<0)
        {ReportCreationFailure(TEXT("새 게임 정의가 누락되었거나 초기 지급 설정이 올바르지 않습니다."));return;}

        auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
        const auto* Choice=Flow->bPendingNewGame?Flow->FindOption(3,Flow->Answers.Music):nullptr;
        const auto* Story=Flow->bPendingNewGame?Flow->FindOption(2,Flow->Answers.Story):nullptr;
        if(Flow->bPendingNewGame&&(!Flow->Answers.IsComplete()||!Choice||!Story))
        {ReportCreationFailure(TEXT("질문지 답변이나 보상 데이터가 올바르지 않습니다."));return;}
        const FName StartingWeapon=Choice?Choice->WeaponItemID:Rules->StartingWeapons[Job];
        const FName StartingSkill=Choice?Choice->SkillID:Rules->StartingSkills[Job];
        if(!Inventory->GetDefinitions()->Find<FICSkillRow>(StartingSkill,Lookup))
        {ReportCreationFailure(TEXT("시작 기술 정의를 찾을 수 없습니다."));return;}
        UICInventoryComponent* Staging = NewObject<UICInventoryComponent>(this);
        Staging->BeginStaging(Inventory->GetDefinitions(),Job);
        if (!Staging->AddItem(StartingWeapon, 1, false) || !Staging->Equip(0))
        { ReportCreationFailure(TEXT("초기 무기 지급/장착에 실패했습니다. 아이템 정의를 확인하세요.")); return; }
        for (const auto& Item : Rules->StartingItems)
            if (!Staging->AddItem(Item.ItemID, Item.Count, false))
            { ReportCreationFailure(FString::Printf(TEXT("초기 아이템 %s 지급에 실패했습니다."), *Item.ItemID.ToString())); return; }
        FICInventoryState Data = Staging->GetSnapshot();
        Data.QuickSlots = Rules->StartingQuickSlots;
        if (!Inventory->ValidateSnapshot(Data))
        { ReportCreationFailure(TEXT("초기 인벤토리 검증에 실패했습니다.")); return; }

        FICWorldState Initial;
        Initial.bPlayerCreated = true;
        Initial.PlayerName = PlayerName.TrimStartAndEnd();
        Initial.Preset = Preset;
        Initial.Job = Job;
        Initial.FirstSkill = StartingSkill;
        if(Choice)
        {
            Initial.CreationAnswers=Flow->Answers;
            Initial.Flags.Add(FName(*FString::Printf(TEXT("Creation.Story.%d"),Flow->Answers.Story)),true);
            if(!Choice->NPCID.IsNone())
            {
                FICDefinitionResult Result;
                const auto* NPC=Inventory->GetDefinitions()->Find<FICNPCRow>(Choice->NPCID,Result);
                const auto* Combat=NPC?Inventory->GetDefinitions()->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,Result):nullptr;
                if(!NPC||!Combat||Combat->UniqueSkillID!=StartingSkill||!State->EnsureCharacter(Choice->NPCID,EICCharacterKind::Recruitable))
                {ReportCreationFailure(TEXT("시작 NPC와 고유 기술 연결을 확인하세요."));return;}
                auto NPCState=State->GetSnapshot(Choice->NPCID);NPCState.Affinity=FMath::Clamp(NPCState.Affinity+300,0,500);
                Initial.Characters.Add(Choice->NPCID,NPCState);
            }
            if(Story&&!Story->RecipeID.IsNone())
            {
                if(!Inventory->GetDefinitions()->Find<FICRecipeRow>(Story->RecipeID,Lookup)){ReportCreationFailure(TEXT("시작 레시피 정의를 찾을 수 없습니다."));return;}
                Initial.UnlockedRecipeIDs.Add(Story->RecipeID);
            }
            if(Story&&!Story->CodexID.IsNone())
            {
                if(!Inventory->GetDefinitions()->Find<FICCodexRow>(Story->CodexID,Lookup)){ReportCreationFailure(TEXT("시작 인물 정보 정의를 찾을 수 없습니다."));return;}
                Initial.CodexEntryIDs.Add(Story->CodexID);
            }
        }
        Initial.Gold = Rules->InitialGold;
        Initial.PlayerPosition = GetPawn()->GetActorLocation();
        Initial.PlayerRotation = GetControlRotation();
        Initial.Inventory = MoveTemp(Data);
        FICCharacterState InitialCharacterState;
        InitialCharacterState.CharacterId = UICWorldStateSubsystem::PlayerId();
        InitialCharacterState.Kind = EICCharacterKind::Player;
        InitialCharacterState.UnlockedSkillIDs.Add(StartingSkill);
        InitialCharacterState.MainStats.STR=Rules->InitialStats.STR;InitialCharacterState.MainStats.DEX=Rules->InitialStats.DEX;
        InitialCharacterState.MainStats.INT=Rules->InitialStats.INT;InitialCharacterState.MainStats.CON=Rules->InitialStats.CON;
        InitialCharacterState.MainStats.CHA=Rules->InitialStats.CHA;InitialCharacterState.MainStats.LUK=Rules->InitialStats.LUK;
        InitialCharacterState.MaxHP=Rules->InitialMaxHP;InitialCharacterState.MaxMP=Rules->InitialMaxMP;InitialCharacterState.MaxSP=Rules->InitialMaxSP;
        InitialCharacterState.HP=Rules->bStartWithFullResources?Rules->InitialMaxHP:Rules->InitialHP;
        InitialCharacterState.MP=Rules->bStartWithFullResources?Rules->InitialMaxMP:Rules->InitialMP;
        InitialCharacterState.SP=Rules->bStartWithFullResources?Rules->InitialMaxSP:Rules->InitialSP;
        InitialCharacterState.LastSimGameMinute=Rules->InitialGameMinute;
        for(FName ID:Rules->InitiallyUnlockedRecipeIDs)Initial.UnlockedRecipeIDs.Add(ID);
        Initial.Characters.Add(InitialCharacterState.CharacterId, InitialCharacterState);
        if (!State->CanRestoreState(Initial))
        {
            FString Failure;UICWorldStateSubsystem::ValidateStateWithReason(Initial,Failure);
            if(auto* Avatar=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=Avatar->GetICAbilitySystem())ASC->CanRestore(InitialCharacterState,Failure);
            ReportCreationFailure(TEXT("캐릭터 초기 상태 검증에 실패했습니다. ")+Failure);return;
        }
        bInitialSavePending = true;
        if (!State->RestoreState(Initial))
        { bInitialSavePending = false; ReportCreationFailure(TEXT("캐릭터 초기 상태 적용에 실패했습니다.")); return; }
        FICClockState InitialClock;InitialClock.GameMinute=Rules->InitialGameMinute;
        if(!Time->RestoreClock(InitialClock)){ReportCreationFailure(TEXT("초기 시간 설정에 실패했습니다."));return;}
        Saves->ResetSession();
    }

    Saves->SetAutoSaveSuspended(true);
    SyncSaveSafety();
    Show(TEXT("캐릭터 생성 및 최초 저장 중…"));
    if (!Saves->SaveSlot(0))
    {
        ReportCreationFailure(Saves->GetLastMessage() + TEXT(" 게임 시작 버튼으로 저장을 재시도할 수 있습니다."));
        UpdateGamePhase();
    }
}

void AICPlayerController::RetryNewGame()
{
    if(!IsMainMenuLevel()&&!bCreated&&GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->bPendingNewGame)CreatePlayer();
}

bool AICPlayerController::SavePlayer()
{
    IC_TRACE_FUNCTION();
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||!bCreated||SelectedSaveSlot<1||SelectedSaveSlot>4){Show(TEXT("수동 저장 슬롯 1~4를 선택하세요."));return false;}
    UE_LOG(LogICSave, Verbose, TEXT("[Player.Save.Request] Slot=%d"),SelectedSaveSlot);
    const bool Accepted=Saves->SaveSlot(SelectedSaveSlot);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
bool AICPlayerController::LoadPlayer()
{
    IC_TRACE_FUNCTION();
    SyncSaveSafety();auto* Saves=GetSaves();if(!Saves)return false;
    const bool WasPending=bInitialSavePending;bInitialSavePending=false;
    const bool Loaded=Saves->LoadSlot(SelectedLoadSlot);
    if(Loaded)RefreshSaveSlots();else bInitialSavePending=WasPending;
    Show(Saves->GetLastMessage());return Loaded;
}

bool AICPlayerController::AutoSavePlayer()
{
    IC_TRACE_FUNCTION();
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||(!bCreated&&!bInitialSavePending))return false;
    const bool Accepted=Saves->SaveSlot(0);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
