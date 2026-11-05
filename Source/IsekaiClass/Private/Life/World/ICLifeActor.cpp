#include "Life/World/ICLifeActor.h"
#include "Life/World/ICLifeTargetSubsystem.h"
#include "Life/Production/ICGatheringSubsystem.h"
#include "UI/Life/ICLifeWidgets.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "UI/Framework/ICText.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"

AICLifeActor::AICLifeActor()
{
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetVisibility(false,false);Shape->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Shape->SetWorldScale3D(FVector(.6f));
    Label->SetVisibility(false);
    LifeLabel=CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeLabel"));LifeLabel->SetupAttachment(Shape);
    LifeLabel->SetWidgetSpace(EWidgetSpace::Screen);LifeLabel->SetDrawSize(FVector2D(320,60));
    LifeLabel->SetRelativeLocation(FVector(0,0,220));LifeLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));VisualMesh->SetupAttachment(Shape);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);VisualMesh->SetAbsolute(false,false,true);
    AnimalMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimalMesh"));AnimalMesh->SetupAttachment(Shape);
    AnimalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);AnimalMesh->SetAbsolute(false,false,true);
    OreMesh=CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("OreMesh"));OreMesh->SetupAttachment(Shape);
    OreMesh->SetSimulatePhysics(false);OreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);OreMesh->SetAbsolute(false,false,true);
}
FName AICLifeActor::PointKey() const { return PointID.IsNone()?GetFName():PointID; }
void AICLifeActor::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UICLifeTargetSubsystem>()->Register(this);
    if(GetGameInstance())
    {
        GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->OnTimeJumped.AddUniqueDynamic(this,&ThisClass::GameTimeJumped);
        GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RefreshLife);
    }
    RefreshLife();
    LifeLabel->SetVisibility(bShowLifeLabel);
    if(bShowLifeLabel)
        if(auto* Type=LoadClass<UUserWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_LifeLabel.WBP_IC_LifeLabel_C")))
        {
            LifeLabel->SetWidgetClass(Type);LifeLabel->InitWidget();
            if(auto* Widget=LifeLabel->GetUserWidgetObject())
                if(auto* Text=Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("LifeLabelText"))))Text->SetText(InteractionText);
        }
    GetWorldTimerManager().SetTimer(RefreshTimer,this,&ThisClass::RefreshLife,.25f,true);
}
void AICLifeActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Targets=GetWorld()->GetSubsystem<UICLifeTargetSubsystem>()) Targets->Unregister(this);
    GetWorldTimerManager().ClearTimer(RefreshTimer);
    if(GetGameInstance())
    {
        GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->OnTimeJumped.RemoveDynamic(this,&ThisClass::GameTimeJumped);
        GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->OnWorldRestored.RemoveDynamic(this,&ThisClass::RefreshLife);
    }
    Super::EndPlay(Reason);
}
void AICLifeActor::GameTimeJumped(double From,double To){RefreshLife();}
int32 AICLifeActor::ResolveGrowthStage(const FICCropRow& Crop,double ElapsedMinutes)
{
    if(Crop.GrowthStages.IsEmpty())return INDEX_NONE;
    double Elapsed=FMath::Max(0.,ElapsedMinutes);
    for(int32 Index=0;Index<Crop.GrowthStages.Num()-1;++Index)
    {
        if(Elapsed<Crop.GrowthStages[Index].DurationGameMinutes)return Index;
        Elapsed-=Crop.GrowthStages[Index].DurationGameMinutes;
    }
    return Crop.GrowthStages.Num()-1;
}
int32 AICLifeActor::GetGrowthStage() const
{
    auto* GI=GetGameInstance();if(!GI||LifeType!=EICDefinitionType::Crop)return INDEX_NONE;
    const auto World=GI->GetSubsystem<UICWorldStateSubsystem>()->ExportState();const auto* Saved=World.Crops.Find(PointKey());
    FICDefinitionResult R;const auto* Crop=GI->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(DefinitionID,R);
    const double Now=GI->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute;
    return Saved&&Crop&&Saved->CropID==DefinitionID&&Now>=Saved->RegrowthAtGameMinute
        ?ResolveGrowthStage(*Crop,Now-FMath::Max(Saved->PlantedAtGameMinute,Saved->RegrowthAtGameMinute)):INDEX_NONE;
}
bool AICLifeActor::IsHarvestable() const
{
    if(!GetGameInstance())return false;FICDefinitionResult R;
    const auto* Crop=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(DefinitionID,R);
    const int32 Stage=GetGrowthStage();return Crop&&Stage==Crop->GrowthStages.Num()-1&&Crop->GrowthStages.IsValidIndex(Stage)&&Crop->GrowthStages[Stage].bHarvestable;
}
bool AICLifeActor::IsResourceAvailable() const
{
    if(!GetGameInstance()||LifeType!=EICDefinitionType::Resource)return false;
    const auto Snapshot=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
    const auto Clock=GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock();
    if(const auto* Ready=Snapshot.ResourceReadyAtGameMinute.Find(PointKey()))return Clock.GameMinute>=*Ready;
    return Clock.PlaySecond>=Snapshot.ResourceReadyAt.FindRef(PointKey());
}
void AICLifeActor::EnsureNaturalCrop()
{
    if(!GetGameInstance()||LifeType!=EICDefinitionType::Crop)return;FICDefinitionResult Result;
    auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();const auto* Crop=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(DefinitionID,Result);
    auto Next=State->ExportState();if(bPlantingPlot||!Crop||!Crop->bGrowsNaturally||!Next.bPlayerCreated||Next.Crops.Contains(PointKey()))return;
    auto& Saved=Next.Crops.Add(PointKey());Saved.CropID=DefinitionID;Saved.OwnerCharacterID=UICWorldStateSubsystem::PlayerId();
    Saved.PlantedAtGameMinute=GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute;
    Saved.HarvestableAtGameMinute=Saved.PlantedAtGameMinute;for(const auto& Stage:Crop->GrowthStages)Saved.HarvestableAtGameMinute+=Stage.DurationGameMinutes;
    State->CommitGameplayTransaction(Next,FGuid::NewGuid());
}
FText AICLifeActor::GetInteractionText() const
{
    if(!GetGameInstance())return Super::GetInteractionText();
    if(LifeType==EICDefinitionType::Resource&&!IsResourceAvailable())
    {
        const auto Saved=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
        const auto Clock=GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock();
        if(const auto* Ready=Saved.ResourceReadyAtGameMinute.Find(PointKey()))
            return ICText::FormatText(TEXT("Life_Regrowing"),InteractionText,FMath::CeilToInt(FMath::Max(0.,*Ready-Clock.GameMinute)));
    }
    if(LifeType==EICDefinitionType::Crop)
    {
        if(bPlantingPlot){const auto Saved=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();if(!Saved.Crops.Contains(PointKey()))return NSLOCTEXT("ICFarm","EmptyPlot","빈 밭 · 씨앗 심기");}
        FICDefinitionResult R;const auto* Crop=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(DefinitionID,R);
        const int32 Stage=GetGrowthStage();if(!Crop)return Super::GetInteractionText();
        if(IsHarvestable())return ICText::FormatText(TEXT("Life_HarvestReady"),InteractionText);
        if(Stage>=0)return ICText::FormatText(TEXT("Life_Growing"),InteractionText,Stage+1,Crop->GrowthStages.Num());
        const auto Saved=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
        if(const auto* Plot=Saved.Crops.Find(PointKey()))return ICText::FormatText(TEXT("Life_Regrowing"),InteractionText,FMath::CeilToInt(FMath::Max(0.,Plot->RegrowthAtGameMinute-GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute)));
    }
    return Super::GetInteractionText();
}
void AICLifeActor::RefreshLife()
{
    if(!GetGameInstance())return;
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    if(LifeType==EICDefinitionType::Crop)
    {
        if(bPlantingPlot){const auto Saved=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();if(const auto* Crop=Saved.Crops.Find(PointKey()))DefinitionID=Crop->CropID;else DefinitionID=NAME_None;}
        EnsureNaturalCrop();
        const auto* Crop=D->Find<FICCropRow>(DefinitionID,R);const int32 Stage=GetGrowthStage();
        auto* Mesh=Crop&&Crop->GrowthStages.IsValidIndex(Stage)?Crop->GrowthStages[Stage].Mesh.LoadSynchronous():nullptr;
        const auto Saved=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
        if(!bPlantingPlot&&Stage==INDEX_NONE&&!Saved.Crops.Contains(PointKey())&&Crop&&!Crop->GrowthStages.IsEmpty()&&(bPreviewUnplantedCrop||Crop->bGrowsNaturally))Mesh=Crop->GrowthStages[0].Mesh.LoadSynchronous();
        if(VisualMesh->GetStaticMesh()!=Mesh)VisualMesh->SetStaticMesh(Mesh);
        if(bPlantingPlot&&Mesh)
        {const auto Bounds=Mesh->GetBoundingBox();const auto Center=Bounds.GetCenter();const auto Scale=Shape->GetComponentScale();VisualMesh->SetRelativeLocation(FVector(-Center.X/FMath::Max(.01f,Scale.X),-Center.Y/FMath::Max(.01f,Scale.Y),-83.333-Bounds.Min.Z/FMath::Max(.01f,Scale.Z)));}
        VisualMesh->SetVisibility(Mesh!=nullptr);PresentedStage=Stage;
        if(Crop&&!Crop->NameKey.IsEmpty())InteractionText=FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Crop->NameKey);
    }
    else if(LifeType==EICDefinitionType::Resource)
    {
        const auto* Resource=D->Find<FICResourceRow>(DefinitionID,R);const bool Available=IsResourceAvailable();
        if(Resource)
        {
            auto* Mesh=Available?Resource->AvailableMesh.LoadSynchronous():Resource->DepletedMesh.LoadSynchronous();
            if(VisualMesh->GetStaticMesh()!=Mesh)VisualMesh->SetStaticMesh(Mesh);
            VisualMesh->SetRelativeTransform(Resource->VisualTransform);
            if(auto* Collection=Resource->OreCollection.LoadSynchronous();OreMesh->GetRestCollection()!=Collection)OreMesh->SetRestCollection(Collection);
            if(auto* Material=Resource->MaterialOverride.LoadSynchronous()){VisualMesh->SetMaterial(0,Material);for(int32 I=0;I<OreMesh->GetNumMaterials();++I)OreMesh->SetMaterial(I,Material);}
            if(!Resource->NameKey.IsEmpty())InteractionText=FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Resource->NameKey);
        }
        VisualMesh->SetVisibility(VisualMesh->GetStaticMesh()!=nullptr);OreMesh->SetVisibility(Available&&OreMesh->GetRestCollection()!=nullptr);
    }
    if(auto* Widget=LifeLabel->GetUserWidgetObject())if(auto* Text=Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("LifeLabelText"))))Text->SetText(GetInteractionText());
}
bool AICLifeActor::CanPlantCrop(AICPlayerController* Controller,FName CropID,FText& Failure) const
{
    Failure=FText::GetEmpty();
    if(!Controller||!GetGameInstance()||Controller->GetWorld()!=GetWorld()||LifeType!=EICDefinitionType::Crop||!bPlantingPlot)
    {Failure=NSLOCTEXT("ICFarm","InvalidPlot","씨앗을 심을 수 있는 밭이 아닙니다.");return false;}
    FICDefinitionResult Result;
    const auto* Crop=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(CropID,Result);
    if(!Crop){Failure=NSLOCTEXT("ICFarm","InvalidCrop","심을 작물 정보를 찾을 수 없습니다.");return false;}
    if(!Crop->AllowedPlotAnchorIDs.IsEmpty()&&!Crop->AllowedPlotAnchorIDs.Contains(PointKey()))
    {Failure=NSLOCTEXT("ICFarm","PlotRestricted","이 밭에는 선택한 씨앗을 심을 수 없습니다.");return false;}
    if(GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Crops.Contains(PointKey()))
    {Failure=NSLOCTEXT("ICFarm","PlotOccupied","이미 작물이 심어진 밭입니다.");return false;}
    if(!Controller->GetInventory()||Controller->GetInventory()->CountItem(Crop->SeedItemID)<1)
    {Failure=NSLOCTEXT("ICFarm","NeedSeed","심을 씨앗이 부족합니다.");return false;}
    return true;
}
bool AICLifeActor::Plant(AICPlayerController* Controller)
{
    FText Failure;if(!CanPlantCrop(Controller,DefinitionID,Failure))return false;
    auto* GI=GetGameInstance();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    const auto* Crop=D->Find<FICCropRow>(DefinitionID,R);if(!Crop||!State->FlushParticipants())return false;
    auto Next=State->ExportState();if(Next.Crops.Contains(PointKey()))return false;
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);
    if(!Staging->ApplySnapshot(Next.Inventory)||!Staging->Consume(Crop->SeedItemID,1))return false;
    auto& Planted=Next.Crops.Add(PointKey());Planted.CropID=DefinitionID;Planted.OwnerCharacterID=UICWorldStateSubsystem::PlayerId();
    Planted.PlantedAtGameMinute=GI->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute;
    Planted.HarvestableAtGameMinute=Planted.PlantedAtGameMinute;
    for(const auto& Stage:Crop->GrowthStages)Planted.HarvestableAtGameMinute+=Stage.DurationGameMinutes;
    Next.Inventory=Staging->GetSnapshot();const bool Success=State->CommitGameplayTransaction(Next,FGuid::NewGuid());RefreshLife();return Success;
}
bool AICLifeActor::TryInteract_Implementation(AICPlayerController* Controller)
{
    if(!Controller||!GetGameInstance())return false;
    if(LifeType==EICDefinitionType::FishingPoint)return false; // The controller resolves the aimed landing before starting fishing.
    if(LifeType==EICDefinitionType::Crop&&bPlantingPlot&&!GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Crops.Contains(PointKey()))return UICSeedPickerWidget::Open(Controller,this);
    return GetGameInstance()->GetSubsystem<UICGatheringSubsystem>()->StartGather(this,Controller);
}
namespace
{
void FishingSurface(const UStaticMeshComponent* Surface,FVector& Center,FVector& Normal)
{
    const FBox Bounds=Surface->GetStaticMesh()->GetBoundingBox();
    const FVector Local=FVector(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Max.Z);
    Center=Surface->GetComponentTransform().TransformPosition(Local);Normal=Surface->GetUpVector();
}
bool InsideFishingSurface(const UStaticMeshComponent* Surface,const FVector& Point)
{
    const FBox Bounds=Surface->GetStaticMesh()->GetBoundingBox();
    const FVector Local=Surface->GetComponentTransform().InverseTransformPosition(Point);
    FVector Center,Normal;FishingSurface(Surface,Center,Normal);
    return Local.X>=Bounds.Min.X&&Local.X<=Bounds.Max.X&&Local.Y>=Bounds.Min.Y&&Local.Y<=Bounds.Max.Y
        &&FMath::Abs(FVector::DotProduct(Point-Center,Normal))<=12.f;
}
}
void AICLifeActor::GetFishingRegions(TArray<UStaticMeshComponent*>& Regions) const
{
    TInlineComponentArray<UStaticMeshComponent*> Meshes;GetComponents(Meshes);
    for(auto* Mesh:Meshes)if(Mesh!=Shape&&Mesh->GetStaticMesh())Regions.Add(Mesh);
    if(Regions.IsEmpty()&&Shape&&Shape->GetStaticMesh())Regions.Add(Shape);
}
void AICLifeActor::GetFishingSurface(FVector& Center,FVector& Normal) const
{
    TArray<UStaticMeshComponent*> Regions;GetFishingRegions(Regions);
    if(Regions.IsEmpty()){Center=GetActorLocation();Normal=GetActorUpVector();return;}
    FishingSurface(Regions[0],Center,Normal);
}
void AICLifeActor::GetFishingGuideSamples(const FVector& Origin,TArray<FVector>& Samples) const
{
    Samples.Reset();
    TArray<UStaticMeshComponent*> Regions;GetFishingRegions(Regions);
    for(const auto* Surface:Regions)
    {
        const FBox Box=Surface->GetStaticMesh()->GetBoundingBox().ExpandBy(FVector(-.01,-.01,0));
        const FTransform Transform=Surface->GetComponentTransform();
        const FVector LocalOrigin=Transform.InverseTransformPosition(Origin);
        // Include the nearest point as well as spaced edge samples for blocked banks.
        Samples.Add(Transform.TransformPosition(FVector(
            FMath::Clamp(LocalOrigin.X,Box.Min.X,Box.Max.X),
            FMath::Clamp(LocalOrigin.Y,Box.Min.Y,Box.Max.Y),Box.Max.Z)));
        for(int32 Index=0;Index<5;++Index)
        {
            const double X=FMath::Lerp(Box.Min.X,Box.Max.X,Index/4.);
            const double Y=FMath::Lerp(Box.Min.Y,Box.Max.Y,Index/4.);
            Samples.Add(Transform.TransformPosition(FVector(X,Box.Min.Y,Box.Max.Z)));
            Samples.Add(Transform.TransformPosition(FVector(X,Box.Max.Y,Box.Max.Z)));
            Samples.Add(Transform.TransformPosition(FVector(Box.Min.X,Y,Box.Max.Z)));
            Samples.Add(Transform.TransformPosition(FVector(Box.Max.X,Y,Box.Max.Z)));
        }
    }
}
bool AICLifeActor::ResolveFishingLanding(const FVector& Origin,const FVector& Direction,FVector& Landing) const
{
    if(LifeType!=EICDefinitionType::FishingPoint)return false;
    TArray<UStaticMeshComponent*> Regions;GetFishingRegions(Regions);double Best=MAX_dbl;bool Found=false;
    for(const auto* Surface:Regions)
    {
        FVector Center,Normal;FishingSurface(Surface,Center,Normal);
        const double Denominator=FVector::DotProduct(Direction,Normal);if(Denominator>=-.01)continue;
        const double Distance=FVector::DotProduct(Center-Origin,Normal)/Denominator;
        if(Distance<=0||Distance>=Best)continue;
        const FVector Point=Origin+Direction*Distance;
        if(InsideFishingSurface(Surface,Point)){Best=Distance;Landing=Point;Found=true;}
    }
    return Found;
}
bool AICLifeActor::ContainsFishingLanding(FVector Point) const
{
    if(LifeType!=EICDefinitionType::FishingPoint)return false;
    // These meshes describe mathematical fishing areas, even when hidden or non-colliding.
    TArray<UStaticMeshComponent*> Regions;GetFishingRegions(Regions);
    for(const auto* Surface:Regions)if(InsideFishingSurface(Surface,Point))return true;
    return false;
}
bool AICLifeActor::CompletePlayerAction(AICPlayerController* Controller,FName CropToPlant,FText& Failure)
{
    if(!Controller||!GetGameInstance())return false;
    if(LifeType==EICDefinitionType::Resource)return CompleteResourceGather(Controller,Failure);
    if(!CropToPlant.IsNone())
    {
        if(!CanPlantCrop(Controller,CropToPlant,Failure))return false;
        DefinitionID=CropToPlant;const bool Done=Plant(Controller);
        if(!Done){RefreshLife();Failure=NSLOCTEXT("ICFarm","PlantFailed","씨앗을 심지 못했습니다. 밭과 씨앗을 확인하고 다시 시도해 주세요.");}
        return Done;
    }
    if(LifeType==EICDefinitionType::FishingPoint)
    {
        auto* GI=GetGameInstance();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
        const auto* Point=D->Find<FICFishingPointRow>(DefinitionID,R);if(!Point||!State->FlushParticipants())return false;
        auto Next=State->ExportState();auto* Inventory=NewObject<UICInventoryComponent>();Inventory->BeginStaging(D,Next.Job);if(!Inventory->ApplySnapshot(Next.Inventory))return false;
        double Weight=0;for(const auto& Fish:Point->FishSelections)Weight+=FMath::Max(0.,Fish.Weight);if(Weight<=0)return false;
        const FGuid Receipt=FGuid::NewGuid();FRandomStream Random(GetTypeHash(Receipt));
        if(Random.FRand()>FMath::Clamp(Point->SuccessChance,0.,1.)){Controller->ShowLocalized(NSLOCTEXT("ICFishing","NoCatch","아무것도 낚지 못했습니다. 다시 시도할 수 있습니다."));return true;}
        double Choice=Random.FRand()*Weight;const FICFishingSelectionEntry* Selected=nullptr;
        for(const auto& Fish:Point->FishSelections){Choice-=FMath::Max(0.,Fish.Weight);if(Choice<=0){Selected=&Fish;break;}}
        if(!Selected)return false;const auto* Fish=D->Find<FICFishRow>(Selected->FishID,R);
        if(!Fish||Selected->CatchCount<=0||!Inventory->AddItem(Fish->ItemID,Selected->CatchCount,false)){Failure=ICText::Get(TEXT("Life_BagFull"));return false;}
        Next.Inventory=Inventory->GetSnapshot();if(!State->CommitGameplayTransaction(Next,Receipt))return false;
        FICItemAmount Reward;Reward.ItemID=Fish->ItemID;Reward.Count=Selected->CatchCount;State->OnItemsReceived.Broadcast({Reward});return true;
    }
    if(LifeType!=EICDefinitionType::Crop)return false;
    auto* GI=GetGameInstance();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    if(!State->FlushParticipants())return false;auto Next=State->ExportState();TArray<FICItemAmount> Yields;
    if(LifeType==EICDefinitionType::Crop)
    {
        if(!Next.Crops.Contains(PointKey()))return false;
        const auto* Crop=D->Find<FICCropRow>(DefinitionID,R);if(!Crop||!IsHarvestable())return false;
        Yields=Crop->HarvestYields;
        if(Crop->bGrowsNaturally&&!bPlantingPlot)
        {
            auto& Saved=Next.Crops.FindChecked(PointKey());Saved.PlantedAtGameMinute=GI->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute;
            Saved.RegrowthAtGameMinute=Saved.PlantedAtGameMinute+Crop->RegrowthGameMinutes;
            Saved.HarvestableAtGameMinute=Saved.RegrowthAtGameMinute;for(const auto& Stage:Crop->GrowthStages)Saved.HarvestableAtGameMinute+=Stage.DurationGameMinutes;
        }
        else Next.Crops.Remove(PointKey());
    }
    else return false;
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);
    if(Yields.IsEmpty()||!Staging->ApplySnapshot(Next.Inventory))return false;
    for(const auto& Item:Yields)if(!Staging->AddItem(Item.ItemID,Item.Count,false))return false;
    Next.Inventory=Staging->GetSnapshot();const bool Success=State->CommitGameplayTransaction(Next,FGuid::NewGuid());
    if(Success)State->OnItemsReceived.Broadcast(Yields);
    RefreshLife();return Success;
}

