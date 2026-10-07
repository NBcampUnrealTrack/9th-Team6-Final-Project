#include "World/Spawn/ICSpawnSubsystem.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
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
bool UICSpawnSubsystem::ResolveLoadedAnchor(FName ID,FTransform& Transform) const { IC_TRACE_FUNCTION();const auto* A=Anchors.Find(ID);if(!A||!A->IsValid())return false;Transform=A->Get()->GetActorTransform();return true; }
AICCharacterBase* UICSpawnSubsystem::FindCharacter(FName ID) const
{ IC_TRACE_FUNCTION();for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->GetCharacterID()==ID&&!It->IsActorBeingDestroyed())return *It;return nullptr; }
bool UICSpawnSubsystem::FindSafePartyPlacement(FName ID,FTransform& Transform) const
{
    IC_TRACE_FUNCTION();auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);if(!Player)return false;
    FCollisionQueryParams Q;Q.AddIgnoredActor(Player);if(auto* Existing=FindCharacter(ID))Q.AddIgnoredActor(Existing);
    for(int32 I=0;I<12;++I)
    {
        const double Angle=I*PI/6.;FVector Position=Player->GetActorLocation()+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*(150+(I/6)*100);
        FHitResult Floor;if(!GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,150),Position-FVector(0,0,400),ECC_Visibility,Q)||Floor.ImpactNormal.Z<.65)continue;Position=Floor.ImpactPoint+FVector(0,0,96);
        if(GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,90),Q))continue;
        if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))if(Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) {FNavLocation Point;if(!Nav->ProjectPointToNavigation(Floor.ImpactPoint,Point,FVector(80,80,200)))continue;}
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
        else Wanted=ResolveLoadedAnchor(Stored&&!Stored->CurrentAnchorID.IsNone()?Stored->CurrentAnchorID:Row->InitialAnchorID,Position);
        auto* Existing=Cast<AICNPCBase>(FindCharacter(ID));if(Existing){NPCs.Add(ID,Existing);if(!Wanted&&!Existing->IsActorBeingDestroyed())Existing->Destroy();continue;}if(!Wanted)continue;
        UClass* Type=Row->ActorClass.IsNull()?AICNPCBase::StaticClass():Row->ActorClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICNPCBase::StaticClass())){UE_LOG(LogICData,Warning,TEXT("[Spawn.NPCClassInvalid] %s"),*ID.ToString());continue;}
        if(Stored&&!Party)Position=FTransform(Stored->Rotation,Stored->Position);
        auto* Actor=GetWorld()->SpawnActor<AICNPCBase>(Type,Position);if(Actor){if(!Actor->InitializeNPC(ID))Actor->Destroy();else NPCs.Add(ID,Actor);}
    }
    D->GetDefinitionIDs(EICDefinitionType::Spawner,IDs);TSet<FName> WantedKeys;
    for(FName ID:IDs)if(const auto* Row=D->Find<FICSpawnerRow>(ID,R))for(const auto& Slot:Row->Slots)
    {
        FTransform Origin;if(!ResolveLoadedAnchor(Slot.OriginAnchorID,Origin))continue;const auto* Monster=D->Find<FICMonsterRow>(Slot.MonsterID,R);if(!Monster)continue;
        if(auto* Player=Cast<AICCharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(),0)))if(Player->GetICAbilitySystem()&&!Player->GetICAbilitySystem()->Conditions(Slot.Availability))continue;
        const FName Key=FName(*(ID.ToString()+TEXT(":")+FString::FromInt(Slot.SpawnSlotIndex)));WantedKeys.Add(Key);if(auto* A=Enemies.Find(Key);A&&A->IsValid())continue;
        const auto World=S->ExportState();const auto* Saved=World.SpawnSlots.Find(Key);if(Saved&&Saved->bDefeated&&(Saved->bRewardPending||T->GetClock().PlaySecond<Saved->RespawnReadyAtPlaySecond))continue;
        UClass* Type=Monster->PawnClass.IsNull()?AICEnemyBase::StaticClass():Monster->PawnClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICEnemyBase::StaticClass())){UE_LOG(LogICData,Warning,TEXT("[Spawn.EnemyClassInvalid] %s"),*Slot.MonsterID.ToString());continue;}
        // Preserve the corpse's deadline until it has expired; reset the new incarnation only after successful spawn.
        auto* Actor=GetWorld()->SpawnActor<AICEnemyBase>(Type,Origin);if(Actor){if(!Actor->InitializeEnemy(ID,Slot,Origin))Actor->Destroy();else {Enemies.Add(Key,Actor);auto Next=S->ExportState();auto& New=Next.SpawnSlots.FindChecked(Key);New.bDefeated=false;New.RespawnReadyAtPlaySecond=0;S->ApplyWorldProgress(Next);}}
    }
    for(auto It=Enemies.CreateIterator();It;++It)if(!WantedKeys.Contains(It.Key())){if(It.Value().IsValid())It.Value()->Destroy();It.RemoveCurrent();}
}
bool UICSpawnSubsystem::FlushActors() { IC_TRACE_FUNCTION();auto* GI=GetWorld()->GetGameInstance();auto* S=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;if(!S)return true;for(const auto& E:Enemies)if(E.Value.IsValid()&&!E.Value->FlushToWorldState_Implementation(S))return false;return true; }
bool UICSpawnSubsystem::CanRestoreActors(const FICWorldState& State) const
{
    IC_TRACE_FUNCTION();auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    for(const auto& Pair:State.SpawnSlots){const auto* Row=D?D->Find<FICSpawnerRow>(Pair.Value.SpawnerID,R):nullptr;if(!Row||!D->Find<FICMonsterRow>(Pair.Value.MonsterID,R)||!Row->Slots.ContainsByPredicate([&Pair](const auto& Slot){return Slot.SpawnSlotIndex==Pair.Value.SlotIndex&&Slot.MonsterID==Pair.Value.MonsterID;}))return false;}
    return true;
}
void UICSpawnSubsystem::RestoreActors()
{
    IC_TRACE_FUNCTION();TGuardValue<bool> Guard(bRefreshing,true);auto N=NPCs;auto E=Enemies;NPCs.Reset();Enemies.Reset();ReviveReservations.Reset();
    // EndPlay must not flush an old incarnation over the newly loaded persistent state.
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
    IC_TRACE_FUNCTION();Reason.Empty();if(!Caster){Reason=TEXT("시전자가 없습니다.");return nullptr;}
    auto* S=GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();const auto State=S->GetSnapshot(ID);FTransform Transform;FICDefinitionResult R;
    if(State.CharacterId.IsNone()||!State.IsDowned()||State.PartySlot<0||State.Kind!=EICCharacterKind::Recruitable){Reason=TEXT("부활할 Downed 동료가 없습니다.");return nullptr;}
    if(auto* Existing=Cast<AICNPCBase>(FindCharacter(ID))){if(!CanReviveAtCurrentPlacement(Existing)){Reason=TEXT("안전하게 부활할 공간이 없습니다.");return nullptr;}Existing->HoldDownedActor();ReviveReservations.Add(ID,Caster);return Existing;}
    if(!FindSafePartyPlacement(ID,Transform)){Reason=TEXT("안전하게 부활할 공간이 없습니다.");return nullptr;}
    const auto* Row=D->Find<FICNPCRow>(ID,R);UClass* Type=Row?(Row->ActorClass.IsNull()?AICNPCBase::StaticClass():Row->ActorClass.LoadSynchronous()):nullptr;
    if(!Type||!Type->IsChildOf(AICNPCBase::StaticClass())){Reason=TEXT("NPC 실행 클래스가 올바르지 않습니다.");return nullptr;}
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Actor=GetWorld()->SpawnActor<AICNPCBase>(Type,Transform,Params);
    if(!Actor||!Actor->InitializeNPC(ID)){if(Actor){Actor->ReleaseForLoad();Actor->Destroy();}Reason=TEXT("부활 대상의 상태를 준비하지 못했습니다.");return nullptr;}
    Actor->SetActorEnableCollision(false);NPCs.Add(ID,Actor);ReviveReservations.Add(ID,Caster);return Actor;
}
void UICSpawnSubsystem::Deinitialize()
{
    IC_TRACE_FUNCTION();bTearingDown=true;if(auto* GI=GetWorld()->GetGameInstance())if(auto* S=GI->GetSubsystem<UICWorldStateSubsystem>())S->OnWorldRestored.RemoveDynamic(this,&ThisClass::WorldRestored);Anchors.Reset();NPCs.Reset();Enemies.Reset();Super::Deinitialize();
}
