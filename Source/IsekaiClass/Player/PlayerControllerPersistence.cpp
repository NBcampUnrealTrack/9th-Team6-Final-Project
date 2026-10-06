#include "PlayerPrototypeController.h"
#include "Engine/Engine.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"

bool APlayerPrototypeController::CanSubmitCreation() const
{
    const UICSaveSubsystem* Saves = GetSaves();
    return Menu == TEXT("Create") && !bCreated && !bEffectWaiting && Saves && !Saves->bSaving;
}

bool APlayerPrototypeController::ValidateCreationRequest(FString& OutReason) const
{
    OutReason.Empty();
    if (!GetWorldState() || !GetSaves() || !GetTime() || !GetInventory() || !GetPawn())
        OutReason = TEXT("플레이어 또는 저장 시스템이 준비되지 않았습니다.");
    else if (bCreated)
        OutReason = TEXT("이미 생성된 캐릭터입니다.");
    else if (GetSaves()->bSaving)
        OutReason = TEXT("저장 중입니다. 완료될 때까지 기다려 주세요.");
    else if (bEffectWaiting)
        OutReason = TEXT("아이템 효과 처리 완료 후 다시 시도하세요.");
    else if (Job < 0 || Job > 3 || Preset < 0 || Preset > 1)
        OutReason = TEXT("성별과 직업을 다시 선택하세요.");
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

void APlayerPrototypeController::ReportCreationFailure(const FString& Reason)
{
    Show(Reason);
    UE_LOG(LogTemp, Warning, TEXT("Character creation: %s"), *Reason);
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Reason);
}

void APlayerPrototypeController::CreatePlayer()
{
    FString Reason;
    if (!ValidateCreationRequest(Reason)) { ReportCreationFailure(Reason); return; }
    UICWorldStateSubsystem* State = GetWorldState();
    UICSaveSubsystem* Saves = GetSaves();
    UICTimeSubsystem* Time = GetTime();
    UPlayerInventory* Inventory = GetInventory();
    if (!bInitialSavePending)
    {
        const FName Weapons[] = { TEXT("EQP_WPN_SWORD_001"), TEXT("EQP_WPN_BOW_001"), TEXT("EQP_WPN_STAFF_001"), TEXT("EQP_WPN_WAND_001") };
        const FName Skills[] = { TEXT("SKL_WAR_001"), TEXT("SKL_ARC_001"), TEXT("SKL_MAG_001"), TEXT("SKL_HEAL_001") };
        // Prepare and validate the whole starting inventory before changing live state.
        UPlayerInventory* Staging = NewObject<UPlayerInventory>(this);
        Staging->Definitions = Inventory->Definitions;
        Staging->ItemTable = Inventory->ItemTable;
        Staging->CurrentClass=Job;
        if (!Staging->AddItem(Weapons[Job], 1, false) || !Staging->Equip(0))
        { ReportCreationFailure(TEXT("초기 무기 지급/장착에 실패했습니다. 아이템 정의를 확인하세요.")); return; }
        const TPair<FName, int32> Items[] = {
            { TEXT("ITM_CONS_001"), 5 }, { TEXT("ITM_FOOD_001"), 3 }, { TEXT("ITM_BOOK_001"), 2 }, { TEXT("ITM_TKT_001"), 10 },
            { TEXT("ITM_MAT_001"), 98 }, { TEXT("EQP_ARM_001"), 1 }, { TEXT("EQP_RNG_001"), 1 }, { TEXT("EQP_NCK_001"), 1 }
        };
        for (const auto& Item : Items)
            if (!Staging->AddItem(Item.Key, Item.Value, false))
            { ReportCreationFailure(FString::Printf(TEXT("초기 아이템 %s 지급에 실패했습니다."), *Item.Key.ToString())); return; }
        FICInventoryState Data = Staging->GetSnapshot();
        Data.QuickSlots = { TEXT("ITM_CONS_001"), TEXT("ITM_FOOD_001"), TEXT("ITM_BOOK_001"), TEXT("ITM_TKT_001") };
        if (!Inventory->ValidateSnapshot(Data))
        { ReportCreationFailure(TEXT("초기 인벤토리 검증에 실패했습니다.")); return; }

        FICWorldState Initial;
        Initial.bPlayerCreated = true;
        Initial.PlayerName = PlayerName.TrimStartAndEnd();
        Initial.Preset = Preset;
        Initial.Job = Job;
        Initial.FirstSkill = Skills[Job];
        Initial.PlayerPosition = GetPawn()->GetActorLocation();
        Initial.PlayerRotation = GetControlRotation();
        Initial.Inventory = MoveTemp(Data);
        FICCharacterState InitialCharacterState;
        InitialCharacterState.CharacterId = UICWorldStateSubsystem::PlayerId();
        InitialCharacterState.Kind = EICCharacterKind::Player;
        Initial.Characters.Add(InitialCharacterState.CharacterId, InitialCharacterState);
        if (!State->CanRestoreState(Initial))
        { ReportCreationFailure(TEXT("캐릭터 초기 상태 검증에 실패했습니다.")); return; }
        bInitialSavePending = true;
        if (!State->RestoreState(Initial))
        { bInitialSavePending = false; ReportCreationFailure(TEXT("캐릭터 초기 상태 적용에 실패했습니다.")); return; }
        Time->ResetClock();
        Saves->ResetSession();
    }
    // An explicit SaveSlot call is permitted while automatic saving is suspended.
    Saves->SetAutoSaveSuspended(true);
    SyncSaveSafety();
    Show(TEXT("캐릭터 생성 및 최초 저장 중…"));
    if (!Saves->SaveSlot(0))
    {
        ReportCreationFailure(Saves->GetLastMessage() + TEXT(" 게임 시작 버튼으로 저장을 재시도할 수 있습니다."));
        UpdateGamePhase();
    }
}

bool APlayerPrototypeController::SavePlayer()
{
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||!bCreated||SelectedSaveSlot<1||SelectedSaveSlot>4){Show(TEXT("수동 저장 슬롯 1~4를 선택하세요."));return false;}
    const bool Accepted=Saves->SaveSlot(SelectedSaveSlot);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
bool APlayerPrototypeController::LoadPlayer()
{
    SyncSaveSafety();auto* Saves=GetSaves();if(!Saves)return false;
    const bool WasPending=bInitialSavePending;bInitialSavePending=false;
    const bool Loaded=Saves->LoadSlot(SelectedLoadSlot);
    if(Loaded)RefreshSaveSlots();else bInitialSavePending=WasPending;
    Show(Saves->GetLastMessage());return Loaded;
}

bool APlayerPrototypeController::AutoSavePlayer()
{
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||(!bCreated&&!bInitialSavePending))return false;
    const bool Accepted=Saves->SaveSlot(0);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