bool AICLifeActor::CompleteResourceGather(AICPlayerController* Controller, FText& Failure)
{
    if(!Controller||!GetGameInstance()||LifeType!=EICDefinitionType::Resource)return false;
    auto* GI=GetGameInstance();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();auto* Clock=GI->GetSubsystem<UICTimeSubsystem>();
    FICDefinitionResult Result;const auto* Resource=Definitions->Find<FICResourceRow>(DefinitionID,Result);
    if(!Resource||!State->FlushParticipants())return false;
    auto Next=State->ExportState();
    if(!IsResourceAvailable())return false;
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(Definitions,Next.Job);
    if(Resource->Yields.IsEmpty()||!Staging->ApplySnapshot(Next.Inventory))return false;
    for(const auto& Item:Resource->Yields)
        if(!Staging->AddItem(Item.ItemID,Item.Count,false))
        {Failure=ICText::Get(TEXT("Life_BagFull"));return false;}
    Next.Inventory=Staging->GetSnapshot();
    Next.ResourceReadyAt.Remove(PointKey());
    Next.ResourceReadyAtGameMinute.Add(PointKey(),Clock->GetClock().GameMinute+Resource->RegenerationGameMinutes);
    const bool Success=State->CommitGameplayTransaction(Next,FGuid::NewGuid());
    if(Success)State->OnItemsReceived.Broadcast(Resource->Yields);
    RefreshLife();return Success;
}
