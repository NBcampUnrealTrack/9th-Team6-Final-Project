#include "Life/Production/ICGatheringSubsystem.h"
#include "Life/World/ICLifeActor.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Gameplay/ICGatherProgressWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Life/World/ICFishingLine.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Components/TextBlock.h"

void UICGatheringSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UICDefinitionSubsystem>();
    Collection.InitializeDependency<UICTimeSubsystem>();
    auto* State = Collection.InitializeDependency<UICWorldStateSubsystem>();
    State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::CancelGather);
    State->OnCombatDamageReceived.AddUniqueDynamic(this, &ThisClass::OnDamaged);
    // Preload once so pressing F does not synchronously load the WBP.
    ProgressWidgetClass = LoadClass<UICGatherProgressWidget>(nullptr,
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_GatherProgress.WBP_IC_GatherProgress_C"));
}

void UICGatheringSubsystem::Deinitialize()
{
    CancelGather();
    if (auto* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>())
    {
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::CancelGather);
        State->OnCombatDamageReceived.RemoveDynamic(this, &ThisClass::OnDamaged);
    }
    ProgressWidgetClass = nullptr;
    Super::Deinitialize();
}

bool UICGatheringSubsystem::StartGather(AICLifeActor* Resource, AICPlayerController* Player)
{return Resource&&Resource->LifeType!=EICDefinitionType::FishingPoint&&StartAction(Resource,Player,NAME_None);}
bool UICGatheringSubsystem::StartPlant(AICLifeActor* Plot,AICPlayerController* Player,FName CropID)
{
    if(!IsValid(Plot)||!IsValid(Player)||CropID.IsNone())return false;
    FText Failure;
    if(!Plot->CanPlantCrop(Player,CropID,Failure)){Player->ShowLocalized(Failure);return false;}
    return StartAction(Plot,Player,CropID);
}
bool UICGatheringSubsystem::StartFishing(AICLifeActor* Zone,AICPlayerController* Player,FVector Landing)
{return Zone&&Zone->LifeType==EICDefinitionType::FishingPoint&&StartAction(Zone,Player,NAME_None,&Landing);}
bool UICGatheringSubsystem::StartAction(AICLifeActor* Resource,AICPlayerController* Player,FName CropID,const FVector* FishingLanding)
{
    if(!IsGathering())CancelGather();
    if (IsGathering() || !IsValid(Resource) || !IsValid(Player) || !Player->CanHandleGameplayInput()
        || Resource->GetWorld() != Player->GetWorld()) return false;
    auto* Character=Cast<AICPlayerCharacter>(Player->GetPawn());
    if(FishingLanding)
    {
        if(!Character||!Character->LifeActions||!Resource->ContainsFishingLanding(*FishingLanding)
            ||FVector::DistSquared(*FishingLanding,Character->GetActorLocation())>FMath::Square(Character->LifeActions->FishingCastRangeCm))return false;
    }
    else if(FVector::DistSquared(Resource->GetActorLocation(),Player->GetPawn()->GetActorLocation())>FMath::Square(300.f))return false;
    auto* GI = GetGameInstance();
    auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
    auto* Clock = GI->GetSubsystem<UICTimeSubsystem>();
    FICDefinitionResult Result;
    if(!Clock->IsRunning())return false;
    FName ProductionID;EICLifeActivity Activity=EICLifeActivity::Gathering;
    if(Resource->LifeType==EICDefinitionType::Resource)
    {const auto* Row=Definitions->Find<FICResourceRow>(Resource->DefinitionID,Result);if(!Row||Row->Yields.IsEmpty()||!Resource->IsResourceAvailable())return false;ProductionID=Row->ProductionID;Activity=Row->Activity;}
    else if(Resource->LifeType==EICDefinitionType::Crop)
    {
        const auto* Row=Definitions->Find<FICCropRow>(CropID.IsNone()?Resource->DefinitionID:CropID,Result);if(!Row)return false;
        if(CropID.IsNone()){if(!Resource->IsHarvestable())return false;ProductionID=Row->HarvestProductionID;Activity=EICLifeActivity::Harvesting;}
        else
        {
            const auto State=GI->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
            if(!Resource->bPlantingPlot||State.Crops.Contains(Resource->PointKey())||Player->GetInventory()->CountItem(Row->SeedItemID)<1)return false;
            ProductionID=Row->PlantingProductionID;Activity=EICLifeActivity::Planting;
        }
    }
    else if(Resource->LifeType==EICDefinitionType::FishingPoint)
    {
        const auto* Row=Definitions->Find<FICFishingPointRow>(Resource->DefinitionID,Result);if(!Row||Row->FishSelections.IsEmpty())return false;
        if(!Row->FishingRodItemID.IsNone()&&Player->GetInventory()->CountItem(Row->FishingRodItemID)<1){Player->ShowLocalized(NSLOCTEXT("ICFishing","NeedRod","낚싯대가 필요합니다."));return false;}
        ProductionID=Row->ProductionID;Activity=EICLifeActivity::Fishing;
    }
    else return false;
    const auto* Production=ProductionID.IsNone()?nullptr:Definitions->Find<FICProductionRow>(ProductionID,Result);
    if(Production)for(const auto& Tool:Production->Tools)if(Player->GetInventory()->CountItem(Tool.ItemID)<Tool.Count)return false;
    Duration = Production && FMath::IsFinite(Production->DurationPlaySeconds) && Production->DurationPlaySeconds > 0
        ? Production->DurationPlaySeconds : Activity==EICLifeActivity::Fishing?20.:Activity==EICLifeActivity::Planting?2.:5.;
    if(Character&&Character->LifeActions&&!Character->LifeActions->BeginWork(Activity,Production,Resource))return false;
    if(FishingLanding)
    {
        Character->SetActorRotation((*FishingLanding-Character->GetActorLocation()).GetSafeNormal2D().Rotation());
        if(!Character->LifeActions->AttachFishingLine(Resource,*FishingLanding)){Character->LifeActions->EndWork();return false;}
    }
    PendingCrop=CropID;
    ActiveResource = Resource; Controller = Player; GatheringPawn = Player->GetPawn();
    GatheringDefinition = Resource->DefinitionID; GatheringPoint = Resource->PointKey();
    StartLocation = GatheringPawn->GetActorLocation(); StartedAt = Clock->GetClock().PlaySecond;
    if (auto* GatheringCharacter = Cast<ACharacter>(GatheringPawn.Get())) GatheringCharacter->GetCharacterMovement()->StopMovementImmediately();
    UClass* WidgetType = ProgressWidgetClass ? ProgressWidgetClass.Get() : UICGatherProgressWidget::StaticClass();
    ProgressWidget = CreateWidget<UICGatherProgressWidget>(Player, WidgetType);
    if (ProgressWidget)
    {
        ProgressWidget->SetGatherProgress(Resource->GetInteractionText(), 0);
        if(Activity==EICLifeActivity::Fishing)
            if(auto* Hint=Cast<UTextBlock>(ProgressWidget->GetWidgetFromName(TEXT("TXT_GatherCancel"))))
                Hint->SetText(NSLOCTEXT("ICFishing","CancelHint","이동 / H / ESC  낚시 취소"));
        ProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        ProgressWidget->AddToViewport(20);
    }
    return true;
}

