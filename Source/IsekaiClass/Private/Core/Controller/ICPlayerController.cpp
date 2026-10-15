#include "Core/Controller/ICPlayerController.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/ICGameState.h"
#include "Core/ICGameMode.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "Player/State/ICPlayerStateConversions.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Quest/ICQuestWidget.h"
#include "UI/Quest/ICQuestTrackerWidget.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "Core/ICPlayerState.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Debug/ICDebugCheatManager.h"
#include "Quest/ICQuestSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AICPlayerController::AICPlayerController()
{
    IC_TRACE_FUNCTION();
    PrimaryActorTick.bTickEvenWhenPaused = true;
    CheatClass = UICDebugCheatManager::StaticClass();
    DroppedItemClass = AICDroppedItem::StaticClass();
    QuestTrackerWidgetClass = TSoftClassPtr<UICQuestTrackerWidget>(FSoftObjectPath(
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_QuestTracker.WBP_IC_QuestTracker_C")));
}
UICInventoryComponent* AICPlayerController::GetInventory() const
{ IC_TRACE_FUNCTION(); const auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Inventory:nullptr; }
UICCraftingComponent* AICPlayerController::GetCrafting() const
{ IC_TRACE_FUNCTION(); const auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Crafting:nullptr; }
UICWorldStateSubsystem* AICPlayerController::GetWorldState() const
{ IC_TRACE_FUNCTION(); return GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr; }
UICTimeSubsystem* AICPlayerController::GetTime() const
{ IC_TRACE_FUNCTION(); return GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr; }
UICSaveSubsystem* AICPlayerController::GetSaves() const
{ IC_TRACE_FUNCTION(); return GetGameInstance()?GetGameInstance()->GetSubsystem<UICSaveSubsystem>():nullptr; }
FICCharacterState AICPlayerController::GetPlayerSnapshot() const
{ IC_TRACE_FUNCTION(); return GetWorldState()?GetWorldState()->GetSnapshot(UICWorldStateSubsystem::PlayerId()):FICCharacterState(); }

void AICPlayerController::BeginPlay()
{
    IC_TRACE_FUNCTION();
    Super::BeginPlay();
    UE_LOG(LogICCore, Log, TEXT("[Player.BeginPlay] Controller=%s Pawn=%s Local=%d"),*GetNameSafe(this),*GetNameSafe(GetPawn()),IsLocalController());
    if (IsLocalController() && IsMainMenuLevel())
    {
        InitializeMainMenu();
        return;
    }
    if(IsLocalController()&&!DefaultMappingContext) UE_LOG(LogICInput, Warning, TEXT("[Input.MappingMissing] Controller=%s: BP 컨트롤러의 Default Mapping Context를 지정하세요."),*GetNameSafe(this));
    if (!IsLocalController()) return;
    ApplyInputMapping();
    TryInitializeGameplay();
}

void AICPlayerController::ReceivedPlayer()
{
    IC_TRACE_FUNCTION();
    Super::ReceivedPlayer();
    ApplyInputMapping();
}

void AICPlayerController::OnPossess(APawn* InPawn)
{
    IC_TRACE_FUNCTION();
    Super::OnPossess(InPawn);
    UE_LOG(LogICCore, Log, TEXT("[Player.Possess] Controller=%s Pawn=%s"),*GetNameSafe(this),*GetNameSafe(InPawn));
    if (!IsLocalController() || !HasActorBegunPlay() || IsMainMenuLevel()) return;
    BindPawnSystems();
    if (bGameplayInitialized)
    {
        if (auto* TargetCharacter=Cast<AICPlayerCharacter>(InPawn)) TargetCharacter->ApplyPreset(Preset);
    }
    else TryInitializeGameplay();
}

void AICPlayerController::OnUnPossess()
{
    IC_TRACE_FUNCTION();
    FlushWorldState();
    ResetHeldInput();
    ReleaseInputCommand(EICInputCommand::Block);
    ReleaseInputCommand(EICInputCommand::Jump);
    UnbindPawnSystems();UE_LOG(LogICCore, Log, TEXT("[Player.UnPossess] Controller=%s Pawn=%s"),*GetNameSafe(this),*GetNameSafe(GetPawn()));
    Super::OnUnPossess();
}

void AICPlayerController::BindPawnSystems()
{
    IC_TRACE_FUNCTION();
    UICInventoryComponent* Inventory=GetInventory();
    if (BoundInventory==Inventory) return;
    UnbindPawnSystems();
    BoundInventory=Inventory;
    if (!BoundInventory) return;
    BoundInventory->BindWorldState(GetWorldState());
    BoundInventory->OnChanged.AddUniqueDynamic(this,&ThisClass::InventoryChanged);
    BoundInventory->OnMessage.AddUniqueDynamic(this,&ThisClass::InventoryMessage);
    BoundInventory->OnLootResult.AddUniqueDynamic(this,&ThisClass::InventoryLootResult);
    BoundInventory->CurrentClass=Job;
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn())){C->InitializeState(UICWorldStateSubsystem::PlayerId());if(auto* ASC=C->GetICAbilitySystem())ASC->OnStateChanged.AddUniqueDynamic(this,&ThisClass::CombatCacheChanged);}
}

