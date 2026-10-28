#include "Life/World/ICPlayerHomeActors.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Inventory/Actors/ICChestPresentationComponent.h"
AICFarmPlot::AICFarmPlot()
{
    LifeType=EICDefinitionType::Crop;bPlantingPlot=true;DefinitionID=NAME_None;
    InteractionText=NSLOCTEXT("ICFarm","EmptyPlot","빈 밭 · 씨앗 심기");
    Soil=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Soil"));Soil->SetupAttachment(Shape);
    Soil->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Soil->SetAbsolute(false,false,true);Soil->SetRelativeScale3D(FVector(2.1,2.1,.12));Soil->SetRelativeLocation(FVector(0,0,-80));Soil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeLocation(FVector(0,0,-83.333));
}
AICPlayerStorage::AICPlayerStorage(){Action=TEXT("Storage");ContainerID=TEXT("PlayerHome");InteractionText=NSLOCTEXT("ICHome","Storage","개인 창고");ChestPresentation=CreateDefaultSubobject<UICChestPresentationComponent>(TEXT("ChestPresentation"));ChestPresentation->SetupAttachment(Shape);}
AICPlayerBed::AICPlayerBed()
{
    Action=TEXT("Sleep");InteractionText=NSLOCTEXT("ICHome","Bed","플레이어 침대 · 잠자기");Label->SetVisibility(false);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Shape->SetVisibility(false,false);Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Shape->SetCollisionResponseToAllChannels(ECR_Ignore);Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    BedVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedVisual"));BedVisual->SetupAttachment(Shape);BedVisual->SetRelativeLocation(FVector(0,0,-96));BedVisual->SetCollisionProfileName(TEXT("BlockAll"));
}
void AICPlayerStorage::BeginPlay(){Super::BeginPlay();RefreshAvailability();GetWorldTimerManager().SetTimer(RefreshTimer,this,&ThisClass::RefreshAvailability,.25f,true);}
void AICPlayerStorage::EndPlay(const EEndPlayReason::Type Reason){GetWorldTimerManager().ClearTimer(RefreshTimer);Super::EndPlay(Reason);}
void AICPlayerStorage::RefreshAvailability()
{
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    const bool Available=RequiredExtraStorage==0||(State&&State->ExportState().ProgressionStages.FindRef(TEXT("PLAYER_STORAGE"))>=RequiredExtraStorage);
    SetActorHiddenInGame(!Available);SetActorEnableCollision(Available);
}
bool AICPlayerStorage::TryInteract_Implementation(AICPlayerController* Controller)
{RefreshAvailability();return !IsHidden()&&ChestPresentation->OpenForPlayer(Controller,ContainerID,bHome);}
