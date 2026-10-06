#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/ICGameState.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "Player/State/ICPlayerStateConversions.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "Core/ICPlayerState.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Debug/ICDebugCheatManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

AICPlayerController::AICPlayerController()
{
    PrimaryActorTick.bTickEvenWhenPaused = true;
    CheatClass = UICDebugCheatManager::StaticClass();
    DroppedItemClass = AICDroppedItem::StaticClass();
    StartingWeapons={TEXT("EQP_WPN_SWORD_001"),TEXT("EQP_WPN_BOW_001"),TEXT("EQP_WPN_STAFF_001"),TEXT("EQP_WPN_WAND_001")};
    StartingSkills={TEXT("SKL_WAR_001"),TEXT("SKL_ARC_001"),TEXT("SKL_MAG_001"),TEXT("SKL_HEAL_001")};
    StartingItems={{TEXT("ITM_CONS_001"),5},{TEXT("ITM_FOOD_001"),3},{TEXT("ITM_BOOK_001"),2},{TEXT("ITM_TKT_001"),10},
        {TEXT("EQP_ARM_001"),1},{TEXT("EQP_RNG_001"),1},{TEXT("EQP_NCK_001"),1}};
    StartingQuickSlots={TEXT("ITM_CONS_001"),TEXT("ITM_FOOD_001"),TEXT("ITM_BOOK_001"),TEXT("ITM_TKT_001")};
}
UICInventoryComponent* AICPlayerController::GetInventory() const
{ const auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Inventory:nullptr; }
UICCraftingComponent* AICPlayerController::GetCrafting() const
{ const auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Crafting:nullptr; }
UICWorldStateSubsystem* AICPlayerController::GetWorldState() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr; }
UICTimeSubsystem* AICPlayerController::GetTime() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr; }
UICSaveSubsystem* AICPlayerController::GetSaves() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICSaveSubsystem>():nullptr; }
FICCharacterState AICPlayerController::GetPlayerSnapshot() const
{ return GetWorldState()?GetWorldState()->GetSnapshot(UICWorldStateSubsystem::PlayerId()):FICCharacterState(); }

void AICPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    ApplyInputMapping();
    TryInitializeGameplay();
}

void AICPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    ApplyInputMapping();
}

void AICPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (!IsLocalController() || !HasActorBegunPlay()) return;
    BindPawnSystems();
    if (bGameplayInitialized)
    {
        if (auto* TargetCharacter=Cast<AICPlayerCharacter>(InPawn)) TargetCharacter->ApplyPreset(Preset);
    }
    else TryInitializeGameplay();
}

void AICPlayerController::OnUnPossess()
{
    ResetHeldInput();
    ReleaseInputCommand(EICInputCommand::Block);
    ReleaseInputCommand(EICInputCommand::Jump);
    UnbindPawnSystems();
    Super::OnUnPossess();
}

void AICPlayerController::BindPawnSystems()
{
    UICInventoryComponent* Inventory=GetInventory();
    if (BoundInventory==Inventory) return;
    UnbindPawnSystems();
    BoundInventory=Inventory;
    if (!BoundInventory) return;
    BoundInventory->BindWorldState(GetWorldState());
    BoundInventory->OnChanged.AddUniqueDynamic(this,&ThisClass::InventoryChanged);
    BoundInventory->OnMessage.AddUniqueDynamic(this,&ThisClass::InventoryMessage);
    BoundInventory->OnLootResult.AddUniqueDynamic(this,&ThisClass::InventoryLootResult);
    BoundInventory->OnDropRequested.AddUniqueDynamic(this,&ThisClass::InventoryDropRequested);
    BoundInventory->CurrentClass=Job;
}

void AICPlayerController::UnbindPawnSystems()
{
    if (BoundInventory)
    {
        BoundInventory->OnChanged.RemoveDynamic(this,&ThisClass::InventoryChanged);
        BoundInventory->OnMessage.RemoveDynamic(this,&ThisClass::InventoryMessage);
        BoundInventory->OnLootResult.RemoveDynamic(this,&ThisClass::InventoryLootResult);
        BoundInventory->OnDropRequested.RemoveDynamic(this,&ThisClass::InventoryDropRequested);
        BoundInventory->BindWorldState(nullptr);
        BoundInventory=nullptr;
    }
}

