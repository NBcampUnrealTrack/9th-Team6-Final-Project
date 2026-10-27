#include "Life/World/ICLifeActor.h"
#include "Life/Production/ICGatheringSubsystem.h"
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
#include "Engine/GameInstance.h"
#include "TimerManager.h"

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
    Super::BeginPlay();RefreshLife();
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
{GetWorldTimerManager().ClearTimer(RefreshTimer);Super::EndPlay(Reason);}
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
    return Saved&&Crop&&Saved->CropID==DefinitionID?ResolveGrowthStage(*Crop,GI->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute-Saved->PlantedAtGameMinute):INDEX_NONE;
}
bool AICLifeActor::IsHarvestable() const
{
    if(!GetGameInstance())return false;FICDefinitionResult R;
    const auto* Crop=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(DefinitionID,R);
    const int32 Stage=GetGrowthStage();return Crop&&Crop->GrowthStages.IsValidIndex(Stage)&&Crop->GrowthStages[Stage].bHarvestable;
}
void AICLifeActor::RefreshLife()
{
    if(!GetGameInstance())return;
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    if(LifeType==EICDefinitionType::Crop)
    {
        const auto* Crop=D->Find<FICCropRow>(DefinitionID,R);const int32 Stage=GetGrowthStage();
        auto* Mesh=Crop&&Crop->GrowthStages.IsValidIndex(Stage)?Crop->GrowthStages[Stage].Mesh.LoadSynchronous():nullptr;
        if(Stage==INDEX_NONE&&bPreviewUnplantedCrop&&Crop&&!Crop->GrowthStages.IsEmpty())Mesh=Crop->GrowthStages.Last().Mesh.LoadSynchronous();
        if(VisualMesh->GetStaticMesh()!=Mesh)VisualMesh->SetStaticMesh(Mesh);
        VisualMesh->SetVisibility(Mesh!=nullptr);PresentedStage=Stage;
    }
    else if(LifeType==EICDefinitionType::Resource)
    {
        const auto World=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
        const double Ready=World.ResourceReadyAt.FindRef(PointKey());
        const bool Available=GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().PlaySecond>=Ready;
        VisualMesh->SetVisibility(Available);OreMesh->SetVisibility(Available);
    }
}
bool AICLifeActor::Plant(AICPlayerController* Controller)
{
    if(!Controller||!GetGameInstance()||LifeType!=EICDefinitionType::Crop)return false;
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
    if(LifeType==EICDefinitionType::Resource)return GetGameInstance()->GetSubsystem<UICGatheringSubsystem>()->StartGather(this,Controller);
    if(LifeType!=EICDefinitionType::Crop)return false;
    auto* GI=GetGameInstance();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    if(!State->FlushParticipants())return false;auto Next=State->ExportState();TArray<FICItemAmount> Yields;
    if(LifeType==EICDefinitionType::Crop)
    {
        if(!Next.Crops.Contains(PointKey()))return Plant(Controller);
        const auto* Crop=D->Find<FICCropRow>(DefinitionID,R);if(!Crop||!IsHarvestable())return false;
        Yields=Crop->HarvestYields;Next.Crops.Remove(PointKey());
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
    if(Clock->GetClock().PlaySecond<Next.ResourceReadyAt.FindRef(PointKey()))return false;
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(Definitions,Next.Job);
    if(Resource->Yields.IsEmpty()||!Staging->ApplySnapshot(Next.Inventory))return false;
    for(const auto& Item:Resource->Yields)
        if(!Staging->AddItem(Item.ItemID,Item.Count,false))
        {Failure=NSLOCTEXT("ICLife","GatherBagFull","가방에 빈 공간이 부족합니다.");return false;}
    Next.Inventory=Staging->GetSnapshot();
    Next.ResourceReadyAt.Add(PointKey(),Clock->GetClock().PlaySecond+Resource->RegenerationPlaySeconds);
    const bool Success=State->CommitGameplayTransaction(Next,FGuid::NewGuid());
    if(Success)State->OnItemsReceived.Broadcast(Resource->Yields);
    RefreshLife();return Success;
}
