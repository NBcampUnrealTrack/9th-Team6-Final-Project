#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "Components/BoxComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMovementInputCheck,"IC.Input.MovementAndCooldown",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMovementInputCheck::RunTest(const FString& Parameters)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PCClass=LoadClass<AICPlayerController>(nullptr,TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController.BP_ICPlayerController_C"));
    if(!TestNotNull(TEXT("Actual player controller BP compiles"),PCClass))return false;
    auto* PC=World->SpawnActor<AICPlayerController>(PCClass);FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
    PC->bCreated=true;PC->Menu.Empty();Player->SetActorLocation({0,0,100});Player->SetActorRotation(FRotator::ZeroRotator);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(GI->GetSubsystem<UICDefinitionSubsystem>(),0);
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("이동 테스트");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");Initial.Inventory=Staging->GetSnapshot();
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;P.UnlockedSkillIDs.Add(Initial.FirstSkill);Initial.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Actual player state initializes"),State->RestoreState(Initial));
    TestTrue(TEXT("Actual player GAS initializes"),Player->InitializeState(P.CharacterId));Player->Inventory->BindWorldState(State);
    auto* ASC=Player->GetICAbilitySystem();Time->SetRunning(true);Time->SetMenuPaused(false);
    if(!PC->InputComponent)PC->InitInputSystem();
    auto* Input=CastChecked<UEnhancedPlayerInput>(PC->PlayerInput);
    auto* Subsystem=PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    FModifyContextOptions Options;Options.bForceImmediately=true;Options.bIgnoreAllPressedKeysUntilRelease=false;
    Subsystem->AddMappingContext(PC->DefaultMappingContext,0,Options);
    const TArray<UInputComponent*> Stack{PC->InputComponent};
    const auto Key=[&](FKey K,bool Pressed)
    {
        Input->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),K,
            Pressed?IE_Pressed:IE_Released,Pressed?1.f:0.f,false,FPlatformTime::Cycles64()));
        Input->ProcessInputStack(Stack,.016f,false);
    };
    const auto Step=[&](double Seconds){Time->AdvanceClock(Seconds,false);ASC->TickComponent(Seconds,LEVELTICK_All,nullptr);};
    const auto FindAction=[&](const TCHAR* Name){return LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/IsekaiClass/Input/Actions/%s.%s"),Name,Name));};
    auto* Roll=FindAction(TEXT("IA_Roll"));auto* Follow=FindAction(TEXT("IA_QuestFollow"));
    // UE 5.8 exposes raw action values even when a modifier gate rejects the input.
    const auto Triggered=[&](UInputAction* Action)
    {
        const auto* Instance=Input->FindActionInstanceData(Action);
        return Instance&&Instance->GetTriggerEvent()==ETriggerEvent::Triggered;
    };
    TestNotNull(TEXT("Reserved roll input asset"),Roll);TestNotNull(TEXT("Reserved follow input asset"),Follow);
    const FKey Directions[]={EKeys::W,EKeys::A,EKeys::S,EKeys::D};
    for(const FKey Control:{EKeys::LeftControl,EKeys::RightControl})for(const FKey Direction:Directions)
    {
        Key(Direction,true);
        TestTrue(TEXT("WASD retains ordinary movement"),Triggered(PC->MoveAction));
        TestFalse(TEXT("WASD alone is not a roll"),Triggered(Roll));
        Key(Control,true);
        TestTrue(TEXT("Ctrl + every WASD direction maps to roll"),Triggered(Roll));
        TestTrue(TEXT("Reserved roll does not consume normal movement"),Triggered(PC->MoveAction));
        TestFalse(TEXT("Reserved roll starts no ability"),ASC->GetActionID().IsValid());
        Key(Control,false);Key(Direction,false);
    }
    Key(EKeys::G,true);TestTrue(TEXT("G maps to follow command"),Input->GetActionValue(Follow).IsNonZero());
    TestFalse(TEXT("Reserved follow starts no ability"),ASC->GetActionID().IsValid());Key(EKeys::G,false);
    TestTrue(TEXT("Reserved commands leave gameplay active"),PC->Menu.IsEmpty());
    Key(EKeys::SpaceBar,true);TestTrue(TEXT("Space maps to jump"),Input->GetActionValue(FindAction(TEXT("IA_Jump"))).IsNonZero());Key(EKeys::SpaceBar,false);
    Player->ConsumeMovementInputVector();Player->ConsumeMovementInputVector();

    auto* WidgetClass=LoadClass<UICMovementCooldownWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_MovementCooldown.WBP_IC_MovementCooldown_C"));
    auto* Widget=WidgetClass?CreateWidget<UICMovementCooldownWidget>(PC,WidgetClass):nullptr;
    TestNotNull(TEXT("Movement cooldown WBP compiles"),Widget);
    auto* Ring=Widget?Cast<UICRadialCooldownBar>(Widget->GetWidgetFromName(TEXT("RING_Cooldown"))):nullptr;
    TestNotNull(TEXT("The WBP contains a standalone ring"),Ring);
    if(Ring)TestEqual(TEXT("The arc uses a thicker stroke"),Ring->Thickness,14.f);
    if(Widget)
    {
        TestNull(TEXT("The ring has no key label"),Widget->GetWidgetFromName(TEXT("TXT_Key")));
        TestNull(TEXT("The ring has no action caption"),Widget->GetWidgetFromName(TEXT("TXT_Action")));
    }
    const auto Refresh=[&]{if(Widget)Widget->RefreshCooldown();};
    Refresh();if(Widget)TestEqual(TEXT("Ready ring is complete"),Widget->Progress,1.f);
    if(Ring)TestEqual(TEXT("Ready characters show no ring"),Ring->GetVisibility(),ESlateVisibility::Hidden);
    Key(EKeys::LeftShift,true);
    TestTrue(TEXT("Shift holds sprint out of combat"),Player->bSprintHeld);
    TestFalse(TEXT("Sprinting starts no dash ability or cooldown"),ASC->GetActionID().IsValid());
    Key(EKeys::LeftShift,false);TestFalse(TEXT("Releasing Shift ends sprint intent"),Player->bSprintHeld);
    TestTrue(TEXT("The dash ability remains available"),ASC->RequestAction(TEXT("Dash"),NAME_None,nullptr).bSuccess);
    TestFalse(TEXT("Dash grants no combat invulnerability"),ASC->HasMatchingGameplayTag(ICTags::Invulnerable));
    TestEqual(TEXT("Dash starts the existing 3-second cooldown"),ASC->GetMovementCooldownRemaining(),3.);
    const FGuid DashID=ASC->GetActionID();Key(EKeys::LeftShift,false);Key(EKeys::LeftShift,true);
    TestEqual(TEXT("Early retries do not cancel the active dash"),ASC->GetActionID(),DashID);Key(EKeys::LeftShift,false);
    Refresh();if(Widget)TestEqual(TEXT("Used ring starts empty"),Widget->Progress,0.f);
    if(Ring)TestEqual(TEXT("A new cooldown reveals the ring"),Ring->GetVisibility(),ESlateVisibility::HitTestInvisible);
    const FVector Start=Player->GetActorLocation();Step(.125);
    TestTrue(TEXT("Dash moves gradually rather than teleporting"),FMath::IsNearlyEqual(Player->GetActorLocation().X-Start.X,250.,.1));
    Step(.125);TestTrue(TEXT("Dash reaches configured 500cm distance"),FMath::IsNearlyEqual(Player->GetActorLocation().X-Start.X,500.,.1));
    TestFalse(TEXT("Dash does not create a combat encounter"),ASC->HasRecentCombat());
    Time->SetMenuPaused(true);const double BeforePause=ASC->GetMovementCooldownRemaining();Step(5);
    TestEqual(TEXT("Paused gameplay does not refill cooldown"),ASC->GetMovementCooldownRemaining(),BeforePause);Time->SetMenuPaused(false);
    Step(1.25);Refresh();if(Widget)TestTrue(TEXT("Actual cooldown drives a half-filled ring"),FMath::IsNearlyEqual(Widget->Progress,.5f));
    Step(1.5);Refresh();if(Widget)TestTrue(TEXT("A complete ring is usable"),Widget->bReady&&Widget->Progress==1.f);
    if(Ring)TestEqual(TEXT("The completed ring disappears"),Ring->GetVisibility(),ESlateVisibility::Hidden);
    ASC->MarkCombat();Key(EKeys::RightShift,true);
    TestTrue(TEXT("Shift switches to an invulnerable dodge in combat"),ASC->HasMatchingGameplayTag(ICTags::Invulnerable));
    const FGuid DodgeID=ASC->GetActionID();Key(EKeys::RightShift,false);Key(EKeys::RightShift,true);
    TestEqual(TEXT("Rejected dodge retry preserves current action"),ASC->GetActionID(),DodgeID);
    TestTrue(TEXT("Rejected retry preserves dodge invulnerability"),ASC->HasMatchingGameplayTag(ICTags::Invulnerable));Key(EKeys::RightShift,false);
    Refresh();if(Ring)TestEqual(TEXT("Dodge reveals the ring again without a label"),Ring->GetVisibility(),ESlateVisibility::HitTestInvisible);
    const FVector DodgeStart=Player->GetActorLocation();Step(.25);
    TestTrue(TEXT("No-input combat dodge moves 300cm backward"),FMath::IsNearlyEqual(Player->GetActorLocation().X-DodgeStart.X,-300.,.1));
    TestFalse(TEXT("Dodge immunity ends with the action"),ASC->HasMatchingGameplayTag(ICTags::Invulnerable));
    TestFalse(TEXT("Dash and dodge share one cooldown"),ASC->RequestAction(TEXT("Dash"),NAME_None,nullptr).bSuccess);
    ASC->ResetExecutionForLoad();Refresh();if(Widget)TestTrue(TEXT("Loading resets transient movement cooldown UI"),Widget->bReady&&Widget->Progress==1.f);

    Player->SetActorLocation({0,0,100});Player->SetActorRotation(FRotator::ZeroRotator);auto* Wall=World->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Wall->AddInstanceComponent(Box);
    Box->SetBoxExtent({10,500,500});Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Wall->SetActorLocation({200,0,100});
    TestTrue(TEXT("Dash can start toward a wall"),ASC->RequestAction(TEXT("Dash"),NAME_None,nullptr).bSuccess);Step(.25);
    TestTrue(TEXT("Swept dash stops before a wall"),Player->GetActorLocation().X>130&&Player->GetActorLocation().X<170);
    TestTrue(TEXT("Ring starts at 12 o'clock"),UICRadialCooldownBar::ClockwisePoint(0).Equals({0,-1},.001));
    TestTrue(TEXT("Quarter fill advances clockwise to 3 o'clock"),UICRadialCooldownBar::ClockwisePoint(.25).Equals({1,0},.001));
    TestTrue(TEXT("Half fill advances to 6 o'clock"),UICRadialCooldownBar::ClockwisePoint(.5).Equals({0,1},.001));
    auto* HUD=PC->GameplayWidgetClass?CreateWidget<UICGameplayWidget>(PC,PC->GameplayWidgetClass):nullptr;
    TestNotNull(TEXT("HUD includes the separate movement cooldown WBP"),HUD?Cast<UICMovementCooldownWidget>(HUD->GetWidgetFromName(TEXT("HUD_MovementCooldown"))):nullptr);
    auto* PlayerStatus=HUD?Cast<UICCharacterStatusWidget>(HUD->GetWidgetFromName(TEXT("HUD_PlayerStatus"))):nullptr;
    auto* Portrait=PlayerStatus?Cast<UImage>(PlayerStatus->GetWidgetFromName(TEXT("IMG_Portrait"))):nullptr;
    auto* SkillSeconds=PlayerStatus?Cast<UTextBlock>(PlayerStatus->GetWidgetFromName(TEXT("TXT_SkillCooldown"))):nullptr;
    auto* SkillKey=PlayerStatus?PlayerStatus->GetWidgetFromName(TEXT("TXT_SkillKey")):nullptr;
    TestNotNull(TEXT("HUD reuses the player portrait for Q cooldown"),Portrait);
    if(ASC->GetCurrentWeaponSkill().IsNone())
    {
        TestTrue(TEXT("Add a sword before testing its Q skill"),Player->Inventory->AddItem(TEXT("EQP_WPN_SWORD_001"),1,false));
        const auto Inventory=Player->Inventory->GetSnapshot();
        const int32 Sword=Inventory.Bag.Slots.IndexOfByPredicate([](const FPlayerItemInstance& Item){return Item.ItemID==TEXT("EQP_WPN_SWORD_001");});
        TestTrue(TEXT("Equip the sword before testing its Q skill"),Sword!=INDEX_NONE&&Player->Inventory->Equip(Sword));
    }
    if(HUD)HUD->RefreshSkillCooldown();
    TestEqual(TEXT("Ready Q skill shows its key on the portrait"),SkillKey?SkillKey->GetVisibility():ESlateVisibility::Collapsed,ESlateVisibility::HitTestInvisible);
    TestEqual(TEXT("Ready Q skill hides its countdown"),SkillSeconds?SkillSeconds->GetVisibility():ESlateVisibility::Visible,ESlateVisibility::Hidden);
    TestTrue(TEXT("Ready Q skill keeps the portrait fully bright"),Portrait&&Portrait->GetColorAndOpacity().Equals(FLinearColor::White));
    const FName SkillID=ASC->GetCurrentWeaponSkill();
    ASC->CancelAction();
    const auto SkillAction=ASC->RequestAction(TEXT("Skill"),SkillID,nullptr);
    TestTrue(TEXT("Q starts its equipped weapon skill"),SkillAction.bSuccess);
    TestTrue(TEXT("Q effect starts its configured cooldown"),ASC->NotifyCombatEvent(SkillAction.ActionID,TEXT("AN_EffectCommit"),0));
    if(HUD)HUD->RefreshSkillCooldown();
    TestEqual(TEXT("Active Q cooldown shows whole remaining seconds"),SkillSeconds?SkillSeconds->GetText().ToString():FString(),FString(TEXT("20")));
    TestTrue(TEXT("Used Q skill darkens only the portrait"),Portrait&&FMath::IsNearlyEqual(Portrait->GetColorAndOpacity().R,.3f));
    ASC->CancelAction();Step(10);if(HUD)HUD->RefreshSkillCooldown();
    TestTrue(TEXT("Half of Q cooldown restores half of the portrait brightness"),Portrait&&FMath::IsNearlyEqual(Portrait->GetColorAndOpacity().R,.65f));
    TestEqual(TEXT("Q countdown follows play time"),SkillSeconds?SkillSeconds->GetText().ToString():FString(),FString(TEXT("10")));
    if(Widget)Widget->ReleaseSlateResources(true);Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