void AICPlayerController::TryInitializeGameplay()
{
    if (bGameplayInitialized || !IsLocalController()) return;
    auto* State=GetWorldState();auto* Time=GetTime();auto* Saves=GetSaves();
    if (!State || !Time || !Saves || !GetInventory()) return;
    BindPawnSystems();
    if (!State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this))
    { Show(TEXT("플레이어 상태 참여자 중복 등록"));return; }
    bGameplayInitialized=true;
    State->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);
    Time->OnClockChanged.AddUniqueDynamic(this,&ThisClass::ClockChanged);
    Time->OnGameMinutesAdvanced.AddUniqueDynamic(this,&ThisClass::HandleGameMinutesAdvanced);
    Saves->OnSaveStarted.AddUniqueDynamic(this,&ThisClass::SaveStarted);
    Saves->OnSaveFinished.AddUniqueDynamic(this,&ThisClass::SaveFinished);
    RefreshSaveSlots();
    if (GameplayWidgetClass)
    {
        Screen=CreateWidget<UICGameplayWidget>(this,GameplayWidgetClass);
        if (Screen) Screen->AddToViewport();
    }
    if (State->ExportState().bPlayerCreated) RestoreFromWorldState_Implementation(State->ExportState());
    else { SetMenu(TEXT("Create"));Show(TEXT("캐릭터 생성 또는 저장 슬롯을 선택하세요.")); }
#if !UE_BUILD_SHIPPING
    EnableCheats();
#endif
}

void AICPlayerController::ApplyInputMapping()
{
    if (!IsLocalController() || AppliedMappingContext==DefaultMappingContext) return;
    if (ULocalPlayer* LocalPlayer=GetLocalPlayer())
        if (auto* Input=LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            RemoveInputMapping();
            if (DefaultMappingContext)
            {
                Input->AddMappingContext(DefaultMappingContext,MappingPriority);
                AppliedMappingContext=DefaultMappingContext;
            }
        }
}

void AICPlayerController::RemoveInputMapping()
{
    if (ULocalPlayer* LocalPlayer=GetLocalPlayer())
        if (auto* Input=LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            if (AppliedMappingContext) Input->RemoveMappingContext(AppliedMappingContext);
    AppliedMappingContext=nullptr;
}

void AICPlayerController::UpdateScreens()
{
    if (!IsLocalController()) return;
    if (Menu==TEXT("Create") && CharacterCreateWidgetClass)
    {
        if (!CharacterCreateScreen) CharacterCreateScreen=CreateWidget<UICCharacterCreateWidget>(this,CharacterCreateWidgetClass);
        if (CharacterCreateScreen)
        {
            if (!CharacterCreateScreen->IsInViewport()) CharacterCreateScreen->AddToViewport(10);
            CharacterCreateScreen->RefreshState();
        }
    }
    else if (CharacterCreateScreen && CharacterCreateScreen->IsInViewport()) CharacterCreateScreen->RemoveFromParent();
    if (Screen) Screen->RefreshCurrentScreen();
}

void AICPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    ResetHeldInput();
    if(auto* State=GetWorldState())
    {State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this);State->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);}
    if(auto* Time=GetTime())
    {Time->OnClockChanged.RemoveDynamic(this,&ThisClass::ClockChanged);Time->OnGameMinutesAdvanced.RemoveDynamic(this,&ThisClass::HandleGameMinutesAdvanced);}
    if(auto* Saves=GetSaves())
    {Saves->OnSaveStarted.RemoveDynamic(this,&ThisClass::SaveStarted);Saves->OnSaveFinished.RemoveDynamic(this,&ThisClass::SaveFinished);}
    UnbindPawnSystems();
    RemoveInputMapping();
    if(CharacterCreateScreen){CharacterCreateScreen->RemoveFromParent();CharacterCreateScreen=nullptr;}
    if(Screen){Screen->RemoveFromParent();Screen=nullptr;}
    Super::EndPlay(Reason);
}

