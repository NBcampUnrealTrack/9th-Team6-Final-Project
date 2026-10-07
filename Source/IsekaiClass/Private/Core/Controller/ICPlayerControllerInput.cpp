#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Interaction/ICInteractionActor.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "Save/ICSaveSubsystem.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

void AICPlayerController::SetupInputComponent()
{
    IC_TRACE_FUNCTION();
    Super::SetupInputComponent();
    auto* Input=Cast<UEnhancedInputComponent>(InputComponent);
    if (!Input) { UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] Controller=%s: EnhancedInputComponent가 필요합니다."),*GetNameSafe(this));return; }
    if (MoveAction && MoveAction->ValueType==EInputActionValueType::Axis2D) Input->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ThisClass::Move);
    else if(MoveAction)UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] MoveAction=%s: Axis2D가 필요합니다."),*GetPathNameSafe(MoveAction));
    if (LookAction && LookAction->ValueType==EInputActionValueType::Axis2D) Input->BindAction(LookAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
    else if(LookAction)UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] LookAction=%s: Axis2D가 필요합니다."),*GetPathNameSafe(LookAction));
    if(!MoveAction||!LookAction)UE_LOG(LogICInput,Warning,TEXT("[Input.Setup.Incomplete] Move=%s Look=%s: 이동/카메라 Input Action을 확인하세요."),*GetPathNameSafe(MoveAction),*GetPathNameSafe(LookAction));
    TSet<const UInputAction*> BoundActions;
    for (const FICInputCommandBinding& Binding:CommandActions)
    {
        if (!Binding.Action) continue;
        if (Binding.Action==MoveAction || Binding.Action==LookAction || BoundActions.Contains(Binding.Action))
        { UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] Action=%s Command=%d: Input Action 중복 연결"),*Binding.Action->GetPathName(),static_cast<int32>(Binding.Command));continue; }
        BoundActions.Add(Binding.Action);
        Input->BindAction(Binding.Action,ETriggerEvent::Started,this,&ThisClass::HandleInputCommand,Binding.Command,Binding.Slot);
        Input->BindAction(Binding.Action,ETriggerEvent::Completed,this,&ThisClass::ReleaseInputCommand,Binding.Command,Binding.Slot);
        Input->BindAction(Binding.Action,ETriggerEvent::Canceled,this,&ThisClass::CancelInputCommand,Binding.Command,Binding.Slot);
    }
}

bool AICPlayerController::CanHandleGameplayInput() const
{ IC_TRACE_FUNCTION(); return IsLocalController() && bCreated && !bDead && Menu.IsEmpty() && !IsPaused() && GetPawn(); }

void AICPlayerController::FlushPressedKeys()
{
    IC_TRACE_FUNCTION();
    ResetHeldInput();
    Super::FlushPressedKeys();
}

void AICPlayerController::Move(const FInputActionValue& Value)
{
    IC_TRACE_FUNCTION();
    if (!CanHandleGameplayInput()) return;
    if (auto* TargetCharacter=Cast<AICPlayerCharacter>(GetPawn()))
    {
        const FVector2D Axis=Value.Get<FVector2D>().GetClampedToMaxSize(1.f);
        const FRotator Yaw(0,GetControlRotation().Yaw,0);
        TargetCharacter->AddMovementInput(Yaw.Vector(),Axis.Y);
        TargetCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y),Axis.X);
    }
}

void AICPlayerController::Look(const FInputActionValue& Value)
{
    IC_TRACE_FUNCTION();
    if (!CanHandleGameplayInput()) return;
    if (const auto* TargetCharacter=Cast<AICPlayerCharacter>(GetPawn()))
    {
        const FVector2D Axis=Value.Get<FVector2D>();
        FRotator Rotation=GetControlRotation();
        Rotation.Yaw+=Axis.X*TargetCharacter->CameraSensitivity;
        Rotation.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch)+Axis.Y*TargetCharacter->CameraSensitivity,-65.f,65.f);
        SetControlRotation(Rotation);
    }
}

