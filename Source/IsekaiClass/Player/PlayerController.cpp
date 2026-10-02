#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"
#include "PlayerPrototypeWidget.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"
#include "../Debug/ICDebugCheatManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"

APlayerPrototypeController::APlayerPrototypeController()
{
    PrimaryActorTick.bTickEvenWhenPaused=true;
    CheatClass=UICDebugCheatManager::StaticClass();
}
UPlayerInventory* APlayerPrototypeController::GetInventory() const
{ const auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Inventory:nullptr; }
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
    if(!State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(),this)){Show(TEXT("플레이어 상태 참여자 중복 등록"));return;}
    State->OnCharacterChanged.AddDynamic(this,&ThisClass::CharacterChanged);
    Time->OnClockChanged.AddDynamic(this,&ThisClass::ClockChanged);
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
    if(auto* Time=GetTime())Time->OnClockChanged.RemoveDynamic(this,&ThisClass::ClockChanged);
    if(auto* Saves=GetSaves())
    {Saves->OnSaveStarted.RemoveDynamic(this,&ThisClass::SaveStarted);Saves->OnSaveFinished.RemoveDynamic(this,&ThisClass::SaveFinished);}
    if(auto* Inventory=GetInventory())Inventory->BindWorldState(nullptr);
    Super::EndPlay(Reason);
}

void APlayerPrototypeController::SetFinalMoveSpeed(float Speed)
{if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))PlayerPawn->FinalMoveSpeed=FMath::IsFinite(Speed)?FMath::Max(0.f,Speed):0.f;}
void APlayerPrototypeController::SetCombatState(bool Active){bInCombat=Active;SyncSaveSafety();}
void APlayerPrototypeController::InventoryChanged(){bDirty=true;}
void APlayerPrototypeController::InventoryMessage(const FText& Message){Show(Message.ToString());}
void APlayerPrototypeController::CharacterChanged(FName ID){bDirty=true;}
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
        else UpdateGamePhase();
    }
    RefreshSaveSlots();Show((Slot==0?FString(TEXT("자동 저장: ")):FString::Printf(TEXT("수동 슬롯 %d: "),Slot))+Message);
    FinishSessionSave(Slot,bSuccess);
}

void APlayerPrototypeController::SetMenu(const FString& NewMenu)
{
    if(bEffectWaiting&&NewMenu!=TEXT("Quit")&&NewMenu!=TEXT("Confirm")){Show(TEXT("효과 요청 처리 중입니다."));return;}
    CancelTarget();bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0);
    Menu=NewMenu;
    if(auto* Saves=GetSaves())Saves->SetAutoSaveSuspended(NewMenu==TEXT("Save")||NewMenu==TEXT("Quit")||NewMenu==TEXT("Confirm")||bSessionClosing);
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
    PendingItem=NAME_None;PendingQuick=INDEX_NONE;PendingRequest.Invalidate();bDirty=true;
}

bool APlayerPrototypeController::ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const
{
    const auto* Stored=GetWorldState()?GetWorldState()->GetStoredCharacter(CharacterId):nullptr;
    if(!Stored||CharacterId!=UICWorldStateSubsystem::PlayerId())return false;
    Snapshot=*Stored;return true;
}
bool APlayerPrototypeController::FlushToWorldState_Implementation(UICWorldStateSubsystem* State)
{
    return State&&GetPawn()&&GetInventory()&&GetInventory()->ValidateState()
        &&State->SetPlayerTransform(GetPawn()->GetActorLocation(),GetControlRotation());
}
bool APlayerPrototypeController::CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const
{return GetInventory()&&GetInventory()->ValidateSnapshot(Candidate.Inventory);}
void APlayerPrototypeController::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    PlayerName=Candidate.PlayerName;Preset=Candidate.Preset;Job=Candidate.Job;FirstSkill=Candidate.FirstSkill;
    bCreated=Candidate.bPlayerCreated&&!bInitialSavePending;bDead=false;bInCombat=bInDialogue=bInEvent=false;
    SelectedBag=SelectedStorage=MoveFrom=INDEX_NONE;
    if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))
    {PlayerPawn->ApplyPreset(Preset);PlayerPawn->SetActorLocation(Candidate.PlayerPosition);PlayerPawn->GetCharacterMovement()->StopMovementImmediately();}
    SetControlRotation(Candidate.PlayerRotation);SetMenu(bCreated?TEXT(""):TEXT("Create"));
    const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
    if(Candidate.bPlayerCreated&&!bInitialSavePending)OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);
}
