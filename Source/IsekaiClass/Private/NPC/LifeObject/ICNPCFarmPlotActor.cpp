#include "NPC/LifeObject/ICNPCFarmPlotActor.h"

#include "NPC/Character/ICNPCBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"

AICNPCFarmPlotActor::AICNPCFarmPlotActor()
{
    ObjectType = EICLifeObjectType::FarmSpot;
    CropVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropVisual"));
    CropVisual->SetupAttachment(RootComponent);
    CropVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AICNPCFarmPlotActor::BeginPlay()
{
    Super::BeginPlay();
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UICTimeSubsystem* Time = GI->GetSubsystem<UICTimeSubsystem>())
        {
            LastObservedDay = FMath::FloorToInt(Time->GetClock().GameMinute / 1440.0);
            Time->OnClockChanged.AddUniqueDynamic(this, &ThisClass::HandleClockChanged);
        }
    }
    RefreshFarmPresentation();
}

void AICNPCFarmPlotActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
        if (UICTimeSubsystem* Time = GI->GetSubsystem<UICTimeSubsystem>()) Time->OnClockChanged.RemoveDynamic(this, &ThisClass::HandleClockChanged);
    Super::EndPlay(EndPlayReason);
}

FName AICNPCFarmPlotActor::GetPlotKey() const { return PlotID.IsNone() ? GetFName() : PlotID; }

FName AICNPCFarmPlotActor::GetActiveCropID() const
{
    const UGameInstance* GI = GetGameInstance();
    const UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (State)
        if (const FICNPCFarmPlotState* Plot = State->ExportState().NPCFarmPlots.Find(GetPlotKey()); Plot && !Plot->CropID.IsNone()) return Plot->CropID;
    return NAME_None;
}

FName AICNPCFarmPlotActor::ChoosePlantableCropID() const
{
    const UGameInstance* GI = GetGameInstance();
    const UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    TArray<FName> ValidCrops;
    for (FName Candidate : PlantableCropIDs)
    {
        FICDefinitionResult Result;
        if (!Candidate.IsNone() && Definitions && Definitions->Find<FICCropRow>(Candidate, Result)) ValidCrops.AddUnique(Candidate);
    }
    return ValidCrops.IsEmpty() ? NAME_None : ValidCrops[FMath::RandRange(0, ValidCrops.Num() - 1)];
}

int32 AICNPCFarmPlotActor::GetStageCount() const
{
    if (const UGameInstance* GI = GetGameInstance())
    {
        FICDefinitionResult Result;
        if (const UICDefinitionSubsystem* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>())
            if (const FICCropRow* Crop = Definitions->Find<FICCropRow>(GetActiveCropID(), Result); Crop && !Crop->GrowthStages.IsEmpty()) return Crop->GrowthStages.Num();
    }
    return FallbackStageMeshes.IsEmpty() ? 5 : FallbackStageMeshes.Num();
}

int32 AICNPCFarmPlotActor::GetNPCGrowthStage() const
{
    const UGameInstance* GI = GetGameInstance();
    const UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (!State) return 0;
    return FMath::Clamp(State->ExportState().NPCFarmPlots.FindRef(GetPlotKey()).GrowthStage, 0, FMath::Max(0, GetStageCount() - 1));
}

bool AICNPCFarmPlotActor::IsMature() const { return GetStageCount() > 0 && GetNPCGrowthStage() >= GetStageCount() - 1; }

void AICNPCFarmPlotActor::HandleClockChanged()
{
    UGameInstance* GI = GetGameInstance();
    UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (!Time) return;
    const int32 Day = FMath::FloorToInt(Time->GetClock().GameMinute / 1440.0);
    if (LastObservedDay == INDEX_NONE) LastObservedDay = Day;
    if (Day > LastObservedDay)
    {
        ProcessMidnightGrowth();
        LastObservedDay = Day;
    }
}

bool AICNPCFarmPlotActor::ProcessMidnightGrowth()
{
    UGameInstance* GI = GetGameInstance();
    UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (!State || !Time || !State->FlushParticipants()) return false;
    const int32 Day = FMath::FloorToInt(Time->GetClock().GameMinute / 1440.0);
    FICWorldState Next = State->ExportState();
    FICNPCFarmPlotState& Plot = Next.NPCFarmPlots.FindOrAdd(GetPlotKey());
    // Only yesterday's care produces tonight's single growth step. Older care must
    // not be replayed after a sleep/time skip.
    if (Plot.CropID.IsNone() || Plot.LastTendedDay != Day - 1 || Plot.LastGrowthDay >= Day || IsMature()) return false;
    ++Plot.GrowthStage;
    Plot.LastGrowthDay = Day;
    const bool bCommitted = State->CommitGameplayTransaction(Next, FGuid::NewGuid());
    if (bCommitted) RefreshFarmPresentation();
    return bCommitted;
}