void AICPlayerController::UnbindPawnSystems()
{
    IC_TRACE_FUNCTION();
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem())ASC->OnStateChanged.RemoveDynamic(this,&ThisClass::CombatCacheChanged);
    if (BoundInventory)
    {
        BoundInventory->OnChanged.RemoveDynamic(this,&ThisClass::InventoryChanged);
        BoundInventory->OnMessage.RemoveDynamic(this,&ThisClass::InventoryMessage);
        BoundInventory->OnLootResult.RemoveDynamic(this,&ThisClass::InventoryLootResult);
        BoundInventory->BindWorldState(nullptr);
        BoundInventory=nullptr;
    }
}

void AICPlayerController::TryInitializeGameplay()
{
    IC_TRACE_FUNCTION();
    if (bGameplayInitialized || !IsLocalController() || IsMainMenuLevel()) return;
    auto* State=GetWorldState();auto* Time=GetTime();auto* Saves=GetSaves();
    if (!State || !Time || !Saves || !GetInventory()) return;
    BindPawnSystems();
    if (!State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this))
    { Show(TEXT("플레이어 상태 참여자 중복 등록"));return; }
    bGameplayInitialized=true;
    UE_LOG(LogICCore, Log, TEXT("[Player.Initialized] Controller=%s State=%s Time=%s Saves=%s Inventory=%s"),*GetNameSafe(this),*GetNameSafe(State),*GetNameSafe(Time),*GetNameSafe(Saves),*GetNameSafe(GetInventory()));
    State->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);
    State->OnDropsChanged.AddUniqueDynamic(this,&ThisClass::SynchronizeDrops);
    Time->OnClockChanged.AddUniqueDynamic(this,&ThisClass::ClockChanged);
    Time->OnPlaySecondsAdvanced.AddUniqueDynamic(this,&ThisClass::HandlePlaySecondsAdvanced);
    Saves->OnSaveStarted.AddUniqueDynamic(this,&ThisClass::SaveStarted);
    Saves->OnSaveFinished.AddUniqueDynamic(this,&ThisClass::SaveFinished);
    if(auto* Production=GetGameInstance()->GetSubsystem<UICProductionSubsystem>())Production->OnProductionChanged.AddUniqueDynamic(this,&ThisClass::ProductionChanged);
    RefreshSaveSlots();
    if (GameplayWidgetClass)
    {
        Screen=CreateWidget<UICGameplayWidget>(this,GameplayWidgetClass);
        if (Screen) Screen->AddToViewport();
    }
    if (UClass* WidgetClass = QuestTrackerWidgetClass.LoadSynchronous())
    {
        QuestTrackerScreen = CreateWidget<UICQuestTrackerWidget>(this, WidgetClass);
        if (QuestTrackerScreen) QuestTrackerScreen->AddToPlayerScreen(1);
    }
    if (State->ExportState().bPlayerCreated)
    {
        RestoreFromWorldState_Implementation(State->ExportState());
        if (auto* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>())
        {
            Quests->RefreshAvailableQuests();
        }
    }
    else { SetMenu(TEXT("Create"));Show(TEXT("캐릭터 생성 또는 저장 슬롯을 선택하세요.")); }
#if !UE_BUILD_SHIPPING
    EnableCheats();
#endif
}

