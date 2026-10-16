#include "Core/Controller/ICPlayerController.h"
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
    const auto* NPC=CanHandleGameplayInput()?FindNPCInteractionTarget():nullptr;
    const FString Hint=NPC?NPC->GetInteractionText().ToString()
        :Target?Target->GetInteractionText().ToString():FString();
    if (Hint!=InteractionHint) { InteractionHint=Hint;bDirty=true; }
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem())
    {
        bInCombat=ASC->HasRecentCombat();
        FICDefinitionResult R;FICCombatSettingsRow Defaults;const auto* Settings=ASC->Definitions()?ASC->Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;if(!Settings)Settings=&Defaults;
        if(CombatTarget.IsValid())
        {
            auto* Selected=CombatTarget.Get();auto* SelectedASC=Selected->GetICAbilitySystem();const double Now=ASC->PlaySecond();
            if(!SelectedASC||!SelectedASC->IsAlive()||FVector::DistSquared(C->GetActorLocation(),Selected->GetActorLocation())>FMath::Square(Settings->LockOnMaintainDistanceCm)){CombatTarget.Reset();TargetOccludedAt=-1;}
            else
            {
                FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(C);Q.AddIgnoredActor(Selected);
                if(GetWorld()->LineTraceSingleByChannel(Hit,C->GetActorLocation(),Selected->GetActorLocation(),ECC_Visibility,Q))
                {if(TargetOccludedAt<0)TargetOccludedAt=Now;if(Now-TargetOccludedAt>=Settings->LockOnOcclusionPlaySeconds){CombatTarget.Reset();TargetOccludedAt=-1;}}
                else TargetOccludedAt=-1;
            }
        }
        const double Run=ASC->Stat(EICStat::MoveSpeed);const double Speed=bSprinting?Run:Run*Settings->WalkSpeedCmPerSecond/FMath::Max(1.,Settings->RunSpeedCmPerSecond);
        C->GetCharacterMovement()->MaxWalkSpeed=Speed*(Survival.IsExhausted()?ExhaustedMoveSpeedMultiplier:1.f)*(bBlocking?Settings->GuardMoveSpeedMultiplier:1.);
    }
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
    { bBlocking=false;ExecuteCombatInput(TEXT("BlockEnd"),0);OnCombatRequest.Broadcast(TEXT("BlockEnd"),0); }
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
        if(Command==EICInputCommand::QuickSlot)SelectSessionSlot(Slot+1);
        else if(Command==EICInputCommand::Confirm)RequestManualSave();
        else if(Command==EICInputCommand::Menu)CloseSessionMenu();
        return;
    }
    if(Menu==TEXT("Load"))
    {
        if(Command==EICInputCommand::QuickSlot)SelectSessionSlot(Slot+1);
        else if(Command==EICInputCommand::Confirm||Command==EICInputCommand::Load)RequestLoadPlayer();
        else if(Command==EICInputCommand::Menu)CloseSessionMenu();
        return;
    }
    if(Command==EICInputCommand::Load){OpenLoadMenu();return;}
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
        ExecuteCombatInput(Action,Slot);
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
        Show(!bAny?TEXT("이 맵에 상호작용 상자가 없습니다."):!bNear?TEXT("상자에 250cm 이내로 접근하세요."):!bFront?TEXT("상자가 있는 방향을 바라보세요."):TEXT("상자가 가려져 있습니다. 벽 너머에서는 사용할 수 없습니다."));return;
    }
    UE_LOG(LogICInteraction, Verbose, TEXT("[Interaction.Request] Object=%s"),*GetPathNameSafe(Object));
    if(!Object->TryInteract(this))Show(TEXT("현재 이 대상과 상호작용할 수 없습니다. 액터 설정을 확인하세요."));
}

void AICPlayerController::ExecuteCombatInput(FName Action,int32 Slot)
{
    IC_TRACE_FUNCTION();auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());auto* ASC=PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr;if(!ASC)return;
    if(Action==TEXT("ClearTarget")){CombatTarget.Reset();TargetOccludedAt=-1;return;}
    if(Action==TEXT("NextTarget"))
    {
        FICDefinitionResult R;FICCombatSettingsRow Defaults;const auto* Settings=ASC->Definitions()?ASC->Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;if(!Settings)Settings=&Defaults;
        TArray<AICCharacterBase*> Targets;for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(PlayerCharacter->IsHostileTo(*It)&&It->GetICAbilitySystem()&&It->GetICAbilitySystem()->IsAlive()&&FVector::DistSquared(PlayerCharacter->GetActorLocation(),It->GetActorLocation())<=FMath::Square(Settings->LockOnDistanceCm))
        {FHitResult Hit;FCollisionQueryParams Q;Q.AddIgnoredActor(PlayerCharacter);Q.AddIgnoredActor(*It);if(!GetWorld()->LineTraceSingleByChannel(Hit,PlayerCharacter->GetActorLocation(),It->GetActorLocation(),ECC_Visibility,Q))Targets.Add(*It);}
        Targets.Sort([PlayerCharacter](const AICCharacterBase& A,const AICCharacterBase& B){return FVector::DistSquared(A.GetActorLocation(),PlayerCharacter->GetActorLocation())<FVector::DistSquared(B.GetActorLocation(),PlayerCharacter->GetActorLocation());});
        const int32 Index=Targets.IndexOfByKey(CombatTarget.Get());CombatTarget=Targets.IsEmpty()?nullptr:Targets[(Index+1)%Targets.Num()];TargetOccludedAt=-1;return;
    }
    FName SkillID;
    if(Action==TEXT("CompanionSkill"))
    {
        AICCharacterBase* Companion=nullptr;for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It){const auto S=GetWorldState()->GetSnapshot(It->GetCharacterID());if(S.PartySlot==Slot&&S.Mode==EICCharacterMode::Companion){Companion=*It;break;}}
        ASC=Companion?Companion->GetICAbilitySystem():nullptr;if(!ASC){Show(TEXT("활성 동료가 없습니다."));return;}
        FICDefinitionResult R;const auto* NPC=ASC->Definitions()->Find<FICNPCRow>(Companion->GetCharacterID(),R);const auto* Base=NPC?ASC->Definitions()->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,R):nullptr;
        if(!Base){Show(R.Reason);return;}SkillID=Base->UniqueSkillID;Action=TEXT("Skill");
    }
    else if(Action==TEXT("Skill"))
    {
        TArray<FName> IDs;ASC->Definitions()->GetDefinitionIDs(EICDefinitionType::Class,IDs);FICDefinitionResult R;
        for(FName ID:IDs)if(const auto* Class=ASC->Definitions()->Find<FICClassRow>(ID,R))if(static_cast<int32>(Class->CharacterClass)-1==Job){const FName Slots[]={Class->SkillSlot1ID,Class->SkillSlot2ID,Class->SkillSlot3ID,Class->SkillSlot4ID};SkillID=Slots[Slot];break;}
        if(SkillID.IsNone()||!ASC->Capture().UnlockedSkillIDs.Contains(SkillID)){Show(TEXT("해당 슬롯의 해금 스킬이 없습니다."));return;}
    }
    AICCharacterBase* Target=CombatTarget.Get();
    if(Action==TEXT("Skill")){FICDefinitionResult R;if(const auto* S=ASC->Definitions()->Find<FICSkillRow>(SkillID,R))if(S->TargetRules.Target==EICCombatTarget::Self)Target=Cast<AICCharacterBase>(ASC->GetAvatarActor());}
    const auto Result=ASC->RequestAction(Action,SkillID,Target);if(!Result.bSuccess){bBlocking=false;Show(Result.Reason);}
}
