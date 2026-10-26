#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICText.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Interaction/ICInteractionActor.h"
#include "NPC/Character/ICNPCBase.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "Save/ICSaveSubsystem.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
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
    if(!CameraZoomAction)
    {
        CameraZoomAction=NewObject<UInputAction>(this);
        CameraZoomAction->ValueType=EInputActionValueType::Axis1D;
        CameraMappingContext=NewObject<UInputMappingContext>(this);
        CameraMappingContext->MapKey(CameraZoomAction,EKeys::MouseWheelAxis);
    }
    Input->BindAction(CameraZoomAction,ETriggerEvent::Triggered,this,&ThisClass::ZoomCamera);
    if(IsLocalController()&&!IsMainMenuLevel())
        if(auto* Local=GetLocalPlayer())
            if(auto* Subsystem=Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                Subsystem->AddMappingContext(CameraMappingContext,MappingPriority);
    if (MoveAction && MoveAction->ValueType==EInputActionValueType::Axis2D) Input->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ThisClass::Move);
    else if(MoveAction)UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] MoveAction=%s: Axis2D가 필요합니다."),*GetPathNameSafe(MoveAction));
    if (LookAction && LookAction->ValueType==EInputActionValueType::Axis2D) Input->BindAction(LookAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
    else if(LookAction)UE_LOG(LogICInput,Error,TEXT("[Input.Setup.Failed] LookAction=%s: Axis2D가 필요합니다."),*GetPathNameSafe(LookAction));
    if(!MoveAction||!LookAction)UE_LOG(LogICInput,Warning,TEXT("[Input.Setup.Incomplete] Move=%s Look=%s: 이동/카메라 Input Action을 확인하세요."),*GetPathNameSafe(MoveAction),*GetPathNameSafe(LookAction));
    if(MoveAction)
    {
        Input->BindAction(MoveAction,ETriggerEvent::Completed,this,&ThisClass::StopMove);
        Input->BindAction(MoveAction,ETriggerEvent::Canceled,this,&ThisClass::StopMove);
    }
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
        const FRotator InputYaw(0,GetControlRotation().Yaw,0);
        TargetCharacter->MovementIntent=(InputYaw.Vector()*Axis.Y+FRotationMatrix(InputYaw).GetUnitAxis(EAxis::Y)*Axis.X).GetClampedToMaxSize(1.f);
        if(Axis.IsNearlyZero())return;
        auto* MoveASC=TargetCharacter->GetICAbilitySystem();
        if(!MoveASC||!MoveASC->HandleMovementInput())return;
        const FRotator Yaw(0,GetControlRotation().Yaw,0);
        TargetCharacter->AddMovementInput(Yaw.Vector(),Axis.Y);
        TargetCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y),Axis.X);
    }
}

void AICPlayerController::StopMove(const FInputActionValue& Value)
{
    if(auto* MoveCharacter=Cast<AICPlayerCharacter>(GetPawn()))MoveCharacter->MovementIntent=FVector::ZeroVector;
}

void AICPlayerController::Look(const FInputActionValue& Value)
{
    IC_TRACE_FUNCTION();
    if (!CanHandleGameplayInput()) return;
    if (const auto* TargetCharacter=Cast<AICPlayerCharacter>(GetPawn()))
    {
        const FVector2D Axis=Value.Get<FVector2D>();
        if(!Axis.IsNearlyZero())bCameraLockOn=false;
        FRotator Rotation=GetControlRotation();
        Rotation.Yaw+=Axis.X*TargetCharacter->CameraSensitivity;
        Rotation.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch)+Axis.Y*TargetCharacter->CameraSensitivity,-65.f,65.f);
        SetControlRotation(Rotation);
    }
}


void AICPlayerController::ZoomCamera(const FInputActionValue& Value)
{
    if(!CanHandleGameplayInput())return;
    if(auto* CameraCharacter=Cast<AICPlayerCharacter>(GetPawn()))
        CameraCharacter->AdjustCameraDistance(Value.Get<float>());
}