void AICPlayerController::ApplyInputMapping()
{
    IC_TRACE_FUNCTION();
    if (!IsLocalController() || IsMainMenuLevel() || AppliedMappingContext==DefaultMappingContext) return;
    if (ULocalPlayer* LocalPlayer=GetLocalPlayer())
        if (auto* Input=LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            RemoveInputMapping();
            if (DefaultMappingContext)
            {
                Input->AddMappingContext(DefaultMappingContext,MappingPriority);
                AppliedMappingContext=DefaultMappingContext;
    UE_LOG(LogICInput, Log, TEXT("[Input.MappingApplied] Controller=%s IMC=%s Priority=%d"),*GetNameSafe(this),*GetPathNameSafe(DefaultMappingContext),MappingPriority);
            }
        }
}

void AICPlayerController::RemoveInputMapping()
{
    IC_TRACE_FUNCTION();
    if (ULocalPlayer* LocalPlayer=GetLocalPlayer())
        if (auto* Input=LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            if (AppliedMappingContext) Input->RemoveMappingContext(AppliedMappingContext);
    if(AppliedMappingContext) UE_LOG(LogICInput, Log, TEXT("[Input.MappingRemoved] Controller=%s IMC=%s"),*GetNameSafe(this),*GetPathNameSafe(AppliedMappingContext));
    AppliedMappingContext=nullptr;
}

void AICPlayerController::UpdateScreens()
{
    IC_TRACE_FUNCTION();
    if (!IsLocalController()) return;
    if(!IsMainMenuLevel())
    {
        const FString SessionMenu=GetSessionMenu();
        const bool bSessionOpen=SessionMenu==TEXT("Save")||SessionMenu==TEXT("Load");
        if(bSessionOpen&&SaveLoadWidgetClass)
        {
            if(!SessionSaveLoadScreen)
            {
                SessionSaveLoadScreen=CreateWidget<UICSaveLoadWidget>(this,SaveLoadWidgetClass);
                if(SessionSaveLoadScreen)
                {
                    SessionSaveLoadScreen->Configure(SessionMenu==TEXT("Save"),TSoftObjectPtr<UWorld>());
                    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->OpenScreen(SessionSaveLoadScreen,true,100);
                }
            }
        }
        else if(SessionSaveLoadScreen)
        {
            auto* Closing=SessionSaveLoadScreen.Get();SessionSaveLoadScreen=nullptr;
            if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->CloseScreen(Closing);
        }
    }
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
    if (Menu==TEXT("Quest") && !QuestScreen)
    {
        if (UClass* WidgetClass = QuestWidgetClass.LoadSynchronous())
        {
            QuestScreen = CreateWidget<UICQuestWidget>(this, WidgetClass);
            if (QuestScreen && GetLocalPlayer())
            {
                if (auto* UI = GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())
                {
                    UI->OpenScreen(QuestScreen, true, 50);
                }
            }
        }
    }
    else if (Menu!=TEXT("Quest") && QuestScreen)
    {
        UICQuestWidget* Closing = QuestScreen.Get();
        QuestScreen = nullptr;
        if (GetLocalPlayer())
        {
            if (auto* UI = GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())
            {
                UI->CloseScreen(Closing);
            }
        }
    }
    if (Screen) Screen->RefreshCurrentScreen();
}

void AICPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICCore, Log, TEXT("[Player.EndPlay] Controller=%s Reason=%d Created=%d"),*GetNameSafe(this),static_cast<int32>(Reason),bCreated);
    ResetHeldInput();
    if(bGameplayInitialized && bCreated)FlushWorldState();
    if(auto* State=GetWorldState())
    {State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this);State->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);State->OnDropsChanged.RemoveDynamic(this,&ThisClass::SynchronizeDrops);}
    if(auto* Time=GetTime())
    {Time->OnClockChanged.RemoveDynamic(this,&ThisClass::ClockChanged);Time->OnPlaySecondsAdvanced.RemoveDynamic(this,&ThisClass::HandlePlaySecondsAdvanced);Time->ReleasePauseRequests(this);}
    if(auto* Saves=GetSaves())
    {Saves->OnSaveStarted.RemoveDynamic(this,&ThisClass::SaveStarted);Saves->OnSaveFinished.RemoveDynamic(this,&ThisClass::SaveFinished);}
    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->CloseAllScreens();
    UnbindPawnSystems();
    RemoveInputMapping();
    if(CharacterCreateScreen){CharacterCreateScreen->RemoveFromParent();CharacterCreateScreen=nullptr;}
    if(Screen){Screen->RemoveFromParent();Screen=nullptr;}
    if(QuestTrackerScreen){QuestTrackerScreen->RemoveFromParent();QuestTrackerScreen=nullptr;}
    QuestScreen=nullptr;
    if(MainMenuWidget){MainMenuWidget->RemoveFromParent();MainMenuWidget=nullptr;}
    SessionSaveLoadScreen=nullptr;
    if(GetGameInstance())if(auto* Production=GetGameInstance()->GetSubsystem<UICProductionSubsystem>())Production->OnProductionChanged.RemoveDynamic(this,&ThisClass::ProductionChanged);
    Super::EndPlay(Reason);
}

