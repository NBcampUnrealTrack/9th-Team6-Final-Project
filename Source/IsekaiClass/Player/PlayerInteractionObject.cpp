#include "PlayerInteractionObject.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

APlayerPrototypeObject::APlayerPrototypeObject()
{
    Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
    RootComponent=Shape;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Shape->SetStaticMesh(BodyMesh.Object);
    Shape->SetCollisionProfileName(TEXT("BlockAll"));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Shape);
    Label->SetRelativeLocation(FVector(0,0,95));
    Label->SetRelativeRotation(FRotator(0,180,0));
    Label->SetWorldSize(20);
    Label->SetHorizontalAlignment(EHTA_Center);
}
