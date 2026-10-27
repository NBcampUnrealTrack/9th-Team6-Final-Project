#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICText.h"
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
        OutReason = ICText::Get(TEXT("Native_e0ede76ffc83")).ToString();
    else if (bCreated)
        OutReason = ICText::Get(TEXT("Native_bcc3507d6632")).ToString();
    else if (GetSaves()->bSaving)
        OutReason = ICText::Get(TEXT("Native_d3600620c89f")).ToString();
    else if (bItemTransactionInProgress)
        OutReason = ICText::Get(TEXT("Native_64c7fe1ad82c")).ToString();
    else if (Job < 0 || Job > 3 || Preset < 0 || Preset > 1)
        OutReason = ICText::Get(TEXT("Native_5507152d3f55")).ToString();
    else
    {
        const FString Clean = PlayerName.TrimStartAndEnd();
        bool bInvalid = Clean.IsEmpty() || Clean.Len() > 12;
        for (TCHAR Ch : PlayerName)
            if (Ch < TEXT(' ') || Ch == 0x2028 || Ch == 0x2029) bInvalid = true;
        if (bInvalid) OutReason = ICText::Get(TEXT("Native_6ed5f8122e6b")).ToString();
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
        ReportCreationFailure(*ICText::Get(TEXT("Native_bcc3507d6632")).ToString());
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
        {ReportCreationFailure(*ICText::Get(TEXT("Native_441ae20e4d47")).ToString());return;}

        auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
        const auto* Choice=Flow->bPendingNewGame?Flow->FindOption(3,Flow->Answers.Music):nullptr;
        const auto* Story=Flow->bPendingNewGame?Flow->FindOption(2,Flow->Answers.Story):nullptr;
        if(Flow->bPendingNewGame&&(!Flow->Answers.IsComplete()||!Choice||!Story))
        {ReportCreationFailure(*ICText::Get(TEXT("Native_914e4fd91023")).ToString());return;}
        const FName StartingWeapon=Choice?Choice->WeaponItemID:Rules->StartingWeapons[Job];
        const FName StartingSkill=Choice?Choice->SkillID:Rules->StartingSkills[Job];
        if(!Inventory->GetDefinitions()->Find<FICSkillRow>(StartingSkill,Lookup))
        {ReportCreationFailure(*ICText::Get(TEXT("Native_529dd1461bfe")).ToString());return;}
        UICInventoryComponent* Staging = NewObject<UICInventoryComponent>(this);
        Staging->BeginStaging(Inventory->GetDefinitions(),Job);
        if (!Staging->AddItem(StartingWeapon, 1, false) || !Staging->Equip(0))
        { ReportCreationFailure(*ICText::Get(TEXT("Native_1415298995d1")).ToString()); return; }
        for (const auto& Item : Rules->StartingItems)
            if (!Staging->AddItem(Item.ItemID, Item.Count, false))
            { ReportCreationFailure(ICText::Format(TEXT("Native_076d416ae618"), *Item.ItemID.ToString())); return; }
        FICInventoryState Data = Staging->GetSnapshot();
        Data.bWeaponSheathed = true;
        Data.QuickSlots = Rules->StartingQuickSlots;
        if (!Inventory->ValidateSnapshot(Data))
        { ReportCreationFailure(*ICText::Get(TEXT("Native_b322167a84ef")).ToString()); return; }

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
                {ReportCreationFailure(*ICText::Get(TEXT("Native_0e2e404d5095")).ToString());return;}
                auto NPCState=State->GetSnapshot(Choice->NPCID);NPCState.Affinity=FMath::Clamp(NPCState.Affinity+300,0,500);
                Initial.Characters.Add(Choice->NPCID,NPCState);
            }
            if(Story&&!Story->RecipeID.IsNone())
            {
                if(!Inventory->GetDefinitions()->Find<FICRecipeRow>(Story->RecipeID,Lookup)){ReportCreationFailure(*ICText::Get(TEXT("Native_cd3eb7a6d18a")).ToString());return;}
                Initial.UnlockedRecipeIDs.Add(Story->RecipeID);
            }
            if(Story&&!Story->CodexID.IsNone())
            {
                if(!Inventory->GetDefinitions()->Find<FICCodexRow>(Story->CodexID,Lookup)){ReportCreationFailure(*ICText::Get(TEXT("Native_77fc3db18545")).ToString());return;}
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
            ReportCreationFailure(*ICText::Get(TEXT("Native_7c1694148ff4")).ToString()+Failure);return;
        }
        bInitialSavePending = true;
        if (!State->RestoreState(Initial))
        { bInitialSavePending = false; ReportCreationFailure(*ICText::Get(TEXT("Native_9bc7a2b31165")).ToString()); return; }
        FICClockState InitialClock;InitialClock.GameMinute=Rules->InitialGameMinute;
        if(!Time->RestoreClock(InitialClock)){ReportCreationFailure(*ICText::Get(TEXT("Native_41dccdc5dc71")).ToString());return;}
        Saves->ResetSession();
    }

    Saves->SetAutoSaveSuspended(true);
    SyncSaveSafety();
    ShowLocalized(ICText::Get(TEXT("Native_b081bff2bbb6")));
    if (!Saves->SaveSlot(0))
    {
        ReportCreationFailure(ICText::FormatText(TEXT("Save_RetryNewGame"),Saves->GetLastMessageText()).ToString());
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
    if(!Saves||!bCreated||SelectedSaveSlot<1||SelectedSaveSlot>4){ShowLocalized(ICText::Get(TEXT("Native_a443b14e9863")));return false;}
    UE_LOG(LogICSave, Verbose, TEXT("[Player.Save.Request] Slot=%d"),SelectedSaveSlot);
    const bool Accepted=Saves->SaveSlot(SelectedSaveSlot);
    if(!Accepted)ShowLocalized(Saves->GetLastMessageText());
    return Accepted;
}
bool AICPlayerController::LoadPlayer()
{
    IC_TRACE_FUNCTION();
    SyncSaveSafety();auto* Saves=GetSaves();if(!Saves)return false;
    const bool WasPending=bInitialSavePending;bInitialSavePending=false;
    const bool Loaded=Saves->LoadSlot(SelectedLoadSlot);
    if(Loaded)RefreshSaveSlots();else bInitialSavePending=WasPending;
    ShowLocalized(Saves->GetLastMessageText());return Loaded;
}

bool AICPlayerController::AutoSavePlayer()
{
    IC_TRACE_FUNCTION();
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||(!bCreated&&!bInitialSavePending))return false;
    const bool Accepted=Saves->SaveSlot(0);
    if(!Accepted)ShowLocalized(Saves->GetLastMessageText());
    return Accepted;
}