void AICPlayerController::SetFinalMoveSpeed(float Speed)
{ IC_TRACE_FUNCTION();if(auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=PlayerPawn->GetICAbilitySystem())ASC->SetNumericAttributeBase(UICCombatAttributeSet::GetMoveSpeedAttribute(),FMath::IsFinite(Speed)?FMath::Max(0.f,Speed):0.f);}
void AICPlayerController::SetCombatState(bool Active){ IC_TRACE_FUNCTION();if(bInCombat!=Active)UE_LOG(LogICCore, Log, TEXT("[Player.CombatState] Active=%d"),Active);bInCombat=Active;SyncSaveSafety();}
bool AICPlayerController::SetGold(int32 Amount)
{
    IC_TRACE_FUNCTION();
    if(Amount<0||!GetWorldState())return false;
    if(!GetWorldState()->SetPlayerGold(Amount))return false;
    Gold=Amount;bDirty=true;return true;
}
void AICPlayerController::InventoryChanged()
{
    IC_TRACE_FUNCTION();auto* Inventory=GetInventory();if(!Inventory)return;TArray<FString> Keys;
    for(const auto& Item:Inventory->GetSnapshot().Equipment){FString Key=Item.InstanceID.ToString()+Item.ItemID.ToString()+FString::FromInt(Item.Grade)+TEXT(":")+FString::FromInt(Item.Upgrade);TArray<FName> Options;Item.Options.GetKeys(Options);Options.Sort(FNameLexicalLess());for(FName ID:Options)Key+=ID.ToString()+FString::SanitizeFloat(Item.Options[ID]);Keys.Add(Key);}Keys.Sort();const FString Current=FString::Join(Keys,TEXT("|"));
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem()){if(Current!=LastEquipmentKey)ASC->NotifyEquipmentChanged();ASC->RebuildDerivedStats();}LastEquipmentKey=Current;OnEquipmentStatsChanged.Broadcast(Inventory->GetEquipmentStats());bDirty=true;
}
void AICPlayerController::InventoryMessage(const FText& Message){ IC_TRACE_FUNCTION();Show(Message.ToString());}
void AICPlayerController::CharacterChanged(FName ID)
{
    IC_TRACE_FUNCTION();
    if(ID==UICWorldStateSubsystem::PlayerId())
    {
        if(const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(ID):nullptr)
        {MainStats=ICPlayerStateConversions::Main(Stored->MainStats);Survival=ICPlayerStateConversions::Survival(*Stored);}
        if(GetWorldState())Gold=GetWorldState()->ExportState().Gold;
        const bool Exhausted=Survival.IsExhausted();
        if(bLastExhausted!=Exhausted){bLastExhausted=Exhausted;OnExhaustionChanged.Broadcast(Exhausted);}
    }
    bDirty=true;
}
void AICPlayerController::ClockChanged(){ IC_TRACE_FUNCTION();bDirty=true;}
void AICPlayerController::Show(const FString& Text){ IC_TRACE_FUNCTION();if(Notice!=Text)UE_LOG(LogICUI, Verbose, TEXT("[UI.Notice] Controller=%s Message=%s"),*GetNameSafe(this),*Text);Notice=Text;bDirty=true;OnNoticeChanged.Broadcast(Notice);}
void AICPlayerController::RefreshSaveSlots(){ IC_TRACE_FUNCTION();if(auto* Saves=GetSaves())SaveSlots=Saves->GetSlotList();bDirty=true;OnSaveSessionChanged.Broadcast();}

