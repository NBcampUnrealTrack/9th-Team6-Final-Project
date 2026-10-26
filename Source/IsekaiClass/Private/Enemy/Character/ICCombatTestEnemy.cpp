#include "Enemy/Character/ICCombatTestEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AICCombatTestEnemy::AICCombatTestEnemy()
{
    auto* Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TestBody"));
    Body->SetupAttachment(GetRootComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cube.Succeeded())Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(.65f,.65f,1.6f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCanEverAffectNavigation(false);
    auto* Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("TestLabel"));
    Label->SetupAttachment(GetRootComponent());
    Label->SetRelativeLocation(FVector(0,0,125));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(26);
    Label->SetTextRenderColor(FColor::Red);
    Label->SetText(FText::FromString(TEXT("TEST ENEMY")));
}