void AICPlayerController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    if (!IsLocalController() || IsMainMenuLevel()) return;
    TryInitializeGameplay();
    SyncSaveSafety();
    if (!CanHandleGameplayInput()) ResetHeldInput();
    if (bDirty) { bDirty=false; UpdateScreens(); }
    const auto* Target=CanHandleGameplayInput()?FindInteractionTarget():nullptr;
    const auto* NPC=CanHandleGameplayInput()?FindNPCInteractionTarget():nullptr;
    const FString Hint=NPC?NPC->GetInteractionText().ToString():Target?Target->GetInteractionText().ToString():FString();
    if (Hint!=InteractionHint) { InteractionHint=Hint; bDirty=true; }
    auto* C=Cast<AICPlayerCharacter>(GetPawn());
    auto* ASC=C?C->GetICAbilitySystem():nullptr;
    if (!ASC) return;
    const bool CombatNow=ASC->HasRecentCombat();
    if(CombatNow&&!bInCombat&&!CombatTarget.IsValid())SelectCombatTarget(false);
    if(CombatNow)C->bSprintHeld=false;
    if(bInCombat!=CombatNow){SetCombatState(CombatNow);bDirty=true;}
    FICDefinitionResult Result;
    FICCombatSettingsRow Defaults;
    const auto* Settings=ASC->Definitions()?ASC->Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),Result):nullptr;
    if (!Settings) Settings=&Defaults;
    if (CombatTarget.IsValid())
    {
        auto* Selected=CombatTarget.Get();
        auto* SelectedASC=Selected->GetICAbilitySystem();
        if (!SelectedASC || !SelectedASC->IsAlive() || FVector::DistSquared(C->GetActorLocation(),Selected->GetActorLocation())>FMath::Square(Settings->LockOnMaintainDistanceCm))
        { CombatTarget.Reset(); TargetOccludedAt=-1; }
        else
        {
            FHitResult Hit;
            FCollisionQueryParams Query;
            Query.AddIgnoredActor(C);
            Query.AddIgnoredActor(Selected);
            if (GetWorld()->LineTraceSingleByChannel(Hit,C->GetActorLocation(),Selected->GetActorLocation(),ECC_Visibility,Query))
            {
                if (TargetOccludedAt<0) TargetOccludedAt=ASC->PlaySecond();
                if (ASC->PlaySecond()-TargetOccludedAt>=Settings->LockOnOcclusionPlaySeconds) { CombatTarget.Reset(); TargetOccludedAt=-1; }
            }
            else TargetOccludedAt=-1;
        }
    }
    C->GetCharacterMovement()->bOrientRotationToMovement=!CombatNow;
    if(CombatNow&&CanHandleGameplayInput()&&ASC->CanAcceptMovementInput()&&!C->IsSwitchingWeapon())
    {
        const FVector Facing=CombatTarget.IsValid()?CombatTarget->GetActorLocation()-C->GetActorLocation():GetControlRotation().Vector();
        if(!Facing.IsNearlyZero())C->SetActorRotation(FMath::RInterpTo(C->GetActorRotation(),FRotator(0,Facing.Rotation().Yaw,0),DeltaSeconds,12.f));
    }
    if(!CombatTarget.IsValid())bCameraLockOn=false;
    if(CanHandleGameplayInput())
    {
        C->UpdateCameraDistance(DeltaSeconds);
        if(bCameraLockOn&&CombatTarget.IsValid())
        {
            const FVector Direction=CombatTarget->GetActorLocation()-C->Camera->GetComponentLocation();
            if(!Direction.IsNearlyZero())
            {
                FRotator Aim=Direction.Rotation();
                Aim.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch),-65.f,65.f);
                Aim.Roll=0;
                SetControlRotation(FMath::RInterpTo(GetControlRotation(),Aim,DeltaSeconds,5.f));
            }
        }
    }
    C->GetCharacterMovement()->MaxWalkSpeed=ASC->Stat(EICStat::MoveSpeed)*(Survival.IsExhausted()?ExhaustedMoveSpeedMultiplier:1.f)*(C->IsSprinting()?C->SprintSpeedMultiplier:1.f);
    if(!CanHandleGameplayInput()||!ASC->CanAcceptMovementInput())
    {
        C->ConsumeMovementInputVector();
        C->GetCharacterMovement()->MaxWalkSpeed=0;
        // Preserve vertical velocity so an airborne hit does not suspend gravity.
        auto& Velocity=C->GetCharacterMovement()->Velocity;
        Velocity.X=0;Velocity.Y=0;
        C->StopJumping();
        C->bSprintHeld=false;
    }
}

