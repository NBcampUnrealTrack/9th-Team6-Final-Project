#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"
#include "PlayerPrototypeWidget.h"
#include "Misc/CommandLine.h"

APlayerPrototypeController::APlayerPrototypeController() { PrimaryActorTick.bTickEvenWhenPaused=true; }
UPlayerInventory* APlayerPrototypeController::GetInventory() const { const auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());return PlayerPawn?PlayerPawn->Inventory:nullptr; }

void APlayerPrototypeController::BeginPlay()
{
    Super::BeginPlay();
    if(auto* Inventory=GetInventory()) { Inventory->OnChanged.AddDynamic(this,&ThisClass::InventoryChanged);Inventory->OnMessage.AddDynamic(this,&ThisClass::InventoryMessage); }
    Screen=CreateWidget<UPlayerPrototypeWidget>(this,UPlayerPrototypeWidget::StaticClass());
    Screen->AddToViewport();
    SetMenu(TEXT("Create"));
    Show(TEXT("캐릭터 정보를 입력하세요. 로컬 남녀 캐릭터가 적용됩니다."));
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("PlayerPrototypeSmoke"))) StartSmokeTest();
#endif
}

void APlayerPrototypeController::SetFinalMoveSpeed(float Speed) { if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))PlayerPawn->FinalMoveSpeed=FMath::Max(0.f,Speed); }
void APlayerPrototypeController::InventoryChanged() { bDirty=true; }
void APlayerPrototypeController::InventoryMessage(const FText& Message) { Show(Message.ToString()); }
void APlayerPrototypeController::Show(const FString& Text) { Notice=Text;bDirty=true; }

void APlayerPrototypeController::SetMenu(const FString& NewMenu)
{
    if(bEffectWaiting) { Show(TEXT("효과 요청 처리 중입니다."));return; }
    CancelTarget();bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0);
    Menu=NewMenu;bShowMouseCursor=!NewMenu.IsEmpty();SetPause(!NewMenu.IsEmpty());
    if(NewMenu.IsEmpty()) { FInputModeGameOnly Mode;SetInputMode(Mode); }
    else { FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode); }
    FlushPressedKeys();bDirty=true;UpdateGamePhase();
}

void APlayerPrototypeController::UpdateGamePhase()
{
    auto* State=GetWorld()?GetWorld()->GetGameState<APlayerPrototypeGameState>():nullptr;
    if(!State) return;
    if(bDead) State->SetPhase(EPlayerGamePhase::Dead);
    else if(!bCreated) State->SetPhase(EPlayerGamePhase::CharacterCreation);
    else if(!Menu.IsEmpty()) State->SetPhase(EPlayerGamePhase::Menu);
    else State->SetPhase(EPlayerGamePhase::Playing);
}

void APlayerPrototypeController::CancelTarget()
{
    if(bEffectWaiting) return;
    PendingItem=NAME_None;PendingQuick=INDEX_NONE;PendingRequest.Invalidate();bDirty=true;
}
