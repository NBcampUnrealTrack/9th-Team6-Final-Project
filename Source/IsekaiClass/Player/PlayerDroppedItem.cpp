#include "PlayerDroppedItem.h"
#include "PlayerPrototypeController.h"
#include "PlayerInventory.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

APlayerDroppedItem::APlayerDroppedItem()
{
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=Mesh;
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Mesh->SetWorldScale3D(FVector(0.25f));Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(0,0,140));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(28.f);
    InitialLifeSpan=1800.f;
}

void APlayerDroppedItem::Initialize(const FPlayerItemInstance& InItem)
{
    Item=InItem;
    if(DropID.IsNone())DropID=FName(*FString::Printf(TEXT("DRP_%s"),*Item.InstanceID.ToString(EGuidFormats::Digits).ToUpper()));
    Label->SetText(FText::FromString(FString::Printf(TEXT("%s x%d"),*Item.ItemID.ToString(),Item.Count)));
}

void APlayerDroppedItem::TryPickup(APlayerPrototypeController* Controller)
{
    if(!Controller||Item.IsEmpty())return;
    const int32 Remaining=Controller->GetInventory()->TryAddItemInstance(Item);
    if(Remaining<=0){Destroy();return;}
    Item.Count=Remaining;Initialize(Item);Controller->Show(TEXT("가방 공간만큼만 습득했습니다."));
}