void AICPlayerController::ResetHeldInput()
{
    ReleaseInputCommand(EICInputCommand::Jump);
    ReleaseInputCommand(EICInputCommand::Dodge);
    if(auto* InputCharacter=Cast<AICPlayerCharacter>(GetPawn()))
    {
        InputCharacter->ConsumeMovementInputVector();
        InputCharacter->MovementIntent=FVector::ZeroVector;
        if(auto* InputASC=InputCharacter->GetICAbilitySystem())InputASC->ClearBufferedInput();
        auto& Velocity=InputCharacter->GetCharacterMovement()->Velocity;
        Velocity.X=0;Velocity.Y=0;
    }
}

void AICPlayerController::CancelInputCommand(EICInputCommand Command,int32 Slot)
{
    ReleaseInputCommand(Command,Slot);
}

void AICPlayerController::ReleaseInputCommand(EICInputCommand Command,int32 Slot)
{
    if (Command==EICInputCommand::Dodge || Command==EICInputCommand::SprintDodge)
        if (auto* SprintCharacter=Cast<AICPlayerCharacter>(GetPawn())) SprintCharacter->bSprintHeld=false;
    if (Command==EICInputCommand::Jump)
        if (auto* InputCharacter=Cast<ACharacter>(GetPawn())) InputCharacter->StopJumping();
}

