#include "World/Spawn/ICSpawnSubsystem.h"
#include "UI/Framework/ICText.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "World/Spawn/ICPlacedMonsterSpawnPoint.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Debug/ICLog.h"
bool UICSpawnSubsystem::DoesSupportWorldType(EWorldType::Type Type) const { IC_TRACE_FUNCTION();return Type==EWorldType::Game||Type==EWorldType::PIE; }
void UICSpawnSubsystem::OnWorldBeginPlay(UWorld& World)
{
    IC_TRACE_FUNCTION();Super::OnWorldBeginPlay(World);bReady=true;if(auto* GI=GetWorld()->GetGameInstance())if(auto* S=GI->GetSubsystem<UICWorldStateSubsystem>())S->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::WorldRestored);
}
TStatId UICSpawnSubsystem::GetStatId() const { IC_TRACE_FUNCTION();RETURN_QUICK_DECLARE_CYCLE_STAT(UICSpawnSubsystem,STATGROUP_Tickables); }
void UICSpawnSubsystem::Tick(float DeltaTime)
{
    IC_TRACE_FUNCTION();if(!bReady||bTearingDown)return;auto* GI=GetWorld()->GetGameInstance();auto* T=GI?GI->GetSubsystem<UICTimeSubsystem>():nullptr;if(!T||!T->IsRunning())return;
    if(T->GetClock().PlaySecond>=NextRefreshAt){NextRefreshAt=T->GetClock().PlaySecond+.5;RefreshSpawns();}
}
bool UICSpawnSubsystem::RegisterAnchor(AICWorldAnchor* Anchor)
{
    IC_TRACE_FUNCTION();if(!Anchor||Anchor->AnchorID.IsNone())return false;auto* GI=GetWorld()->GetGameInstance();auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    if(!D||!D->Find<FICAnchorRow>(Anchor->AnchorID,R)){UE_LOG(LogICData,Warning,TEXT("[Anchor.MissingDefinition] %s %s"),*Anchor->AnchorID.ToString(),*R.Reason);return false;}
    auto* Existing=Anchors.Find(Anchor->AnchorID);if(Existing&&Existing->IsValid()&&Existing->Get()!=Anchor){UE_LOG(LogICWorldState,Warning,TEXT("[Anchor.Duplicate] %s"),*Anchor->AnchorID.ToString());return false;}
    Anchors.Add(Anchor->AnchorID,Anchor);NextRefreshAt=-1;return true;
}
void UICSpawnSubsystem::UnregisterAnchor(AICWorldAnchor* Anchor) { IC_TRACE_FUNCTION();if(Anchor){auto* A=Anchors.Find(Anchor->AnchorID);if(A&&A->Get()==Anchor)Anchors.Remove(Anchor->AnchorID);}if(!bTearingDown)RefreshSpawns(); }
bool UICSpawnSubsystem::IsPlacedSpawnerID(FName ID)
{
    const FString Text=ID.ToString();FGuid Guid;
    return Text.StartsWith(TEXT("PLACED_MONSTER_"))&&FGuid::ParseExact(Text.Mid(15),EGuidFormats::Digits,Guid)&&Guid.IsValid();
}
bool UICSpawnSubsystem::RegisterPlacedMonster(AICEnemyBase* Monster)
{
    if(!IsValid(Monster)||bTearingDown||!Monster->PlacementGuid.IsValid())return false;
    auto* GI=GetWorld()->GetGameInstance();auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    if(!D||!D->Find<FICMonsterRow>(Monster->MonsterID,R))
    {UE_LOG(LogICData,Warning,TEXT("[Spawn.PlacedMonsterMissingDefinition] Actor=%s MonsterID=%s"),*Monster->GetPathName(),*Monster->MonsterID.ToString());return false;}
    const FString Level=UWorld::RemovePIEPrefix(Monster->GetLevel()->GetOutermost()->GetName());
    const FName ID(*(TEXT("PLACED_MONSTER_")+FGuid::NewDeterministicGuid(Level+TEXT(":")+Monster->PlacementGuid.ToString(EGuidFormats::Digits)).ToString(EGuidFormats::Digits)));
    if(const auto* Existing=PlacedMonsterPoints.Find(ID);Existing&&Existing->IsValid())
    {UE_LOG(LogICData,Warning,TEXT("[Spawn.PlacedMonsterDuplicateID] Actor=%s ID=%s"),*Monster->GetPathName(),*ID.ToString());return false;}
    FActorSpawnParameters Params;Params.OverrideLevel=Monster->GetLevel();Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Point=GetWorld()->SpawnActor<AICPlacedMonsterSpawnPoint>(AICPlacedMonsterSpawnPoint::StaticClass(),Monster->GetActorTransform(),Params);
    if(!Point)return false;
    Point->SpawnerID=ID;Point->PlacementGuid=Monster->PlacementGuid;Point->MonsterClass=Monster->GetClass();Point->Slot.MonsterID=Monster->MonsterID;Point->Slot.LeashRadiusCm=FMath::Max(50.,Monster->PlacedLeashRadiusCm);
    PlacedMonsterPoints.Add(ID,Point);Enemies.Add(FName(*(ID.ToString()+TEXT(":0"))),Monster);
    Monster->SetActorHiddenInGame(true);Monster->SetActorEnableCollision(false);NextRefreshAt=-1;
    return true;
}
void UICSpawnSubsystem::UnregisterPlacedMonster(AICPlacedMonsterSpawnPoint* Point)
{
    if(Point)if(const auto* Existing=PlacedMonsterPoints.Find(Point->SpawnerID);Existing&&Existing->Get()==Point)
    {PlacedMonsterPoints.Remove(Point->SpawnerID);NextRefreshAt=-1;}
}
bool UICSpawnSubsystem::ResolveLoadedAnchor(FName ID,FTransform& Transform) const { IC_TRACE_FUNCTION();const auto* A=Anchors.Find(ID);if(!A||!A->IsValid())return false;Transform=A->Get()->GetActorTransform();return true; }
AICCharacterBase* UICSpawnSubsystem::FindCharacter(FName ID) const
{ IC_TRACE_FUNCTION();for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->GetCharacterID()==ID&&!It->IsActorBeingDestroyed())return *It;return nullptr; }
bool UICSpawnSubsystem::FindSafePartyPlacement(FName ID,FTransform& Transform) const
{
    IC_TRACE_FUNCTION();auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);if(!Player)return false;
    auto* Existing = FindCharacter(ID);
    const auto* Capsule = Existing ? Existing->GetCapsuleComponent() : nullptr;
    const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 42.f;
    const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
    FCollisionQueryParams Q;Q.AddIgnoredActor(Player);if(Existing)Q.AddIgnoredActor(Existing);
    FCollisionQueryParams Clearance;
    if (Existing) Clearance.AddIgnoredActor(Existing);
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    const bool HasNavigation = Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
    // Prefer navigation, then allow clear walkable ground when the player has left the navigation bounds.
    for (int32 Pass = 0; Pass < (HasNavigation ? 2 : 1); ++Pass)
    for(int32 I=0;I<12;++I)
    {
        const double Angle=PI+I*PI/6.;FVector Position=Player->GetActorLocation()+Player->GetActorRotation().RotateVector(FVector(FMath::Cos(Angle),FMath::Sin(Angle),0))*(150+(I/6)*100);
        if (HasNavigation && Pass == 0)
        {
            FNavLocation Point;
            if (!Nav->ProjectPointToNavigation(Position, Point, FVector(80,80,200))) continue;
            Position = Point.Location;
        }
        FHitResult Floor;if(!GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,150),Position-FVector(0,0,400),ECC_Visibility,Q)||Floor.ImpactNormal.Z<.65)continue;
        // A capsule resting on a slope needs more vertical clearance than on a
        // flat floor. Otherwise every candidate can overlap walkable terrain.
        const float SlopeClearance=Radius*(1.f/Floor.ImpactNormal.Z-1.f)+2.f;
        Position=Floor.ImpactPoint+FVector(0,0,HalfHeight+SlopeClearance);
        if(GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,HalfHeight),Clearance))continue;
        Transform=FTransform(Player->GetActorRotation(),Position);return true;
    }
    return false;
}
void UICSpawnSubsystem::RefreshSpawns()
{
    IC_TRACE_FUNCTION();if(!bReady||bRefreshing||bTearingDown)return;TGuardValue<bool> Guard(bRefreshing,true);auto* GI=GetWorld()->GetGameInstance();auto* S=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;auto* T=GI?GI->GetSubsystem<UICTimeSubsystem>():nullptr;if(!S||!D||!T||!S->ExportState().bPlayerCreated)return;
    FICDefinitionResult R;
    for(const auto& Pair:S->ExportState().SpawnSlots)if(Pair.Value.bDefeated&&Pair.Value.bRewardPending)if(const auto* Monster=D->Find<FICMonsterRow>(Pair.Value.MonsterID,R))
    {auto Rewards=S->ExportState();FString Reason;if(FICRewardService::PrepareMonsterDefeat(Rewards,Pair.Key,*Monster,D,T->GetClock().PlaySecond,Reason))S->CommitGameplayTransaction(Rewards,Pair.Value.IncarnationID);}
    TArray<FName> IDs;D->GetDefinitionIDs(EICDefinitionType::NPC,IDs);
    for(FName ID:IDs)
    {
        const auto* Row=D->Find<FICNPCRow>(ID,R);if(!Row)continue;const auto* Stored=S->GetStoredCharacter(ID);FTransform Position;bool Wanted=false;
        const bool Party=Stored&&Stored->PartySlot>=0;
        const auto* Reservation=ReviveReservations.Find(ID);const bool Reserved=Reservation&&Reservation->IsValid()&&Reservation->Get()->GetActionID().IsValid();
        if(Party)Wanted=Reserved||(!Stored->IsDowned()&&(FindCharacter(ID)||FindSafePartyPlacement(ID,Position)));
        else
        {
            const FName RequiredAnchor=Stored&&!Stored->CurrentAnchorID.IsNone()?Stored->CurrentAnchorID:Row->InitialAnchorID;
            Wanted=ResolveLoadedAnchor(RequiredAnchor,Position);
            if(!Wanted)
            {
                if(!NPCsWaitingForAnchor.Contains(ID))UE_LOG(LogICWorldState,Warning,TEXT("[Spawn.NPCWaitingForAnchor] NPC=%s Anchor=%s"),*ID.ToString(),*RequiredAnchor.ToString());
                NPCsWaitingForAnchor.Add(ID);
            }
            else NPCsWaitingForAnchor.Remove(ID);
        }
        auto* Existing=Cast<AICNPCBase>(FindCharacter(ID));if(Existing){NPCs.Add(ID,Existing);if(!Wanted&&!Existing->IsActorBeingDestroyed())Existing->Destroy();continue;}if(!Wanted)continue;
        UClass* Type=Row->ActorClass.IsNull()?AICNPCBase::StaticClass():Row->ActorClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICNPCBase::StaticClass())){UE_LOG(LogICData,Warning,TEXT("[Spawn.NPCClassInvalid] %s"),*ID.ToString());continue;}
        // 앵커가 바닥·벽에 살짝 겹쳐 있어도 근처 빈 자리로 옮겨 생성한다(완전히 막혀 있으면 생성하지 않는다).
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        auto* Actor=GetWorld()->SpawnActor<AICNPCBase>(Type,Position,SpawnParams);
        if(!Actor)
        {
            UE_LOG(LogICWorldState,Warning,TEXT("[Spawn.NPCCreateFailed] NPC=%s Class=%s Location=%s 앵커 위치가 바닥/벽/다른 오브젝트에 막혀 있습니다. 앵커를 바닥에서 약 100cm 위, 빈 공간으로 옮겨 주세요."),*ID.ToString(),*GetNameSafe(Type),*Position.GetLocation().ToCompactString());
        }
        else if(!Actor->InitializeNPC(ID))
        {
            UE_LOG(LogICWorldState,Warning,TEXT("[Spawn.NPCInitializeFailed] NPC=%s Class=%s"),*ID.ToString(),*GetNameSafe(Type));
            Actor->Destroy();
        }
        else
        {
            NPCs.Add(ID,Actor);
            UE_LOG(LogICWorldState,Log,TEXT("[Spawn.NPCCreated] NPC=%s Actor=%s AnchorLocation=%s"),*ID.ToString(),*Actor->GetName(),*Position.GetLocation().ToCompactString());
        }
    }
    D->GetDefinitionIDs(EICDefinitionType::Spawner,IDs);TSet<FName> WantedKeys;
    for(FName ID:IDs)if(const auto* Row=D->Find<FICSpawnerRow>(ID,R))for(const auto& Slot:Row->Slots)
    {
        FTransform Origin;if(!ResolveLoadedAnchor(Slot.OriginAnchorID,Origin))continue;const auto* Monster=D->Find<FICMonsterRow>(Slot.MonsterID,R);if(!Monster)continue;
        if(auto* Player=Cast<AICCharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(),0)))if(Player->GetICAbilitySystem()&&!Player->GetICAbilitySystem()->Conditions(Slot.Availability))continue;
        const FName Key=FName(*(ID.ToString()+TEXT(":")+FString::FromInt(Slot.SpawnSlotIndex)));WantedKeys.Add(Key);
        UClass* Type=Monster->PawnClass.IsNull()?AICEnemyBase::StaticClass():Monster->PawnClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICEnemyBase::StaticClass())){UE_LOG(LogICData,Warning,TEXT("[Spawn.EnemyClassInvalid] %s"),*Slot.MonsterID.ToString());continue;}
        RefreshEnemySlot(ID,Slot,Origin,Type,false);
    }
    for(auto It=PlacedMonsterPoints.CreateIterator();It;++It)
    {
        auto* Point=It.Value().Get();if(!Point){It.RemoveCurrent();continue;}
        WantedKeys.Add(FName(*(Point->SpawnerID.ToString()+TEXT(":0"))));
        RefreshEnemySlot(Point->SpawnerID,Point->Slot,Point->GetActorTransform(),Point->MonsterClass,true);
    }
    for(auto It=Enemies.CreateIterator();It;++It)if(!WantedKeys.Contains(It.Key())){if(It.Value().IsValid())It.Value()->Destroy();It.RemoveCurrent();}
}
void UICSpawnSubsystem::RefreshEnemySlot(FName ID,const FICSpawnSlotDefinition& Slot,const FTransform& Origin,UClass* Class,bool bPlaced)
{
    auto* GI=GetWorld()->GetGameInstance();auto* S=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* T=GI->GetSubsystem<UICTimeSubsystem>();FICDefinitionResult R;
    const auto* Monster=D->Find<FICMonsterRow>(Slot.MonsterID,R);if(!Monster||!Class||!Class->IsChildOf(AICEnemyBase::StaticClass()))return;
    const FName Key(*(ID.ToString()+TEXT(":")+FString::FromInt(Slot.SpawnSlotIndex)));
    auto World=S->ExportState();auto* Saved=World.SpawnSlots.Find(Key);
    AICEnemyBase* Existing=Enemies.FindRef(Key).Get();
    if(Saved&&Saved->bDefeated)
    {
        if(Saved->RespawnReadyAtGameMinute<0)
        {Saved->RespawnReadyAtGameMinute=T->GetClock().GameMinute+FMath::Max(0.,Monster->RespawnGameMinutes);S->ApplyWorldProgress(World);}
        // A placed copy loaded during its cooldown must not resurrect the enemy.
        if(Existing&&!Existing->HasInitializedEnemy()){Existing->ReleaseForLoad();Existing->Destroy();Enemies.Remove(Key);Existing=nullptr;}
        if(Saved->bRewardPending||T->GetClock().GameMinute<Saved->RespawnReadyAtGameMinute)return;
        if(Existing){Existing->Destroy();Enemies.Remove(Key);Existing=nullptr;}
    }
    if(Existing&&Existing->HasInitializedEnemy())return;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if(bPlaced)if(auto* Point=PlacedMonsterPoints.FindRef(ID).Get())Params.OverrideLevel=Point->GetLevel();
    auto* Actor=Existing?Existing:GetWorld()->SpawnActor<AICEnemyBase>(Class,Origin,Params);
    if(!Actor)return;
    if(bPlaced)if(auto* Point=PlacedMonsterPoints.FindRef(ID).Get())Actor->PlacementGuid=Point->PlacementGuid;
    if(!Actor->InitializeEnemy(ID,Slot,Origin,bPlaced)){Actor->ReleaseForLoad();Actor->Destroy();Enemies.Remove(Key);return;}
    Enemies.Add(Key,Actor);
}
bool UICSpawnSubsystem::FlushActors() { IC_TRACE_FUNCTION();auto* GI=GetWorld()->GetGameInstance();auto* S=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;if(!S)return true;for(const auto& E:Enemies)if(E.Value.IsValid()&&!E.Value->FlushToWorldState_Implementation(S))return false;return true; }
bool UICSpawnSubsystem::CanRestoreActors(const FICWorldState& State) const
{
    IC_TRACE_FUNCTION();auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    if(!D)return false;
    for(const auto& Pair:State.SpawnSlots)
    {
        if(!D->Find<FICMonsterRow>(Pair.Value.MonsterID,R))return false;
        if(IsPlacedSpawnerID(Pair.Value.SpawnerID))
        {
            if(Pair.Value.SlotIndex!=0||Pair.Key!=FName(*(Pair.Value.SpawnerID.ToString()+TEXT(":0"))))return false;
            if(auto* Point=PlacedMonsterPoints.FindRef(Pair.Value.SpawnerID).Get();Point&&Point->Slot.MonsterID!=Pair.Value.MonsterID)return false;
            continue; // Other streamed levels/maps may not have loaded their points yet.
        }
        const auto* Row=D->Find<FICSpawnerRow>(Pair.Value.SpawnerID,R);
        if(!Row||!Row->Slots.ContainsByPredicate([&Pair](const auto& Slot){return Slot.SpawnSlotIndex==Pair.Value.SlotIndex&&Slot.MonsterID==Pair.Value.MonsterID;}))return false;
    }
    return true;
}
void UICSpawnSubsystem::RestoreActors()
{
    IC_TRACE_FUNCTION();TGuardValue<bool> Guard(bRefreshing,true);auto N=NPCs;auto E=Enemies;NPCs.Reset();Enemies.Reset();ReviveReservations.Reset();NPCsWaitingForAnchor.Reset();

    for(const auto& P:N)if(P.Value.IsValid()){P.Value->ReleaseForLoad();P.Value->Destroy();}
    for(const auto& P:E)if(P.Value.IsValid()){P.Value->ReleaseForLoad();P.Value->Destroy();}NextRefreshAt=-1;
}
void UICSpawnSubsystem::WorldRestored() { IC_TRACE_FUNCTION();RestoreActors(); }
bool UICSpawnSubsystem::CanReviveAtCurrentPlacement(AICCharacterBase* Target) const
{
    IC_TRACE_FUNCTION();if(!Target)return false;FCollisionQueryParams Q;Q.AddIgnoredActor(Target);
    return !GetWorld()->OverlapBlockingTestByChannel(Target->GetActorLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,90),Q);
}
AICNPCBase* UICSpawnSubsystem::ReserveReviveTarget(FName ID,UICAbilitySystemComponent* Caster,FString& Reason)
{
    IC_TRACE_FUNCTION();Reason.Empty();if(!Caster){Reason=ICText::Get(TEXT("Native_01e3d20fd15d")).ToString();return nullptr;}
    auto* S=GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();const auto State=S->GetSnapshot(ID);FTransform Transform;FICDefinitionResult R;
    if(State.CharacterId.IsNone()||!State.IsDowned()||State.PartySlot<0||State.Kind!=EICCharacterKind::Recruitable){Reason=ICText::Get(TEXT("Native_e9d0ed549f03")).ToString();return nullptr;}
    if(auto* Existing=Cast<AICNPCBase>(FindCharacter(ID))){if(!CanReviveAtCurrentPlacement(Existing)){Reason=ICText::Get(TEXT("Native_06b639551f1b")).ToString();return nullptr;}Existing->HoldDownedActor();ReviveReservations.Add(ID,Caster);return Existing;}
    if(!FindSafePartyPlacement(ID,Transform)){Reason=ICText::Get(TEXT("Native_06b639551f1b")).ToString();return nullptr;}
    const auto* Row=D->Find<FICNPCRow>(ID,R);UClass* Type=Row?(Row->ActorClass.IsNull()?AICNPCBase::StaticClass():Row->ActorClass.LoadSynchronous()):nullptr;
    if(!Type||!Type->IsChildOf(AICNPCBase::StaticClass())){Reason=ICText::Get(TEXT("Native_d21587f276c1")).ToString();return nullptr;}
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Actor=GetWorld()->SpawnActor<AICNPCBase>(Type,Transform,Params);
    if(!Actor||!Actor->InitializeNPC(ID)){if(Actor){Actor->ReleaseForLoad();Actor->Destroy();}Reason=ICText::Get(TEXT("Native_8bee5d678230")).ToString();return nullptr;}
    Actor->SetActorEnableCollision(false);NPCs.Add(ID,Actor);ReviveReservations.Add(ID,Caster);return Actor;
}
void UICSpawnSubsystem::Deinitialize()
{
    IC_TRACE_FUNCTION();bTearingDown=true;if(auto* GI=GetWorld()->GetGameInstance())if(auto* S=GI->GetSubsystem<UICWorldStateSubsystem>())S->OnWorldRestored.RemoveDynamic(this,&ThisClass::WorldRestored);Anchors.Reset();NPCs.Reset();Enemies.Reset();PlacedMonsterPoints.Reset();NPCsWaitingForAnchor.Reset();Super::Deinitialize();
}