void AICPlayerController::SyncSaveSafety()
{
    IC_TRACE_FUNCTION();
    if(auto* Saves=GetSaves())
    {
        FICSaveSafety Safety;Safety.bInCombat=bInCombat;Safety.bInDialogue=bInDialogue;
        Safety.bInEvent=bInEvent;Safety.bDead=bDead;Safety.bEffectPending=bItemTransactionInProgress;
        Saves->SetSafetyState(Safety);
    }
}
void AICPlayerController::SaveStarted(int32 Slot){ IC_TRACE_FUNCTION();Show(Slot==0?TEXT("자동 저장 중..."):FString::Printf(TEXT("수동 슬롯 %d 저장 중..."),Slot));}
void AICPlayerController::SaveFinished(int32 Slot,bool bSuccess,const FString& Message)
{
    IC_TRACE_FUNCTION();
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
    IC_TRACE_FUNCTION();
    if(bItemTransactionInProgress&&NewMenu!=TEXT("Quit")&&NewMenu!=TEXT("Confirm")){Show(TEXT("효과 요청 처리 중입니다."));return;}
    if (NewMenu != TEXT("Dialogue") && DialogueScreen)
    {
        UICNPCDialogueWidget* Closing = DialogueScreen;
        DialogueScreen = nullptr;
        if (GetLocalPlayer())
        {
            if (UICUIManagerSubsystem* UI = GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())
            {
                UI->CloseScreen(Closing);
            }
        }
        bInDialogue = false;
    }
    ResetHeldInput();CancelTarget();
    if(Menu!=NewMenu)UE_LOG(LogICUI, Log, TEXT("[UI.Menu] From=%s To=%s Created=%d Dead=%d"),*Menu,*NewMenu,bCreated,bDead);
    Menu=NewMenu;
    if(auto* Saves=GetSaves())Saves->SetAutoSaveSuspended(NewMenu==TEXT("Create")||NewMenu==TEXT("Save")||NewMenu==TEXT("Quit")||NewMenu==TEXT("Confirm")||bInitialSavePending||bSessionClosing);
    UpdateScreens();
    InteractionHint.Empty();bShowMouseCursor=!NewMenu.IsEmpty();
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(false);}
    UUserWidget* Focus=SessionSaveLoadScreen?static_cast<UUserWidget*>(SessionSaveLoadScreen.Get()):CharacterCreateScreen&&CharacterCreateScreen->IsInViewport()?static_cast<UUserWidget*>(CharacterCreateScreen.Get()):static_cast<UUserWidget*>(Screen.Get());
    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->SetMenuContext(this,Focus,!NewMenu.IsEmpty());
    FlushPressedKeys();bDirty=true;UpdateGamePhase();SyncSaveSafety();OnMenuChanged.Broadcast(Menu);
}
void AICPlayerController::UpdateGamePhase()
{
    IC_TRACE_FUNCTION();
    if(auto* Time=GetTime()){Time->SetRunning(bCreated&&!bDead);Time->SetMenuPaused(false);}
    auto* State=GetWorld()?GetWorld()->GetGameState<AICGameState>():nullptr;
    if(State)State->SetPhase(IsMainMenuLevel()?EICGamePhase::Menu:bDead?EICGamePhase::Dead:!bCreated?EICGamePhase::CharacterCreation:!Menu.IsEmpty()?EICGamePhase::Menu:EICGamePhase::Playing);
}
void AICPlayerController::CancelTarget()
{
    IC_TRACE_FUNCTION();
    if(bItemTransactionInProgress)return;
    if(!PendingItem.IsNone())UE_LOG(LogICInventory, Verbose, TEXT("[Item.TargetCancelled] Item=%s Quick=%d"),*PendingItem.ToString(),PendingQuick);
    PendingItem=NAME_None;PendingQuick=INDEX_NONE;PendingPartyKey=INDEX_NONE;bDirty=true;
}

