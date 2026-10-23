#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimMontage.h"
#include "Save/ICSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "NPC/Character/ICNPCBase.h"
#include "AI/ICAIController.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Inventory/Use/ICItemUseService.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/Effects/ICGameplayEffects.h"
#include "Combat/GAS/Cues/ICCombatCueNotify.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
namespace
{
struct FGASFixture
{
    UGameInstance* GI;
    UWorld* World;
    UICDefinitionCatalog* Catalog;
    UICDefinitionSubsystem* Definitions;
    UICWorldStateSubsystem* State;
    UICTimeSubsystem* Time;
    AICPlayerCharacter* Player;
    UICAbilitySystemComponent* ASC;
    UDataTable* Skills;
    UDataTable* Status;
    FName SkillID=TEXT("SKL_GAS_998");
    template<class T> UDataTable* Table(EICDefinitionType Type,const TArray<T>& Rows)
    {
        IC_TRACE_FUNCTION();auto* Data=NewObject<UDataTable>();Data->RowStruct=T::StaticStruct();for(const auto& Row:Rows)Data->AddRow(Row.ID,Row);
        FICDefinitionBinding B;B.Type=Type;B.RowStruct=T::StaticStruct();B.Table=Data;Catalog->Tables.Add(B);return Data;
    }
    FGASFixture()
    {
        IC_TRACE_FUNCTION();GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();World=GI->GetWorld();Catalog=NewObject<UICDefinitionCatalog>(GI);
        Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();State=GI->GetSubsystem<UICWorldStateSubsystem>();Time=GI->GetSubsystem<UICTimeSubsystem>();
        FICStatSettingsRow Stats;Stats.ID=TEXT("CFG_STAT_001");Table<FICStatSettingsRow>(EICDefinitionType::Settings,{Stats});
        FICCombatSettingsRow Combat;Combat.ID=TEXT("CFG_COMBAT_001");Table<FICCombatSettingsRow>(EICDefinitionType::Settings,{Combat});
        FICNewGameRow NewGame;NewGame.ID=TEXT("CFG_NEWGAME_001");NewGame.InitialMaxHP=100;NewGame.InitialMaxMP=100;NewGame.InitialMaxSP=100;Table<FICNewGameRow>(EICDefinitionType::Settings,{NewGame});
        FICClassRow Class;Class.ID=TEXT("CLS_GAS_998");Class.CharacterClass=EICCharacterClass::Warrior;Table<FICClassRow>(EICDefinitionType::Class,{Class});
        FICSkillEffectRow Effect;Effect.ID=TEXT("SFX_GAS_998");Effect.Kind=EICSkillEffectKind::Heal;Effect.Magnitude.FlatAmount=10;Effect.bCanCritical=false;Table<FICSkillEffectRow>(EICDefinitionType::SkillEffect,{Effect});
        FICSkillRow Skill;Skill.ID=SkillID;Skill.ClassID=Class.ID;Skill.bInitiallyUnlocked=true;Skill.bRequiresEquippedWeapon=false;Skill.TargetRules.Target=EICCombatTarget::Self;Skill.TargetRules.bAllowCaster=true;Skill.TargetRules.bRequireLineOfSight=false;
        Skill.MPCost=10;Skill.CastPlaySeconds=.1;Skill.RecoveryPlaySeconds=.5;Skill.CooldownPlaySeconds=5;Skill.FirstEffectEvent=EICFirstEffectEvent::FirstRecovery;
        FICSkillHitDefinition Hit;Hit.EffectID=Effect.ID;Hit.StartOffsetPlaySeconds=.1;Hit.RepeatCount=2;Hit.RepeatIntervalPlaySeconds=.1;Skill.Hits={Hit};Skills=Table<FICSkillRow>(EICDefinitionType::Skill,{Skill});
        FICStatusEffectRow Shield;Shield.ID=TEXT("STE_GAS_998");Shield.Kind=EICStatusKind::Shield;Shield.DurationPlaySeconds=30;Shield.InitialShieldMagnitude.FlatAmount=20;
        FICStatusEffectRow Poison;Poison.ID=TEXT("STE_GAS_997");Poison.Kind=EICStatusKind::DamageOverTime;Poison.DurationPlaySeconds=3;Poison.TickIntervalPlaySeconds=1;Poison.PeriodicMagnitude.FlatAmount=10;Poison.bPeriodicCanCritical=false;Poison.bPeriodicCanBeAutoDodged=false;
        Status=Table<FICStatusEffectRow>(EICDefinitionType::StatusEffect,{Shield,Poison});Definitions->ConfigureCatalog(Catalog);
        World->InitializeActorsForPlay(FURL(),false);
        Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);ASC=Player->GetICAbilitySystem();
        auto* Inventory=Player->Inventory.Get();auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(Definitions,0);FICWorldState S;S.bPlayerCreated=true;S.PlayerName=TEXT("GAS 검증");S.FirstSkill=SkillID;S.Inventory=Staging->GetSnapshot();
        FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;P.UnlockedSkillIDs.Add(SkillID);S.Characters.Add(P.CharacterId,P);State->RestoreState(S);Player->InitializeState(P.CharacterId);Inventory->BindWorldState(State);
        Time->SetRunning(true);Time->SetMenuPaused(false);
    }
    ~FGASFixture() { IC_TRACE_FUNCTION();Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false); }
    void Step(double Seconds) { IC_TRACE_FUNCTION();Time->AdvanceClock(Seconds,false);ASC->TickComponent(Seconds,LEVELTICK_All,nullptr); }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASCommitCheck,"IC.GAS.FirstEffectCommit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASCommitCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;const auto Before=F.ASC->Capture();auto A=F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player);
    if(!TestTrue(TEXT("Real GAS ability activates"),A.bSuccess)){AddError(A.Reason);return false;}
    TestEqual(TEXT("Preparation costs no MP"),F.ASC->Capture().MP,Before.MP);F.ASC->CancelAction();TestEqual(TEXT("Pre-effect cancel costs no MP"),F.ASC->Capture().MP,Before.MP);
    TestFalse(TEXT("Retry delay blocks immediate recast"),F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player).bSuccess);F.Step(1.1);
    A=F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player);TestTrue(TEXT("Retry ready"),A.bSuccess);
    TestTrue(TEXT("First hit commits"),F.ASC->NotifyCombatEvent(A.ActionID,TEXT("AN_HitStart"),0));TestEqual(TEXT("MP paid once"),F.ASC->Capture().MP,90.f);TestEqual(TEXT("First recovery applies once"),F.ASC->Capture().HP,70.f);
    TestFalse(TEXT("Duplicate notify rejected"),F.ASC->NotifyCombatEvent(A.ActionID,TEXT("AN_HitStart"),0));TestTrue(TEXT("Next repeat hits"),F.ASC->NotifyCombatEvent(A.ActionID,TEXT("AN_HitStart"),1));TestEqual(TEXT("Repeat does not charge again"),F.ASC->Capture().MP,90.f);TestEqual(TEXT("Two distinct recoveries"),F.ASC->Capture().HP,80.f);
    F.ASC->CancelAction();TestFalse(TEXT("Old action notify rejected"),F.ASC->NotifyCombatEvent(A.ActionID,TEXT("AN_HitStart"),2));TestFalse(TEXT("Full cooldown persists after committed cancellation"),F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player).bSuccess);TestTrue(TEXT("Cooldown is captured by definition ID"),F.ASC->Capture().Combat.SkillReadyAtPlaySecond.FindRef(F.SkillID)>F.Time->GetClock().PlaySecond);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASSkillEventCheck,"IC.GAS.SkillGameplayEventAndCooldownEffect",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASSkillEventCheck::RunTest(const FString& Parameters)
{
    FGASFixture F;
    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_SWORD_001"));
    auto* Row = F.Skills->FindRow<FICSkillRow>(F.SkillID,TEXT("GAS test"));
    if (!TestNotNull(TEXT("Fixture skill"),Row)) return false;
    Row->CooldownTag=Tag;
    const auto Action=F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player);
    if (!TestTrue(TEXT("Dedicated skill ability starts"),Action.bSuccess)) return false;
    FGameplayEventData Event;Event.EventTag=ICTags::HitStartEvent;Event.Instigator=F.Player;Event.EventMagnitude=0;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(F.Player,ICTags::HitStartEvent,Event);
    TestEqual(TEXT("Gameplay Event reaches skill ability and commits MP"),F.ASC->Capture().MP,90.f);
    TestTrue(TEXT("Cooldown GE grants only this skill tag"),F.ASC->HasMatchingGameplayTag(Tag));
    const auto Saved=F.ASC->Capture();
    F.ASC->CancelAction();
    TestTrue(TEXT("Committed cooldown restores"),F.ASC->Restore(Saved));
    TestTrue(TEXT("Restored cooldown GE grants tag"),F.ASC->HasMatchingGameplayTag(Tag));
    F.Step(5.1);
    TestFalse(TEXT("PlayClock expiry removes cooldown GE tag"),F.ASC->HasMatchingGameplayTag(Tag));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASSkillAssetCheck,"IC.GAS.SkillAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASSkillAssetCheck::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Skill.DT_IC_Skill"));
    if (!TestNotNull(TEXT("Skill data table"),Table)) return false;
    TArray<FICSkillRow*> Rows;Table->GetAllRows(TEXT("GAS asset validation"),Rows);
    TestEqual(TEXT("Authored skill count"),Rows.Num(),16);
    for (const auto& Entry : TArray<TPair<FName,double>>{
        {TEXT("SKL_START_SWORD_001"),20.}, {TEXT("SKL_START_BOW_001"),30.}, {TEXT("SKL_START_STAFF_001"),30.}})
        if (const auto* Row=Table->FindRow<FICSkillRow>(Entry.Key,TEXT("Starter cooldown")))
            TestEqual(Entry.Key.ToString()+TEXT(" cooldown seconds"),Row->CooldownPlaySeconds,Entry.Value);
    for (const auto* Row:Rows)
    {
        const FString Label=Row->ID.ToString();
        auto* AbilityClass=Row->AbilityClass.LoadSynchronous();
        const auto* Effect=Row->CooldownEffectClass.LoadSynchronous();
        if (!TestNotNull(Label+TEXT(" ability class"),AbilityClass) || !TestNotNull(Label+TEXT(" effect"),Effect)) continue;
        const auto* Ability=Cast<UICSkillGameplayAbility>(AbilityClass->GetDefaultObject());
        if (!TestNotNull(Label+TEXT(" skill ability"),Ability)) continue;
        TestEqual(Label+TEXT(" ability ID"),Ability->SkillID,Row->ID);
        TestTrue(Label+TEXT(" cooldown tag"),Row->CooldownTag.IsValid()&&Ability->CooldownTag==Row->CooldownTag);
        TestTrue(Label+TEXT(" duration GE"),Effect->IsChildOf(UICSkillCooldownEffect::StaticClass()));
    }
    for (const TCHAR* Path:{TEXT("/Game/IsekaiClass/Combat/GAS/Cues/GC_IC_Combat_Cast.GC_IC_Combat_Cast_C"),TEXT("/Game/IsekaiClass/Combat/GAS/Cues/GC_IC_Combat_Impact.GC_IC_Combat_Impact_C")})
    {
        auto* Cue=LoadClass<UICCombatCueNotify>(nullptr,Path);
        if (TestNotNull(Path,Cue))
        {
            const auto* Defaults=Cue->GetDefaultObject<UICCombatCueNotify>();
            TestTrue(TEXT("Cue has a gameplay tag"),Defaults->GameplayCueTag.IsValid());
            TestFalse(TEXT("Cue has a default Niagara effect"),Defaults->DefaultVFX.IsNull());
            TestFalse(TEXT("Cue has a default sound"),Defaults->DefaultSound.IsNull());
        }
    }
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASResourcesCheck,"IC.GAS.ResourcesAndPause",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASResourcesCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;F.ASC->ChangeResource(TEXT("HP"),10000);TestEqual(TEXT("Instant GE clamps healing"),F.ASC->Capture().HP,100.f);F.ASC->ChangeResource(TEXT("MP"),-10000);TestEqual(TEXT("Instant GE clamps costs"),F.ASC->Capture().MP,0.f);
    F.ASC->AdvanceNaturalSP(9);TestEqual(TEXT("Incomplete SP interval"),F.ASC->Capture().SP,70.f);F.ASC->AdvanceNaturalSP(1);TestEqual(TEXT("Ten seconds cost one SP"),F.ASC->Capture().SP,69.f);
    auto* Pause=NewObject<UICDefinitionCatalog>(F.GI);F.Time->SetPauseRequest(Pause,TEXT("Menu"),true);const double Before=F.Time->GetClock().PlaySecond;F.Step(10);TestEqual(TEXT("Menus freeze PlayClock"),F.Time->GetClock().PlaySecond,Before);F.Time->ReleasePauseRequests(Pause);
    const auto Original=F.ASC->Capture();auto Stats=Original.MainStats;Stats.CON=100;F.ASC->SetBaseStats(Stats);TestEqual(TEXT("Max resource growth does not heal"),F.ASC->Capture().HP,Original.HP);F.ASC->SetBaseStats(Original.MainStats);TestTrue(TEXT("Current value clamps when maximum shrinks"),F.ASC->Capture().HP<=F.ASC->Capture().MaxHP);
    const auto Dodge=F.ASC->RequestAction(TEXT("Dodge"),NAME_None,nullptr);TestTrue(TEXT("Dodge starts without SP cost"),Dodge.bSuccess);TestTrue(TEXT("Dodge has invulnerability"),F.ASC->HasMatchingGameplayTag(ICTags::Invulnerable));F.ASC->CancelAction();TestFalse(TEXT("Interrupted dodge always clears invulnerability"),F.ASC->HasMatchingGameplayTag(ICTags::Invulnerable));
    F.ASC->SetGuarding(true);F.ASC->ResetExecutionForLoad();TestFalse(TEXT("Load clears transient guard"),F.ASC->HasMatchingGameplayTag(ICTags::Guarding));
    TestTrue(TEXT("Load clears unsaved dodge retry"),F.ASC->RequestAction(TEXT("Dodge"),NAME_None,nullptr).bSuccess);F.ASC->CancelAction();
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASEffectCheck,"IC.GAS.EffectCaptureAndOffline",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASEffectCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;F.ASC->SetVitals(60,100,70);TestTrue(TEXT("Shield applies through GAS"),F.ASC->ApplyStatus(TEXT("STE_GAS_998"),F.ASC));TestTrue(TEXT("Same shield refreshes"),F.ASC->ApplyStatus(TEXT("STE_GAS_998"),F.ASC));TestEqual(TEXT("Same shield ID is not duplicated"),F.ASC->Capture().Combat.Effects.Num(),1);
    FICAttackCapture Attack;Attack.Magnitude=10;TestTrue(TEXT("Effective shield hit accepted"),F.ASC->ApplyDamage(Attack,nullptr,false,false,false,false));TestEqual(TEXT("Shield protects HP"),F.ASC->Capture().HP,60.f);TestTrue(TEXT("Shield amount consumed"),F.ASC->Capture().Combat.Effects[0].RemainingShield<20);
    auto Captured=F.ASC->Capture();F.ASC->RemoveStatus(Captured.Combat.Effects[0].InstanceID);TestTrue(TEXT("Persistent capture restores GAS marker"),F.ASC->Restore(Captured));TestEqual(TEXT("Remaining shield survives restoration"),F.ASC->Capture().Combat.Effects[0].RemainingShield,Captured.Combat.Effects[0].RemainingShield);
    Captured.Combat.Effects[0].EffectID=TEXT("STE_MISSING");FString Reason;TestFalse(TEXT("Missing definition rejects restore before applying"),F.ASC->CanRestore(Captured,Reason));TestTrue(TEXT("Restore failure explains reason"),!Reason.IsEmpty());
    FICCharacterState NPC;NPC.CharacterId=TEXT("NPC_R08_001");NPC.Kind=EICCharacterKind::Recruitable;NPC.MaxHP=NPC.HP=300;NPC.MaxMP=NPC.MP=80;NPC.MaxSP=1;NPC.SP=0;
    FICActiveEffectState DOT;DOT.InstanceID=FGuid::NewGuid();DOT.EffectID=TEXT("STE_GAS_997");DOT.bHarmful=true;DOT.ExpiresAtPlaySecond=3;DOT.NextTickPlaySecond=1;DOT.CapturedMagnitudes.Add(TEXT("Magnitude"),10);DOT.CapturedMagnitudes.Add(TEXT("Output"),1);NPC.Combat.Effects.Add(DOT);
    auto W=F.State->ExportState();TestTrue(TEXT("Offscreen simulation needs no actor"),ICCombatSimulation::Advance(NPC,W,F.Definitions,3,480));TestEqual(TEXT("Each elapsed DOT tick executes once"),NPC.HP,270.f);TestEqual(TEXT("Expired effect is removed"),NPC.Combat.Effects.Num(),0);TestTrue(TEXT("Same timestamp is safe"),ICCombatSimulation::Advance(NPC,W,F.Definitions,3,480));TestEqual(TEXT("Repeated query cannot repeat damage"),NPC.HP,270.f);
    NPC.HP=2;NPC.Combat.Effects={DOT};NPC.LastSimPlaySecond=0;ICCombatSimulation::Advance(NPC,W,F.Definitions,3,480);TestEqual(TEXT("Scheduled recruit DOT floor is one HP"),NPC.HP,1.f);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASItemCheck,"IC.GAS.AtomicItemStatus",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASItemCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;FICItemRow Food;Food.ID=TEXT("ITM_GAS_998");Food.Kind=EPlayerItemKind::Consumable;Food.UseTarget=EICItemTarget::PlayerOnly;Food.UseEffect=EICItemEffect::None;Food.StatusEffectIDs={TEXT("STE_GAS_998")};
    FICItemRow Recovery;Recovery.ID=TEXT("ITM_GAS_997");Recovery.Kind=EPlayerItemKind::Consumable;Recovery.UseEffect=EICItemEffect::Recovery;Recovery.UseTarget=EICItemTarget::LivingParty;Recovery.RestoreSP=10;
    F.Table<FICItemRow>(EICDefinitionType::Item,{Food,Recovery});F.Definitions->ConfigureCatalog(F.Catalog);
    auto* Inventory=F.Player->Inventory.Get();TestTrue(TEXT("Test item is staged into inventory"),Inventory->AddItem(Food.ID,2,false));
    const auto Prepared=UICItemUseService::Prepare(Inventory,F.Definitions,Food.ID,F.ASC->Capture());TestTrue(TEXT("Food prepares its status effect"),Prepared.bSuccess);
    TestEqual(TEXT("Preparation never consumes original item"),Inventory->CountItem(Food.ID),2);TestEqual(TEXT("Preparation never modifies live effect cache"),F.ASC->Capture().Combat.Effects.Num(),0);TestEqual(TEXT("Candidate contains one shield"),Prepared.Character.Combat.Effects.Num(),1);
    auto Candidate=F.State->ExportState();Candidate.Characters.Add(Prepared.TargetID,Prepared.Character);Candidate.Inventory=Prepared.Inventory;
    TestTrue(TEXT("Effect and item consumption commit atomically"),F.State->CommitGameplayTransaction(Candidate,FGuid::NewGuid()));TestEqual(TEXT("Exactly one item consumed"),Inventory->CountItem(Food.ID),1);TestEqual(TEXT("Committed status reaches GAS"),F.ASC->Capture().Combat.Effects.Num(),1);
    Inventory->AddItem(Recovery.ID,1,false);auto NPC=F.ASC->Capture();NPC.CharacterId=TEXT("NPC_R08_001");NPC.Kind=EICCharacterKind::Recruitable;NPC.Mode=EICCharacterMode::Companion;NPC.PartySlot=0;NPC.SP=0;NPC.MaxSP=1;NPC.Combat.Effects.Reset();
    TestFalse(TEXT("SP-only item cannot change a companion or consume the item"),UICItemUseService::Prepare(Inventory,F.Definitions,Recovery.ID,NPC).bSuccess);TestEqual(TEXT("Companion has no SP recovery consumption"),Inventory->CountItem(Recovery.ID),1);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASBindingsCheck,"IC.GAS.FrameworkAssetBindings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASBindingsCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    for(const TCHAR* Path:{TEXT("/Game/IsekaiClass/Player/Blueprints/BP_ICPlayerCharacter.BP_ICPlayerCharacter_C"),TEXT("/Game/IsekaiClass/Enemy/Blueprints/BP_ICEnemyBase.BP_ICEnemyBase_C")})
    {
        auto* Type=LoadClass<AICCharacterBase>(nullptr,Path);if(!TestNotNull(Path,Type))continue;auto* ASC=Type->GetDefaultObject<AICCharacterBase>()->GetICAbilitySystem();if(!TestNotNull(TEXT("Native ASC survives BP save/reload"),ASC))continue;
        TestTrue(TEXT("Action GA asset is assigned"),ASC->ActionAbilityClass&&ASC->ActionAbilityClass->GetPathName().Contains(TEXT("GA_IC_Action_C")));
        TestTrue(TEXT("HP GE asset is assigned"),ASC->HealthEffectClass&&ASC->HealthEffectClass->GetPathName().Contains(TEXT("GE_IC_Health_C")));
        TestTrue(TEXT("MP GE asset is assigned"),ASC->ManaEffectClass&&ASC->ManaEffectClass->GetPathName().Contains(TEXT("GE_IC_Mana_C")));
        TestTrue(TEXT("SP GE asset is assigned"),ASC->SPEffectClass&&ASC->SPEffectClass->GetPathName().Contains(TEXT("GE_IC_SP_C")));
        TestTrue(TEXT("Status GE asset is assigned"),ASC->StatusEffectClass&&ASC->StatusEffectClass->GetPathName().Contains(TEXT("GE_IC_Status_C")));
        if(Type->IsChildOf(AICEnemyBase::StaticClass()))TestTrue(TEXT("Enemy has authored AI BP"),Type->GetDefaultObject<AICEnemyBase>()->AIControllerClass->GetPathName().Contains(TEXT("BP_ICAIController_C")));
    }
    auto* NPC=LoadClass<AICNPCBase>(nullptr,TEXT("/Game/IsekaiClass/NPC/Blueprints/BP_ICNPCBase.BP_ICNPCBase_C"));if(TestNotNull(TEXT("NPC BP loads"),NPC))
    {TestNull(TEXT("Default non-recruit NPC has no ASC"),NPC->GetDefaultObject<AICNPCBase>()->GetICAbilitySystem());TestTrue(TEXT("NPC has authored AI BP"),NPC->GetDefaultObject<AICNPCBase>()->AIControllerClass->GetPathName().Contains(TEXT("BP_ICAIController_C")));}
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASSpawnCheck,"IC.GAS.SpawnDeathRewardLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASSpawnCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;FICAnchorRow Anchor;Anchor.ID=TEXT("ANC_GAS_998");F.Table<FICAnchorRow>(EICDefinitionType::Anchor,{Anchor});
    FICMonsterRow Monster;Monster.ID=TEXT("MON_GAS_998");Monster.CombatStats.MaxHP=100;Monster.RespawnPlaySeconds=90;
    F.Table<FICMonsterRow>(EICDefinitionType::Monster,{Monster});FICSpawnSlotDefinition Slot;Slot.MonsterID=Monster.ID;Slot.OriginAnchorID=Anchor.ID;
    FICSpawnerRow Spawner;Spawner.ID=TEXT("SPN_GAS_998");Spawner.Slots={Slot};F.Table<FICSpawnerRow>(EICDefinitionType::Spawner,{Spawner});F.Definitions->ConfigureCatalog(F.Catalog);
    auto* Spawn=F.World->GetSubsystem<UICSpawnSubsystem>();if(!TestNotNull(TEXT("Game world creates spawn subsystem"),Spawn))return false;
    Spawn->OnWorldBeginPlay(*F.World);auto* A=F.World->SpawnActor<AICWorldAnchor>();A->AnchorID=Anchor.ID;TestTrue(TEXT("Definition anchor registers"),Spawn->RegisterAnchor(A));Spawn->RefreshSpawns();
    const FName Key(*(Spawner.ID.ToString()+TEXT(":0")));auto* Enemy=Cast<AICEnemyBase>(Spawn->FindCharacter(Key));if(!TestNotNull(TEXT("Loaded anchor creates enemy"),Enemy))return false;
    auto* ASC=Enemy->GetICAbilitySystem();TestFalse(TEXT("Monster has no primary attributes"),ASC->HasAttributeSetForAttribute(UICPrimaryAttributeSet::GetSTRAttribute()));TestFalse(TEXT("Monster has no SP attributes"),ASC->HasAttributeSetForAttribute(UICSurvivalAttributeSet::GetSPAttribute()));
    ASC->ChangeResource(TEXT("HP"),-25);Spawn->UnregisterAnchor(A);auto Alive=F.State->ExportState().SpawnSlots.FindChecked(Key);TestEqual(TEXT("Alive unload restores full HP"),Alive.HP,100.f);TestEqual(TEXT("Alive unload clears effects"),Alive.Combat.Effects.Num(),0);
    Spawn->RegisterAnchor(A);Spawn->RefreshSpawns();Enemy=Cast<AICEnemyBase>(Spawn->FindCharacter(Key));if(!TestNotNull(TEXT("Anchor reload creates new incarnation"),Enemy))return false;
    ASC=Enemy->GetICAbilitySystem();ASC->ChangeResource(TEXT("HP"),-1000);ASC->TickComponent(.1f,LEVELTICK_All,nullptr);auto Dead=F.State->ExportState().SpawnSlots.FindChecked(Key);
    TestTrue(TEXT("Defeat is persistent"),Dead.bDefeated);TestFalse(TEXT("Empty reward still finishes receipt"),Dead.bRewardPending);TestTrue(TEXT("Reward receipt prevents duplicate kill reward"),F.State->ExportState().CompletedTransactionIDs.Contains(Dead.IncarnationID));TestEqual(TEXT("Respawn deadline is PlayClock 90 seconds"),Dead.RespawnReadyAtPlaySecond,90.);
    Enemy->HandleZeroHP();TestEqual(TEXT("Duplicate death preserves receipt"),F.State->ExportState().SpawnSlots.FindChecked(Key).IncarnationID,Dead.IncarnationID);
    F.Time->JumpGameMinutes(480);TestEqual(TEXT("Sleep cannot advance monster respawn deadline"),F.Time->GetClock().PlaySecond,0.);Spawn->UnregisterAnchor(A);A->Destroy();
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGASNPCCheck,"IC.GAS.NPCStateAndSafeRevive",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGASNPCCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FGASFixture F;FICAnchorRow Anchor;Anchor.ID=TEXT("ANC_GAS_998");F.Table<FICAnchorRow>(EICDefinitionType::Anchor,{Anchor});
    FICNPCRow Recruit;Recruit.ID=TEXT("NPC_R08_001");Recruit.Type=EICNPCType::Recruitable;Recruit.InitialAnchorID=Anchor.ID;Recruit.ClassID=TEXT("CLS_GAS_998");Recruit.RecruitCombatID=TEXT("RCB_GAS_998");
    FICNPCRow Resident;Resident.ID=TEXT("NPC_R08_101");Resident.Type=EICNPCType::Resident;Resident.InitialAnchorID=Anchor.ID;
    F.Table<FICNPCRow>(EICDefinitionType::NPC,{Recruit,Resident});FICRecruitCombatRow Combat;Combat.ID=Recruit.RecruitCombatID;Combat.NPCID=Recruit.ID;Combat.ClassID=Recruit.ClassID;Combat.UniqueSkillID=F.SkillID;F.Table<FICRecruitCombatRow>(EICDefinitionType::RecruitCombat,{Combat});F.Definitions->ConfigureCatalog(F.Catalog);
    auto* NPC=F.World->SpawnActor<AICNPCBase>();TestTrue(TEXT("Recruit restores static definition and persistent state"),NPC->InitializeNPC(Recruit.ID));auto* ASC=NPC->GetICAbilitySystem();if(!TestNotNull(TEXT("Recruit creates ASC"),ASC))return false;
    TestTrue(TEXT("Recruit creates primary attributes"),ASC->HasAttributeSetForAttribute(UICPrimaryAttributeSet::GetSTRAttribute()));TestFalse(TEXT("Recruit never creates SP attributes"),ASC->HasAttributeSetForAttribute(UICSurvivalAttributeSet::GetSPAttribute()));
    TestTrue(TEXT("AI starts only after restored stats exist"),Cast<AICAIController>(NPC->GetController())!=nullptr);
    auto* Town=F.World->SpawnActor<AICNPCBase>();TestTrue(TEXT("Resident restores definition without combat"),Town->InitializeNPC(Resident.ID));TestNull(TEXT("Resident has no ASC"),Town->GetICAbilitySystem());
    auto Down=ASC->Capture();Down.Mode=EICCharacterMode::Companion;Down.PartySlot=0;Down.Affinity=100;Down.LifeState=EICCharacterLifeState::Downed;Down.HP=0;Down.MP=37;TestTrue(TEXT("Downed companion is persistent"),F.State->SetCharacterState(Down));
    NPC->FlushToWorldState_Implementation(F.State);NPC->ReleaseForLoad();NPC->Destroy();auto* Spawn=F.World->GetSubsystem<UICSpawnSubsystem>();FString Reason;const auto Before=F.ASC->Capture();
    TestNull(TEXT("No player/floor prevents unsafe revive placement"),Spawn->ReserveReviveTarget(Recruit.ID,F.ASC,Reason));TestTrue(TEXT("Safe placement failure has a reason"),!Reason.IsEmpty());TestEqual(TEXT("Failed placement costs no MP"),F.ASC->Capture().MP,Before.MP);TestTrue(TEXT("Failed placement keeps persistent Downed"),F.State->GetSnapshot(Recruit.ID).IsDowned());
    Town->ReleaseForLoad();Town->Destroy();
    return !HasAnyErrors();
}
namespace
{
void ConfigureInputWeapons(FGASFixture& F)
{
    UDataTable* Classes=nullptr;
    for(const auto& Binding:F.Catalog->Tables)if(Binding.RowStruct==FICClassRow::StaticStruct())Classes=Binding.Table.LoadSynchronous();
    FICClassRow Sword;
    Sword.ID=TEXT("CLS_GAS_998");Sword.CharacterClass=EICCharacterClass::Warrior;Sword.WeaponFamily=EICWeaponFamily::Sword;Sword.NormalAttackStages.SetNum(3);
    FICClassRow Bow=Sword;Bow.ID=TEXT("CLS_INPUT_998");Bow.CharacterClass=EICCharacterClass::Archer;Bow.WeaponFamily=EICWeaponFamily::Bow;
    Classes->AddRow(Sword.ID,Sword);Classes->AddRow(Bow.ID,Bow);
    FICItemRow SwordItem;SwordItem.ID=TEXT("ITM_INPUT_998");SwordItem.ItemID=SwordItem.ID;SwordItem.Kind=EPlayerItemKind::Equipment;SwordItem.EquipSlot=EPlayerEquipSlot::Weapon;SwordItem.StackLimit=1;SwordItem.EquipmentID=TEXT("EQP_INPUT_998");
    FICItemRow BowItem=SwordItem;BowItem.ID=TEXT("ITM_INPUT_997");BowItem.ItemID=BowItem.ID;BowItem.EquipmentID=TEXT("EQP_INPUT_997");
    F.Table<FICItemRow>(EICDefinitionType::Item,{SwordItem,BowItem});
    FICEnhancementRow Enhancement;Enhancement.ID=TEXT("ENH_INPUT_998");Enhancement.MaximumLevel=0;
    F.Table<FICEnhancementRow>(EICDefinitionType::Enhancement,{Enhancement});
    FICEquipmentRow SwordGear;SwordGear.ID=SwordItem.EquipmentID;SwordGear.ItemID=SwordItem.ID;SwordGear.Slot=EPlayerEquipSlot::Weapon;SwordGear.WeaponFamily=EICWeaponFamily::Sword;SwordGear.RequiredClass=EICCharacterClass::Warrior;SwordGear.EnhancementID=Enhancement.ID;
    FICEquipmentRow BowGear=SwordGear;BowGear.ID=BowItem.EquipmentID;BowGear.ItemID=BowItem.ID;BowGear.WeaponFamily=EICWeaponFamily::Bow;BowGear.RequiredClass=EICCharacterClass::Archer;
    F.Table<FICEquipmentRow>(EICDefinitionType::Equipment,{SwordGear,BowGear});
    auto SwordSkill=*F.Skills->FindRow<FICSkillRow>(F.SkillID,TEXT("Input"));SwordSkill.Purpose=EICSkillPurpose::Heal;SwordSkill.RequiredWeaponFamily=EICWeaponFamily::Sword;SwordSkill.bRequiresEquippedWeapon=true;SwordSkill.TargetRules.Target=EICCombatTarget::PartyMember;SwordSkill.TargetRules.bAllowCaster=true;
    SwordSkill.CooldownTag=FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_SWORD_001"));
    F.Skills->AddRow(SwordSkill.ID,SwordSkill);
    auto BowSkill=SwordSkill;BowSkill.ID=TEXT("SKL_INPUT_997");BowSkill.ClassID=Bow.ID;BowSkill.RequiredWeaponFamily=EICWeaponFamily::Bow;BowSkill.CooldownPlaySeconds=7;
    BowSkill.CooldownTag=FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_BOW_001"));
    F.Skills->AddRow(BowSkill.ID,BowSkill);
    F.Definitions->ConfigureCatalog(F.Catalog);
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(F.Definitions,0);
    check(Staging->AddItem(SwordItem.ID,1,false));check(Staging->Equip(0));check(Staging->AddItem(BowItem.ID,1,false));check(Staging->Equip(0));
    check(F.Player->Inventory->ApplySnapshot(Staging->GetSnapshot()));
    auto Player=F.ASC->Capture();Player.UnlockedSkillIDs.Add(BowSkill.ID);check(F.ASC->Restore(Player));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICInputWeaponCheck,"IC.Input.WeaponSwitchAndIndependentCooldowns",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICInputWeaponCheck::RunTest(const FString& Parameters)
{
    FGASFixture F;ConfigureInputWeapons(F);
    auto* Inventory=F.Player->Inventory.Get();
    TestEqual(TEXT("Starts with the equipped sword"),F.ASC->GetEquippedWeaponFamily(),EICWeaponFamily::Sword);
    TestEqual(TEXT("Q resolves the sword skill"),F.ASC->GetCurrentWeaponSkill(),F.SkillID);
    const auto Sword=F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player);
    TestTrue(TEXT("Sword skill activates"),Sword.bSuccess);
    TestTrue(TEXT("Sword effect commits its own cooldown"),F.ASC->NotifyCombatEvent(Sword.ActionID,TEXT("AN_HitStart"),0));
    TestTrue(TEXT("Sword cooldown GE owns sword tag"),F.ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_SWORD_001"))));
    TestFalse(TEXT("Sword cooldown GE does not own bow tag"),F.ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_BOW_001"))));
    F.ASC->CancelAction();
    const double SwordCooldown=F.ASC->GetSkillCooldownRemaining(F.SkillID);
    TestTrue(TEXT("Changing weapon ignores the original class restriction"),Inventory->CycleWeapon());
    TestEqual(TEXT("Weapon family changes to bow"),F.ASC->GetEquippedWeaponFamily(),EICWeaponFamily::Bow);
    const FName BowID=TEXT("SKL_INPUT_997");
    TestEqual(TEXT("Q changes with the weapon"),F.ASC->GetCurrentWeaponSkill(),BowID);
    TestEqual(TEXT("Sword cooldown survives switching"),F.ASC->GetSkillCooldownRemaining(F.SkillID),SwordCooldown);
    const auto Bow=F.ASC->RequestAction(TEXT("Skill"),BowID,F.Player);
    TestTrue(TEXT("Sword cooldown does not block bow skill"),Bow.bSuccess);
    TestTrue(TEXT("Bow starts an independent cooldown"),F.ASC->NotifyCombatEvent(Bow.ActionID,TEXT("AN_HitStart"),0));
    TestTrue(TEXT("Bow cooldown GE owns bow tag"),F.ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_BOW_001"))));
    TestTrue(TEXT("Bow does not remove sword GE"),F.ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("IC.Cooldown.SKL_START_SWORD_001"))));
    F.ASC->CancelAction();
    TestTrue(TEXT("Skill can be unequipped during its cooldown"),F.ASC->EquipWeaponSkill(EICWeaponFamily::Bow,NAME_None));
    TestTrue(TEXT("Empty skill slot stays empty"),F.ASC->GetCurrentWeaponSkill().IsNone());
    TestTrue(TEXT("The same skill can be re-equipped"),F.ASC->EquipWeaponSkill(EICWeaponFamily::Bow,BowID));
    TestTrue(TEXT("Re-equipping never resets cooldown"),F.ASC->GetSkillCooldownRemaining(BowID)>0);
    const auto Saved=F.ASC->Capture();
    TestTrue(TEXT("Load restores skill loadout and cooldowns"),F.ASC->Restore(Saved));
    TestEqual(TEXT("Saved weapon skill remains equipped"),F.ASC->GetCurrentWeaponSkill(),BowID);
    TestEqual(TEXT("Saved bow cooldown is unchanged"),F.ASC->GetSkillCooldownRemaining(BowID),7.);
    TestEqual(TEXT("Both weapons remain in their equipment slots"),Inventory->GetWeaponLoadout()[0].Count+Inventory->GetWeaponLoadout()[1].Count,2);
    TestTrue(TEXT("Switching does not put either weapon in the bag"),Inventory->GetSnapshot().Bag.Slots[0].IsEmpty());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICEmptyHandCycleCheck,"IC.Input.WeaponCycleEmptyHandsAndFullBag",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICEmptyHandCycleCheck::RunTest(const FString&)
{
    FGASFixture F;ConfigureInputWeapons(F);
    auto* Inventory=F.Player->Inventory.Get();
    auto OnlySword=Inventory->GetSnapshot();
    OnlySword.WeaponSlots[1]=FPlayerItemInstance();
    OnlySword.bWeaponSheathed=true;
    const FGuid SwordID=OnlySword.Equipment[0].InstanceID;
    TestTrue(TEXT("Single owned weapon can start put away"),Inventory->ApplySnapshot(OnlySword));
    TestEqual(TEXT("Put-away weapon is not an active family"),F.ASC->GetEquippedWeaponFamily(),EICWeaponFamily::None);
    TestTrue(TEXT("Empty hands have no Q skill"),F.ASC->GetCurrentWeaponSkill().IsNone());
    TestFalse(TEXT("An owned but put-away weapon cannot attack"),F.ASC->RequestAction(TEXT("Attack"),NAME_None,F.Player).bSuccess);
    TestFalse(TEXT("An owned but put-away weapon cannot cast its weapon skill"),F.ASC->RequestAction(TEXT("Skill"),F.SkillID,F.Player).bSuccess);
    TestTrue(TEXT("Own an unequipped bow in the bag"),Inventory->AddItem(TEXT("ITM_INPUT_997"),1,false));
    for(int32 Index=1;Index<OnlySword.Bag.Slots.Num();++Index)
        TestTrue(TEXT("Fill every bag slot with a separate same-family weapon"),Inventory->AddItem(OnlySword.Equipment[0].ItemID,1,false));
    for(int32 Repeat=0;Repeat<3;++Repeat)
    {
        TestTrue(TEXT("Single-weapon cycle draws even when the bag is full"),Inventory->CycleWeapon());
        TestEqual(TEXT("Draw selects the owned sword"),F.ASC->GetEquippedWeaponFamily(),EICWeaponFamily::Sword);
        TestTrue(TEXT("Single-weapon cycle returns to empty hands"),Inventory->CycleWeapon());
        TestTrue(TEXT("Weapon is put away"),Inventory->IsWeaponSheathed());
        TestEqual(TEXT("Repeated cycles preserve the selected instance"),Inventory->GetSnapshot().Equipment[0].InstanceID,SwordID);
        TestTrue(TEXT("Bow in the bag never enters the equipment cycle"),Inventory->GetWeaponLoadout()[1].IsEmpty());
    }
    TestTrue(TEXT("Full bag remains valid"),Inventory->ValidateState());
    auto Empty=FICInventoryState();
    TestTrue(TEXT("No-weapon state is valid"),Inventory->ApplySnapshot(Empty));
    TestFalse(TEXT("Right click without any weapon does nothing"),F.Player->RequestCycleWeapon());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWeaponCycleSaveCheck,"IC.Input.WeaponCycleOrderAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICWeaponCycleSaveCheck::RunTest(const FString&)
{
    FGASFixture F;ConfigureInputWeapons(F);
    auto* Inventory=F.Player->Inventory.Get();
    const FGuid SwordID=Inventory->GetSnapshot().Equipment[0].InstanceID;
    for(const auto Family:{EICWeaponFamily::Bow,EICWeaponFamily::None,EICWeaponFamily::Sword,EICWeaponFamily::Bow,EICWeaponFamily::None})
    {
        TestTrue(TEXT("Cycle advances through owned families and empty hands"),Inventory->CycleWeapon());
        TestEqual(TEXT("Missing families are skipped and empty hands follow the last family"),Inventory->GetEquippedWeaponFamily(),Family);
        auto* Save=NewObject<UICSaveGame>();Save->WorldState=F.State->ExportState();Save->Clock=F.Time->GetClock();
        TArray<uint8> Bytes;
        TestTrue(TEXT("Weapon state encodes in the real save envelope"),UICSaveGame::EncodeForFile(Save,Bytes));
        auto* Loaded=UICSaveGame::DecodeFile(Bytes);
        if(!TestNotNull(TEXT("Weapon state decodes from the real save envelope"),Loaded))return false;
        TestEqual(TEXT("Drawn/put-away flag survives serialization"),Loaded->WorldState.Inventory.bWeaponSheathed,Inventory->IsWeaponSheathed());
        TestEqual(TEXT("Selected weapon instance survives serialization"),Loaded->WorldState.Inventory.Equipment[0].InstanceID,Inventory->GetSnapshot().Equipment[0].InstanceID);
        TestTrue(TEXT("Loaded inventory restores"),Inventory->ApplySnapshot(Loaded->WorldState.Inventory));
    }
    TestTrue(TEXT("Removing the sword from its equipment slot puts it in the bag"),Inventory->UnequipWeapon(EICWeaponFamily::Sword));
    TestTrue(TEXT("Empty hands can still draw the selected bow"),Inventory->CycleWeapon());
    TestEqual(TEXT("An unequipped sword in the bag is skipped"),Inventory->GetEquippedWeaponFamily(),EICWeaponFamily::Bow);
    TestEqual(TEXT("The unequipped sword instance remains owned"),Inventory->GetSnapshot().Bag.Slots[0].InstanceID,SwordID);
    TestTrue(TEXT("Sword equipment slot is now empty"),Inventory->GetWeaponLoadout()[0].IsEmpty());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWeaponMontageCycleCheck,"IC.Input.WeaponCycleMontageAndInterruption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICWeaponMontageCycleCheck::RunTest(const FString&)
{
    FGASFixture F;ConfigureInputWeapons(F);
    auto* Player=F.Player;auto* Inventory=Player->Inventory.Get();
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/IsekaiClass/Player/SamplePlayer/Male/SK_Player_M.SK_Player_M"));
    auto* Profile=LoadObject<UICCharacterAnimationProfile>(nullptr,TEXT("/Game/IsekaiClass/Animation/Player/MainMale/DA_ANIM_PLAYER_M.DA_ANIM_PLAYER_M"));
    if(!TestNotNull(TEXT("Player mesh"),Mesh)||!TestNotNull(TEXT("Authored player animation profile"),Profile))return false;
    TestNotNull(TEXT("Dedicated empty-hand locomotion"),Profile->Unarmed.Locomotion.Get());
    for(const auto& Entry:Profile->WeaponSets)
    { TestNotNull(TEXT("Every weapon family has a draw montage"),Entry.Value.Draw.Get());TestNotNull(TEXT("Every weapon family has a put-away montage"),Entry.Value.Sheathe.Get()); }
    Player->AnimationProfile=Profile;Player->GetMesh()->SetSkeletalMesh(Mesh);Player->GetMesh()->SetAnimInstanceClass(UICCharacterAnimInstance::StaticClass());
    auto Step=[&](float Seconds)
    {
        for(float Elapsed=0;Elapsed<Seconds;Elapsed+=.025f)
        {Player->GetMesh()->TickAnimation(.025f,false);Player->GetMesh()->RefreshBoneTransforms();Player->GetMesh()->ConditionallyDispatchQueuedAnimEvents();}
    };
    Inventory->SetWeaponSheathed(true);Step(.05f);
    auto* Anim=Cast<UICCharacterAnimInstance>(Player->GetMesh()->GetAnimInstance());
    if(!TestNotNull(TEXT("Player native animation instance"),Anim))return false;
    TestEqual(TEXT("Empty hands select the neutral locomotion"),Anim->Locomotion.Get(),Profile->Unarmed.Locomotion.Get());
    TestTrue(TEXT("Right click begins drawing"),Player->RequestCycleWeapon());
    TestTrue(TEXT("Drawing is an active transition"),Player->IsSwitchingWeapon());
    TestTrue(TEXT("Weapon remains put away before the draw notify"),Inventory->IsWeaponSheathed());
    TestFalse(TEXT("Repeated right click cannot skip the current transition"),Player->RequestCycleWeapon());
    TestFalse(TEXT("Attack is blocked during the transition"),F.ASC->RequestAction(TEXT("Attack"),NAME_None,Player).bSuccess);
    TestFalse(TEXT("Weapon skill is blocked during the transition"),F.ASC->RequestAction(TEXT("Skill"),F.SkillID,Player).bSuccess);
    const auto* Sword=Profile->WeaponSets.Find(EICWeaponFamily::Sword);
    Step(Sword->Draw->GetPlayLength()*.7f);
    TestFalse(TEXT("Draw notify reveals the weapon before the montage ends"),Inventory->IsWeaponSheathed());
    TestTrue(TEXT("Recovery remains locked after reveal"),Player->IsSwitchingWeapon());
    Step(Sword->Draw->GetPlayLength());
    TestFalse(TEXT("Draw completion unlocks input"),Player->IsSwitchingWeapon());
    TestEqual(TEXT("Sword is active after drawing"),Inventory->GetEquippedWeaponFamily(),EICWeaponFamily::Sword);
    TestTrue(TEXT("Right click begins sword-to-bow transition"),Player->RequestCycleWeapon());
    Step(Sword->Sheathe->GetPlayLength()*.7f);
    TestTrue(TEXT("Put-away notify creates a temporary empty-hand interval"),Inventory->IsWeaponSheathed());
    TestTrue(TEXT("Transition continues without another click"),Player->IsSwitchingWeapon());
    Step(Sword->Sheathe->GetPlayLength()+Profile->WeaponSets[EICWeaponFamily::Bow].Draw->GetPlayLength()+.5f);
    TestFalse(TEXT("Both montages complete from one click"),Player->IsSwitchingWeapon());
    TestEqual(TEXT("Bow is now active"),Inventory->GetEquippedWeaponFamily(),EICWeaponFamily::Bow);
    TestTrue(TEXT("Last weapon begins putting away"),Player->RequestCycleWeapon());
    Step(Profile->WeaponSets[EICWeaponFamily::Bow].Sheathe->GetPlayLength()+.5f);
    TestEqual(TEXT("Last weapon ends at empty hands"),Inventory->GetEquippedWeaponFamily(),EICWeaponFamily::None);
    TestFalse(TEXT("Empty-hand state unlocks input"),Player->IsSwitchingWeapon());
    TestTrue(TEXT("New cycle begins drawing the first weapon"),Player->RequestCycleWeapon());
    Anim->Montage_Stop(0);
    Step(.1f);
    TestFalse(TEXT("Interrupted montage cannot leave switching stuck"),Player->IsSwitchingWeapon());
    TestTrue(TEXT("Interrupted early draw keeps the weapon put away"),Inventory->IsWeaponSheathed());
    TestFalse(TEXT("Stale or invalid montage notifications are rejected"),Player->NotifyWeaponVisibility(INDEX_NONE,true));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFourWeaponCycleCheck,"IC.Input.WeaponCycleFourFamilies",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFourWeaponCycleCheck::RunTest(const FString&)
{
    FGASFixture F;ConfigureInputWeapons(F);
    UDataTable* Items=nullptr;UDataTable* Equipment=nullptr;
    for(const auto& Binding:F.Catalog->Tables)
    {
        if(Binding.RowStruct==FICItemRow::StaticStruct())Items=Binding.Table.LoadSynchronous();
        if(Binding.RowStruct==FICEquipmentRow::StaticStruct())Equipment=Binding.Table.LoadSynchronous();
    }
    for(const auto& Pair:TArray<TPair<EICWeaponFamily,FString>>{{EICWeaponFamily::Staff,TEXT("996")},{EICWeaponFamily::Wand,TEXT("995")}})
    {
        auto Item=*Items->FindRow<FICItemRow>(TEXT("ITM_INPUT_998"),TEXT("Four weapons"));
        auto Gear=*Equipment->FindRow<FICEquipmentRow>(TEXT("EQP_INPUT_998"),TEXT("Four weapons"));
        Item.ID=FName(*(TEXT("ITM_INPUT_")+Pair.Value));Item.ItemID=Item.ID;Item.EquipmentID=FName(*(TEXT("EQP_INPUT_")+Pair.Value));
        Gear.ID=Item.EquipmentID;Gear.ItemID=Item.ID;Gear.WeaponFamily=Pair.Key;
        Items->AddRow(Item.ID,Item);Equipment->AddRow(Gear.ID,Gear);
    }
    F.Definitions->ConfigureCatalog(F.Catalog);
    auto* Inventory=F.Player->Inventory.Get();
    TestTrue(TEXT("Own staff"),Inventory->AddItem(TEXT("ITM_INPUT_996"),1,false));
    TestTrue(TEXT("Staff has its own equipment slot"),Inventory->Equip(0));
    for(const auto Expected:{EICWeaponFamily::Bow,EICWeaponFamily::Staff,EICWeaponFamily::None,EICWeaponFamily::Sword})
    {
        TestTrue(TEXT("Three equipped weapons cycle"),Inventory->CycleWeapon());
        TestEqual(TEXT("Three weapons return to empty hands after the third"),Inventory->GetEquippedWeaponFamily(),Expected);
    }
    TestTrue(TEXT("Own wand"),Inventory->AddItem(TEXT("ITM_INPUT_995"),1,false));
    TestTrue(TEXT("Healing wand has its own equipment slot"),Inventory->Equip(0));
    const auto Slots=Inventory->GetWeaponLoadout();
    for(const auto Expected:{EICWeaponFamily::Bow,EICWeaponFamily::Staff,EICWeaponFamily::Wand,EICWeaponFamily::None,EICWeaponFamily::Sword})
    {
        TestTrue(TEXT("Four-weapon cycle advances"),Inventory->CycleWeapon());
        TestEqual(TEXT("Sword, bow, staff, wand, empty hands repeat in order"),Inventory->GetEquippedWeaponFamily(),Expected);
        const auto CurrentSlots=Inventory->GetWeaponLoadout();
        for(int32 Index=0;Index<4;++Index)
            TestEqual(TEXT("Every weapon stays in the same family equipment slot"),CurrentSlots[Index].InstanceID,Slots[Index].InstanceID);
        for(const auto& Item:Inventory->GetSnapshot().Bag.Slots)
            TestTrue(TEXT("Cycling all four weapons leaves the bag empty"),Item.IsEmpty());
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICInputTargetCheck,"IC.Input.SupportTargetSelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICInputTargetCheck::RunTest(const FString& Parameters)
{
    FGASFixture F;ConfigureInputWeapons(F);
    auto* PC=F.World->SpawnActor<AICPlayerController>();PC->Possess(F.Player);PC->SetAsLocalPlayerController();PC->bCreated=true;PC->Menu.Empty();
    FICCharacterState Companion;Companion.CharacterId=TEXT("NPC_R08_001");Companion.Mode=EICCharacterMode::Companion;Companion.PartySlot=0;Companion.Affinity=100;Companion.MaxSP=1;Companion.SP=0;
    TestTrue(TEXT("Companion target exists in persistent state"),F.State->SetCharacterState(Companion));
    PC->HandleInputCommand(EICInputCommand::Skill);
    TestEqual(TEXT("Q opens support targeting"),PC->PendingSkill,F.SkillID);
    const float MP=F.ASC->Capture().MP;
    PC->HandleInputCommand(EICInputCommand::CompanionSkill,0);
    TestEqual(TEXT("E selects the first companion"),PC->PendingTargetID,Companion.CharacterId);
    TestEqual(TEXT("Selection does not spend MP"),F.ASC->Capture().MP,MP);
    PC->HandleInputCommand(EICInputCommand::CompanionSkill,0);
    TestEqual(TEXT("Repeated E selects the player"),PC->PendingTargetID,UICWorldStateSubsystem::PlayerId());
    PC->HandleInputCommand(EICInputCommand::CompanionSkill,3);
    TestEqual(TEXT("There is no fourth companion input"),PC->PendingTargetID,UICWorldStateSubsystem::PlayerId());
    PC->HandleInputCommand(EICInputCommand::Menu);
    TestFalse(TEXT("Escape cancels without casting"),PC->HasTargetSelection());
    TestEqual(TEXT("Cancel preserves MP"),F.ASC->Capture().MP,MP);
    PC->HandleInputCommand(EICInputCommand::Skill);
    PC->HandleInputCommand(EICInputCommand::Confirm);
    TestFalse(TEXT("Confirm leaves target selection"),PC->HasTargetSelection());
    TestTrue(TEXT("Confirmed support skill starts a real GAS action"),F.ASC->GetActionID().IsValid());
    F.ASC->CancelAction();PC->Destroy();
    return !HasAnyErrors();
}
#endif
