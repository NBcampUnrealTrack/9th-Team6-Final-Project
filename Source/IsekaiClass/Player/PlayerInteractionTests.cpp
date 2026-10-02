#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInteractionObject.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerInteractionTest,"Isekai.Player.InteractionTarget",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerInteractionTest::RunTest(const FString& Parameters)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* PC=World->SpawnActor<APlayerPrototypeController>();
    FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn=World->SpawnActor<APlayerPrototypeCharacter>(FVector(0,0,100),FRotator::ZeroRotator,Spawn);
    PC->Possess(Pawn);PC->SetControlRotation(FRotator::ZeroRotator);
    auto* Box=World->SpawnActor<APlayerPrototypeObject>(FVector(150,70,100),FRotator::ZeroRotator,Spawn);
    TestEqual(TEXT("Nearby front off-center box selected"),PC->FindInteractionTarget(),Box);
    Box->SetActorLocation(FVector(300,0,100));
    TestNull(TEXT("Outside 250cm rejected"),PC->FindInteractionTarget());
    Box->SetActorLocation(FVector(-150,0,100));
    TestNull(TEXT("Behind player rejected"),PC->FindInteractionTarget());
    Box->SetActorLocation(FVector(180,0,100));
    auto* Blocker=World->SpawnActor<APlayerPrototypeObject>(FVector(90,0,100),FRotator::ZeroRotator,Spawn);
    Blocker->Shape->SetWorldScale3D(FVector(0.3,4,4));
    // A generic mesh actor blocks the trace but cannot itself be an interaction target.
    auto* Wall=World->SpawnActor<AActor>(FVector(90,0,100),FRotator::ZeroRotator,Spawn);
    auto* Mesh=NewObject<UStaticMeshComponent>(Wall);Wall->SetRootComponent(Mesh);
    Mesh->SetStaticMesh(Blocker->Shape->GetStaticMesh());Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->RegisterComponent();Mesh->SetWorldLocation(FVector(90,0,100));Mesh->SetWorldScale3D(FVector(0.3,4,4));Blocker->Destroy();
    TestNull(TEXT("Wall blocks interaction"),PC->FindInteractionTarget());
    Wall->Destroy();
    TestEqual(TEXT("Visible target returns after wall removed"),PC->FindInteractionTarget(),Box);
    TestEqual(TEXT("Development uses F6"),PC->MenuCancelKey(),EKeys::F6);
    World->DestroyWorld(false);
    return true;
}
#endif

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "PlayerPrototypeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "../Core/ICSaveSubsystem.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"

