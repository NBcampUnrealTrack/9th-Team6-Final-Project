#include "Life/World/ICRanchAnimal.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Core/Controller/ICPlayerController.h"
#include "Animation/AnimSequence.h"

AICRanchAnimal::AICRanchAnimal()
{
    Tags.Add(TEXT("IC.Enemy"));GetCapsuleComponent()->InitCapsuleSize(24,40);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->SetMovementMode(MOVE_None);
}
FName AICRanchAnimal::PointKey() const{return PointID.IsNone()?DefinitionID:PointID;}
void AICRanchAnimal::BeginPlay()
{
    Super::BeginPlay();EnableCombat(false,false);
    AbilitySystem->OnStateChanged.AddUniqueDynamic(this,&ThisClass::PersistVitals);
    if(auto* State=WorldState())State->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RestoreAnimal);
    if(GetGameInstance())GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->OnTimeJumped.AddUniqueDynamic(this,&ThisClass::GameTimeJumped);
    RefreshAnimal();GetWorldTimerManager().SetTimer(RefreshTimer,this,&ThisClass::RefreshAnimal,.5f,true);
}
void AICRanchAnimal::GameTimeJumped(double From,double To){RefreshAnimal();}
void AICRanchAnimal::RefreshAnimal()
{
    if(bRefreshing||bSlaughtering||!WorldState()||!GetGameInstance()||!AbilitySystem)return;
    TGuardValue<bool> Guard(bRefreshing,true);auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    const auto* Row=D->Find<FICRanchingRow>(DefinitionID,R);if(!Row)return;
    if(GetMesh()->GetSkeletalMeshAsset()!=Row->Mesh.LoadSynchronous())GetMesh()->SetSkeletalMesh(Row->Mesh.LoadSynchronous());
    GetMesh()->SetRelativeTransform(Row->MeshTransform);
    if(auto* Anim=Row->AnimationClass.LoadSynchronous())if(GetMesh()->GetAnimClass()!=Anim)GetMesh()->SetAnimInstanceClass(Anim);
    if(Row->AnimationClass.IsNull())if(auto* Idle=Row->IdleAnimation.LoadSynchronous())
        if(!GetMesh()->GetSingleNodeInstance()||GetMesh()->GetSingleNodeInstance()->GetCurrentAsset()!=Idle)GetMesh()->PlayAnimation(Idle,true);
    auto Next=WorldState()->ExportState();if(!Next.bPlayerCreated)return;
    const double Now=GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute;
    auto* Saved=Next.RanchAnimals.Find(PointKey());
    if(!Saved||(Saved->bDefeated&&Row->RespawnGameMinutes>0&&Now>=Saved->RespawnAtGameMinute))
    {
        auto& New=Next.RanchAnimals.FindOrAdd(PointKey());New={};New.DefinitionID=DefinitionID;New.IncarnationID=FGuid::NewGuid();New.HP=Row->MaxHP;
        if(!WorldState()->CommitGameplayTransaction(Next,FGuid::NewGuid()))return;Saved=Next.RanchAnimals.Find(PointKey());
    }
    const bool Dead=Saved->bDefeated;
    SetActorHiddenInGame(Dead);SetActorEnableCollision(!Dead);bZeroHPHandled=Dead;
    if(!Dead&&(!AbilitySystem->IsAlive()||!FMath::IsNearlyEqual(AbilitySystem->Capture().HP,Saved->HP)))
    {
        AbilitySystem->SetCombatBase(Row->MaxHP,0,0,0,0,0,1,0,1);
        FICCharacterState Vitals;Vitals.CharacterId=PointKey();Vitals.Kind=EICCharacterKind::Resident;Vitals.HP=FMath::Clamp(Saved->HP,0.f,Row->MaxHP);Vitals.MaxHP=Row->MaxHP;
        Vitals.MP=Vitals.MaxMP=0;Vitals.SP=Vitals.MaxSP=0;AbilitySystem->Restore(Vitals);
        if(Saved->HP<=0)HandleZeroHP();
    }
}
void AICRanchAnimal::PersistVitals()
{
    if(bRefreshing||bZeroHPHandled||!WorldState()||!AbilitySystem)return;
    auto Next=WorldState()->ExportState();auto* Saved=Next.RanchAnimals.Find(PointKey());if(!Saved||Saved->bDefeated)return;
    const float HP=AbilitySystem->Capture().HP;if(FMath::IsNearlyEqual(Saved->HP,HP))return;
    Saved->HP=HP;WorldState()->CommitGameplayTransaction(Next,FGuid::NewGuid());
}
void AICRanchAnimal::HandleZeroHP()
{BeginSlaughter();}
bool AICRanchAnimal::TrySlaughter(AICPlayerController* Player)
{
    if(!Player||!Player->bCreated||Player->IsDead()||!Player->Menu.IsEmpty()||!Player->GetPawn()||IsHidden()
        ||FVector::DistSquared(Player->GetPawn()->GetActorLocation(),GetActorLocation())>FMath::Square(300.f))return false;
    return BeginSlaughter();
}
bool AICRanchAnimal::BeginSlaughter()
{
    if(bSlaughtering||bZeroHPHandled||!WorldState()||!GetGameInstance())return false;
    FICDefinitionResult R;const auto* Row=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICRanchingRow>(DefinitionID,R);if(!Row)return false;
    bSlaughtering=true;SetActorEnableCollision(false);GetCharacterMovement()->StopMovementImmediately();
    float Duration=FMath::Max(.1f,Row->SlaughterSeconds);
    if(auto* Death=Row->DeathAnimation.LoadSynchronous()){GetMesh()->PlayAnimation(Death,false);Duration=FMath::Max(Duration,Death->GetPlayLength());}
    GetWorldTimerManager().SetTimer(SlaughterTimer,this,&ThisClass::FinishSlaughter,Duration,false);return true;
}
void AICRanchAnimal::RestoreAnimal()
{GetWorldTimerManager().ClearTimer(SlaughterTimer);bSlaughtering=false;RefreshAnimal();}
void AICRanchAnimal::FinishSlaughter()
{
    bSlaughtering=false;
    if(bZeroHPHandled||!WorldState()||!GetGameInstance())return;
    auto* State=WorldState();if(!State->FlushParticipants())return;
    auto Next=State->ExportState();auto* Saved=Next.RanchAnimals.Find(PointKey());if(!Saved||Saved->bDefeated)return;
    FICDefinitionResult R;auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();const auto* Row=D->Find<FICRanchingRow>(DefinitionID,R);if(!Row)return;
    auto* Time=GetGameInstance()->GetSubsystem<UICTimeSubsystem>();TArray<FICItemAmount> Received;FString Reason;
    const FGuid Receipt=Saved->IncarnationID;
    if(!Row->DropTableID.IsNone()&&!FICRewardService::PrepareLifeLoot(Next,Row->DropTableID,Receipt,D,GetActorLocation(),Time->GetClock().PlaySecond,Received,Reason))return;
    auto& Defeated=Next.RanchAnimals.FindChecked(PointKey());Defeated.HP=0;Defeated.bDefeated=true;Defeated.RespawnAtGameMinute=Time->GetClock().GameMinute+Row->RespawnGameMinutes;
    Next.CompletedTransactionIDs.Add(Receipt);
    if(State->CommitGameplayTransaction(Next,FGuid::NewGuid()))
    {bZeroHPHandled=true;SetActorHiddenInGame(true);SetActorEnableCollision(false);if(!Received.IsEmpty())State->OnItemsReceived.Broadcast(Received);OnDefeated();}
    else RefreshAnimal();
}
void AICRanchAnimal::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(RefreshTimer);
    GetWorldTimerManager().ClearTimer(SlaughterTimer);
    if(AbilitySystem)AbilitySystem->OnStateChanged.RemoveDynamic(this,&ThisClass::PersistVitals);
    if(auto* State=WorldState())State->OnWorldRestored.RemoveDynamic(this,&ThisClass::RestoreAnimal);
    if(GetGameInstance())GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->OnTimeJumped.RemoveDynamic(this,&ThisClass::GameTimeJumped);
    Super::EndPlay(Reason);
}