void AICPlayerController::HandleInputCommand(EICInputCommand Command,int32 Slot)
{
    if (!IsLocalController() || Slot<0 || Slot>3) return;
    if (Command==EICInputCommand::CompanionSkill && Slot>=3) return;
    if (Command==EICInputCommand::Skill && Slot!=0) return;
    if (Command==EICInputCommand::Block) return;
    if (Command==EICInputCommand::SprintDodge) Command=EICInputCommand::Dodge;
    if (Command==EICInputCommand::TargetToggle) Command=EICInputCommand::NextTarget;
    if(IsMainMenuLevel()||Menu==TEXT("Create")||Menu==TEXT("Loading"))return;
    if (Menu==TEXT("Confirm"))
    { if(Command==EICInputCommand::Confirm)ConfirmModal(true); else if(Command==EICInputCommand::Menu)ConfirmModal(false); return; }
    if (Menu==TEXT("Quit")) { if(Command==EICInputCommand::Menu)CloseSessionMenu(); return; }
    if (Menu==TEXT("Save") || Menu==TEXT("Load"))
    {
        if(Command==EICInputCommand::QuickSlot)SelectSessionSlot(Slot+1);
        else if(Command==EICInputCommand::Confirm) { if(Menu==TEXT("Save"))RequestManualSave(); else RequestLoadPlayer(); }
        else if(Command==EICInputCommand::Menu)CloseSessionMenu();
        return;
    }
    if (Command==EICInputCommand::Load) { OpenLoadMenu(); return; }
    if (Command==EICInputCommand::Menu)
    {
        if (HasTargetSelection()) CancelTarget();
        else if (!Menu.IsEmpty()) CloseSessionMenu();
        else if (bCreated&&!bDead) SetMenu(TEXT("Pause"));
        return;
    }
    if (Command==EICInputCommand::Save) { OpenSaveMenu(); return; }
    if (!bCreated || bDead) return;
    if (HasTargetSelection())
    {
        if (Command==EICInputCommand::CompanionSkill)
        {
            if (PendingPartyKey==Slot) SelectTarget(UICWorldStateSubsystem::PlayerId());
            else
            {
                PendingPartyKey=Slot;
                const auto State=GetWorldState()?GetWorldState()->ExportState():FICWorldState();
                bool Found=false;
                for (const auto& Pair:State.Characters)
                    if(Pair.Value.Mode==EICCharacterMode::Companion && Pair.Value.PartySlot==Slot)
                    { Found=SelectTarget(Pair.Key); break; }
                if(!Found)ShowLocalized(ICText::Get(TEXT("Native_daafd25225b3")));
            }
        }
        else if (Command==EICInputCommand::Confirm || (Command==EICInputCommand::Skill&&!PendingSkill.IsNone())) ConfirmSelectedTarget();
        else if (Command==EICInputCommand::QuickSlot && !PendingItem.IsNone())
        {
            if(Slot==PendingQuick)ConfirmSelectedTarget();
            else if(GetInventory())BeginItem(GetInventory()->GetSnapshot().QuickSlots[Slot],Slot);
        }
        return;
    }
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
        case EICInputCommand::Settings:TargetMenu=TEXT("Settings");break;
        default:break;
    }
    if(TargetMenu) { SetMenu(Menu==TargetMenu?TEXT(""):TargetMenu); return; }
    if(!CanHandleGameplayInput())return;
    if(Command==EICInputCommand::Dodge)
    {
        if(auto* SprintCharacter=Cast<AICPlayerCharacter>(GetPawn()))
        {
            auto* SprintASC=SprintCharacter->GetICAbilitySystem();
            if(SprintASC&&SprintASC->HasRecentCombat())
            {SprintCharacter->bSprintHeld=false;ExecuteCombatInput(TEXT("Dodge"),Slot);OnCombatRequest.Broadcast(TEXT("Dodge"),Slot);}
            else SprintCharacter->bSprintHeld=true;
        }
        return;
    }
    if(Command==EICInputCommand::QuickSlot) { if(GetInventory())BeginItem(GetInventory()->GetSnapshot().QuickSlots[Slot],Slot); return; }
    if(Command==EICInputCommand::Jump)
    {
        if(auto* InputCharacter=Cast<AICPlayerCharacter>(GetPawn()))
            if(auto* JumpASC=InputCharacter->GetICAbilitySystem();JumpASC&&JumpASC->CanAcceptJumpInput()&&!InputCharacter->IsSwitchingWeapon())
                InputCharacter->Jump();
        return;
    }
    if(Command==EICInputCommand::Interact||Command==EICInputCommand::Fishing)
    {
        auto* InteractionCharacter=Cast<AICPlayerCharacter>(GetPawn());
        auto* InteractionASC=InteractionCharacter?InteractionCharacter->GetICAbilitySystem():nullptr;
        if(!InteractionASC||!InteractionASC->CanAcceptInteractionInput()||InteractionCharacter->IsSwitchingWeapon())return;
        if(Command==EICInputCommand::Interact)Interact();else RequestFishing();
        return;
    }
    if(Command==EICInputCommand::SwitchWeapon)
    { if(auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());PlayerPawn&&PlayerPawn->RequestCycleWeapon()) { FlushWorldState(); bDirty=true; OnCombatRequest.Broadcast(TEXT("SwitchWeapon"),0); } return; }
    FName Action=NAME_None;
    switch(Command)
    {
        case EICInputCommand::Attack:Action=TEXT("Attack");break;
        case EICInputCommand::NextTarget:Action=TEXT("NextTarget");break;
        case EICInputCommand::ClearTarget:Action=TEXT("ClearTarget");break;
        case EICInputCommand::Skill:Action=TEXT("Skill");break;
        case EICInputCommand::CompanionSkill:Action=TEXT("CompanionSkill");break;
        default:break;
    }
    if(!Action.IsNone()) { ExecuteCombatInput(Action,Slot); OnCombatRequest.Broadcast(Action,Slot); }
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
AICNPCBase* AICPlayerController::FindNPCInteractionTarget() const
{
    const AICPlayerCharacter* PlayerPawn = Cast<AICPlayerCharacter>(GetPawn());
    if (!PlayerPawn || !GetWorld()) return nullptr;
    const FVector Origin = PlayerPawn->GetActorLocation();
    const FVector Forward = FRotator(0, GetControlRotation().Yaw, 0).Vector();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerNPCInteraction), false);
    Params.AddIgnoredActor(PlayerPawn);
    AICNPCBase* Closest = nullptr;
    float ClosestDistance = 250.f;
    for (TActorIterator<AICNPCBase> It(GetWorld()); It; ++It)
    {
        AICNPCBase* Candidate = *It;
        if (!Candidate || !Candidate->IsActorInitialized()) continue;
        const FVector Offset = Candidate->GetActorLocation() - Origin;
        const float Distance = Offset.Size();
        if (Distance > ClosestDistance || FVector::DotProduct(Forward, Offset.GetSafeNormal2D()) < 0.35f) continue;
        FHitResult Hit;
        GetWorld()->LineTraceSingleByChannel(Hit, PlayerPawn->GetPawnViewLocation(),
            Candidate->GetActorLocation(), ECC_Visibility, Params);
        if (Hit.bBlockingHit && Hit.GetActor() != Candidate) continue;
        Closest = Candidate;
        ClosestDistance = Distance;
    }
    return Closest;
}

