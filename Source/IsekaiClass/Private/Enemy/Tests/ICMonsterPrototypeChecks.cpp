#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "UI/Gameplay/ICCombatFeedbackWidget.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMonsterPrototypeCheck,"IC.GAS.MonsterPrototype",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMonsterPrototypeCheck::RunTest(const FString& Parameters)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->SetPawn(Player);
    Player->SetActorLocation({0,0,100});Player->SetActorRotation(FRotator::ZeroRotator);
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,0);
    TestTrue(TEXT("Existing starter sword can be granted"),Staging->AddItem(TEXT("EQP_WPN_SWORD_001"),1,false));
    TestTrue(TEXT("Existing starter sword can be equipped"),Staging->Equip(0));
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("전투 테스트");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");Initial.Inventory=Staging->GetSnapshot();
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;P.UnlockedSkillIDs.Add(Initial.FirstSkill);Initial.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Real world state initializes"),State->RestoreState(Initial));
    TestTrue(TEXT("Player uses actual GAS state"),Player->InitializeState(P.CharacterId));Player->Inventory->BindWorldState(State);
    Time->SetRunning(true);Time->SetMenuPaused(false);
    TArray<AICEnemyBase*> Enemies;
    const TCHAR* Numbers[]={TEXT("07"),TEXT("03"),TEXT("164"),TEXT("153"),TEXT("150"),TEXT("114"),TEXT("107"),TEXT("10"),TEXT("38"),TEXT("08")};
    FICDefinitionResult R;const auto* Spawner=D->Find<FICSpawnerRow>(TEXT("SPN_R01_TEST_001"),R);
    if(TestNotNull(TEXT("Test spawner exists"),Spawner))
    {
        TestEqual(TEXT("10 normal dragons and one boss"),Spawner->Slots.Num(),11);
        for(int32 Index=0;Index<Spawner->Slots.Num();++Index)
        {
            const auto& Slot=Spawner->Slots[Index];const auto* Row=D->Find<FICMonsterRow>(Slot.MonsterID,R);
            if(!TestNotNull(TEXT("Every slot resolves a real monster definition"),Row))continue;
            auto* Type=Row->PawnClass.LoadSynchronous();
            if(!TestNotNull(TEXT("Combat-ready BP exists"),Type))continue;
            TestTrue(TEXT("BP inherits GAS enemy"),Type->IsChildOf(AICEnemyBase::StaticClass()));
            TestTrue(TEXT("Idle animation exists"),!Row->IdleAnimation.IsNull()&&Row->IdleAnimation.LoadSynchronous());
            TestTrue(TEXT("Hit animation exists"),!Row->HitAnimation.IsNull()&&Row->HitAnimation.LoadSynchronous());
            TestTrue(TEXT("Attack animation exists"),!Row->AttackAnimation.IsNull()&&Row->AttackAnimation.LoadSynchronous());
            TestTrue(TEXT("Requested hit VFX"),Row->HitVFX.ToString().Contains(TEXT("NS_Muzzle_Bullet01")));
            const auto* Default=CastChecked<AICEnemyBase>(Type->GetDefaultObject());
            TestTrue(TEXT("Dragon mesh +Y front is aligned to character +X"),
                FVector::DotProduct(Default->GetMesh()->GetRelativeRotation().RotateVector(FVector::RightVector),FVector::ForwardVector)>.999f);
            if(Index<10)
            {
                TestEqual(TEXT("Requested normal monster order"),Type->GetName(),FString::Printf(TEXT("BP_DragonSD_%s_C"),Numbers[Index]));
                TestTrue(TEXT("Each normal gets a different whimper"),Row->HitSound.ToString().Contains(FString::Printf(TEXT("WAV_SmallCreatureWimper%03d"),Index+1)));
            }
            else
            {
                TestEqual(TEXT("Regional boss kind"),Row->Kind,EICMonsterKind::RegionalBoss);
                TestTrue(TEXT("Boss uses GreenMat"),Default->GetMesh()->GetMaterial(0)&&Default->GetMesh()->GetMaterial(0)->GetName()==TEXT("GreenMat"));
                TestTrue(TEXT("Boss uses requested scream"),Row->HitSound.ToString().Contains(TEXT("WAV_DeepScream086")));
            }
            const auto* Drop=D->Find<FICDropTableRow>(Row->DropTableID,R);
            if(TestNotNull(TEXT("Drop definition"),Drop))
            {
                TestEqual(TEXT("Exactly one random item kind"),Drop->Rolls,1);
                TestEqual(TEXT("Weighted choice"),Drop->Selection,EICDropSelection::WeightedChoice);
                TestEqual(TEXT("Normal has 10 choices, boss 11"),Drop->Entries.Num(),Index<10?10:11);
                TestTrue(TEXT("Items collect automatically"),Drop->bAutoCollectItems);
                TestEqual(TEXT("Gold minimum"),Drop->MinimumGold,100);TestEqual(TEXT("Gold maximum"),Drop->MaximumGold,1000);
                TestFalse(TEXT("Luck cannot exceed requested gold range"),Drop->bApplyPlayerLUKToGold);
            }
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FTransform Origin(FRotator::ZeroRotator,FVector(Index?3000:120,Index*1000,100));
            auto* Enemy=World->SpawnActor<AICEnemyBase>(Type,Origin,Params);Enemies.Add(Enemy);
            TestTrue(TEXT("Monster initializes real GAS"),Enemy&&Enemy->InitializeEnemy(Spawner->ID,Slot,Origin));
            if(Enemy)
            {
                auto* Bar=Cast<UICMonsterHealthWidget>(Enemy->HealthBar->GetUserWidgetObject());
                TestTrue(TEXT("GAS requests automatic component ticks for PlayClock actions"),Enemy->GetICAbilitySystem()->GetShouldTick());
                TestTrue(TEXT("Restoring a monster enables its real component tick"),Enemy->GetICAbilitySystem()->IsComponentTickEnabled());
                TestNotNull(TEXT("Reusable health bar is attached to each monster"),Bar);
                if(Bar)
                {
                    Bar->RefreshHealth();auto* HP=Cast<UProgressBar>(Bar->GetWidgetFromName(TEXT("BAR_MonsterHP")));
                    if(TestNotNull(TEXT("Head bar exists"),HP))TestEqual(TEXT("New monster starts full"),HP->GetPercent(),1.f);
                }
            }
        }
    }
    auto* FeedbackClass=LoadClass<UICCombatFeedbackWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CombatFeedback.WBP_IC_CombatFeedback_C"));
    auto* Feedback=FeedbackClass?CreateWidget<UICCombatFeedbackWidget>(PC,FeedbackClass):nullptr;
    if(TestNotNull(TEXT("Combat feedback WBP"),Feedback)&&!Enemies.IsEmpty()&&Enemies[0])
    {
        const auto Slate=Feedback->TakeWidget();
        auto* Canvas=CastChecked<UCanvasPanel>(Feedback->GetWidgetFromName(TEXT("FeedbackCanvas")));
        auto* Enemy=Enemies[0];auto* EnemyASC=Enemy->GetICAbilitySystem();auto* PlayerASC=Player->GetICAbilitySystem();auto* AI=CastChecked<AICAIController>(Enemy->GetController());
        const auto Step=[&](double Seconds){Time->AdvanceClock(Seconds,false);EnemyASC->TickComponent(Seconds,LEVELTICK_All,nullptr);PlayerASC->TickComponent(Seconds,LEVELTICK_All,nullptr);};
        const float HPBefore=PlayerASC->Capture().HP;
        AI->Tick(.3f);Enemy->Tick(.01f);
        TestTrue(TEXT("Monster faces the player before its first attack"),
            FVector::DotProduct(Enemy->GetActorForwardVector(),(Player->GetActorLocation()-Enemy->GetActorLocation()).GetSafeNormal2D())>.999f);
        TestTrue(TEXT("Nearby player triggers a real GAS attack"),EnemyASC->GetActionID().IsValid());
        Step(.3);
        TestTrue(TEXT("Monster's GAS attack damages player"),PlayerASC->Capture().HP<HPBefore);
        TestTrue(TEXT("Actual player damage adds a floating widget"),Canvas->GetChildrenCount()>0);
        EnemyASC->CancelAction();
        // Move around a stationary monster; rotation must keep following its focus
        // even while the next attack decision is waiting for the attack interval.
        const FVector OldPlayerPosition=Player->GetActorLocation();
        Player->SetActorLocation(Enemy->GetActorLocation()+FVector(0,120,0));
        AI->UpdateControlRotation(.2f,false);
        Enemy->GetCharacterMovement()->PhysicsRotation(.2f);
        TestTrue(TEXT("Stationary monster tracks a player moving to its side"),
            FVector::DotProduct(Enemy->GetActorForwardVector(),(Player->GetActorLocation()-Enemy->GetActorLocation()).GetSafeNormal2D())>.999f);
        Player->SetActorLocation(OldPlayerPosition);
        EnemyASC->CancelAction();Step(2);
        const auto Attack=EnemyASC->RequestAction(TEXT("Attack"),NAME_None,Player);
        TestTrue(TEXT("Enemy can attack again"),Attack.bSuccess);
        Player->SetActorLocation({2000,0,100});const float FarHP=PlayerASC->Capture().HP;
        Step(.3);TestEqual(TEXT("Leaving reach before impact avoids remote damage"),PlayerASC->Capture().HP,FarHP);
        Enemy->SetActorLocation({400,0,100});AI->Tick(.3f);
        TestTrue(TEXT("Player leaving detection sends monster home"),Enemy->IsReturning());
        TestFalse(TEXT("Returning monster cannot attack"),EnemyASC->RequestAction(TEXT("Attack"),NAME_None,Player).bSuccess);
        Enemy->SetActorLocation(Enemy->GetOrigin());Step(.3);AI->Tick(.3f);
        TestFalse(TEXT("Arriving at original position ends return"),Enemy->IsReturning());
        Player->SetActorLocation({0,0,100});Player->SetActorRotation(FRotator::ZeroRotator);Step(1);
        const float EnemyHP=EnemyASC->Capture().HP;
        auto PlayerAttack=PlayerASC->RequestAction(TEXT("Attack"),NAME_None,Enemy);
        TestTrue(TEXT("Player's actual equipped sword activates GAS attack"),PlayerAttack.bSuccess);
        Step(.3);Enemy->Tick(.01f);
        TestTrue(TEXT("Player attack damages monster"),EnemyASC->Capture().HP<EnemyHP);
        auto* Bar=Cast<UICMonsterHealthWidget>(Enemy->HealthBar->GetUserWidgetObject());
        if(Bar){Bar->RefreshHealth();auto* HP=CastChecked<UProgressBar>(Bar->GetWidgetFromName(TEXT("BAR_MonsterHP")));TestTrue(TEXT("Head bar reflects damage"),HP->GetPercent()<1);}
        PlayerASC->CancelAction();
        const int32 GoldBefore=State->ExportState().Gold;const int32 FeedBefore=Canvas->GetChildrenCount();
        FICAttackCapture Fatal;Fatal.Magnitude=99999;
        TestTrue(TEXT("GAS killing blow applies"),EnemyASC->ApplyDamage(Fatal,PlayerASC,false,false,false,true));EnemyASC->TickComponent(.01f,LEVELTICK_All,nullptr);
        const auto Awarded=State->ExportState();const int32 Gold=Awarded.Gold-GoldBefore;
        TestTrue(TEXT("Kill grants 100..1000 gold"),Gold>=100&&Gold<=1000);
        const int32 EssenceIndex=Enemy->GetMonsterDefinition().PetCodexIndex;
        TestTrue(TEXT("Actual monster has its own codex essence index"),EssenceIndex>=0&&EssenceIndex<55);
        const int32 Essence=Awarded.Codex.PetEssence.FindRef(EssenceIndex);
        TestTrue(TEXT("Killing the monster grants 1..10 matching essences"),Essence>=1&&Essence<=10);
        int32 ItemKinds=0,Count=0;for(const auto& Item:Awarded.Inventory.Bag.Slots)if(!Item.IsEmpty()&&Item.ItemID.ToString().StartsWith(TEXT("ITEM_R01_"))){++ItemKinds;Count+=Item.Count;}
        TestEqual(TEXT("One item kind is received"),ItemKinds,1);TestTrue(TEXT("1..5 items are received"),Count>=1&&Count<=5);
        TestTrue(TEXT("Actual reward adds HUD toast"),Canvas->GetChildrenCount()>FeedBefore);
        bool FoundToast=false;for(auto* Child:Canvas->GetAllChildren())if(auto* Toast=Cast<UICRewardNoticeWidget>(Child))
        {
            FoundToast=true;TestTrue(TEXT("Gold toast has localized unit"),CastChecked<UTextBlock>(Toast->GetWidgetFromName(TEXT("TXT_Gold")))->GetText().ToString().Contains(TEXT("골드")));
            TestTrue(TEXT("Item toast has displayed R01 item name"),CastChecked<UTextBlock>(Toast->GetWidgetFromName(TEXT("TXT_Items")))->GetText().ToString().Contains(TEXT("R01_")));
            const FString EssenceText=CastChecked<UTextBlock>(Toast->GetWidgetFromName(TEXT("TXT_Essence")))->GetText().ToString();
            TestTrue(TEXT("Essence toast uses the actual monster name and acquisition text"),EssenceText.Contains(Enemy->GetMonsterDefinition().PetName.ToString())&&EssenceText.Contains(TEXT("정수 획득")));
        }
        TestTrue(TEXT("Reward uses separate reusable WBP"),FoundToast);
        Enemy->HandleZeroHP();TestEqual(TEXT("Duplicate death cannot duplicate rewards"),State->ExportState().Gold,Awarded.Gold);
        TestEqual(TEXT("Duplicate death cannot duplicate essence"),State->ExportState().Codex.PetEssence.FindRef(EssenceIndex),Essence);
        TestTrue(TEXT("Save snapshot contains reward receipt"),Awarded.CompletedTransactionIDs.Contains(Awarded.SpawnSlots.FindChecked(Enemy->GetSpawnKey()).IncarnationID));
        for(const int32 MonsterIndex:{1,10})if(Enemies.IsValidIndex(MonsterIndex)&&Enemies[MonsterIndex])
        {
            const auto* Monster=Enemies[MonsterIndex];const auto& Definition=Monster->GetMonsterDefinition();
            for(int32 Roll=0;Roll<32;++Roll)
            {
                auto Candidate=Initial;auto& Slot=Candidate.SpawnSlots.Add(Monster->GetSpawnKey());Slot.SpawnerID=Spawner->ID;Slot.SlotIndex=MonsterIndex;Slot.MonsterID=Definition.ID;Slot.bDefeated=true;Slot.HP=0;Slot.MaxHP=Definition.CombatStats.MaxHP;Slot.IncarnationID=FGuid::NewGuid();Slot.bRewardPending=true;
                FString Reason;const bool Prepared=FICRewardService::PrepareMonsterDefeat(Candidate,Monster->GetSpawnKey(),Definition,D,Time->GetClock().PlaySecond,Reason);
                TestTrue(TEXT("Random reward transaction prepares"),Prepared);
                if(!Prepared){AddError(Reason);continue;}
                TestTrue(TEXT("Every gold roll is within bounds"),Candidate.Gold-Initial.Gold>=100&&Candidate.Gold-Initial.Gold<=1000);
                TestTrue(TEXT("Normal and boss essences always roll 1..10"),Candidate.Codex.PetEssence.FindRef(Definition.PetCodexIndex)>=1&&Candidate.Codex.PetEssence.FindRef(Definition.PetCodexIndex)<=10);
                int32 Kinds=0,Quantity=0;for(const auto& Item:Candidate.Inventory.Bag.Slots)if(!Item.IsEmpty()&&Item.ItemID.ToString().StartsWith(TEXT("ITEM_R01_"))){++Kinds;Quantity+=Item.Count;}
                TestEqual(TEXT("Every roll picks one kind"),Kinds,1);TestTrue(TEXT("Every roll yields 1..5"),Quantity>=1&&Quantity<=5);
                const auto Once=Candidate;FICRewardService::PrepareMonsterDefeat(Candidate,Monster->GetSpawnKey(),Definition,D,Time->GetClock().PlaySecond,Reason);
                TestEqual(TEXT("Receipt makes reward retry idempotent"),Candidate.Gold,Once.Gold);
                TestEqual(TEXT("Receipt makes essence retry idempotent"),Candidate.Codex.PetEssence.FindRef(Definition.PetCodexIndex),Once.Codex.PetEssence.FindRef(Definition.PetCodexIndex));
            }
            auto Full=Initial;
            Full.Codex.PetEssence.Add(Definition.PetCodexIndex,MAX_int32-1);
            for(auto& Item:Full.Inventory.Bag.Slots){Item=FPlayerItemInstance();Item.ItemID=TEXT("ITEM_R01_01");Item.InstanceID=FGuid::NewGuid();Item.Count=99;}
            auto& FullSlot=Full.SpawnSlots.Add(Monster->GetSpawnKey());FullSlot.SpawnerID=Spawner->ID;FullSlot.SlotIndex=MonsterIndex;FullSlot.MonsterID=Definition.ID;FullSlot.bDefeated=true;FullSlot.HP=0;FullSlot.MaxHP=Definition.CombatStats.MaxHP;FullSlot.IncarnationID=FGuid::NewGuid();FullSlot.bRewardPending=true;
            FString OverflowReason;
            TestTrue(TEXT("A full bag does not block the kill reward transaction"),FICRewardService::PrepareMonsterDefeat(Full,Monster->GetSpawnKey(),Definition,D,Time->GetClock().PlaySecond,OverflowReason));
            TestEqual(TEXT("Loot that cannot fit remains one world drop"),Full.Drops.Num(),1);
            TestEqual(TEXT("Essence counter saturates without overflow"),Full.Codex.PetEssence.FindRef(Definition.PetCodexIndex),MAX_int32);
            if(!Full.Drops.IsEmpty())TestTrue(TEXT("Overflow retains the entire rolled quantity"),Full.Drops[0].Contents.Num()==1&&Full.Drops[0].Contents[0].Count>=1&&Full.Drops[0].Contents[0].Count<=5);
            TestTrue(TEXT("Overflow snapshot remains saveable without duplicate instance IDs"),UICWorldStateSubsystem::ValidateState(Full));
        }
    }
    if(Feedback)Feedback->ReleaseSlateResources(true);
    PC->SetPawn(nullptr);Player->ReleaseForLoad();for(auto* Enemy:Enemies)if(Enemy)Enemy->ReleaseForLoad();
    GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
