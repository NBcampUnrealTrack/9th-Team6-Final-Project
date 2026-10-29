#include "Life/Production/ICPlayerLifeComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/Controller/ICPlayerController.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Data/Rows/Life/ICProductionRow.h"
#include "UI/Life/ICLifeWidgets.h"
#include "Life/World/ICPlayerHomeActors.h"
#include "Life/World/ICFishingLine.h"
#include "Life/World/ICLifeActor.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "NPC/LifeObject/ICLifeObjectSubsystem.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/DataTable.h"

UICPlayerLifeComponent::UICPlayerLifeComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=false;
    FishingLineClass=TSoftClassPtr<AICFishingLine>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Life/Fishing/Blueprints/FishingPole/BP_IC_FishingLine.BP_IC_FishingLine_C")));
    SleepPresentationTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Life/DT_IC_SleepPresentation.DT_IC_SleepPresentation")));
}
AICLifeActor* UICPlayerLifeComponent::ResolveFishingCast(FVector& Landing) const
{
    Landing=FVector::ZeroVector;
    const auto* Player=Cast<AICPlayerCharacter>(GetOwner());if(!Player||!GetWorld()||!Player->Camera)return nullptr;
    const FVector Origin=Player->Camera->GetComponentLocation(),Direction=Player->Camera->GetForwardVector();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FishingCast),false);Query.AddIgnoredActor(Player);
    // The visible water plane defines the zone; its old small interaction cube is irrelevant to casting.
    for(TActorIterator<AICLifeActor> It(GetWorld());It;++It)if(It->LifeType==EICDefinitionType::FishingPoint)Query.AddIgnoredActor(*It);
    AICLifeActor* Best=nullptr;double BestDistance=MAX_dbl;
    for(TActorIterator<AICLifeActor> It(GetWorld());It;++It)
    {
        if(It->LifeType!=EICDefinitionType::FishingPoint)continue;
        FVector Point;if(!It->ResolveFishingLanding(Origin,Direction,Point))continue;
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Origin,Point,ECC_Visibility,Query))
        {
            // A shallow cast can hit the real water well before the authored plane.
            // Accept only an upward-facing surface within the zone's water-height tolerance.
            if(Hit.ImpactNormal.Z<.5f||!It->ContainsFishingLanding(Hit.ImpactPoint))continue;
            Point=Hit.ImpactPoint;
        }
        const double Distance=FVector::Distance(Origin,Point);if(Distance>=BestDistance)continue;
        if(FVector::DistSquared(Player->GetActorLocation(),Point)>FMath::Square(FishingCastRangeCm))continue;
        Best=*It;BestDistance=Distance;Landing=Point;
    }
    return Best;
}
bool UICPlayerLifeComponent::AttachFishingLine(AICLifeActor* Zone,const FVector& Landing)
{
    auto* Rod=Cast<AICWeaponActor>(Tool);if(!bWorking||!Rod||!IsValid(Zone)||!Zone->ContainsFishingLanding(Landing))return false;
    auto* Type=FishingLineClass.LoadSynchronous();
    FActorSpawnParameters Params;Params.Owner=GetOwner();Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    FishingLine=GetWorld()->SpawnActor<AICFishingLine>(Type?Type:AICFishingLine::StaticClass(),FTransform::Identity,Params);
    if(!FishingLine)return false;
    FishingLine->InitializeLine(Rod,Zone,Landing);
    return FishingLine->HasValidLanding();
}
bool UICPlayerLifeComponent::BeginWork(EICLifeActivity Activity,const FICProductionRow* Production,AActor* Focus)
{
    auto* Player=Cast<AICPlayerCharacter>(GetOwner());if(!Player||bWorking||bSleeping)return false;
    if(auto* ASC=Player->GetICAbilitySystem())ASC->CancelAction();
    Player->CancelWeaponTransition();Player->GetCharacterMovement()->StopMovementImmediately();
    if(Focus){const FVector Direction=(Focus->GetActorLocation()-Player->GetActorLocation()).GetSafeNormal2D();if(!Direction.IsNearlyZero())Player->SetActorRotation(Direction.Rotation());}
    bWorking=true;SetComponentTickEnabled(true);
    auto* Profile=Player->GetAnimationProfile();WorkMontage=Profile?Profile->LifeActions.FindRef(Activity):nullptr;
    if(!WorkMontage&&Production)WorkMontage=Production->WorkMontage.LoadSynchronous();
    if(auto* Anim=Player->GetMesh()->GetAnimInstance();Anim&&WorkMontage)Anim->Montage_Play(WorkMontage);
    if(Production)if(auto* ToolClass=Production->WorkToolClass.LoadSynchronous())
    {
        FActorSpawnParameters Params;Params.Owner=Player;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Tool=GetWorld()->SpawnActor<AActor>(ToolClass,Player->GetActorTransform(),Params);
        if(Tool){Tool->SetActorEnableCollision(false);Tool->AttachToComponent(Player->GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,Production->WorkToolSocket);Tool->SetActorRelativeTransform(Production->WorkToolTransform);}
    }
    if(Player->EquippedWeaponActor)Player->EquippedWeaponActor->SetActorHiddenInGame(true);
    return true;
}
void UICPlayerLifeComponent::EndWork()
{
    auto* Player=Cast<AICPlayerCharacter>(GetOwner());
    if(Player&&WorkMontage)if(auto* Anim=Player->GetMesh()->GetAnimInstance())Anim->Montage_Stop(.15f,WorkMontage);
    WorkMontage=nullptr;if(Tool)Tool->Destroy();Tool=nullptr;bWorking=false;
    if(FishingLine)FishingLine->Destroy();FishingLine=nullptr;
    if(Fade)Fade->RemoveFromParent();Fade=nullptr;
    if(bSleeping&&Player)
    {
        if(bSleepPoseActive)SetSleepPresentation(false);
        if(auto* PC=Cast<AICPlayerController>(Player->GetController()))PC->bInEvent=false;
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        if(SleepingLifeBed.IsValid())if(auto* Life=UICLifeObjectSubsystem::Get(SleepingLifeBed.Get()))Life->UnlockForPlayer(*SleepingLifeBed);
    }
    SleepingLifeBed.Reset();
    bSleeping=bSleepPoseActive=false;SetComponentTickEnabled(false);
    if(SleepCamera)
    {
        if(Player)if(auto* PC=Cast<AICPlayerController>(Player->GetController()))
        {
            PC->SetViewTarget(SleepPreviousViewTarget.IsValid()?SleepPreviousViewTarget.Get():Player);
            if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
        }
        SleepCamera->Destroy();SleepCamera=nullptr;SleepPreviousViewTarget.Reset();
    }
    if(Player)Player->RefreshWeaponAppearance();
}
bool UICPlayerLifeComponent::BeginSleep(AActor* Bed)
{
    auto* Player=Cast<AICPlayerCharacter>(GetOwner());auto* PC=Player?Cast<AICPlayerController>(Player->GetController()):nullptr;
    if(!PC||!IsValid(Bed)||bWorking||bSleeping||!PC->bCreated||PC->bDead||PC->bInCombat||PC->bInDialogue||PC->bInEvent||!PC->Menu.IsEmpty())return false;
    if(FVector::DistSquared(Player->GetActorLocation(),Bed->GetActorLocation())>FMath::Square(350.f))return false;
    if(auto* ASC=Player->GetICAbilitySystem())ASC->CancelAction();Player->CancelWeaponTransition();
    if(auto* Anim=Player->GetMesh()->GetAnimInstance())Anim->StopAllMontages(.1f);
    WakeRotation=Player->GetActorRotation();
    BedTransform=Bed->GetActorTransform();BedTransform.SetScale3D(FVector::OneVector);
    WakePosition=BedTransform.TransformPosition(FVector(-160,0,0));
    if(const auto* PlayerBed=Cast<AICPlayerBed>(Bed))BedTransform=PlayerBed->SleepPoseOffset*BedTransform;
    if(auto* LifeBed=Cast<AICLifeObjectActor>(Bed))
    {SleepingLifeBed=LifeBed;WakePosition=LifeBed->GetActorTransform().TransformPosition(LifeBed->PlayerWakeOffset);BedTransform=LifeBed->PlayerSleepPoseOffset*BedTransform;}
    Player->GetCharacterMovement()->StopMovementImmediately();Player->GetCharacterMovement()->DisableMovement();
    PC->bInEvent=true;bSleeping=true;bSleepPoseActive=false;SleepElapsed=0;bSleepCommitted=false;SetComponentTickEnabled(true);
    SleepRules=FICSleepPresentationRow();if(auto* Table=SleepPresentationTable.LoadSynchronous())if(const auto* Row=Table->FindRow<FICSleepPresentationRow>(TEXT("Default"),TEXT("Sleep"),false))SleepRules=*Row;
    SleepPreviousViewTarget=PC->GetViewTarget();
    const FVector Focus=BedTransform.TransformPosition(SleepRules.CameraFocusOffset),Position=BedTransform.TransformPosition(SleepRules.CameraOffset);
    FActorSpawnParameters Params;Params.Owner=Player;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SleepCamera=GetWorld()->SpawnActor<ACameraActor>(Position,(Focus-Position).Rotation(),Params);
    if(SleepCamera)
    {
        auto* Camera=SleepCamera->GetCameraComponent();Camera->SetFieldOfView(FMath::Clamp(SleepRules.FieldOfView,30.f,100.f));Camera->bConstrainAspectRatio=false;
        Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;Camera->PostProcessSettings.MotionBlurAmount=0;
    }
    auto* Type=LoadClass<UICSleepFadeWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SleepFade.WBP_IC_SleepFade_C"));
    Fade=CreateWidget<UUserWidget>(PC,Type);
    if(Fade){Fade->SetVisibility(ESlateVisibility::HitTestInvisible);Fade->SetRenderOpacity(0);Fade->AddToViewport(1000);}
    return true;
}
FTransform UICPlayerLifeComponent::GetSleepAnchor() const
{
    return BedTransform;
}
void UICPlayerLifeComponent::SetSleepPresentation(bool bInBed)
{
    auto* Player=Cast<AICPlayerCharacter>(GetOwner());if(!Player)return;
    auto* PC=Cast<AICPlayerController>(Player->GetController());
    bSleepPoseActive=bInBed;
    // Both pose and camera cuts occur under full black, never as a visible root-position blend.
    Player->SetActorLocationAndRotation(bInBed?BedTransform.GetLocation():WakePosition,bInBed?BedTransform.Rotator():WakeRotation,false,nullptr,ETeleportType::TeleportPhysics);
    Player->UpdatePresentationTransform(0.f,bInBed?&BedTransform:nullptr);
    if(PC)
    {
        AActor* Target=bInBed&&SleepCamera?SleepCamera.Get():SleepPreviousViewTarget.IsValid()?SleepPreviousViewTarget.Get():Player;
        PC->SetViewTarget(Target);
        if(PC->PlayerCameraManager){PC->PlayerCameraManager->SetGameCameraCutThisFrame();PC->PlayerCameraManager->UpdateCamera(0);}
    }
}
void UICPlayerLifeComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta,TickType,Tick);auto* Player=Cast<AICPlayerCharacter>(GetOwner());if(!Player){EndWork();return;}
    if(Player->EquippedWeaponActor)Player->EquippedWeaponActor->SetActorHiddenInGame(true);
    if(bSleeping)
    {
        auto* PC=Cast<AICPlayerController>(Player->GetController());if(!PC||PC->bDead){EndWork();return;}
        SleepElapsed+=FMath::Max(0.f,Delta);
        const float Out=FMath::Max(.1f,SleepRules.FadeOutSeconds),In=FMath::Max(.1f,SleepRules.FadeInSeconds),Black=FMath::Max(0.f,SleepRules.BlackHoldSeconds);
        const float BedAt=Out,RevealBedAt=BedAt+Black,RestAt=RevealBedAt+In;
        const float FadeBedAt=RestAt+FMath::Max(.1f,SleepRules.EnterSeconds),WakeAt=FadeBedAt+Out,RevealAwakeAt=WakeAt+Black;
        const float Opacity=SleepElapsed<BedAt?SleepElapsed/Out:SleepElapsed<RevealBedAt?1.f:SleepElapsed<RestAt?1.f-(SleepElapsed-RevealBedAt)/In:SleepElapsed<FadeBedAt?0.f:SleepElapsed<WakeAt?(SleepElapsed-FadeBedAt)/Out:SleepElapsed<RevealAwakeAt?1.f:1.f-(SleepElapsed-RevealAwakeAt)/In;
        bool bPoseCut=false;
        if(!bSleepPoseActive&&!bSleepCommitted&&SleepElapsed>=BedAt)
        {
            if(Fade)Fade->SetRenderOpacity(1.f);
            SetSleepPresentation(true);
            bPoseCut=true;
        }
        if(!bSleepCommitted&&SleepElapsed>=WakeAt)
        {
            if(Fade)Fade->SetRenderOpacity(1.f);
            bSleepCommitted=true;
            SetSleepPresentation(false);
            bPoseCut=true;
            if(!PC->Sleep()){EndWork();return;}
        }
        if(Fade)Fade->SetRenderOpacity(bPoseCut?1.f:FMath::Clamp(Opacity,0.f,1.f));
        if(SleepElapsed>=RevealAwakeAt+In){EndWork();if(PC->GetWorldState())PC->GetWorldState()->FlushParticipants();PC->AutoSavePlayer();}
    }
    else if(bWorking&&WorkMontage)if(auto* Anim=Player->GetMesh()->GetAnimInstance();Anim&&!Anim->Montage_IsPlaying(WorkMontage))Anim->Montage_Play(WorkMontage);
}
void UICPlayerLifeComponent::EndPlay(const EEndPlayReason::Type Reason)
{if(SleepCamera)SleepCamera->Destroy();SleepCamera=nullptr;if(FishingLine)FishingLine->Destroy();FishingLine=nullptr;if(Tool)Tool->Destroy();Tool=nullptr;if(Fade)Fade->RemoveFromParent();Fade=nullptr;bWorking=bSleeping=false;Super::EndPlay(Reason);}