void AICPlayerController::OpenNPCDialogue(AICNPCBase* NPC)
{
    if (!NPC || !DialogueWidgetClass || !GetLocalPlayer())
    {
        if (NPC) NPC->EndInteraction();
        return;
    }
    UICNPCDialogueWidget* Dialogue = CreateWidget<UICNPCDialogueWidget>(this, DialogueWidgetClass);
    UICUIManagerSubsystem* UI = GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    if (!Dialogue || !UI || !UI->OpenScreen(Dialogue, true, 100))
    {
        NPC->EndInteraction();
        return;
    }
    DialogueScreen = Dialogue;
    bInDialogue = true;
    SetMenu(TEXT("Dialogue"));
    Dialogue->OpenForNPC(NPC);
}

void AICPlayerController::HandleNPCInteractionStarted(AICNPCBase* NPC, AICPlayerController* Requester, FName)
{
    if (Requester == this)
    {
        OpenNPCDialogue(NPC);
    }
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

    if (AICNPCBase* NPC = FindNPCInteractionTarget())
    {
        NPC->OnInteractionStarted.AddUniqueDynamic(this, &ThisClass::HandleNPCInteractionStarted);
        NPC->TryInteract(this);
        NPC->OnInteractionStarted.RemoveDynamic(this, &ThisClass::HandleNPCInteractionStarted);
        return;
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
        ShowLocalized(ICText::Get(!bAny?TEXT("Interaction_NoBox"):!bNear?TEXT("Interaction_BoxTooFar"):!bFront?TEXT("Interaction_BoxBehind"):TEXT("Interaction_BoxOccluded")));return;
    }
    UE_LOG(LogICInteraction, Verbose, TEXT("[Interaction.Request] Object=%s"),*GetPathNameSafe(Object));
    if(!Object->TryInteract(this))ShowLocalized(ICText::Get(TEXT("Native_a103f45b5902")));
}


void AICPlayerController::SelectCombatTarget(bool Next)
{
    auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());
    auto* ASC=PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr;
    if(!ASC||!GetWorld())return;
    FVector CameraLocation;
    FRotator CameraRotation;
    GetPlayerViewPoint(CameraLocation,CameraRotation);
    FICDefinitionResult Result;
    FICCombatSettingsRow Defaults;
    const auto* Settings=ASC->Definitions()?ASC->Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),Result):nullptr;
    if(!Settings)Settings=&Defaults;
    struct FCandidate { AICCharacterBase* Actor; double Angle; double Distance; };
    TArray<FCandidate> Targets;
    for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)
    {
        auto* Target=*It;
        auto* TargetASC=Target->GetICAbilitySystem();
        const double Distance=FVector::DistSquared(PlayerCharacter->GetActorLocation(),Target->GetActorLocation());
        if(!PlayerCharacter->IsHostileTo(Target)||!TargetASC||!TargetASC->IsAlive()||Distance>FMath::Square(Settings->LockOnDistanceCm))continue;
        const double Facing=FVector::DotProduct(CameraRotation.Vector(),(Target->GetActorLocation()-CameraLocation).GetSafeNormal());
        if(Facing<=0)continue;
        FHitResult Hit;
        FCollisionQueryParams Query;
        Query.AddIgnoredActor(PlayerCharacter);
        Query.AddIgnoredActor(Target);
        if(GetWorld()->LineTraceSingleByChannel(Hit,CameraLocation,Target->GetActorLocation(),ECC_Visibility,Query))continue;
        Targets.Add({Target,1.-Facing,Distance});
    }
    Targets.Sort([](const FCandidate& A,const FCandidate& B)
    {
        if(!FMath::IsNearlyEqual(A.Angle,B.Angle))return A.Angle<B.Angle;
        if(!FMath::IsNearlyEqual(A.Distance,B.Distance))return A.Distance<B.Distance;
        return A.Actor->GetUniqueID()<B.Actor->GetUniqueID();
    });
    const int32 Current=Targets.IndexOfByPredicate([&](const FCandidate& C){return C.Actor==CombatTarget.Get();});
    CombatTarget=Targets.IsEmpty()?nullptr:Targets[Next&&Current!=INDEX_NONE?(Current+1)%Targets.Num():0].Actor;
    TargetOccludedAt=-1;
    bDirty=true;
}

