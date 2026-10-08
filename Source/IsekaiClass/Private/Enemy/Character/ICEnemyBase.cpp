#include "Enemy/Character/ICEnemyBase.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Engine/GameInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Debug/ICLog.h"
#include "Quest/ICQuestSubsystem.h"
AICEnemyBase::AICEnemyBase()
{
    IC_TRACE_FUNCTION();AbilitySystem=CreateDefaultSubobject<UICAbilitySystemComponent>(TEXT("ICAbilitySystem"));AIControllerClass=AICAIController::StaticClass();AutoPossessAI=EAutoPossessAI::Disabled;Tags.Add(TEXT("IC.Enemy"));
}
bool AICEnemyBase::InitializeEnemy(FName SpawnerID,const FICSpawnSlotDefinition& Slot,const FTransform& Origin)
{
    IC_TRACE_FUNCTION();auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* Row=D?D->Find<FICMonsterRow>(Slot.MonsterID,R):nullptr;if(!Row||!WorldState())return false;
    SpawnerDefinitionID=SpawnerID;SlotDefinition=Slot;Definition=*Row;OriginTransform=Origin;
    SpawnKey=FName(*(SpawnerID.ToString()+TEXT(":")+FString::FromInt(Slot.SpawnSlotIndex)));CharacterID=SpawnKey;
    EnableCombat(false,false);const auto& S=Definition.CombatStats;
    AbilitySystem->SetCombatBase(S.MaxHP,S.MaxMP,S.PhysicalAttack,S.MagicAttack,S.Defense,S.MoveSpeedCmPerSecond,S.AttackSpeedMultiplier,S.CriticalChance,S.CriticalMultiplier);
    FICCharacterState State;State.CharacterId=SpawnKey;State.Kind=EICCharacterKind::Background;State.MaxHP=State.HP=S.MaxHP;State.MaxMP=State.MP=S.MaxMP;State.SP=0;State.MaxSP=1;State.Position=Origin.GetLocation();State.Rotation=Origin.Rotator();
    if(!AbilitySystem->Restore(State))return false;AbilitySystem->SetOrdinaryStaggerImmunity(Definition.bImmuneToOrdinaryHitStagger);
    GetCharacterMovement()->MaxWalkSpeed=AbilitySystem->Stat(EICStat::MoveSpeed);
    if(auto* CharacterMesh=Definition.Mesh.LoadSynchronous())GetMesh()->SetSkeletalMesh(CharacterMesh);
    if(auto* Anim=Definition.AnimationClass.LoadSynchronous())GetMesh()->SetAnimInstanceClass(Anim);
    auto Next=WorldState()->ExportState();auto& InitialSlot=Next.SpawnSlots.FindOrAdd(SpawnKey);InitialSlot.SpawnerID=SpawnerID;InitialSlot.SlotIndex=Slot.SpawnSlotIndex;InitialSlot.MonsterID=Definition.ID;InitialSlot.MaxHP=InitialSlot.HP=S.MaxHP;InitialSlot.MaxMP=InitialSlot.MP=S.MaxMP;InitialSlot.bDefeated=false;InitialSlot.bRewardPending=false;InitialSlot.IncarnationID=FGuid::NewGuid();InitialSlot.RespawnReadyAtPlaySecond=0;if(!WorldState()->ApplyWorldProgress(Next))return false;
    if(!FlushToWorldState_Implementation(WorldState()))return false;
    if(!GetController())SpawnDefaultController();if(auto* AI=Cast<AICAIController>(GetController()))AI->StartExecution(nullptr,Definition.BehaviorTree.LoadSynchronous());
    return true;
}
bool AICEnemyBase::FlushToWorldState_Implementation(UICWorldStateSubsystem* State)
{
    IC_TRACE_FUNCTION();if(!State||SpawnKey.IsNone()||!AbilitySystem)return true;auto Candidate=State->ExportState();auto& S=Candidate.SpawnSlots.FindOrAdd(SpawnKey);const auto C=AbilitySystem->Capture();
    S.SpawnerID=SpawnerDefinitionID;S.SlotIndex=SlotDefinition.SpawnSlotIndex;S.MonsterID=Definition.ID;S.Position=C.Position;S.Rotation=C.Rotation;S.HP=C.HP;S.MP=C.MP;S.MaxHP=C.MaxHP;S.MaxMP=C.MaxMP;S.Combat=C.Combat;S.LastSimPlaySecond=C.LastSimPlaySecond;
    return State->ApplyWorldProgress(Candidate);
}
bool AICEnemyBase::OutsideLeash() const
{
    IC_TRACE_FUNCTION();const auto Local=OriginTransform.InverseTransformPosition(GetActorLocation());return SlotDefinition.LeashShape==EICLeashShape::Radius?Local.SizeSquared()>FMath::Square(SlotDefinition.LeashRadiusCm):!SlotDefinition.OriginRelativeLeashBoundsCm.IsInsideOrOn(Local);
}
void AICEnemyBase::BeginReturn()
{
    IC_TRACE_FUNCTION();if(bReturning||!AbilitySystem->IsAlive())return;bReturning=true;AbilitySystem->CancelAction();AbilitySystem->SetGuarding(false);
    const auto C=AbilitySystem->Capture();for(const auto& E:C.Combat.Effects)AbilitySystem->RemoveStatus(E.InstanceID);
    AbilitySystem->SetLooseGameplayTagCount(ICTags::Returning,1);AbilitySystem->SetVitals(C.MaxHP,C.MaxMP,0);
}
void AICEnemyBase::FinishReturn() { IC_TRACE_FUNCTION();bReturning=false;AbilitySystem->SetLooseGameplayTagCount(ICTags::Returning,0);FlushToWorldState_Implementation(WorldState()); }
void AICEnemyBase::HandleZeroHP()
{
    IC_TRACE_FUNCTION();if(bDefeatCommitted||!WorldState())return;bDefeatCommitted=true;if(auto* AI=Cast<AICAIController>(GetController()))AI->StopExecution();
    FlushToWorldState_Implementation(WorldState());auto State=WorldState()->ExportState();auto& S=State.SpawnSlots.FindChecked(SpawnKey);S.bDefeated=true;S.HP=0;
    S.RespawnReadyAtPlaySecond=AbilitySystem->PlaySecond()+(SlotDefinition.bOverrideRespawnTime?SlotDefinition.RespawnPlaySeconds:Definition.RespawnPlaySeconds);
    State.MonsterKilledAt.Add(SpawnKey,AbilitySystem->PlaySecond());if(Definition.Kind!=EICMonsterKind::Normal)State.DefeatedBosses.Add(Definition.ID);S.bRewardPending=true;
    if(!WorldState()->ApplyWorldProgress(State)){bDefeatCommitted=false;UE_LOG(LogICWorldState,Error,TEXT("[Enemy.DeathCommitFailed] %s"),*SpawnKey.ToString());return;}
    if (UICQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICQuestSubsystem>() : nullptr)
    {
        Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat, Definition.ID, 1, S.IncarnationID);
    }
    auto Rewards=WorldState()->ExportState();FString Reason;if(FICRewardService::PrepareMonsterDefeat(Rewards,SpawnKey,Definition,AbilitySystem->Definitions(),AbilitySystem->PlaySecond(),Reason))WorldState()->CommitGameplayTransaction(Rewards,Rewards.SpawnSlots.FindChecked(SpawnKey).IncarnationID);
    else UE_LOG(LogICInventory,Warning,TEXT("[Enemy.RewardPending] Key=%s Reason=%s"),*SpawnKey.ToString(),*Reason);
    UE_LOG(LogICWorldState,Log,TEXT("[Enemy.Defeated] Key=%s ReadyAt=%.2f"),*SpawnKey.ToString(),S.RespawnReadyAtPlaySecond);SetActorEnableCollision(false);SetLifeSpan(1.f);
    Super::HandleZeroHP();
}
void AICEnemyBase::EndPlay(const EEndPlayReason::Type Reason)
{
    IC_TRACE_FUNCTION();if(!bSuppressPersistence&&!bDefeatCommitted&&WorldState()&&!SpawnKey.IsNone())
    {FlushToWorldState_Implementation(WorldState());auto State=WorldState()->ExportState();auto& S=State.SpawnSlots.FindChecked(SpawnKey);S.Position=OriginTransform.GetLocation();S.Rotation=OriginTransform.Rotator();S.HP=S.MaxHP;S.MP=S.MaxMP;S.Combat=FICCombatState();WorldState()->ApplyWorldProgress(State);}
    Super::EndPlay(Reason);
}