void AICNPCFarmPlotActor::OnNPCUseStarted(AICNPCBase* NPC)
{
    if (!NPC) return;
    if (IsMature()) { HarvestToNPC(*NPC); return; }
    UGameInstance* GI = GetGameInstance();
    UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (!State || !Time || !State->FlushParticipants()) return;
    const int32 Day = FMath::FloorToInt(Time->GetClock().GameMinute / 1440.0);
    FICWorldState Next = State->ExportState();
    FICNPCFarmPlotState& Plot = Next.NPCFarmPlots.FindOrAdd(GetPlotKey());
    if (Plot.CropID.IsNone())
    {
        Plot.CropID = ChoosePlantableCropID();
        if (Plot.CropID.IsNone()) return;
    }
    if (Plot.LastTendedDay == Day) return;
    Plot.LastTendedDay = Day;
    if (State->CommitGameplayTransaction(Next, FGuid::NewGuid())) RefreshFarmPresentation();
}

bool AICNPCFarmPlotActor::HarvestToNPC(AICNPCBase& NPC)
{
    UGameInstance* GI = GetGameInstance();
    UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (!State || !State->FlushParticipants()) return false;
    TArray<FICNPCWorkReward> Harvests = FallbackHarvestRewards;
    if (UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr)
    {
        FICDefinitionResult Result;
        if (const FICCropRow* Crop = Definitions->Find<FICCropRow>(GetActiveCropID(), Result))
        {
            Harvests.Reset();
            for (const FICItemAmount& Yield : Crop->HarvestYields)
            {
                FICNPCWorkReward& Reward = Harvests.AddDefaulted_GetRef();
                Reward.ItemID = Yield.ItemID;
                Reward.Count = Yield.Count;
                Reward.Weight = 1.f;
            }
        }
    }
    FICWorldState Next = State->ExportState();
    const bool bRewardRolled = FMath::FRand() < FMath::Clamp(HarvestRewardChance, 0.f, 1.f);
    if (bRewardRolled && !Harvests.IsEmpty())
    {
        UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
        const FName NPCStorageID = FindPersonalContainerID(NPC);
        if (Definitions && !NPCStorageID.IsNone())
        {
            UICInventoryComponent* Inventory = NewObject<UICInventoryComponent>();
            Inventory->BeginStaging(Definitions, Next.Job);
            for (const FICNPCWorkReward& Yield : Harvests)
            {
                if (!Inventory->AddItemToStorage(Next.Inventory, NPCStorageID, Yield.ItemID, Yield.Count))
                {
                    Next.Inventory = State->ExportState().Inventory;
                    break;
                }
            }
        }
    }

    FICNPCFarmPlotState& Plot = Next.NPCFarmPlots.FindOrAdd(GetPlotKey());
    // A reset begins a distinct cycle. The next NPC interaction will choose a fresh crop.
    Plot.CropID = NAME_None;
    Plot.GrowthStage = 0;
    ++Plot.Cycle;
    Plot.LastTendedDay = INDEX_NONE;
    Plot.LastGrowthDay = INDEX_NONE;
    const bool bCommitted = State->CommitGameplayTransaction(Next, FGuid::NewGuid());
    if (bCommitted)
    {
        RefreshFarmPresentation();
    }
    return bCommitted;
}

void AICNPCFarmPlotActor::RefreshFarmPresentation()
{
    if (!CropVisual) return;
    const int32 Stage = GetNPCGrowthStage();
    TSoftObjectPtr<UStaticMesh> Mesh;
    if (UGameInstance* GI = GetGameInstance())
    {
        FICDefinitionResult Result;
        if (UICDefinitionSubsystem* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>())
            if (const FICCropRow* Crop = Definitions->Find<FICCropRow>(GetActiveCropID(), Result); Crop && Crop->GrowthStages.IsValidIndex(Stage)) Mesh = Crop->GrowthStages[Stage].Mesh;
    }
    if (Mesh.IsNull() && FallbackStageMeshes.IsValidIndex(Stage)) Mesh = FallbackStageMeshes[Stage];
    CropVisual->SetStaticMesh(Mesh.LoadSynchronous());
}
