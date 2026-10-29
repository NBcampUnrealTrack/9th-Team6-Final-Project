#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Map/ICMapWidget.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "NPC/Character/ICNPCBase.h"
#include "GameFramework/Actor.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICItemHUDCooldownCheck,"IC.UI.Firefight.ItemCooldownTransaction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICItemHUDCooldownCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("HUD cooldown");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Player.HP=20;Initial.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Restore isolated state"),State->RestoreState(Initial));
    auto* PC=World->SpawnActor<AICPlayerController>();auto* Pawn=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Pawn);PC->bCreated=true;
    auto* Bag=Pawn->Inventory.Get();Bag->BindWorldState(State);const FName Potion=TEXT("ITM_CONS_001");Bag->AddItem(Potion,4,false);
    const int32 Slot=Bag->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& I){return I.ItemID==Potion;});
    Bag->BindQuickSlot(0,Slot);Bag->BindQuickSlot(1,Slot);FText Reason;
    // An ineffective use must neither consume nor start a cooldown.
    State->SetVitals(Player.CharacterId,100,100,100);
    TestFalse(TEXT("Full HP rejects use"),PC->UseInventoryItem(Potion,Player.CharacterId,Reason));
    TestEqual(TEXT("Failure starts no cooldown"),PC->GetItemCooldown(Potion),0.);
    State->SetVitals(Player.CharacterId,20,100,100);
    TestTrue(TEXT("First potion succeeds"),PC->UseInventoryItem(Potion,Player.CharacterId,Reason));
    const double Remaining=PC->GetItemCooldown(Potion);TestTrue(TEXT("DT starts five seconds"),FMath::IsNearlyEqual(Remaining,5.));
    const auto After=State->ExportState();const auto Clock=Time->GetClock();
    TestFalse(TEXT("Repeated inventory use cannot bypass quick slot cooldown"),PC->UseInventoryItem(Potion,Player.CharacterId,Reason));
    TestEqual(TEXT("Rejected repeat consumes nothing"),Bag->CountItem(Potion),3);
    TestEqual(TEXT("Both slots share one item ID"),After.Inventory.QuickSlots[0],After.Inventory.QuickSlots[1]);
    Time->SetRunning(true);Time->SetMenuPaused(true);Time->AdvanceClock(2,false);
    TestEqual(TEXT("Menu pause preserves remaining cooldown"),PC->GetItemCooldown(Potion),Remaining);
    Time->SetMenuPaused(false);Time->AdvanceClock(5.1,false);
    TestEqual(TEXT("Play time expires cooldown"),PC->GetItemCooldown(Potion),0.);
    TestTrue(TEXT("Potion succeeds again after expiry"),PC->UseInventoryItem(Potion,Player.CharacterId,Reason));
    // Serialized world state is the same path used by saves; loading cannot reset the timer.
    TArray<uint8> Bytes;FMemoryWriter Writer(Bytes);FObjectAndNameAsStringProxyArchive Save(Writer,false);Save.ArIsSaveGame=true;
    auto Stored=After;FICWorldState::StaticStruct()->SerializeItem(Save,&Stored,nullptr);
    FICWorldState Reloaded;FMemoryReader Reader(Bytes);FObjectAndNameAsStringProxyArchive Load(Reader,true);Load.ArIsSaveGame=true;
    FICWorldState::StaticStruct()->SerializeItem(Load,&Reloaded,nullptr);
    Time->RestoreClock(Clock);TestTrue(TEXT("Restore serialized state"),State->RestoreState(Reloaded));
    TestTrue(TEXT("Reload keeps cooldown"),FMath::IsNearlyEqual(PC->GetItemCooldown(Potion),Remaining));
    TestFalse(TEXT("Reload cannot consume another potion immediately"),PC->UseInventoryItem(Potion,Player.CharacterId,Reason));
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFirefightPresentationCheck,"IC.UI.Firefight.ResourceSegmentsAndEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFirefightPresentationCheck::RunTest(const FString&)
{
    auto* Class=LoadClass<UICCharacterStatusWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_PlayerStatus.WBP_IC_PlayerStatus_C"));
    if(!TestNotNull(TEXT("Live status blueprint exists"),Class))return false;
    auto* Card=NewObject<UICCharacterStatusWidget>(GetTransientPackage(),Class);Card->Initialize();Card->TakeWidget();
    FICCharacterState State;State.HP=70;State.MaxHP=100;State.MP=50;State.MaxMP=100;State.SP=20;State.MaxSP=100;Card->UpdateVitals(State);
    TestEqual(TEXT("First MP segment is full at half MP"),CastChecked<UProgressBar>(Card->GetWidgetFromName(TEXT("BAR_MP1")))->GetPercent(),1.f);
    TestEqual(TEXT("Second MP segment is half at half MP"),CastChecked<UProgressBar>(Card->GetWidgetFromName(TEXT("BAR_MP2")))->GetPercent(),.5f);
    TestEqual(TEXT("Last MP segment is empty at half MP"),CastChecked<UProgressBar>(Card->GetWidgetFromName(TEXT("BAR_MP3")))->GetPercent(),0.f);
    Card->UpdateSkillCooldown(true,0,5);
    TestEqual(TEXT("Portrait cooldown remains visible"),Card->GetWidgetFromName(TEXT("TXT_SkillCooldown"))->GetVisibility(),ESlateVisibility::HitTestInvisible);
    TestEqual(TEXT("Portrait visibly dims at cooldown start"),Card->GetWidgetFromName(TEXT("IMG_Portrait"))->GetRenderOpacity(),.3f);
    Card->UpdateSkillCooldown(true,1,0);
    TestEqual(TEXT("Portrait returns to full brightness when ready"),Card->GetWidgetFromName(TEXT("IMG_Portrait"))->GetRenderOpacity(),1.f);
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();
    TArray<FName> IDs;D->GetDefinitionIDs(EICDefinitionType::StatusEffect,IDs);
    if(TestTrue(TEXT("Existing effect definitions available"),IDs.Num()>=2))
    {
        for(int32 I=0;I<2;++I){auto& Effect=State.Combat.Effects.AddDefaulted_GetRef();Effect.InstanceID=FGuid::NewGuid();Effect.EffectID=IDs[I];Effect.ExpiresAtPlaySecond=I+2.;}
        FICWorldState World;Card->UpdateStatusEffects(State,World,D,0.);
        auto* Row=CastChecked<UHorizontalBox>(Card->GetWidgetFromName(TEXT("StatusEffects")));
        auto VisibleChildren=[Row](){int32 Count=0;for(auto* Child:Row->GetAllChildren())if(Child->GetVisibility()!=ESlateVisibility::Collapsed)++Count;return Count;};
        TestEqual(TEXT("Each active effect has its own badge"),VisibleChildren(),2);
        Card->UpdateStatusEffects(State,World,D,2.5);TestEqual(TEXT("Only expired effect disappears"),VisibleChildren(),1);
        Card->UpdateStatusEffects(State,World,D,4.);TestEqual(TEXT("Last expired effect disappears"),VisibleChildren(),0);
        State.Combat.Effects.Reset();
        for(int32 I=0;I<6;++I){auto& Effect=State.Combat.Effects.AddDefaulted_GetRef();Effect.InstanceID=FGuid::NewGuid();Effect.EffectID=IDs[I%IDs.Num()];Effect.ExpiresAtPlaySecond=20.;Effect.bHarmful=I==5;}
        Card->UpdateStatusEffects(State,World,D,0.);
        TestEqual(TEXT("Player card keeps a fixed four-cell status rail"),VisibleChildren(),4);
        auto* Overflow=CastChecked<UBorder>(Card->GetWidgetFromName(TEXT("StatusOverflow")));
        TestEqual(TEXT("Overflow badge is visible"),Overflow->GetVisibility(),ESlateVisibility::HitTestInvisible);
        TestEqual(TEXT("Overflow badge summarizes hidden effects"),CastChecked<UTextBlock>(Card->GetWidgetFromName(TEXT("TXT_StatusOverflow")))->GetText().ToString(),FString(TEXT("+3")));
        Card->UpdateStatusEffects(State,World,D,21.);
        TestEqual(TEXT("Expired overflow effects clear the whole rail"),VisibleChildren(),0);
        TestEqual(TEXT("Overflow badge collapses when empty"),Overflow->GetVisibility(),ESlateVisibility::Collapsed);
    }
    TestEqual(TEXT("Map centre stays opaque"),UICMapWidget::MinimapOpacity({.5,.5}),1.f);
    TestEqual(TEXT("Map edge becomes fully transparent"),UICMapWidget::MinimapOpacity({0,.5}),0.f);
    TestTrue(TEXT("Map fades toward edge"),UICMapWidget::MinimapOpacity({.25,.5})>UICMapWidget::MinimapOpacity({.1,.5}));
    TestTrue(TEXT("Minimap keeps the player at its centre"),UICMapWidget::MinimapDisplayUV({.7,.3},{.7,.3},3.f).Equals(FVector2D(.5,.5)));
    TestTrue(TEXT("Minimap zoom maps nearby positions into view"),UICMapWidget::MinimapDisplayUV({.6,.5},{.5,.5},3.f).Equals(FVector2D(.8,.5)));
    TestTrue(TEXT("Minimap hides positions outside its zoom window"),UICMapWidget::MinimapDisplayUV({.9,.5},{.5,.5},3.f).Equals(FVector2D(1.7,.5)));
    auto* World=GI->GetWorld();GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMinimapWorldMarkerCheck,"IC.UI.Firefight.MinimapWorldMarkers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMinimapWorldMarkerCheck::RunTest(const FString&)
{
    TestTrue(TEXT("Medieval building core is auto detected"),UICMapWidget::IsAutomaticBuildingMeshPath(TEXT("/Game/IsekaiClass/World/SampleLevel/Medieval_Village/meshes/building/SM_building_block_05.SM_building_block_05")));
    TestTrue(TEXT("Ruined village building core is auto detected"),UICMapWidget::IsAutomaticBuildingMeshPath(TEXT("/Game/IsekaiClass/World/SampleLevel/Ruined_Village/meshes/building/SM_building_box.SM_building_box")));
    TestFalse(TEXT("Roof pieces do not create duplicate markers"),UICMapWidget::IsAutomaticBuildingMeshPath(TEXT("/Game/IsekaiClass/World/SampleLevel/Medieval_Village/meshes/building/SM_roof_07.SM_roof_07")));
    TestTrue(TEXT("Modular house floors provide a representative marker"),UICMapWidget::IsAutomaticBuildingMeshPath(TEXT("/Game/IsekaiClass/World/SampleLevel/Medieval_House/meshes/building/SM_floor_01.SM_floor_01")));
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* Building=World->SpawnActor<AActor>();Building->Tags.Add(TEXT("IC.Map.Building"));
    World->SpawnActor<AActor>(); // Ordinary props must not become building markers.
    auto* NPC=World->SpawnActor<AICNPCBase>();
    auto* Monster=World->SpawnActorDeferred<AICEnemyBase>(AICEnemyBase::StaticClass(),FTransform::Identity);Monster->bInitializeWhenPlaced=false;Monster->FinishSpawning(FTransform::Identity);
    auto* Map=NewObject<UICMapWidget>(GI);Map->bMinimap=true;Map->RefreshMap();
    TestEqual(TEXT("Only explicitly tagged buildings are tracked"),Map->GetTrackedBuildingCount(),1);
    TestEqual(TEXT("NPC actors are tracked automatically"),Map->GetTrackedNPCCount(),1);
    TestEqual(TEXT("Monster actors are tracked automatically"),Map->GetTrackedMonsterCount(),1);
    TestTrue(TEXT("Tagged building is the tracked building"),Map->Buildings.Contains(Building));
    TestTrue(TEXT("Spawned NPC is the tracked NPC"),Map->NPCs.Contains(NPC));
    TestTrue(TEXT("Spawned monster is the tracked monster"),Map->Monsters.Contains(Monster));
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFirefightTargetCheck,"IC.UI.Firefight.MonsterTargetLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFirefightTargetCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
    World->InitializeActorsForPlay(FURL(),false);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("Target HUD");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);State->RestoreState(Initial);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));auto* Pawn=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Pawn);PC->bCreated=true;PC->Menu.Empty();
    Pawn->EnableCombat(true,true);Pawn->InitializeState(Player.CharacterId);Pawn->SetActorLocation({0,0,100});PC->SetControlRotation(FRotator::ZeroRotator);
    auto* HUDClass=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
    auto* HUD=CreateWidget<UICGameplayWidget>(PC,HUDClass);auto* Panel=HUD->GetWidgetFromName(TEXT("HUD_TargetInfo"));HUD->RefreshTargetInfo();
    TestEqual(TEXT("No target hides monster panel"),Panel->GetVisibility(),ESlateVisibility::Collapsed);
    FICDefinitionResult Lookup;const auto* Spawner=D->Find<FICSpawnerRow>(TEXT("SPN_R01_TEST_001"),Lookup);
    TArray<AICEnemyBase*> Enemies;
    if(TestNotNull(TEXT("Real monster spawner available"),Spawner))
    {
        for(int32 I=0;I<2;++I)
        {
            const auto& Slot=Spawner->Slots[I];const auto* Monster=D->Find<FICMonsterRow>(Slot.MonsterID,Lookup);
            const FTransform Origin(FRotator::ZeroRotator,FVector(400,I*180,100));
            auto* Enemy=World->SpawnActor<AICEnemyBase>(Monster->PawnClass.LoadSynchronous(),Origin);Enemies.Add(Enemy);
            TestTrue(TEXT("Enemy initializes real monster data"),Enemy->InitializeEnemy(Spawner->ID,Slot,Origin));
        }
        FICAttackCapture Attack;Attack.Magnitude=10;
        auto* EnemyASC=Enemies[0]->GetICAbilitySystem();
        TestTrue(TEXT("Player hit applies actual damage"),EnemyASC->ApplyDamage(Attack,Pawn->GetICAbilitySystem(),false,false,false,false));
        TestEqual(TEXT("Player hit chooses monster HUD"),PC->GetHUDCombatTarget(),static_cast<AICCharacterBase*>(Enemies[0]));HUD->RefreshTargetInfo();
        TestEqual(TEXT("Hit shows the framed panel"),Panel->GetVisibility(),ESlateVisibility::HitTestInvisible);
        const auto Snapshot=EnemyASC->Capture();
        TestEqual(TEXT("Target bar follows actual HP"),CastChecked<UProgressBar>(HUD->GetWidgetFromName(TEXT("ProgressBar_13")))->GetPercent(),Snapshot.HP/Snapshot.MaxHP);
        TestFalse(TEXT("Target name resolves monster DT"),CastChecked<UTextBlock>(HUD->GetWidgetFromName(TEXT("TextBlock_49")))->GetText().IsEmpty());
        Time->SetRunning(true);Time->SetMenuPaused(false);Time->AdvanceClock(8.1,false);
        TestNull(TEXT("Unselected hit target hides after delay"),PC->GetHUDCombatTarget());
        PC->HandleInputCommand(EICInputCommand::NextTarget);
        auto* First=PC->GetHUDCombatTarget();TestNotNull(TEXT("Tab selects an actual monster"),First);
        PC->HandleInputCommand(EICInputCommand::NextTarget);
        TestTrue(TEXT("Tab switches monster data"),PC->GetHUDCombatTarget()!=First&&PC->GetHUDCombatTarget()!=nullptr);
        PC->HandleInputCommand(EICInputCommand::NextTarget);
        TestNull(TEXT("Full Tab cycle clears target"),PC->GetHUDCombatTarget());
        PC->NotifyPlayerHitEnemy(Enemies[0]);EnemyASC->SetVitals(0,0,0);HUD->RefreshTargetInfo();
        TestEqual(TEXT("Dead target hides panel"),Panel->GetVisibility(),ESlateVisibility::Collapsed);
    }
    for(auto* Enemy:Enemies)Enemy->Destroy();Pawn->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