void UICGatheringSubsystem::CancelGather()
{
    if(auto* Player=Cast<AICPlayerCharacter>(GatheringPawn.Get());Player&&Player->LifeActions)Player->LifeActions->EndWork();
    if (ProgressWidget) ProgressWidget->RemoveFromParent();
    ProgressWidget = nullptr; ActiveResource.Reset(); Controller.Reset(); GatheringPawn.Reset();PendingCrop=NAME_None;
}

float UICGatheringSubsystem::GetProgress() const
{
    if (!IsGathering()) return 0;
    const double Elapsed = GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().PlaySecond - StartedAt;
    return static_cast<float>(FMath::Clamp(Elapsed / Duration, 0., 1.));
}

void UICGatheringSubsystem::Tick(float DeltaTime)
{
    if (!Controller.IsValid() || !ActiveResource.IsValid() || !GatheringPawn.IsValid()
        || !Controller->CanHandleGameplayInput() || Controller->GetPawn() != GatheringPawn.Get()
        || ActiveResource->GetWorld() != GetWorld() || ActiveResource->DefinitionID != GatheringDefinition
        || ActiveResource->PointKey() != GatheringPoint
        || FVector::DistSquared(StartLocation, GatheringPawn->GetActorLocation()) > FMath::Square(15.f)
        || (ActiveResource->LifeType!=EICDefinitionType::FishingPoint&&FVector::DistSquared(ActiveResource->GetActorLocation(), GatheringPawn->GetActorLocation()) > FMath::Square(300.f)))
    { CancelGather(); return; }
    if(ActiveResource->LifeType==EICDefinitionType::FishingPoint)
    {
        const auto* Character=Cast<AICPlayerCharacter>(GatheringPawn.Get());
        const auto* Line=Character&&Character->LifeActions?Character->LifeActions->GetFishingLine():nullptr;
        if(!IsValid(Line)||!Line->HasValidLanding()){CancelGather();return;}
    }
    const float Progress = GetProgress();
    if (ProgressWidget) ProgressWidget->SetGatherProgress(ActiveResource->GetInteractionText(), Progress);
    if (Progress < 1) return;
    auto* Resource = ActiveResource.Get(); auto* Player = Controller.Get();const FName Crop=PendingCrop;
    CancelGather(); // Clear first: completion events cannot commit the same action twice.
    FText Failure;
    if (!Resource->CompletePlayerAction(Player,Crop,Failure) && !Failure.IsEmpty()) Player->ShowLocalized(Failure);
}

void UICGatheringSubsystem::OnDamaged(AActor* Victim, float Amount)
{ if (Amount > 0 && Victim == GatheringPawn.Get()) CancelGather(); }

bool UICGatheringSubsystem::IsTickable() const
{ return !IsTemplate() && (ActiveResource.IsValid() || ProgressWidget != nullptr); }

TStatId UICGatheringSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(UICGatheringSubsystem, STATGROUP_Tickables); }