class FVerifyPlayerPIESession : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    double Started=FPlatformTime::Seconds();
    int32 Step=0;
    FString Prefix=TEXT("IC_Test_PIE_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TArray<FString> Paths;
public:
    explicit FVerifyPlayerPIESession(FAutomationTestBase* InTest):Test(InTest){}
    virtual bool Update() override
    {
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World?Cast<APlayerPrototypeController>(World->GetFirstPlayerController()):nullptr;
        if(Step==7&&!World)
        {
            Test->TestNotNull(TEXT("PIE quit leaves editor alive"),GEditor);
            for(const auto& Path:Paths)IFileManager::Get().Delete(*Path,false,false,true);
            return true;
        }
        if(FPlatformTime::Seconds()-Started>40)
        {
            Test->AddError(TEXT("PIE session/input/quit timed out."));
            if(GEditor)GEditor->RequestEndPlayMap();
            for(const auto& Path:Paths)IFileManager::Get().Delete(*Path,false,false,true);
            return true;
        }
        if(!PC||!PC->Screen)return false;
        auto Key=[](FKey K){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));};
        auto BackButton=[PC]()
        {
            TArray<UWidget*> Widgets;PC->Screen->WidgetTree->GetAllWidgets(Widgets);
            for(auto* Widget:Widgets)
                if(auto* Button=Cast<UPlayerPrototypeButton>(Widget);Button&&Button->Command==TEXT("Close"))
                    if(auto* Label=Cast<UTextBlock>(Button->GetContent());Label&&Label->GetText().ToString().StartsWith(TEXT("뒤로가기")))return Button;
            return static_cast<UPlayerPrototypeButton*>(nullptr);
        };
        if((Step==1||Step==5||Step==6)&&PC->bDirty)return false;
        if(Step==0)
        {
            PC->GetSaves()->ConfigureServices(PC->GetWorldState(),PC->GetTime(),Prefix);
            for(int32 Slot=0;Slot<=4;++Slot)
                for(const FString& Suffix:TArray<FString>{TEXT(""),TEXT(".tmp"),TEXT(".bak"),TEXT(".rollback"),TEXT(".bak.tmp")})
                    Paths.Add(PC->GetSaves()->GetSlotPath(Slot)+Suffix);
            PC->OpenLoadMenu();Step=1;return false;
        }
        if(Step==1)
        {
            auto* Back=BackButton();Test->TestNotNull(TEXT("Load screen displays back button before creation"),Back);
            if(Back)Back->OnClicked.Broadcast();
            Test->TestEqual(TEXT("Load back returns to creation"),PC->Menu,FString(TEXT("Create")));
            PC->PlayerName=TEXT("PIEInput");PC->ExecuteUI(TEXT("Create"),0);Step=2;return false;
        }
        if(Step==2)
        {
            if(PC->GetSaves()->bSaving)return false;
            Test->TestTrue(TEXT("PIE initial save completes"),PC->bCreated);
            PC->Screen->SetKeyboardFocus();Key(PC->MenuCancelKey());Step=3;return false;
        }
        if(Step==3)
        {
            Test->TestEqual(TEXT("F6 opens pause without stopping PIE"),PC->Menu,FString(TEXT("Pause")));
            Test->TestNotNull(TEXT("PIE remains active after F6"),GEditor->PlayWorld.Get());
            PC->OpenLoadMenu();Key(PC->MenuCancelKey());
            Test->TestEqual(TEXT("F6 load back returns to pause"),PC->Menu,FString(TEXT("Pause")));
            Key(PC->MenuCancelKey());Step=4;return false;
        }
        if(Step==4)
        {
            Test->TestTrue(TEXT("F6 closes pause"),PC->Menu.IsEmpty());
            PC->SetMenu(TEXT("Inventory"));PC->OpenLoadMenu();Step=5;return false;
        }
        if(Step==5)
        {
            auto* Back=BackButton();Test->TestNotNull(TEXT("Load screen displays back button during play"),Back);
            if(Back)Back->OnClicked.Broadcast();
            Test->TestEqual(TEXT("Load back returns to inventory"),PC->Menu,FString(TEXT("Inventory")));
            PC->PlayerDied();Step=6;return false;
        }
        if(Step==6)
        {
            auto* Back=BackButton();Test->TestNotNull(TEXT("Death load screen displays back button"),Back);
            if(Back)Back->OnClicked.Broadcast();
            Test->TestEqual(TEXT("Death load back returns to pause"),PC->Menu,FString(TEXT("Pause")));
            Test->TestTrue(TEXT("Death back keeps world paused"),PC->IsPaused());
            const auto Before=PC->GetTime()->GetClock();PC->GetTime()->AdvanceClock(1,false);
            Test->TestEqual(TEXT("Death back keeps play clock stopped"),PC->GetTime()->GetClock().PlaySecond,Before.PlaySecond);
            PC->ExecuteUI(TEXT("Quit"),0);PC->ExecuteUI(TEXT("QuitWithout"),0);
            Step=7;return false;
        }
        return false;
    }
};
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FPlayerPIESessionTest,"Isekai.Player.PIESession",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FPlayerPIESessionTest::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    Names.Add(TEXT("PrototypeMap"));Commands.Add(TEXT("/Game/PlayerPrototype/L_PlayerPrototype"));
    Names.Add(TEXT("ExistingTestMap"));Commands.Add(TEXT("/Game/IsekaiClass/World/Maps/MainWorld/L_TestMap"));
}
bool FPlayerPIESessionTest::RunTest(const FString& Map)
{
    FRequestPlaySessionParams Params;Params.GlobalMapOverride=Map;Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    ADD_LATENT_AUTOMATION_COMMAND(FVerifyPlayerPIESession(this));
    return true;
}
#endif
