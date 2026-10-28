#include "Interaction/ICInteractionActor.h"
#include "Debug/ICLog.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"

AICInteractionActor::AICInteractionActor()
{
    IC_TRACE_FUNCTION();
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
{ IC_TRACE_FUNCTION(); return InteractionText.IsEmpty()?Label->Text:InteractionText; }

bool AICInteractionActor::TryInteract_Implementation(AICPlayerController* Controller)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInteraction, Verbose, TEXT("[Interaction.Execute] Object=%s Action=%s Controller=%s Container=%s Anchor=%s Item=%s Count=%d"),*GetNameSafe(this),*Action.ToString(),*GetNameSafe(Controller),*ContainerID.ToString(),*WorkAnchorID.ToString(),*ItemID.ToString(),ItemCount);
    if(!Controller||!Controller->GetInventory())return false;
    bool bSuccess=false;
    if(Action==TEXT("Storage"))
    {
        if(!bAccessGranted||(!bHome&&ContainerID.IsNone()))return false;
        Controller->OpenContainer(ContainerID,bHome,bAccessGranted);bSuccess=true;
    }
    else if(Action==TEXT("Sleep")){if(auto* Player=Cast<AICPlayerCharacter>(Controller->GetPawn());Player&&Player->LifeActions)bSuccess=Player->LifeActions->BeginSleep(this);}
    else if(Action==TEXT("Kitchen")||Action==TEXT("Forge"))
    { if(WorkAnchorID.IsNone())return false;Controller->ActiveStation=Action;Controller->ActiveWorkAnchor=WorkAnchorID;Controller->SetMenu(TEXT("Craft"));bSuccess=true; }
    else if(Action==TEXT("Loot")||Action==TEXT("Quest"))
    {
        if(ItemID.IsNone()||ItemCount<=0)return false;

        bSuccess=Controller->GetInventory()->AddItem(ItemID,ItemCount,false);
    }
    UE_LOG(LogICInteraction, Log, TEXT("[Interaction.Result] Object=%s Action=%s Success=%d DestroyAfterUse=%d"),*GetNameSafe(this),*Action.ToString(),bSuccess,bDestroyAfterUse);
    if(bSuccess&&bDestroyAfterUse)Destroy();
    return bSuccess;
}
