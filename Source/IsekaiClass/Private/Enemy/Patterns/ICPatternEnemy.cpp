#include "Enemy/Patterns/ICPatternEnemy.h"
#include "Enemy/Patterns/ICPatternBT.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"

namespace {
void PatchLocalTree(UBTCompositeNode* Node,UBehaviorTree* Tree)
{
 if (!Node) return;
 for (auto& Service:Node->Services)
  if (Service && Service->IsA<UBTService_ICMonsterState>()) Service=NewObject<UBTService_ICPatternState>(Tree);
 for(auto& Child:Node->Children)
 {
  if(Child.ChildTask && Child.ChildTask->IsA<UBTTask_ICMonsterAttack>()) Child.ChildTask=NewObject<UBTTask_ICPatternAttack>(Tree);
  PatchLocalTree(Child.ChildComposite,Tree);
 }
}
}
AICPatternEnemy::AICPatternEnemy()
{
 TelegraphDecal=CreateDefaultSubobject<UDecalComponent>(TEXT("AttackTelegraph"));
 TelegraphDecal->SetupAttachment(GetRootComponent());
 TelegraphDecal->SetRelativeRotation(FRotator(-90,0,0));
 TelegraphDecal->SetVisibility(false);
 TelegraphMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Enemy/Materials/M_AttackTelegraph.M_AttackTelegraph")));
}
void AICPatternEnemy::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (IsReturning())
 {
  CancelPatternWindup();
  ActivePattern=INDEX_NONE;
  StepIndex=0;
  NextStepAt=0;
 }
 auto* TelegraphASC=GetICAbilitySystem();
 if (bAwaitingTelegraph && (IsReturning() || !TelegraphASC || !TelegraphASC->IsAlive())) CancelPatternWindup();
 const bool bTelegraph=bAwaitingTelegraph && TelegraphASC && TelegraphASC->IsAlive();
 if (TelegraphDecal)
 {
  if (bTelegraph)
  {
   if (!TelegraphDecal->GetDecalMaterial()) TelegraphDecal->SetDecalMaterial(TelegraphMaterial.LoadSynchronous());
   FHitResult Floor;
   FCollisionQueryParams Query;Query.AddIgnoredActor(this);
   const FVector Location=GetActorLocation();
   const bool bFloor=GetWorld()->LineTraceSingleByChannel(Floor,Location+FVector(0,0,100),Location-FVector(0,0,2000),ECC_Visibility,Query);
   TelegraphDecal->DecalSize=FVector(100,TelegraphRadiusCm,TelegraphRadiusCm);
   TelegraphDecal->SetWorldLocation(bFloor?Floor.ImpactPoint:Location);
   TelegraphDecal->SetWorldRotation(FRotator(-90,0,0));
   TelegraphDecal->SetVisibility(bFloor && TelegraphDecal->GetDecalMaterial()!=nullptr);
   if (TelegraphEndsAt<0 && TelegraphDecal->IsVisible())
   {
    TelegraphEndsAt=TelegraphASC->PlaySecond()+FMath::Max(.1f,TelegraphLeadSeconds);
    UE_LOG(LogTemp,Log,TEXT("[MonsterPattern.TelegraphShown] Actor=%s ReleaseAt=%.3f"),*GetName(),TelegraphEndsAt);
   }
  }
  else TelegraphDecal->SetVisibility(false);
 }
 if (!bUseAttackPatterns || !HasInitializedEnemy()) return;
 if (!bProfileLoaded)
 {
  bProfileLoaded = true;
  if (TelegraphDecal && !TelegraphDecal->GetDecalMaterial()) TelegraphDecal->SetDecalMaterial(TelegraphMaterial.LoadSynchronous());
  if (PatternProfile.DataTable)
  {
   const auto* Profile = PatternProfile.GetRow<FICMonsterPatternProfileRow>(TEXT("Local monster patterns"));
   AttackPatterns = Profile ? Profile->Patterns : TArray<FICAttackPatternDefinition>();
   UE_LOG(LogTemp,Log,TEXT("[MonsterPattern.Profile] Actor=%s Row=%s Patterns=%d"),*GetName(),*PatternProfile.RowName.ToString(),AttackPatterns.Num());
  }
  if (auto* ASC=GetICAbilitySystem())
   for (const auto& Pattern:AttackPatterns)
    for (FName SkillID:Pattern.SkillSequence)
    {
     if (SkillID.IsNone()) continue;
     FICDefinitionResult Lookup;
     const auto* Skill=ASC->Definitions()->Find<FICSkillRow>(SkillID,Lookup);
     if (!Skill) continue;
     for (const auto& Reference:{Skill->Presentation.CastVFX,Skill->Presentation.ImpactVFX})
      if (auto* System=Reference.LoadSynchronous())
      {
       PreparedPatternVFX.AddUnique(System);
#if WITH_EDITOR
       if (!System->IsReadyToRun()) { System->RequestCompile(false); System->WaitForCompilationComplete(true,false); }
#endif
      }
    }
 }
 auto* AI=Cast<AICAIController>(GetController());
 if (!AI || LocalBehavior) return;
 auto* Original=GetMonsterDefinition().BehaviorTree.LoadSynchronous();
 if (!Original) return;
 LocalBehavior=DuplicateObject<UBehaviorTree>(Original,this);
 PatchLocalTree(LocalBehavior->RootNode,LocalBehavior);
 AI->StartExecution(nullptr,LocalBehavior);
 UE_LOG(LogTemp,Log,TEXT("[MonsterPattern.Integrated] %s"),*GetName());
}
double AICPatternEnemy::GetPatternChaseLimit() const
{
 const auto* ASC=GetICAbilitySystem();
 return FMath::Max(50.,GetMonsterDefinition().ChaseLimitCm)*(bUseAttackPatterns&&ASC&&ASC->HasRecentCombat()?FMath::Max(1.f,CombatReturnDistanceMultiplier):1.f);
}
bool AICPatternEnemy::IsOutsidePatternLeash(const FVector& Location) const
{
 const auto* ASC=GetICAbilitySystem();
 const double Radius=FMath::Max(50.,PlacedLeashRadiusCm)*(bUseAttackPatterns&&ASC&&ASC->HasRecentCombat()?FMath::Max(1.f,CombatReturnDistanceMultiplier):1.f);
 return FVector::DistSquared(Location,GetOrigin())>FMath::Square(Radius);
}
bool AICPatternEnemy::IsHeavyAttackCommitted() const
{
 const auto* ASC=GetICAbilitySystem();
 return ASC && ASC->IsAlive() && !IsReturning() && (bAwaitingTelegraph || (CommittedHeavyAction.IsValid() && ASC->GetActionID()==CommittedHeavyAction));
}
bool AICPatternEnemy::TryAttackPattern(AICCharacterBase* Target)
{
 if (!bUseAttackPatterns) return false;
 auto* ASC=GetICAbilitySystem();
 auto* AI=Cast<AICAIController>(GetController());
 if (!ASC||!AI||!Target||IsReturning()||!ASC->IsAlive()||!Target->GetICAbilitySystem()||!Target->GetICAbilitySystem()->IsAlive())
 { ActivePattern=INDEX_NONE; return false; }
    const double Now = ASC->PlaySecond();
    if (ASC->GetActionID().IsValid() || Now < NextStepAt) return true;
    const auto Snapshot = ASC->Capture();
    const float HP = Snapshot.MaxHP > 0 ? Snapshot.HP / Snapshot.MaxHP : 0.f;
    const float Distance = FVector::Dist(GetActorLocation(),Target->GetActorLocation());
    if (!AttackPatterns.IsValidIndex(ActivePattern))
    {
        ActivePattern = INDEX_NONE;
        for (int32 Index=0; Index<AttackPatterns.Num(); ++Index)
        {
            const auto& Pattern = AttackPatterns[Index];
            if (!Pattern.bEnabled || Pattern.SkillSequence.IsEmpty() || Pattern.MinHPRatio>Pattern.MaxHPRatio || Pattern.MinDistanceCm>Pattern.MaxDistanceCm
                || HP<Pattern.MinHPRatio || HP>Pattern.MaxHPRatio || Distance<Pattern.MinDistanceCm || Distance>Pattern.MaxDistanceCm
                || Now<ReadyAt.FindRef(Index) || (Pattern.bRequireLineOfSight&&!AI->LineOfSightTo(Target))
                || !ASC->Conditions(Pattern.Conditions,Target->GetICAbilitySystem())) continue;
            if (ActivePattern==INDEX_NONE || Pattern.Priority>AttackPatterns[ActivePattern].Priority) ActivePattern=Index;
        }
        StepIndex=0;
    }
    if (!AttackPatterns.IsValidIndex(ActivePattern)) return false;
    const auto& Pattern=AttackPatterns[ActivePattern];
    if (!Pattern.SkillSequence.IsValidIndex(StepIndex)) { ActivePattern=INDEX_NONE; return true; }
    const FRotator Facing=(Target->GetActorLocation()-GetActorLocation()).Rotation();
    SetActorRotation(FRotator(0,Facing.Yaw,0));
    const FName SkillID = Pattern.SkillSequence[StepIndex];
    AI->StopMovement();
    GetCharacterMovement()->StopMovementImmediately();
    const bool bReleasingTelegraph=bAwaitingTelegraph;
    if (bAwaitingTelegraph)
    {
        if (TelegraphEndsAt<0 || Now<TelegraphEndsAt) return true;
        bAwaitingTelegraph=false;
        TelegraphDecal->SetVisibility(false);
        UE_LOG(LogTemp,Log,TEXT("[MonsterPattern.TelegraphRelease] Actor=%s Time=%.3f"),*GetName(),Now);
    }
    else if (!SkillID.IsNone())
    {
        FICDefinitionResult Lookup;
        if (const auto* Skill=ASC->Definitions()->Find<FICSkillRow>(SkillID,Lookup); Skill && !Skill->Hits.IsEmpty() && Skill->Hits[0].Shape==EICHitShape::Sphere)
        {
            TelegraphRadiusCm=Skill->Hits[0].RadiusCm;
            TelegraphEndsAt=-1;
            CommittedTarget=Target;
            bTelegraphNeedsTarget=Skill->TargetRules.bRequiresTarget;
            bAwaitingTelegraph=true;
            return true;
        }
    }
    const auto Result=ASC->RequestAction(SkillID.IsNone() ? FName(TEXT("Attack")) : FName(TEXT("Skill")),SkillID,bReleasingTelegraph&&!bTelegraphNeedsTarget?nullptr:Target);
    if (!Result.bSuccess)
    {
        if (bShowAttackLog && GEngine)
            GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange,
                FString::Printf(TEXT("[%s] 공격 요청 실패 | 패턴: %s | 단계: %d | 공격: %s"),
                    *GetName(), *Pattern.PatternID.ToString(), StepIndex + 1,
                    SkillID.IsNone() ? TEXT("일반공격") : *SkillID.ToString()));
        UE_LOG(LogTemp,Warning,TEXT("[MonsterPattern.Rejected] Pattern=%s Skill=%s"),*Pattern.PatternID.ToString(),*Pattern.SkillSequence[StepIndex].ToString());
        ReadyAt.Add(ActivePattern,Now+FMath::Max(1.f,Pattern.CooldownSeconds));
        ActivePattern=INDEX_NONE;
        return true;
    }
    UE_LOG(LogTemp,Log,TEXT("[MonsterPattern.Step] Pattern=%s Step=%d"),*Pattern.PatternID.ToString(),StepIndex);
    if (bReleasingTelegraph) CommittedHeavyAction=Result.ActionID;
    if (bShowAttackLog && GEngine)
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.f, FColor::Green,
            FString::Printf(TEXT("[%s] 공격 시작 | 패턴: %s | 단계: %d/%d | 공격: %s"),
                *GetName(), *Pattern.PatternID.ToString(), StepIndex + 1, Pattern.SkillSequence.Num(),
                SkillID.IsNone() ? TEXT("일반공격") : *SkillID.ToString()));
    NextStepAt=Now+FMath::Max(0.f,Pattern.StepIntervalSeconds);
    if (++StepIndex>=Pattern.SkillSequence.Num())
    { ReadyAt.Add(ActivePattern,Now+FMath::Max(0.f,Pattern.CooldownSeconds)); ActivePattern=INDEX_NONE; }
    return true;
}


