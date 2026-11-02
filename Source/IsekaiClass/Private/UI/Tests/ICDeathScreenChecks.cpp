#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/Gameplay/ICDeathWidget.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "Save/ICSaveSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICDeathScreenCheck,"IC.UI.DeathScreen",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICDeathScreenCheck::RunTest(const FString& Parameters)
{
    auto* Class=LoadClass<UICDeathWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Death.WBP_IC_Death_C"));
    if(!TestNotNull(TEXT("Authored death WBP exists"),Class))return false;
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();auto* Saves=GI->GetSubsystem<UICSaveSubsystem>();
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("사망 검증");Data.FirstSkill=TEXT("SKL_NPC_R01_002");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Data.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Isolated player state is valid"),State->RestoreState(Data));
    Saves->ConfigureServices(State,Time,TEXT("IC_Test_Death_")+FGuid::NewGuid().ToString(EGuidFormats::Digits));
    PC->bCreated=true;Time->SetRunning(true);Time->SetMenuPaused(false);
    PC->PlayerDied();
    TestTrue(TEXT("Death is retained"),PC->IsDead());TestEqual(TEXT("Death opens its own screen"),PC->Menu,FString(TEXT("Death")));
    TestFalse(TEXT("Death stops game time"),Time->IsRunning());
    TArray<UUserWidget*> SaveLists;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,SaveLists,UICSaveLoadWidget::StaticClass(),false);
    TestEqual(TEXT("Death does not immediately open save slots"),SaveLists.Num(),0);
    auto* Death=PC->DeathScreen.Get();
    if(TestNotNull(TEXT("Controller owns death widget"),Death))
    {
        const auto Slate=Death->TakeWidget();
        auto* Background=Cast<UBorder>(Death->GetWidgetFromName(TEXT("DeathBackground")));
        if(TestNotNull(TEXT("Opaque fullscreen background"),Background))TestEqual(TEXT("Background is solid black"),Background->GetBrushColor(),FLinearColor::Black);
        auto* Title=Cast<UTextBlock>(Death->GetWidgetFromName(TEXT("TXT_Death")));
        if(TestNotNull(TEXT("Death message"),Title))TestEqual(TEXT("Requested death message"),Title->GetText().ToString(),FString(TEXT("사망하였습니다.")));
        int32 Buttons=0;Death->WidgetTree->ForEachWidget([&](UWidget* Widget){if(Widget->IsA<UButton>())++Buttons;});
        TestEqual(TEXT("Exactly three actions"),Buttons,3);
        const TCHAR* Names[]={TEXT("BTN_Load"),TEXT("BTN_MainMenu"),TEXT("BTN_Quit")};
        const TCHAR* Labels[]={TEXT("불러오기"),TEXT("메인메뉴"),TEXT("그만하기")};
        for(int32 Index=0;Index<3;++Index)
            if(auto* Button=Cast<UButton>(Death->GetWidgetFromName(Names[Index])))
            {TestTrue(TEXT("Death button is wired"),Button->OnClicked.IsBound());TestEqual(TEXT("Requested action label"),CastChecked<UTextBlock>(Button->GetContent())->GetText().ToString(),FString(Labels[Index]));}
            else AddError(FString(TEXT("Missing death action: "))+Names[Index]);
        for(const auto& Key:{EKeys::Escape,EKeys::Equals,EKeys::Gamepad_FaceButton_Right})
        {
            Slate->OnKeyDown(FGeometry(),FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
            TestEqual(TEXT("Back/menu keys cannot leave death"),PC->Menu,FString(TEXT("Death")));
        }
        for(const auto Command:{EICInputCommand::Menu,EICInputCommand::Inventory,EICInputCommand::Save,EICInputCommand::Load})
        {PC->HandleInputCommand(Command);TestEqual(TEXT("Gameplay shortcuts cannot leave death"),PC->Menu,FString(TEXT("Death")));}
        for(const auto* Menu:{TEXT("Pause"),TEXT("Inventory"),TEXT("Settings"),TEXT("")})
        {PC->SetMenu(Menu);TestEqual(TEXT("Programmatic menu requests cannot bypass death"),PC->Menu,FString(TEXT("Death")));}
        PC->OpenSaveMenu();TestEqual(TEXT("Dead player cannot enter saving"),PC->Menu,FString(TEXT("Death")));
        CastChecked<UButton>(Death->GetWidgetFromName(TEXT("BTN_Load")))->OnClicked.Broadcast();
        TestEqual(TEXT("Load button opens load list"),PC->Menu,FString(TEXT("Load")));
        PC->PlayerDied();TestEqual(TEXT("Repeated death does not interrupt load selection"),PC->Menu,FString(TEXT("Load")));
        PC->CloseSessionMenu();TestEqual(TEXT("Load Back returns to death instead of pause"),PC->Menu,FString(TEXT("Death")));
        PC->OpenLoadMenu();PC->HandleInputCommand(EICInputCommand::Menu);
        TestEqual(TEXT("Escape from load returns to death"),PC->Menu,FString(TEXT("Death")));
        PC->CloseSessionMenu();TestEqual(TEXT("Back on death stays on death"),PC->Menu,FString(TEXT("Death")));
        PC->DeathScreen->LoadGame();PC->RequestLoadPlayer();
        TestEqual(TEXT("Missing saves keep dead player in load list"),PC->Menu,FString(TEXT("Load")));TestTrue(TEXT("Failed load cannot revive player"),PC->IsDead());
        PC->CloseSessionMenu();
        Saves->bSaving=true;PC->Notice.Empty();
        CastChecked<UButton>(PC->DeathScreen->GetWidgetFromName(TEXT("BTN_Quit")))->OnClicked.Broadcast();
        TestFalse(TEXT("Quit action reaches existing quit handler"),PC->Notice.IsEmpty());
        TestEqual(TEXT("Quit adds no confirmation or pause menu"),PC->Menu,FString(TEXT("Death")));
        Saves->bSaving=false;
    }
    GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
