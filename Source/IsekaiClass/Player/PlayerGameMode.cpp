#include "PlayerGameMode.h"
#include "PlayerCharacter.h"
#include "PlayerPrototypeController.h"
#include "PlayerGameState.h"
#include "PlayerInteractionObject.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"

APlayerPrototypeGameMode::APlayerPrototypeGameMode()
{
    DefaultPawnClass=APlayerPrototypeCharacter::StaticClass();
    PlayerControllerClass=APlayerPrototypeController::StaticClass();
    GameStateClass=APlayerPrototypeGameState::StaticClass();
}

void APlayerPrototypeGameMode::StartPlay()
{
    Super::StartPlay();
    auto* Floor=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0,0,-60),FRotator::ZeroRotator);
    Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(40,40,1));
    Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    auto* Light=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,600),FRotator(-60,-30,0));
    Light->GetLightComponent()->SetIntensity(4);
    Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    auto* FillLight=GetWorld()->SpawnActor<APointLight>(FVector(-200,0,350),FRotator::ZeroRotator);
    auto* Fill=Cast<UPointLightComponent>(FillLight->GetLightComponent());
    Fill->SetMobility(EComponentMobility::Movable);
    Fill->SetAttenuationRadius(2200);
    Fill->SetIntensity(20000);
    Fill->SetCastShadows(false);
    const TCHAR* Labels[]={TEXT("HOME A"),TEXT("HOME B (SHARED)"),TEXT("NPC STORAGE"),TEXT("WOOD +3"),TEXT("QUEST ITEM"),TEXT("BED - SLEEP"),TEXT("KITCHEN - BREAD"),TEXT("FORGE - SWORD"),TEXT("SUPPLY - MATERIALS")};
    for(int32 Index=0;Index<9;++Index)
    {
        const FVector Position(320,(Index-2)*220,30);
        auto* Object=GetWorld()->SpawnActor<APlayerPrototypeObject>(Position,FRotator::ZeroRotator);
        Object->Action=Index<3?TEXT("Storage"):Index==3?TEXT("Loot"):Index==4?TEXT("Quest"):Index==5?TEXT("Sleep"):Index==6?TEXT("Kitchen"):Index==7?TEXT("Forge"):TEXT("Supply");
        Object->bHome=Index<2;
        Object->ContainerID=Index==2?TEXT("NPC_Test"):TEXT("PlayerHome");
        Object->Label->SetText(FText::FromString(Labels[Index]));
    }
    if(auto* Controller=GetWorld()->GetFirstPlayerController())
        if(Controller->GetPawn()) Controller->GetPawn()->SetActorLocation(FVector(0,0,120));
}
