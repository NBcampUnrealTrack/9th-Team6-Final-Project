#include "Interaction/ICInteractionActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"

AICInteractionActor::AICInteractionActor()
{
    Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
    RootComponent=Shape;
    Shape->SetCollisionProfileName(TEXT("BlockAll"));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Shape);
    Label->SetRelativeLocation(FVector(0,0,95));
    Label->SetRelativeRotation(FRotator(0,180,0));
    Label->SetWorldSize(20);
    Label->SetHorizontalAlignment(EHTA_Center);
}

FText AICInteractionActor::GetInteractionText() const
{ return InteractionText.IsEmpty()?Label->Text:InteractionText; }

bool AICInteractionActor::TryInteract_Implementation(AICPlayerController* Controller)
{
    if(!Controller||!Controller->GetInventory())return false;
    bool bSuccess=false;
    if(Action==TEXT("Storage"))
    {
        if(!bAccessGranted||(!bHome&&ContainerID.IsNone()))return false;
        Controller->OpenContainer(ContainerID,bHome,bAccessGranted);bSuccess=true;
    }
    else if(Action==TEXT("Sleep"))bSuccess=Controller->Sleep();
    else if(Action==TEXT("Kitchen")||Action==TEXT("Forge"))
    { Controller->ActiveStation=Action;Controller->SetMenu(TEXT("Craft"));bSuccess=true; }
    else if(Action==TEXT("Loot")||Action==TEXT("Quest"))
    {
        if(ItemID.IsNone()||ItemCount<=0)return false;
        // All-or-nothing avoids duplicating a partially collected world object.
        bSuccess=Controller->GetInventory()->AddItem(ItemID,ItemCount,false);
    }
    if(bSuccess&&bDestroyAfterUse)Destroy();
    return bSuccess;
}
