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
{
    if(!IsGathering())CancelGather();
    if (IsGathering() || !IsValid(Resource) || !IsValid(Player) || !Player->CanHandleGameplayInput()
        || Resource->LifeType != EICDefinitionType::Resource || Resource->GetWorld() != Player->GetWorld()
        || FVector::DistSquared(Resource->GetActorLocation(), Player->GetPawn()->GetActorLocation()) > FMath::Square(300.f)) return false;
    auto* GI = GetGameInstance();
    auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
    auto* Clock = GI->GetSubsystem<UICTimeSubsystem>();
    FICDefinitionResult Result;
    const auto* Row = Definitions->Find<FICResourceRow>(Resource->DefinitionID, Result);
    if (!Row || !Clock->IsRunning() || Row->Yields.IsEmpty()
        || Clock->GetClock().PlaySecond < GI->GetSubsystem<UICWorldStateSubsystem>()->ExportState().ResourceReadyAt.FindRef(Resource->PointKey())) return false;
    const auto* Production = Definitions->Find<FICProductionRow>(Row->ProductionID, Result);
    Duration = Production && FMath::IsFinite(Production->DurationPlaySeconds) && Production->DurationPlaySeconds > 0
        ? Production->DurationPlaySeconds : 5.;
    ActiveResource = Resource; Controller = Player; GatheringPawn = Player->GetPawn();
    GatheringDefinition = Resource->DefinitionID; GatheringPoint = Resource->PointKey();
    StartLocation = GatheringPawn->GetActorLocation(); StartedAt = Clock->GetClock().PlaySecond;
    if (auto* Character = Cast<ACharacter>(GatheringPawn.Get())) Character->GetCharacterMovement()->StopMovementImmediately();
    UClass* WidgetType = ProgressWidgetClass ? ProgressWidgetClass.Get() : UICGatherProgressWidget::StaticClass();
    ProgressWidget = CreateWidget<UICGatherProgressWidget>(Player, WidgetType);
    if (ProgressWidget)
    {
        ProgressWidget->SetGatherProgress(Resource->GetInteractionText(), 0);
        ProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        ProgressWidget->AddToViewport(20);
    }
    return true;
}

void UICGatheringSubsystem::CancelGather()
{
    if (ProgressWidget) ProgressWidget->RemoveFromParent();
    ProgressWidget = nullptr; ActiveResource.Reset(); Controller.Reset(); GatheringPawn.Reset();
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
        || FVector::DistSquared(ActiveResource->GetActorLocation(), GatheringPawn->GetActorLocation()) > FMath::Square(300.f))
    { CancelGather(); return; }
    const float Progress = GetProgress();
    if (ProgressWidget) ProgressWidget->SetGatherProgress(ActiveResource->GetInteractionText(), Progress);
    if (Progress < 1) return;
    auto* Resource = ActiveResource.Get(); auto* Player = Controller.Get();
    CancelGather(); // Clear first: completion events cannot commit the same action twice.
    FText Failure;
    if (!Resource->CompleteResourceGather(Player, Failure) && !Failure.IsEmpty()) Player->ShowLocalized(Failure);
}

void UICGatheringSubsystem::OnDamaged(AActor* Victim, float Amount)
{ if (Amount > 0 && Victim == GatheringPawn.Get()) CancelGather(); }

bool UICGatheringSubsystem::IsTickable() const
{ return !IsTemplate() && (ActiveResource.IsValid() || ProgressWidget != nullptr); }

TStatId UICGatheringSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(UICGatheringSubsystem, STATGROUP_Tickables); }