void AICPlayerController::SetFinalMoveSpeed(float Speed)
{if(auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn()))PlayerPawn->FinalMoveSpeed=FMath::IsFinite(Speed)?FMath::Max(0.f,Speed):0.f;}
void AICPlayerController::SetCombatState(bool Active){bInCombat=Active;SyncSaveSafety();}
bool AICPlayerController::SetGold(int32 Amount)
{
    if(Amount<0||!GetWorldState())return false;
    Gold=Amount;
    const bool Success=GetWorldState()->SetPlayerGold(Amount);bDirty=true;return Success;
}
void AICPlayerController::InventoryChanged()
{if(auto* Inventory=GetInventory())OnEquipmentStatsChanged.Broadcast(Inventory->GetEquipmentStats());bDirty=true;}
void AICPlayerController::InventoryMessage(const FText& Message){Show(Message.ToString());}
void AICPlayerController::CharacterChanged(FName ID)
{
    if(ID==UICWorldStateSubsystem::PlayerId())
    {
        if(const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(ID):nullptr)
        {MainStats=ICPlayerStateConversions::Main(Stored->MainStats);Survival=ICPlayerStateConversions::Survival(*Stored);}
        const bool Exhausted=Survival.IsExhausted();
        if(bLastExhausted!=Exhausted){bLastExhausted=Exhausted;OnExhaustionChanged.Broadcast(Exhausted);}
    }
    bDirty=true;
}
void AICPlayerController::ClockChanged(){bDirty=true;}
void AICPlayerController::Show(const FString& Text){Notice=Text;bDirty=true;OnNoticeChanged.Broadcast(Notice);}
void AICPlayerController::RefreshSaveSlots(){if(auto* Saves=GetSaves())SaveSlots=Saves->GetSlotList();bDirty=true;}

void AICPlayerController::SyncSaveSafety()
{
    if(auto* Saves=GetSaves())
    {
        FICSaveSafety Safety;Safety.bInCombat=bInCombat;Safety.bInDialogue=bInDialogue;
        Safety.bInEvent=bInEvent;Safety.bDead=bDead;Safety.bEffectPending=bEffectWaiting;
        Saves->SetSafetyState(Safety);
    }
}
void AICPlayerController::SaveStarted(int32 Slot){Show(Slot==0?TEXT("자동 저장 중..."):FString::Printf(TEXT("수동 슬롯 %d 저장 중..."),Slot));}
void AICPlayerController::SaveFinished(int32 Slot,bool bSuccess,const FString& Message)
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
        UpdateScreens();
    }
    RefreshSaveSlots();Show((Slot==0?FString(TEXT("자동 저장: ")):FString::Printf(TEXT("수동 슬롯 %d: "),Slot))+Message);
    FinishSessionSave(Slot,bSuccess);
}

void AICPlayerController::SetMenu(const FString& NewMenu)
{
    if(bEffectWaiting&&NewMenu!=TEXT("Quit")&&NewMenu!=TEXT("Confirm")){Show(TEXT("효과 요청 처리 중입니다."));return;}
    ResetHeldInput();CancelTarget();
    Menu=NewMenu;
    if(auto* Saves=GetSaves())Saves->SetAutoSaveSuspended(NewMenu==TEXT("Create")||NewMenu==TEXT("Save")||NewMenu==TEXT("Quit")||NewMenu==TEXT("Confirm")||bInitialSavePending||bSessionClosing);
    UpdateScreens();
    InteractionHint.Empty();bShowMouseCursor=!NewMenu.IsEmpty();SetPause(!NewMenu.IsEmpty());
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(!NewMenu.IsEmpty());}
    if(NewMenu.IsEmpty()){FInputModeGameOnly Mode;SetInputMode(Mode);}
    else{FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);if(CharacterCreateScreen&&CharacterCreateScreen->IsInViewport())Mode.SetWidgetToFocus(CharacterCreateScreen->TakeWidget());else if(Screen)Mode.SetWidgetToFocus(Screen->TakeWidget());SetInputMode(Mode);}
    FlushPressedKeys();bDirty=true;UpdateGamePhase();SyncSaveSafety();OnMenuChanged.Broadcast(Menu);
}
void AICPlayerController::UpdateGamePhase()
{
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(!Menu.IsEmpty());}
    auto* State=GetWorld()?GetWorld()->GetGameState<AICGameState>():nullptr;
    if(State)State->SetPhase(bDead?EICGamePhase::Dead:!bCreated?EICGamePhase::CharacterCreation:!Menu.IsEmpty()?EICGamePhase::Menu:EICGamePhase::Playing);
}
void AICPlayerController::CancelTarget()
{
    if(bEffectWaiting)return;
    PendingItem=NAME_None;PendingQuick=INDEX_NONE;PendingPartyKey=INDEX_NONE;PendingRequest.Invalidate();bDirty=true;
}