void AICPlayerController::PlayerTick(float DeltaSeconds)
{
    IC_TRACE_FUNCTION();
    Super::PlayerTick(DeltaSeconds);
    if (!IsLocalController() || IsMainMenuLevel()) return;
    TryInitializeGameplay();SyncSaveSafety();
    if (!CanHandleGameplayInput()) ResetHeldInput();
    else
    {
        const double Now=FPlatformTime::Seconds();
        if(bSprintKeyHeld && Now-SprintPressedAt>=SprintHoldSeconds) bSprinting=true;
        if(bTargetKeyHeld && !bTargetHoldHandled && Now-TargetPressedAt>=TargetHoldSeconds)
        { bTargetHoldHandled=true;HandleInputCommand(EICInputCommand::ClearTarget); }
    }
    if (bDirty) { bDirty=false;UpdateScreens(); }
    const auto* Target=CanHandleGameplayInput()?FindInteractionTarget():nullptr;
    const FString Hint=Target?Target->GetInteractionText().ToString():FString();
    if (Hint!=InteractionHint) { InteractionHint=Hint;bDirty=true; }
    if (auto* TargetCharacter=Cast<AICPlayerCharacter>(GetPawn()))
        TargetCharacter->GetCharacterMovement()->MaxWalkSpeed=TargetCharacter->FinalMoveSpeed*
            (bSprinting?1.f:TargetCharacter->WalkSpeedMultiplier)*(Survival.IsExhausted()?ExhaustedMoveSpeedMultiplier:1.f);
}

void AICPlayerController::ResetHeldInput()
{
    IC_TRACE_FUNCTION();
    bSprintKeyHeld=false;bSprinting=false;bTargetKeyHeld=false;bTargetHoldHandled=false;
    ReleaseInputCommand(EICInputCommand::Block);
    ReleaseInputCommand(EICInputCommand::Jump);
}

void AICPlayerController::CancelInputCommand(EICInputCommand Command,int32 Slot)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInput, Verbose, TEXT("[Input.Cancelled] Command=%d Slot=%d"),static_cast<int32>(Command),Slot);
    // Canceled input (focus loss or menu changes) must never execute tap actions.
    if(Command==EICInputCommand::SprintDodge){bSprintKeyHeld=false;bSprinting=false;return;}
    if(Command==EICInputCommand::TargetToggle){bTargetKeyHeld=false;bTargetHoldHandled=false;return;}
    ReleaseInputCommand(Command,Slot);
}

void AICPlayerController::ReleaseInputCommand(EICInputCommand Command,int32 Slot)
{
    IC_TRACE_FUNCTION();
    if(Command==EICInputCommand::SprintDodge)
    {
        const bool bTap=bSprintKeyHeld && FPlatformTime::Seconds()-SprintPressedAt<SprintHoldSeconds;
    UE_LOG(LogICInput, Verbose, TEXT("[Input.ShiftReleased] Tap=%d Sprinting=%d"),bTap,bSprinting);
        bSprintKeyHeld=false;bSprinting=false;
        if(bTap && CanHandleGameplayInput())HandleInputCommand(EICInputCommand::Dodge);
        return;
    }
    if(Command==EICInputCommand::TargetToggle)
    {
        const bool bWasHeld=bTargetKeyHeld;
        const bool bHold=bTargetHoldHandled || FPlatformTime::Seconds()-TargetPressedAt>=TargetHoldSeconds;
        const bool bAlreadyHandled=bTargetHoldHandled;
    UE_LOG(LogICInput, Verbose, TEXT("[Input.TabReleased] Held=%d HoldAction=%d AlreadyHandled=%d"),bWasHeld,bHold,bAlreadyHandled);
        bTargetKeyHeld=false;bTargetHoldHandled=false;
        if(bWasHeld && CanHandleGameplayInput() && !bAlreadyHandled)
            HandleInputCommand(bHold?EICInputCommand::ClearTarget:EICInputCommand::NextTarget);
        return;
    }
    if (Command==EICInputCommand::Block && bBlocking)
    { bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0); }
    if (Command==EICInputCommand::Jump)
        if (auto* TargetCharacter=Cast<ACharacter>(GetPawn())) TargetCharacter->StopJumping();
}

