#include "Inventory/Actors/ICDroppedItem.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"

AICDroppedItem::AICDroppedItem()
{
    IC_TRACE_FUNCTION();
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.25f;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=Mesh;
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0,0,140));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(28.f);
}
void AICDroppedItem::Initialize(const FICDropState& Record)
{
    IC_TRACE_FUNCTION();
    RecordID=Record.ID;DropID=FName(*Record.ID.ToString(EGuidFormats::Digits));Item=Record.Contents.IsEmpty()?FPlayerItemInstance():Record.Contents[0];
    FString Name=Item.ItemID.ToString();
    if(auto* GI=GetGameInstance())if(auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>())
    {FICDefinitionResult Result;if(const auto* Row=Definitions->Find<FICItemRow>(Item.ItemID,Result))
    {Name=Row->GetLabel().ToString();if(auto* Asset=Row->WorldMesh.LoadSynchronous()){Mesh->SetStaticMesh(Asset);Mesh->SetRelativeScale3D(Row->WorldMeshScale);}}}
    Label->SetText(FText::FromString(FString::Printf(TEXT("%s ×%d"),*Name,Item.Count)));
    UE_LOG(LogICInventory, VeryVerbose, TEXT("[Drop.Display] Actor=%s Drop=%s Item=%s Count=%d"),*GetNameSafe(this),*RecordID.ToString(),*Item.ItemID.ToString(),Item.Count);
}
void AICDroppedItem::Tick(float DeltaSeconds)
{
    IC_TRACE_FUNCTION();
    Super::Tick(DeltaSeconds);auto* GI=GetGameInstance();if(!GI||!RecordID.IsValid())return;
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Clock=GI->GetSubsystem<UICTimeSubsystem>();auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();
    if(!State||!Clock||!Definitions)return;FICDefinitionResult R;
    const auto* Rules=Definitions->Find<FICInventorySettingsRow>(TEXT("CFG_INVENTORY_001"),R);if(!Rules)return;
    FICWorldState Next=State->ExportState();const auto* Record=Next.Drops.FindByPredicate([&](const auto& D){ IC_TRACE_FUNCTION();return D.ID==RecordID;});
    if(!Record){Destroy();return;}
    if(Clock->GetClock().PlaySecond-Record->CreatedPlaySecond>=Rules->DropLifetimePlaySeconds)
    {Next.Drops.RemoveAll([&](const auto& D){ IC_TRACE_FUNCTION();return D.ID==RecordID;});if(State->CommitGameplayTransaction(Next,FGuid::NewGuid()))UE_LOG(LogICInventory, Log, TEXT("[Drop.Expired] Drop=%s Lifetime=%.2f"),*RecordID.ToString(),Rules->DropLifetimePlaySeconds);}
}
void AICDroppedItem::TryPickup(AICPlayerController* Controller)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Drop.Pickup.Request] Drop=%s Controller=%s"),*RecordID.ToString(),*GetNameSafe(Controller));
    if(!Controller||!Controller->GetPawn()||FVector::Dist(Controller->GetPawn()->GetActorLocation(),GetActorLocation())>250.f)return;
    auto* Inventory=Controller->GetInventory();auto* State=Controller->GetWorldState();
    if(!Inventory||!State||!State->FlushParticipants())return;
    FICWorldState Next=State->ExportState();auto* Record=Next.Drops.FindByPredicate([&](const auto& D){ IC_TRACE_FUNCTION();return D.ID==RecordID;});if(!Record)return;
    bool Changed=false;
    for(auto& Stored:Record->Contents)
    {
        if(Stored.IsEmpty())continue;const auto* Definition=Inventory->Find(Stored.ItemID);if(!Definition||Definition->Kind==EPlayerItemKind::Quest)return;
        FPlayerItemInstance Copy=Stored;Copy.InstanceID=FGuid::NewGuid();
        const int32 Added=Inventory->Insert(Next.Inventory.Bag,Copy.ItemID,Copy.Count,&Copy);if(Added<=0)continue;

        if(Added==Stored.Count)for(auto& Slot:Next.Inventory.Bag.Slots)if(Slot.InstanceID==Copy.InstanceID)Slot.InstanceID=Stored.InstanceID;
        Stored.Count-=Added;Changed=true;
    }
    if(!Changed){Controller->Show(*ICText::Get(TEXT("Native_79850feffc76")).ToString());return;}
    Record->Contents.RemoveAll([](const auto& I){ IC_TRACE_FUNCTION();return I.IsEmpty();});
    if(Record->Contents.IsEmpty())Next.Drops.RemoveAll([&](const auto& D){ IC_TRACE_FUNCTION();return D.ID==RecordID;});
    if(!Inventory->ValidateSnapshot(Next.Inventory)||!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))
    {Controller->Show(*ICText::Get(TEXT("Native_f9318b541d5b")).ToString());return;}
    UE_LOG(LogICInventory, Log, TEXT("[Drop.PickedUp] Drop=%s RemainingWorldDrops=%d"),*RecordID.ToString(),Next.Drops.Num());
    Controller->Show(*ICText::Get(TEXT("Native_6c9d6b39ff41")).ToString());
}
