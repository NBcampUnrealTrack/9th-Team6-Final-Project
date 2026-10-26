#include "Debug/ICMonsterTestLayout.h"
#include "Components/SceneComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

AICMonsterTestLayout::AICMonsterTestLayout()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("MonsterRowOrigin"));
    PrimaryActorTick.bCanEverTick=false;
}
FTransform AICMonsterTestLayout::OriginTransform(int32 Index) const
{
    // The layout faces the player; the row extends to the player's right (+Y).
    FVector Local(0,-Index*SpacingCm-(Index>=10?600:0),96);
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    FICDefinitionResult R;
    const auto* Spawner=D?D->Find<FICSpawnerRow>(SpawnerID,R):nullptr;
    if(Spawner&&Spawner->Slots.IsValidIndex(Index))
        if(const auto* Monster=D->Find<FICMonsterRow>(Spawner->Slots[Index].MonsterID,R))
            if(auto* Type=Monster->PawnClass.LoadSynchronous())
                if(const auto* Enemy=Cast<AICEnemyBase>(Type->GetDefaultObject()))Local.Z=Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2;
    return FTransform(GetActorQuat(),GetActorTransform().TransformPositionNoScale(Local));
}
void AICMonsterTestLayout::BeginPlay()
{
    Super::BeginPlay();
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>();
    FICDefinitionResult R;
    const auto* Spawner=D?D->Find<FICSpawnerRow>(SpawnerID,R):nullptr;
    if(!Spawner||!Spawns)return;
    for(int32 Index=0;Index<Spawner->Slots.Num();++Index)
    {
        const auto& Definition=Spawner->Slots[Index];FTransform Existing;
        if(Spawns->ResolveLoadedAnchor(Definition.OriginAnchorID,Existing))continue;
        const auto Transform=OriginTransform(Index);
        auto* Anchor=GetWorld()->SpawnActorDeferred<AICWorldAnchor>(AICWorldAnchor::StaticClass(),Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Anchor)continue;
        Anchor->AnchorID=Definition.OriginAnchorID;
        UGameplayStatics::FinishSpawningActor(Anchor,Transform);
        Anchor->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
        GeneratedAnchors.Add(Anchor);
    }
}
void AICMonsterTestLayout::EndPlay(const EEndPlayReason::Type Reason)
{
    for(AICWorldAnchor* Anchor:GeneratedAnchors)if(IsValid(Anchor))Anchor->Destroy();
    GeneratedAnchors.Reset();Super::EndPlay(Reason);
}
