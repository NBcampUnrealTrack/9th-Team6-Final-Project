#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Audio/ICBGMSubsystem.h"
#include "Audio/ICBGMZone.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "NPC/LifeObject/ICLifeObjectUserComponent.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Components/BoxComponent.h"
#include "Widgets/SOverlay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICBGMContextCheck,"IC.Presentation.Music.ZonePriorityAndCombat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICBGMContextCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* Music=World->GetSubsystem<UICBGMSubsystem>();
    if(TestNotNull(TEXT("Game world owns BGM subsystem"),Music))
    {
        auto* Village=World->SpawnActor<AICBGMZone>();Village->Bounds->SetBoxExtent(FVector(1000));Village->MusicRowID=TEXT("BGM_Village");
        auto* Area=World->SpawnActor<AICBGMZone>();Area->Bounds->SetBoxExtent(FVector(100));Area->Priority=20;Area->MusicRowID=TEXT("BGM_Area_01");
        TestEqual(TEXT("Outside every box uses outside music"),Music->ResolveMusicRow(FVector(10000),false),FName(TEXT("BGM_Outside")));
        TestEqual(TEXT("Village uses its row"),Music->ResolveMusicRow(FVector(500),false),FName(TEXT("BGM_Village")));
        TestEqual(TEXT("Higher priority area overrides village"),Music->ResolveMusicRow(FVector::ZeroVector,false),FName(TEXT("BGM_Area_01")));
        TestEqual(TEXT("Combat overrides area music"),Music->ResolveMusicRow(FVector::ZeroVector,true),FName(TEXT("BGM_Combat")));
        TestEqual(TEXT("Combat end returns to current area"),Music->ResolveMusicRow(FVector::ZeroVector,false),FName(TEXT("BGM_Area_01")));
        Music->SetMusicOverride(TEXT("BGM_Area_02"));TestEqual(TEXT("Preview/quest override supported"),Music->ResolveMusicRow(FVector::ZeroVector,false),FName(TEXT("BGM_Area_02")));Music->ClearMusicOverride();
        auto* Table=Music->MusicTable.LoadSynchronous();TestTrue(TEXT("Editable five-context BGM DT exists"),Table&&Table->GetRowNames().Num()>=5);
        if(Table)for(const auto& ID:Table->GetRowNames())if(const auto* Row=Table->FindRow<FICBGMRow>(ID,TEXT("Test")))TestTrue(TEXT("Authored music resolves to a project asset"),Row->Music.LoadSynchronous()!=nullptr);
    }
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICClockPresentationCheck,"IC.Presentation.HUDClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICClockPresentationCheck::RunTest(const FString&)
{
    TestEqual(TEXT("Clock starts with day and two-digit time"),UICGameplayWidget::FormatGameClock(480).ToString(),FString(TEXT("1일차  08:00")));
    TestEqual(TEXT("Midnight advances day"),UICGameplayWidget::FormatGameClock(1440).ToString(),FString(TEXT("2일차  00:00")));
    TestEqual(TEXT("Minute updates correctly"),UICGameplayWidget::FormatGameClock(1505).ToString(),FString(TEXT("2일차  01:05")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCBedAccessCheck,"IC.Presentation.NPCBedAffinityAndOccupancy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCBedAccessCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();auto& Context=*GEngine->GetWorldContextFromWorld(World);
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Pawn=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Pawn);PC->bCreated=true;PC->Menu.Empty();
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("침대 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);State->RestoreState(Initial);
    Pawn->InitializeState(Player.CharacterId);Pawn->Inventory->BindWorldState(State);auto* Time=GI->GetSubsystem<UICTimeSubsystem>();Time->SetRunning(true);Time->SetMenuPaused(false);
    auto* Bed=World->SpawnActor<AICLifeObjectActor>();Bed->ObjectType=EICLifeObjectType::Bed;Bed->OwnerNPCID=TEXT("NPC_R01_001");Bed->ObjectID=TEXT("TEST_OWNER_BED");
    State->EnsureCharacter(Bed->OwnerNPCID,EICCharacterKind::Recruitable);State->SetAffinity(Bed->OwnerNPCID,200);
    TestFalse(TEXT("Exactly 200 affinity is insufficient for NPC bed"),Bed->TryInteract(PC));
    State->SetAffinity(Bed->OwnerNPCID,201);
    TestTrue(TEXT("An empty bed unlocks above 200"),Bed->TryInteract(PC));TestTrue(TEXT("Player sleep animation phase starts"),Pawn->LifeActions->IsSleeping());TestTrue(TEXT("Bed is reserved throughout sleep"),Bed->IsPlayerLocked());
    Pawn->LifeActions->EndWork();TestFalse(TEXT("Waking releases bed"),Bed->IsPlayerLocked());
    auto* NPC=World->SpawnActor<AICNPCBase>();NPC->InitializeNPC(Bed->OwnerNPCID);NPC->SetActorLocation(Bed->GetActorLocation());
    FICClockState Night;Night.GameMinute=60;Time->RestoreClock(Night);NPC->LifeObjectUser->RefreshRoutine(true);NPC->LifeObjectUser->SetFloorSleeping(true);
    TestTrue(TEXT("Actual sleeping NPC is detected"),NPC->IsSleepingForInteraction());TestEqual(TEXT("Sleeping NPC exposes only sleeping text"),NPC->GetInteractionText().ToString(),FString(TEXT("(자는 중)")));
    TestFalse(TEXT("Sleeping NPC cannot open dialogue"),NPC->TryInteract(PC));TestFalse(TEXT("Player cannot use an occupied bed"),Bed->TryInteract(PC));
    NPC->LifeObjectUser->ReleaseAll(TEXT("TestDone"));NPC->ReleaseForLoad();Pawn->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
