#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"
#include "PlayerCrafting.h"
#include "PlayerDroppedItem.h"
#include "PlayerStateConversions.h"
#include "PlayerPrototypeWidget.h"
#include "ICCharacterCreateWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"
#include "../Debug/ICDebugCheatManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"

APlayerPrototypeController::APlayerPrototypeController()
{
    PrimaryActorTick.bTickEvenWhenPaused = true;
    CheatClass = UICDebugCheatManager::StaticClass();

    static ConstructorHelpers::FClassFinder<UICCharacterCreateWidget>
        CharacterCreateWidgetBP(
            TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CharacterCreate")
        );

    if (CharacterCreateWidgetBP.Succeeded())
    {
        CharacterCreateWidgetClass = CharacterCreateWidgetBP.Class;
    }
}
UPlayerInventory* APlayerPrototypeController::GetInventory() const
{ const auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Inventory:nullptr; }
UPlayerCrafting* APlayerPrototypeController::GetCrafting() const
{ const auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Crafting:nullptr; }
UICWorldStateSubsystem* APlayerPrototypeController::GetWorldState() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr; }
UICTimeSubsystem* APlayerPrototypeController::GetTime() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr; }
UICSaveSubsystem* APlayerPrototypeController::GetSaves() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICSaveSubsystem>():nullptr; }
FICCharacterState APlayerPrototypeController::GetPlayerSnapshot() const
{ return GetWorldState()?GetWorldState()->GetSnapshot(UICWorldStateSubsystem::PlayerId()):FICCharacterState(); }

void APlayerPrototypeController::BeginPlay()
{
    Super::BeginPlay();
    auto* State=GetWorldState();auto* Time=GetTime();auto* Saves=GetSaves();auto* Inventory=GetInventory();
    if(!State||!Time||!Saves||!Inventory){Show(TEXT("공용 시스템 초기화 실패"));return;}
    Inventory->BindWorldState(State);
    Inventory->OnChanged.AddDynamic(this,&ThisClass::InventoryChanged);
    Inventory->OnMessage.AddDynamic(this,&ThisClass::InventoryMessage);
    Inventory->OnLootResult.AddDynamic(this,&ThisClass::InventoryLootResult);
    Inventory->OnDropRequested.AddDynamic(this,&ThisClass::InventoryDropRequested);
    if(!State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this)){Show(TEXT("플레이어 상태 참여자 중복 등록"));return;}
    State->OnCharacterChanged.AddDynamic(this,&ThisClass::CharacterChanged);
    Time->OnClockChanged.AddDynamic(this,&ThisClass::ClockChanged);
    Time->OnGameMinutesAdvanced.AddDynamic(this,&ThisClass::HandleGameMinutesAdvanced);
    Saves->OnSaveStarted.AddDynamic(this,&ThisClass::SaveStarted);
    Saves->OnSaveFinished.AddDynamic(this,&ThisClass::SaveFinished);
    RefreshSaveSlots();
    Screen=CreateWidget<UPlayerPrototypeWidget>(this,UPlayerPrototypeWidget::StaticClass());Screen->AddToViewport();
    if(State->ExportState().bPlayerCreated)RestoreFromWorldState_Implementation(State->ExportState());
    else SetMenu(TEXT("Create"));
    Show(TEXT("캐릭터 생성 또는 저장 슬롯을 선택하세요."));
#if !UE_BUILD_SHIPPING
    EnableCheats();
    if(FParse::Param(FCommandLine::Get(),TEXT("PlayerPrototypeSmoke")))StartSmokeTest();
#endif
}

void APlayerPrototypeController::EndPlay(const EEndPlayReason::Type Reason)
{
    if(auto* State=GetWorldState())
    {State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this);State->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);}
    if(auto* Time=GetTime())
    {Time->OnClockChanged.RemoveDynamic(this,&ThisClass::ClockChanged);Time->OnGameMinutesAdvanced.RemoveDynamic(this,&ThisClass::HandleGameMinutesAdvanced);}
    if(auto* Saves=GetSaves())
    {Saves->OnSaveStarted.RemoveDynamic(this,&ThisClass::SaveStarted);Saves->OnSaveFinished.RemoveDynamic(this,&ThisClass::SaveFinished);}
    if(auto* Inventory=GetInventory())
    {
        Inventory->OnChanged.RemoveDynamic(this,&ThisClass::InventoryChanged);
        Inventory->OnMessage.RemoveDynamic(this,&ThisClass::InventoryMessage);
        Inventory->OnLootResult.RemoveDynamic(this,&ThisClass::InventoryLootResult);
        Inventory->OnDropRequested.RemoveDynamic(this,&ThisClass::InventoryDropRequested);
        Inventory->BindWorldState(nullptr);
    }
    if(Screen){Screen->RemoveFromParent();Screen=nullptr;}
    Super::EndPlay(Reason);
}