void AICPlayerController::HandleInputCommand(EICInputCommand Command,int32 Slot)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInput, Verbose, TEXT("[Input.Command] Command=%d Slot=%d Menu=%s Created=%d Dead=%d"),static_cast<int32>(Command),Slot,*Menu,bCreated,bDead);
    if (!IsLocalController() || Slot<0 || Slot>3) return;
    if (Menu==TEXT("Confirm"))
    { if(Command==EICInputCommand::Confirm)ConfirmModal(true);else if(Command==EICInputCommand::Menu)ConfirmModal(false);return; }
    if (Menu==TEXT("Quit")) { if(Command==EICInputCommand::Menu)CloseSessionMenu();return; }
    if (Menu==TEXT("Save"))
    {
        if(Command==EICInputCommand::QuickSlot)ExecuteUI(TEXT("SaveSlot"),Slot+1);
        else if(Command==EICInputCommand::Confirm)RequestManualSave();
        else if(Command==EICInputCommand::Menu)CloseSessionMenu();
        return;
    }
    if(Command==EICInputCommand::Load){LoadPlayer();return;}
    if(Command==EICInputCommand::Menu)
    {
        if(!PendingItem.IsNone())CancelTarget();
        else if(!Menu.IsEmpty())CloseSessionMenu();
        else if(bCreated&&!bDead)SetMenu(TEXT("Pause"));
        return;
    }
    if(Command==EICInputCommand::Save){OpenSaveMenu();return;}
    if(!bCreated||bDead)return;
    const TCHAR* TargetMenu=nullptr;
    switch(Command)
    {
        case EICInputCommand::Inventory:TargetMenu=TEXT("Inventory");break;
        case EICInputCommand::Equipment:TargetMenu=TEXT("Equipment");break;
        case EICInputCommand::Stats:TargetMenu=TEXT("Stats");break;
        case EICInputCommand::Skills:TargetMenu=TEXT("Skills");break;
        case EICInputCommand::Codex:TargetMenu=TEXT("Codex");break;
        case EICInputCommand::Quest:TargetMenu=TEXT("Quest");break;
        case EICInputCommand::Map:TargetMenu=TEXT("Map");break;
        case EICInputCommand::Party:TargetMenu=TEXT("Party");break;
        default:break;
    }
    if(TargetMenu){SetMenu(Menu==TargetMenu?TEXT(""):TargetMenu);return;}
    if(!PendingItem.IsNone())
    {
        if(Command==EICInputCommand::QuickSlot&&Menu.IsEmpty()&&GetInventory())
        {if(Slot==PendingQuick)ConfirmTarget(UICWorldStateSubsystem::PlayerId());else BeginItem(GetInventory()->GetSnapshot().QuickSlots[Slot],Slot);}
        if(Command==EICInputCommand::CompanionSkill)
        {if(PendingPartyKey==Slot)ConfirmTarget(UICWorldStateSubsystem::PlayerId());else{PendingPartyKey=Slot;ConfirmPartyTarget(Slot);}}
        return;
    }
    if(!CanHandleGameplayInput())return;
    if(Command==EICInputCommand::SprintDodge)
    { if(!bSprintKeyHeld){bSprintKeyHeld=true;SprintPressedAt=FPlatformTime::Seconds();}return; }
    if(Command==EICInputCommand::TargetToggle)
    { if(!bTargetKeyHeld){bTargetKeyHeld=true;bTargetHoldHandled=false;TargetPressedAt=FPlatformTime::Seconds();}return; }
    if(Command==EICInputCommand::QuickSlot)
    {if(GetInventory())BeginItem(GetInventory()->GetSnapshot().QuickSlots[Slot],Slot);return;}
    if(Command==EICInputCommand::Jump){if(auto* TargetCharacter=Cast<ACharacter>(GetPawn()))TargetCharacter->Jump();return;}
    if(Command==EICInputCommand::Interact){Interact();return;}
    FName Action=NAME_None;
    switch(Command)
    {
        case EICInputCommand::Attack:Action=TEXT("Attack");break;
        case EICInputCommand::Block:Action=TEXT("BlockStart");bBlocking=true;break;
        case EICInputCommand::Dodge:Action=TEXT("Dodge");break;
        case EICInputCommand::NextTarget:Action=TEXT("NextTarget");break;
        case EICInputCommand::ClearTarget:Action=TEXT("ClearTarget");break;
        case EICInputCommand::Skill:Action=TEXT("Skill");break;
        case EICInputCommand::CompanionSkill:Action=TEXT("CompanionSkill");break;
        default:break;
    }
    if(!Action.IsNone())
    {
        if(!OnCombatRequest.IsBound()){UE_LOG(LogICInput, Warning, TEXT("[Input.Combat.Unbound] Action=%s Slot=%d: OnCombatRequest 실행 경로가 연결되지 않았습니다."),*Action.ToString(),Slot);bBlocking=false;Show(TEXT("전투 동작이 연결되지 않았습니다."));return;}
        UE_LOG(LogICInput, Verbose, TEXT("[Input.Combat.Request] Action=%s Slot=%d"),*Action.ToString(),Slot);
        OnCombatRequest.Broadcast(Action,Slot);
    }
}
AICInteractionActor* AICPlayerController::FindInteractionTarget() const
{
    IC_TRACE_FUNCTION();
    const auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());if(!PlayerPawn||!GetWorld())return nullptr;
    const FVector Origin=PlayerPawn->GetActorLocation();
    const FVector Forward=FRotator(0,GetControlRotation().Yaw,0).Vector();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerInteraction),false);Params.AddIgnoredActor(PlayerPawn);
    auto Eligible=[&](AICInteractionActor* Object)
    {
        IC_TRACE_FUNCTION();
        if(!IsValid(Object))return false;
        const FVector Offset=Object->GetActorLocation()-Origin;
        if(Offset.Size()>250.f||FVector::DotProduct(Forward,Offset.GetSafeNormal2D())<0.35f)return false;
        FHitResult Occlusion;
        GetWorld()->LineTraceSingleByChannel(Occlusion,PlayerPawn->GetPawnViewLocation(),Object->GetActorLocation(),ECC_Visibility,Params);
        return !Occlusion.bBlockingHit||Occlusion.GetActor()==Object;
    };
    FHitResult Hit;
    const FVector Camera=PlayerPawn->Camera->GetComponentLocation();
    GetWorld()->LineTraceSingleByChannel(Hit,Camera,Camera+PlayerPawn->Camera->GetForwardVector()*1000.f,ECC_Visibility,Params);
    if(auto* Exact=Cast<AICInteractionActor>(Hit.GetActor());Eligible(Exact))return Exact;
    AICInteractionActor* Best=nullptr;float BestScore=MAX_flt;
    for(TActorIterator<AICInteractionActor> It(GetWorld());It;++It)
    {
        auto* Object=*It;if(!Eligible(Object))continue;
        const FVector Offset=Object->GetActorLocation()-Origin;
        const float Score=Offset.Size()-100.f*FVector::DotProduct(Forward,Offset.GetSafeNormal2D());
        if(Score<BestScore){Best=Object;BestScore=Score;}
    }
    return Best;
}
void AICPlayerController::Interact()
{
    IC_TRACE_FUNCTION();
    if(auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn()))
    {
        FHitResult Hit;FCollisionQueryParams Params;Params.AddIgnoredActor(PlayerPawn);
        const FVector Start=PlayerPawn->Camera->GetComponentLocation();
        GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+PlayerPawn->Camera->GetForwardVector()*1000.f,ECC_Visibility,Params);
        if(auto* Dropped=Cast<AICDroppedItem>(Hit.GetActor());Dropped&&FVector::Dist(PlayerPawn->GetActorLocation(),Dropped->GetActorLocation())<=250.f)
        {Dropped->TryPickup(this);return;}
    }

    auto* Object=FindInteractionTarget();
    if(!Object)
    {
        bool bAny=false,bNear=false,bFront=false;
        if(GetPawn())for(TActorIterator<AICInteractionActor> It(GetWorld());It;++It)
        {
            bAny=true;const FVector Offset=It->GetActorLocation()-GetPawn()->GetActorLocation();
            if(Offset.Size()<=250.f){bNear=true;bFront|=FVector::DotProduct(FRotator(0,GetControlRotation().Yaw,0).Vector(),Offset.GetSafeNormal2D())>=0.35f;}
        }
        Show(!bAny?TEXT("이 맵에 상호작용 상자가 없습니다."):!bNear?TEXT("상자에 250cm 이내로 접근하세요."):!bFront?TEXT("상자가 있는 방향을 바라보세요."):TEXT("상자가 가려져 있습니다. 벽 너머에서는 사용할 수 없습니다."));return;
    }
    UE_LOG(LogICInteraction, Verbose, TEXT("[Interaction.Request] Object=%s"),*GetPathNameSafe(Object));
    if(!Object->TryInteract(this))Show(TEXT("현재 이 대상과 상호작용할 수 없습니다. 액터 설정을 확인하세요."));
}