void AICPlayerController::ExecuteCombatInput(FName Action,int32 Slot)
{
    auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());
    auto* Caster=static_cast<AICCharacterBase*>(PlayerCharacter);
    auto* ASC=Caster?Caster->GetICAbilitySystem():nullptr;
    if(!ASC)return;
    if(Action==TEXT("ClearTarget")) { CombatTarget.Reset(); TargetOccludedAt=-1; bCameraLockOn=false; return; }
    if(Action==TEXT("NextTarget")) { SelectCombatTarget(true); bCameraLockOn=CombatTarget.IsValid(); return; }
    FName SkillID;
    if(Action==TEXT("CompanionSkill"))
    {
        if(Slot<0||Slot>=3)return;
        Caster=nullptr;
        for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)
        {
            const auto State=GetWorldState()->GetSnapshot(It->GetCharacterID());
            if(State.Mode==EICCharacterMode::Companion&&State.PartySlot==Slot&&!State.IsDowned()) { Caster=*It; break; }
        }
        ASC=Caster?Caster->GetICAbilitySystem():nullptr;
        if(!ASC) { ShowLocalized(ICText::Get(TEXT("Native_921b4891409d"))); return; }
        FICDefinitionResult Result;
        const auto* NPC=ASC->Definitions()?ASC->Definitions()->Find<FICNPCRow>(Caster->GetCharacterID(),Result):nullptr;
        const auto* Base=NPC?ASC->Definitions()->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,Result):nullptr;
        if(!Base) { Show(Result.Reason); return; }
        SkillID=Base->UniqueSkillID;
        Action=TEXT("Skill");
    }
    else if(Action==TEXT("Skill"))
    {
        SkillID=ASC->GetCurrentWeaponSkill();
        if(SkillID.IsNone()) { ShowLocalized(ICText::Get(TEXT("Native_3d573f64fc4c"))); return; }
    }
    if(Action==TEXT("Skill"))
    {
        FICDefinitionResult Result;
        const auto* Skill=ASC->Definitions()?ASC->Definitions()->Find<FICSkillRow>(SkillID,Result):nullptr;
        if(!Skill) { Show(Result.Reason); return; }
        if(Skill->Purpose!=EICSkillPurpose::Attack&&Skill->Purpose!=EICSkillPurpose::Utility)
        { BeginSkillTarget(Caster,SkillID); return; }
        if(Skill->TargetRules.Target==EICCombatTarget::Self)
        { const auto Cast=ASC->RequestAction(Action,SkillID,Caster); if(!Cast.bSuccess)Show(Cast.Reason); return; }
    }
    if(!CombatTarget.IsValid())SelectCombatTarget(false);
    const auto Result=ASC->RequestAction(Action,SkillID,CombatTarget.Get());
    if(!Result.bSuccess)Show(Result.Reason);
}

FName AICPlayerController::GetCurrentWeaponSkill() const
{
    const auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());
    const auto* ASC=PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr;
    return ASC?ASC->GetCurrentWeaponSkill():NAME_None;
}

double AICPlayerController::GetCurrentWeaponSkillCooldown() const
{
    const auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());
    const auto* ASC=PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr;
    return ASC?ASC->GetSkillCooldownRemaining(ASC->GetCurrentWeaponSkill()):0.;
}

bool AICPlayerController::EquipWeaponSkill(EICWeaponFamily Family,FName SkillID)
{
    auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());
    auto* ASC=PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr;
    if(!bCreated||bDead||!ASC||!ASC->EquipWeaponSkill(Family,SkillID))return false;
    CancelTarget();
    FlushWorldState();
    bDirty=true;
    return true;
}

void AICPlayerController::RequestFishing()
{
    if(!CanHandleGameplayInput()||HasTargetSelection())return;
    if(OnFishingRequested.IsBound())OnFishingRequested.Broadcast();
    else ShowLocalized(ICText::Get(TEXT("Native_38e9e70ca3e5")));
}