bool AICPlayerController::ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const
{
    IC_TRACE_FUNCTION();
    const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(CharacterId):nullptr;
    if(!Stored||CharacterId!=UICWorldStateSubsystem::PlayerId())return false;
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(C->GetICAbilitySystem())return C->ReadLiveState_Implementation(CharacterId,Snapshot);
    Snapshot=*Stored;
    if(auto* Time=GetTime()) {const auto Clock=Time->GetClock();Snapshot.LastSimGameMinute=Clock.GameMinute;Snapshot.LastSimPlaySecond=Clock.PlaySecond;}
    if(APawn* PlayerPawn=GetPawn()) {Snapshot.Position=PlayerPawn->GetActorLocation();Snapshot.Rotation=PlayerPawn->GetActorRotation();}
    return true;
}
bool AICPlayerController::FlushToWorldState_Implementation(
    UICWorldStateSubsystem* State)
{
    IC_TRACE_FUNCTION();
    if (!State || !GetPawn() || !GetInventory()
        || !GetInventory()->ValidateState()
        || !MainStats.IsValid() || !Survival.IsValid() || Gold < 0)
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Player.Flush.Rejected] Pawn=%s State=%s Inventory=%s: 인벤토리/능력치/자원/골드 검사 실패"),*GetNameSafe(GetPawn()),*GetNameSafe(State),*GetNameSafe(GetInventory()));
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
{ IC_TRACE_FUNCTION();auto* C=Cast<AICPlayerCharacter>(GetPawn());return GetInventory()&&GetInventory()->ValidateSnapshot(Candidate.Inventory)&&(!C||C->CanRestoreWorldState_Implementation(Candidate));}
void AICPlayerController::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICCore, Log, TEXT("[Player.Restore] Created=%d Preset=%d Job=%d FirstSkill=%s Gold=%d"),Candidate.bPlayerCreated,Candidate.Preset,Candidate.Job,*Candidate.FirstSkill.ToString(),Candidate.Gold);
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
    {PlayerPawn->InitializeState(UICWorldStateSubsystem::PlayerId());PlayerPawn->RestoreFromWorldState_Implementation(Candidate);PlayerPawn->ApplyPreset(Preset);PlayerPawn->SetActorLocation(Candidate.PlayerPosition);PlayerPawn->GetCharacterMovement()->StopMovementImmediately();}
    SynchronizeDrops();
    SetControlRotation(Candidate.PlayerRotation);SetMenu(bCreated?TEXT(""):TEXT("Create"));
    const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
    if(Candidate.bPlayerCreated&&!bInitialSavePending)OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);
}

void AICPlayerController::FlushWorldState() { IC_TRACE_FUNCTION(); if(bCreated||bInitialSavePending)if(!FlushToWorldState_Implementation(GetWorldState()))UE_LOG(LogICWorldState, Verbose, TEXT("[Player.Flush.Failed] Controller=%s"),*GetNameSafe(this)); }
void AICPlayerController::InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow)
{
    IC_TRACE_FUNCTION();
    if(Overflow>0)Show(FString::Printf(TEXT("가방 초과분 %d개를 바닥에 보관했습니다."),Overflow));
}
void AICPlayerController::SynchronizeDrops()
{
    IC_TRACE_FUNCTION();
    if(!GetWorld()||!GetWorldState()||!DroppedItemClass)return;
    const auto Snapshot=GetWorldState()->ExportState();TSet<FGuid> Existing;
    for(const auto& Record:Snapshot.Drops)
    {
        Existing.Add(Record.ID);auto& Handle=DropActors.FindOrAdd(Record.ID);AICDroppedItem* Actor=Handle.Get();
        if(!Actor){Actor=GetWorld()->SpawnActor<AICDroppedItem>(DroppedItemClass,Record.Position+FVector(0,0,35),FRotator::ZeroRotator);Handle=Actor;}
        if(Actor)Actor->Initialize(Record);
    }
    for(auto It=DropActors.CreateIterator();It;++It)if(!Existing.Contains(It.Key()))
    {if(auto* Actor=It.Value().Get())Actor->Destroy();It.RemoveCurrent();}
}

void AICPlayerController::HandlePlaySecondsAdvanced(double Seconds)
{
    IC_TRACE_FUNCTION();
    if(!bCreated || bDead) return;
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem())ASC->AdvanceNaturalSP(Seconds);
    CombatCacheChanged();
}

void AICPlayerController::ProductionChanged(FGuid JobID,const FString& Message){ IC_TRACE_FUNCTION();Show(Message);}

void AICPlayerController::CombatCacheChanged()
{
    IC_TRACE_FUNCTION();auto* C=Cast<AICPlayerCharacter>(GetPawn());if(!C||!C->GetICAbilitySystem())return;
    bBlocking=C->GetICAbilitySystem()->HasMatchingGameplayTag(ICTags::Guarding);const auto S=C->GetICAbilitySystem()->Capture();MainStats=ICPlayerStateConversions::Main(S.MainStats);Survival=ICPlayerStateConversions::Survival(S);
    const bool Exhausted=Survival.IsExhausted();if(bLastExhausted!=Exhausted){bLastExhausted=Exhausted;OnExhaustionChanged.Broadcast(Exhausted);}bDirty=true;
}
