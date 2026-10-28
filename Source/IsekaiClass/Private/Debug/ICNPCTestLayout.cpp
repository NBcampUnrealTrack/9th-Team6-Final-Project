#include "Debug/ICNPCTestLayout.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/Anchors/ICNPCWorldAnchor.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Debug/ICLog.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"

AICNPCTestLayout::AICNPCTestLayout()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LayoutOrigin"));
    PrimaryActorTick.bCanEverTick = false;
    for(int32 Index=1;Index<=11;++Index)NPCIDs.Add(FName(*FString::Printf(TEXT("NPC_R01_%03d"),Index)));
    BedClass = FSoftClassPath(TEXT("/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Meshes/Props/Interior/Cloth/BP_Farm_Bed.BP_Farm_Bed_C"));
    InteractionBedClass=FSoftClassPath(TEXT("/Game/IsekaiClass/NPC/Blueprints/Interaction/BP_IC_NPCBed.BP_IC_NPCBed_C"));
}
FTransform AICNPCTestLayout::StandingTransform(int32 Index) const
{
    const FVector Local(0,(Index-(NPCIDs.Num()-1)*.5f)*RowSpacingCm,AnchorHeightCm);
    return FTransform(GetActorQuat(),GetActorTransform().TransformPositionNoScale(Local));
}
FTransform AICNPCTestLayout::BedTransform(int32 Index) const
{
    const FVector Local(-BedDistanceCm,(Index-(NPCIDs.Num()-1)*.5f)*RowSpacingCm,0);
    // This bed's pillows are on local -Y; rotate its long axis along the NPC's forward axis.
    return FTransform(GetActorQuat()*FRotator(0,-90,0).Quaternion(),GetActorTransform().TransformPositionNoScale(Local));
}
void AICNPCTestLayout::BeginPlay()
{
    Super::BeginPlay();
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>();
    if(!D||!Spawns)return;
    UClass* BedType=BedClass.LoadSynchronous();
    UClass* InteractionType=InteractionBedClass.LoadSynchronous();
    for(int32 Index=0;Index<NPCIDs.Num();++Index)
    {
        FICDefinitionResult Result;
        const auto* NPC=D->Find<FICNPCRow>(NPCIDs[Index],Result);
        const auto* Schedule=NPC?D->Find<FICScheduleRow>(NPC->ScheduleID,Result):nullptr;
        if(!NPC||!Schedule)continue;
        TSet<FName> IDs;IDs.Add(NPC->InitialAnchorID);
        for(const auto& Segment:Schedule->Segments)
        {if(!Segment.DepartureAnchorID.IsNone())IDs.Add(Segment.DepartureAnchorID);IDs.Add(Segment.DestinationAnchorID);}
        for(FName ID:IDs)
        {
            const auto* Definition=D->Find<FICAnchorRow>(ID,Result);FTransform Existing;
            if(!Definition||Definition->OwnerNPCID!=NPCIDs[Index]||Spawns->ResolveLoadedAnchor(ID,Existing))continue;
            FTransform Transform=StandingTransform(Index);
            if(Definition->Type==EICAnchorType::Bed)
                Transform.SetLocation(BedTransform(Index).GetLocation()+FVector(0,0,AnchorHeightCm));
            auto* Anchor=GetWorld()->SpawnActorDeferred<AICNPCWorldAnchor>(AICNPCWorldAnchor::StaticClass(),Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if(!Anchor)continue;
            Anchor->AnchorID=ID;UGameplayStatics::FinishSpawningActor(Anchor,Transform);
            Anchor->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);GeneratedAnchors.Add(Anchor);
        }
        if(InteractionType)
        {
            const FVector Position=BedTransform(Index).GetLocation()+FVector(0,0,AnchorHeightCm);
            const FTransform Transform(GetActorQuat(),Position);
            auto* Bed=GetWorld()->SpawnActorDeferred<AICLifeObjectActor>(InteractionType,Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if(Bed)
            {
                Bed->ObjectID=FName(*FString::Printf(TEXT("BED_%s"),*NPCIDs[Index].ToString()));Bed->OwnerNPCID=NPCIDs[Index];Bed->RegionID=TEXT("R01");
                if(const auto* Home=D->Find<FICHomeRow>(NPC->HomeID,Result);Home&&!Home->BedAnchorIDs.IsEmpty())Bed->LinkedAnchorID=Home->BedAnchorIDs[0];
                UGameplayStatics::FinishSpawningActor(Bed,Transform);Bed->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);GeneratedBedInteractions.Add(Bed);
            }
        }
        if(BedType)
        {
            FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            if(auto* Bed=GetWorld()->SpawnActor<AActor>(BedType,BedTransform(Index),Params))
            {
                Bed->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
                // Test props must not block the schedule target or carve a hole in its NavMesh.
                TInlineComponentArray<UPrimitiveComponent*> Components;Bed->GetComponents(Components);
                for(auto* Primitive:Components){Primitive->SetCanEverAffectNavigation(false);Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
                GeneratedBeds.Add(Bed);
            }
        }
    }
    UE_LOG(LogICWorldState,Log,TEXT("[NPC.TestLayout] NPCs=%d Anchors=%d Beds=%d"),NPCIDs.Num(),GeneratedAnchors.Num(),GeneratedBeds.Num());
}
void AICNPCTestLayout::EndPlay(const EEndPlayReason::Type Reason)
{
    for(AICLifeObjectActor* Bed:GeneratedBedInteractions)if(IsValid(Bed))Bed->Destroy();GeneratedBedInteractions.Reset();
    for(AActor* Bed:GeneratedBeds)if(IsValid(Bed))Bed->Destroy();
    for(AICWorldAnchor* Anchor:GeneratedAnchors)if(IsValid(Anchor))Anchor->Destroy();
    GeneratedBeds.Reset();GeneratedAnchors.Reset();Super::EndPlay(Reason);
}