void APlayerPrototypeController::SetFinalMoveSpeed(float Speed)
{if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))PlayerPawn->FinalMoveSpeed=FMath::IsFinite(Speed)?FMath::Max(0.f,Speed):0.f;}
void APlayerPrototypeController::SetCombatState(bool Active){bInCombat=Active;SyncSaveSafety();}
void APlayerPrototypeController::InventoryChanged()
{if(auto* Inventory=GetInventory())OnEquipmentStatsChanged.Broadcast(Inventory->GetEquipmentStats());bDirty=true;}
void APlayerPrototypeController::InventoryMessage(const FText& Message){Show(Message.ToString());}
void APlayerPrototypeController::CharacterChanged(FName ID)
{
    if(ID==UICWorldStateSubsystem::PlayerId())
    {
        if(const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(ID):nullptr)
        {MainStats=PlayerStateConversions::Main(Stored->MainStats);Survival=PlayerStateConversions::Survival(*Stored);}
        const bool Exhausted=Survival.IsExhausted();
        if(bLastExhausted!=Exhausted){bLastExhausted=Exhausted;OnExhaustionChanged.Broadcast(Exhausted);}
    }
    bDirty=true;
}
void APlayerPrototypeController::ClockChanged(){bDirty=true;}
void APlayerPrototypeController::Show(const FString& Text){Notice=Text;bDirty=true;}
void APlayerPrototypeController::RefreshSaveSlots(){if(auto* Saves=GetSaves())SaveSlots=Saves->GetSlotList();bDirty=true;}

void APlayerPrototypeController::SyncSaveSafety()
{
    if(auto* Saves=GetSaves())
    {
        FICSaveSafety Safety;Safety.bInCombat=bInCombat;Safety.bInDialogue=bInDialogue;
        Safety.bInEvent=bInEvent;Safety.bDead=bDead;Safety.bEffectPending=bEffectWaiting;
        Saves->SetSafetyState(Safety);
    }
}
void APlayerPrototypeController::SaveStarted(int32 Slot){Show(Slot==0?TEXT("자동 저장 중..."):FString::Printf(TEXT("수동 슬롯 %d 저장 중..."),Slot));}
void APlayerPrototypeController::SaveFinished(int32 Slot,bool bSuccess,const FString& Message)
{
    if(bInitialSavePending&&Slot==0)
    {
        bCreated=bSuccess;
        if(bSuccess)
        {
            bInitialSavePending=false;SetMenu(TEXT(""));
            const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
            OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);
        }
        else
        {
            UpdateGamePhase();
            ReportCreationFailure(Message + TEXT(" 게임 시작 버튼으로 저장을 재시도할 수 있습니다."));
        }
        // Close the creation screen immediately on success; refresh the retry state on failure.
        if (Screen) Screen->RefreshCurrentScreen();
    }
    RefreshSaveSlots();Show((Slot==0?FString(TEXT("자동 저장: ")):FString::Printf(TEXT("수동 슬롯 %d: "),Slot))+Message);
    FinishSessionSave(Slot,bSuccess);
}

void APlayerPrototypeController::SetMenu(const FString& NewMenu)
{
    if(bEffectWaiting&&NewMenu!=TEXT("Quit")&&NewMenu!=TEXT("Confirm")){Show(TEXT("효과 요청 처리 중입니다."));return;}
    CancelTarget();bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0);
    Menu=NewMenu;
    if(auto* Saves=GetSaves())Saves->SetAutoSaveSuspended(NewMenu==TEXT("Create")||NewMenu==TEXT("Save")||NewMenu==TEXT("Quit")||NewMenu==TEXT("Confirm")||bInitialSavePending||bSessionClosing);
    InteractionHint.Empty();bShowMouseCursor=!NewMenu.IsEmpty();SetPause(!NewMenu.IsEmpty());
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(!NewMenu.IsEmpty());}
    if(NewMenu.IsEmpty()){FInputModeGameOnly Mode;SetInputMode(Mode);}
    else{FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);if(Screen)Mode.SetWidgetToFocus(Screen->TakeWidget());SetInputMode(Mode);}
    FlushPressedKeys();bDirty=true;UpdateGamePhase();SyncSaveSafety();
}
void APlayerPrototypeController::UpdateGamePhase()
{
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(!Menu.IsEmpty());}
    auto* State=GetWorld()?GetWorld()->GetGameState<APlayerPrototypeGameState>():nullptr;
    if(State)State->SetPhase(bDead?EPlayerGamePhase::Dead:!bCreated?EPlayerGamePhase::CharacterCreation:!Menu.IsEmpty()?EPlayerGamePhase::Menu:EPlayerGamePhase::Playing);
}
void APlayerPrototypeController::CancelTarget()
{
    if(bEffectWaiting)return;
    PendingItem=NAME_None;PendingQuick=INDEX_NONE;PendingPartyKey=INDEX_NONE;PendingRequest.Invalidate();bDirty=true;
}