bool AICPlayerController::ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const
{
    const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(CharacterId):nullptr;
    if(!Stored||CharacterId!=UICWorldStateSubsystem::PlayerId())return false;
    Snapshot=*Stored;ICPlayerStateConversions::Apply(MainStats,Survival,Snapshot);return true;
}
bool AICPlayerController::FlushToWorldState_Implementation(
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
bool AICPlayerController::CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const
{return GetInventory()&&GetInventory()->ValidateSnapshot(Candidate.Inventory);}
void AICPlayerController::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    PlayerName=Candidate.PlayerName;Preset=Candidate.Preset;Job=Candidate.Job;FirstSkill=Candidate.FirstSkill;
    bCreated=Candidate.bPlayerCreated&&!bInitialSavePending;bDead=false;bInCombat=bInDialogue=bInEvent=false;
    Gold=Candidate.Gold;
    if(auto* Inventory=GetInventory())Inventory->CurrentClass=Job;
    if (const auto* StoredPlayerSnapshot =
    Candidate.Characters.Find(UICWorldStateSubsystem::PlayerId()))
    {
        MainStats = ICPlayerStateConversions::Main(StoredPlayerSnapshot->MainStats);
        Survival = ICPlayerStateConversions::Survival(*StoredPlayerSnapshot);
    }
    SelectedBag=SelectedStorage=MoveFrom=INDEX_NONE;
    if(auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn()))
    {PlayerPawn->ApplyPreset(Preset);PlayerPawn->SetActorLocation(Candidate.PlayerPosition);PlayerPawn->GetCharacterMovement()->StopMovementImmediately();}
    SetControlRotation(Candidate.PlayerRotation);SetMenu(bCreated?TEXT(""):TEXT("Create"));
    const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
    if(Candidate.bPlayerCreated&&!bInitialSavePending)OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);
}

void AICPlayerController::HandleGameMinutesAdvanced(int32 Minutes) { if(bCreated&&!bDead)ApplyGameMinutes(Minutes); }
void AICPlayerController::FlushWorldState() { if(bCreated||bInitialSavePending)FlushToWorldState_Implementation(GetWorldState()); }
void AICPlayerController::InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow)
{
    if(Overflow<=0)return;FPlayerItemInstance Item;Item.ItemID=ItemID;Item.InstanceID=FGuid::NewGuid();Item.Count=Overflow;SpawnDroppedItem(Item);
}
void AICPlayerController::InventoryDropRequested(const FPlayerItemInstance& Item) { SpawnDroppedItem(Item); }
void AICPlayerController::SpawnDroppedItem(const FPlayerItemInstance& Item)
{
    if(!GetWorld()||!GetPawn()||!DroppedItemClass||Item.IsEmpty())return;
    const auto* Definition=GetInventory()?GetInventory()->Find(Item.ItemID):nullptr;
    const int32 Limit=Definition?FMath::Clamp(Definition->StackLimit,1,99):99;
    for(int32 Left=Item.Count;Left>0;)
    {
        FPlayerItemInstance Part=Item;Part.Count=FMath::Min(Left,Limit);Left-=Part.Count;
        if(Left>0||Part.Count!=Item.Count)Part.InstanceID=FGuid::NewGuid();
        const FVector Location=GetPawn()->GetActorLocation()+GetPawn()->GetActorForwardVector()*100.f+FVector(0,0,35);
        if(auto* Dropped=GetWorld()->SpawnActor<AICDroppedItem>(DroppedItemClass,Location,FRotator::ZeroRotator))Dropped->Initialize(Part);
    }
}