bool APlayerPrototypeController::ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const
{
    const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(CharacterId):nullptr;
    if(!Stored||CharacterId!=UICWorldStateSubsystem::PlayerId())return false;
    Snapshot=*Stored;PlayerStateConversions::Apply(MainStats,Survival,Snapshot);return true;
}
bool APlayerPrototypeController::FlushToWorldState_Implementation(
    UICWorldStateSubsystem* State)
{
    if (!State || !GetPawn() || !GetInventory()
        || !GetInventory()->ValidateState()
        || !MainStats.IsValid() || !Survival.IsValid() || Gold < 0)
    {
        return false;
    }

    FICCharacterState PlayerSnapshot;

    if (!ReadLiveState_Implementation(
        UICWorldStateSubsystem::PlayerId(), PlayerSnapshot))
    {
        return !State->ExportState().bPlayerCreated;
    }

    return State->SetCharacterState(PlayerSnapshot)
        && State->SetPlayerGold(Gold)
        && State->SetPlayerTransform(
            GetPawn()->GetActorLocation(), GetControlRotation());
}
bool APlayerPrototypeController::CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const
{return GetInventory()&&GetInventory()->ValidateSnapshot(Candidate.Inventory);}
void APlayerPrototypeController::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    PlayerName=Candidate.PlayerName;Preset=Candidate.Preset;Job=Candidate.Job;FirstSkill=Candidate.FirstSkill;
    bCreated=Candidate.bPlayerCreated&&!bInitialSavePending;bDead=false;bInCombat=bInDialogue=bInEvent=false;
    Gold=Candidate.Gold;
    if (const auto* StoredPlayerSnapshot =
    Candidate.Characters.Find(UICWorldStateSubsystem::PlayerId()))
    {
        MainStats = PlayerStateConversions::Main(StoredPlayerSnapshot->MainStats);
        Survival = PlayerStateConversions::Survival(*StoredPlayerSnapshot);
    }
    SelectedBag=SelectedStorage=MoveFrom=INDEX_NONE;
    if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))
    {PlayerPawn->ApplyPreset(Preset);PlayerPawn->SetActorLocation(Candidate.PlayerPosition);PlayerPawn->GetCharacterMovement()->StopMovementImmediately();}
    SetControlRotation(Candidate.PlayerRotation);SetMenu(bCreated?TEXT(""):TEXT("Create"));
    const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
    if(Candidate.bPlayerCreated&&!bInitialSavePending)OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);
}

void APlayerPrototypeController::HandleGameMinutesAdvanced(int32 Minutes) { if(bCreated&&!bDead)ApplyGameMinutes(Minutes); }
void APlayerPrototypeController::FlushWorldState() { if(bCreated||bInitialSavePending)FlushToWorldState_Implementation(GetWorldState()); }
void APlayerPrototypeController::InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow)
{
    if(Overflow<=0)return;FPlayerItemInstance Item;Item.ItemID=ItemID;Item.InstanceID=FGuid::NewGuid();Item.Count=Overflow;SpawnDroppedItem(Item);
}
void APlayerPrototypeController::InventoryDropRequested(const FPlayerItemInstance& Item) { SpawnDroppedItem(Item); }
void APlayerPrototypeController::SpawnDroppedItem(const FPlayerItemInstance& Item)
{
    if(!GetWorld()||!GetPawn()||Item.IsEmpty())return;
    const auto* Definition=GetInventory()?GetInventory()->Find(Item.ItemID):nullptr;
    const int32 Limit=Definition?FMath::Clamp(Definition->StackLimit,1,99):99;
    for(int32 Left=Item.Count;Left>0;)
    {
        FPlayerItemInstance Part=Item;Part.Count=FMath::Min(Left,Limit);Left-=Part.Count;
        if(Left>0||Part.Count!=Item.Count)Part.InstanceID=FGuid::NewGuid();
        const FVector Location=GetPawn()->GetActorLocation()+GetPawn()->GetActorForwardVector()*100.f+FVector(0,0,35);
        if(auto* Dropped=GetWorld()->SpawnActor<APlayerDroppedItem>(Location,FRotator::ZeroRotator))Dropped->Initialize(Part);
    }
}
