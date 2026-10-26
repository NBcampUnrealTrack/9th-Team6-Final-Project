#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Quest/ICQuestMenuWidget.h"
#include "UI/Quest/ICQuestTrackerWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "Quest/ICQuestSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Save/ICSaveGame.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestMenuCheck,"IC.UI.QuestMenu",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICQuestMenuCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* Q=GI->GetSubsystem<UICQuestSubsystem>();
    const FName Talk=TEXT("QST_TEST_TALK_001"),Collect=TEXT("QST_TEST_COLLECT_001");FICDefinitionResult R;
    auto* TalkRow=const_cast<FICQuestRow*>(D->Find<FICQuestRow>(Talk,R));auto* CollectRow=const_cast<FICQuestRow*>(D->Find<FICQuestRow>(Collect,R));
    if(!TestNotNull(TEXT("Real talk definition"),TalkRow)||!TestNotNull(TEXT("Real collect definition"),CollectRow))return false;
    TGuardValue<int32> TalkGoal(TalkRow->ObjectiveGroups[0].Objectives[0].RequiredCount,3);
    TGuardValue<int32> CollectGoal(CollectRow->ObjectiveGroups[0].Objectives[0].RequiredCount,3);
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("퀘스트검증");Data.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Data.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Initialize actual world"),State->RestoreState(Data));Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);PC->bCreated=true;PC->Menu=TEXT("Pause");
    auto* Class=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
    auto* HUD=Class?CreateWidget<UICGameplayWidget>(PC,Class):nullptr;auto* Menu=HUD?Cast<UICGameMenuWidget>(HUD->GetWidgetFromName(TEXT("GameMenu"))):nullptr;
    auto* UI=Menu?Cast<UICQuestMenuWidget>(Menu->GetWidgetFromName(TEXT("QuestContent"))):nullptr;
    if(TestNotNull(TEXT("Quest tab embeds a separate journal WBP"),UI))
    {
        const auto Slate=HUD->TakeWidget();Menu->SelectTab(TEXT("Quest"));
        TestEqual(TEXT("Unaccepted sample quests are hidden from the journal"),UI->QuestIDs.Num(),0);
        const int32 StartingGold=State->ExportState().Gold;
        for(int32 I=1;I<=10;++I)
        {
            const FName ID(*FString::Printf(TEXT("QST_NPC_TALK_%03d"),I));FICQuestRow Definition;D->GetQuest(ID,Definition);FICRewardRow Reward;D->GetReward(Definition.RewardID,Reward);
            TestEqual(TEXT("Each numbered NPC offers its own quest"),Definition.OfferNPCID,FName(*FString::Printf(TEXT("NPC_R01_%03d"),I)));
            TestEqual(TEXT("Each quest targets the next NPC"),Definition.ObjectiveGroups[0].Objectives[0].Target.ID,FName(*FString::Printf(TEXT("NPC_R01_%03d"),I+1)));
            TestEqual(TEXT("Every talk quest pays exactly 100 gold"),Reward.Gold,100);
            TestFalse(TEXT("Journal cannot open an unaccepted quest"),UI->OpenQuest(ID));
            TestTrue(TEXT("NPC accepts the sample quest"),Q->AcceptQuest(ID));UI->RefreshQuests();
            TestEqual(TEXT("Only the accepted sample quest is listed"),UI->QuestIDs.Num(),1);
            TestTrue(TEXT("Journal opens the accepted sample quest"),UI->OpenQuest(ID));
            TestTrue(TEXT("A newly accepted quest starts when the HUD is empty"),State->ExportState().Quests.FindRef(ID).bStarted);
            Q->ReportObjectiveEvent(EICQuestObjectiveType::Talk,Definition.OfferNPCID,1,FGuid::NewGuid());TestFalse(TEXT("Talking to the offer NPC does not complete the objective"),State->ExportState().Quests.FindRef(ID).bCompleted);
            Q->ReportObjectiveEvent(EICQuestObjectiveType::Talk,Definition.ObjectiveGroups[0].Objectives[0].Target.ID,
                1,FGuid::NewGuid(),EICLifeActivity::Gathering,NAME_None,Definition.ObjectiveGroups[0].Objectives[0].DialogueID);TestTrue(TEXT("Talking to the target completes the quest"),State->ExportState().Quests.FindRef(ID).bCompleted);
            TestEqual(TEXT("Actual gold reward is applied once"),State->ExportState().Gold,StartingGold+I*100);
            Q->ReportObjectiveEvent(EICQuestObjectiveType::Talk,Definition.ObjectiveGroups[0].Objectives[0].Target.ID,
                1,FGuid::NewGuid(),EICLifeActivity::Gathering,NAME_None,Definition.ObjectiveGroups[0].Objectives[0].DialogueID);TestEqual(TEXT("Repeated talk does not pay twice"),State->ExportState().Gold,StartingGold+I*100);
        }
        TestTrue(TEXT("Reset to the isolated journal fixture"),State->RestoreState(Data));UI->RefreshQuests();
        auto* Detail=CastChecked<UICQuestDetailWidget>(UI->GetWidgetFromName(TEXT("QuestDetail")));
        auto* Tracker=CastChecked<UICQuestTrackerWidget>(HUD->GetWidgetFromName(TEXT("HUD_QuestTracker")));
        auto TrackerText=[&](){return CastChecked<UTextBlock>(Tracker->GetWidgetFromName(TEXT("QuestText")))->GetText().ToString();};
        auto Label=[&](const TCHAR* Name){return CastChecked<UTextBlock>(Detail->GetWidgetFromName(Name))->GetText().ToString();};
        TestEqual(TEXT("Empty journal has no invented quests"),UI->QuestIDs.Num(),0);
        TestTrue(TEXT("NPC acceptance adds talk quest to journal"),Q->AcceptQuest(Talk));TestTrue(TEXT("NPC acceptance adds collect quest"),Q->AcceptQuest(Collect));UI->RefreshQuests();UI->SelectQuest(0);
        TestFalse(TEXT("Quest tab initially shows the list"),UI->bShowingDetails);TestEqual(TEXT("Two accepted quests are listed"),UI->QuestIDs.Num(),2);
        TestTrue(TEXT("First accepted quest is immediately shown in the HUD"),TrackerText().Contains(TEXT("상인과 대화")));
        TestFalse(TEXT("Later acceptance does not replace the HUD quest"),TrackerText().Contains(TEXT("재료 수집")));
        TestTrue(TEXT("First acceptance auto-starts"),State->ExportState().Quests[Talk].bStarted);
        TestFalse(TEXT("Second accepted quest waits in the list"),State->ExportState().Quests[Collect].bStarted);
        auto* Entries=CastChecked<UVerticalBox>(UI->GetWidgetFromName(TEXT("QuestEntries")));
        auto* First=CastChecked<UICQuestMenuEntryWidget>(Entries->GetChildAt(0));
        auto RowTitle=[&](int32 Index){return CastChecked<UTextBlock>(CastChecked<UICQuestMenuEntryWidget>(Entries->GetChildAt(Index))->GetWidgetFromName(TEXT("TXT_Title")))->GetText().ToString();};
        TestFalse(TEXT("Waiting quest has no running marker"),RowTitle(0).Contains(TEXT("(진행중)")));
        TestTrue(TEXT("HUD quest is marked in the journal list"),RowTitle(1).Contains(TEXT("(진행중)")));
        TestTrue(TEXT("First quest has a selected glowing frame"),First->bSelected);
        TestFalse(TEXT("Summary is a single line"),CastChecked<UTextBlock>(First->GetWidgetFromName(TEXT("TXT_Summary")))->GetText().ToString().Contains(TEXT("\n")));
        Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("S selects the next quest"),UI->SelectedQuest,Talk);
        TestFalse(TEXT("Previous row loses selection"),First->bSelected);TestTrue(TEXT("New row gains selection"),CastChecked<UICQuestMenuEntryWidget>(Entries->GetChildAt(1))->bSelected);
        Menu->HandleMenuKey(EKeys::Enter);TestTrue(TEXT("Enter opens details"),UI->bShowingDetails);
        TestTrue(TEXT("Detail displays the actual title"),Label(TEXT("TXT_Name")).Contains(TEXT("상인과 대화")));
        TestFalse(TEXT("Detail description is present"),Label(TEXT("TXT_Description")).IsEmpty());TestFalse(TEXT("Objectives are present"),Label(TEXT("TXT_Objectives")).IsEmpty());TestFalse(TEXT("Rewards section exists even when empty"),Label(TEXT("TXT_Rewards")).IsEmpty());
        Menu->HandleMenuKey(EKeys::Enter);TestTrue(TEXT("Keyboard Start activates selected quest"),State->ExportState().Quests[Talk].bStarted);
        TestTrue(TEXT("HUD displays the started quest"),TrackerText().Contains(TEXT("상인과 대화")));TestFalse(TEXT("HUD excludes waiting quests"),TrackerText().Contains(TEXT("재료 수집")));
        Q->ReportObjectiveEvent(EICQuestObjectiveType::Talk,TEXT("NPC_R01_101"),1,FGuid::NewGuid());
        Menu->HandleMenuKey(EKeys::S);Menu->HandleMenuKey(EKeys::Enter);TestFalse(TEXT("Bottom Back returns to list"),UI->bShowingDetails);TestEqual(TEXT("Back preserves the selected quest"),UI->SelectedQuest,Talk);
        Menu->HandleMenuKey(EKeys::W);Menu->HandleMenuKey(EKeys::Enter);Menu->HandleMenuKey(EKeys::Enter);
        auto Current=State->ExportState();TestTrue(TEXT("Switch leaves old quest accepted but stopped"),Current.Quests[Talk].bAccepted&&!Current.Quests[Talk].bStarted);
        TestEqual(TEXT("Switch resets old progress"),Current.Quests[Talk].Objectives.Num(),0);TestTrue(TEXT("New quest starts"),Current.Quests[Collect].bStarted);
        TestTrue(TEXT("HUD replaces old quest"),TrackerText().Contains(TEXT("재료 수집"))&&!TrackerText().Contains(TEXT("상인과 대화")));
        TestTrue(TEXT("Explicit switching moves the running marker"),RowTitle(0).Contains(TEXT("(진행중)"))&&!RowTitle(1).Contains(TEXT("(진행중)")));
        Q->ReportObjectiveEvent(EICQuestObjectiveType::Collect,TEXT("ITM_MAT_002"),1,FGuid::NewGuid());
        auto* Save=NewObject<UICSaveGame>();Save->WorldState=State->ExportState();TArray<uint8> Bytes;
        TestTrue(TEXT("Quest selection serializes with the real save type"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
        auto* Loaded=Cast<UICSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));TestNotNull(TEXT("Serialized save reloads"),Loaded);
        if(Loaded){TestTrue(TEXT("Started state survives serialization"),Loaded->WorldState.Quests[Collect].bStarted);TestEqual(TEXT("Progress survives serialization"),Loaded->WorldState.Quests[Collect].Objectives.FindRef(TEXT("OBJ_COLLECT")),1);}
        Menu->HandleMenuKey(EKeys::A);Menu->HandleMenuKey(EKeys::Enter);TestFalse(TEXT("Abandon returns to journal"),UI->bShowingDetails);
        TestEqual(TEXT("Abandoned quest leaves the list"),UI->QuestIDs.Num(),1);TestEqual(TEXT("Abandon clears HUD tracker"),Tracker->GetVisibility(),ESlateVisibility::Collapsed);
        if(Loaded){TestTrue(TEXT("Load restores active quest"),State->RestoreState(Loaded->WorldState));UI->RefreshQuests();TestEqual(TEXT("Journal refreshes after load"),UI->QuestIDs.Num(),2);TestTrue(TEXT("HUD restores started quest"),TrackerText().Contains(TEXT("재료 수집")));}
        UI->OpenQuest(Talk);Menu->HandleMenuKey(EKeys::Escape);TestFalse(TEXT("ESC in details returns to list"),UI->bShowingDetails);TestEqual(TEXT("Detail Back does not close game menu"),PC->Menu,FString(TEXT("Pause")));
        UI->OpenQuest(Collect);Menu->SelectTab(TEXT("Party"));Menu->SelectTab(TEXT("Quest"));TestFalse(TEXT("Reentering tab always starts at quest list"),UI->bShowingDetails);
        Q->ReportObjectiveEvent(EICQuestObjectiveType::Collect,TEXT("ITM_MAT_002"),2,FGuid::NewGuid());UI->RefreshQuests();
        TestTrue(TEXT("Started quest completes through actual objective runner"),State->ExportState().Quests[Collect].bCompleted);
        TestFalse(TEXT("Completed quest cannot be restarted for duplicate rewards"),Q->StartQuest(Collect));TestEqual(TEXT("Completed quest leaves HUD"),Tracker->GetVisibility(),ESlateVisibility::Collapsed);
        // Exercise entry and back buttons through their real delegates too.
        First=CastChecked<UICQuestMenuEntryWidget>(Entries->GetChildAt(0));CastChecked<UButton>(First->GetWidgetFromName(TEXT("BTN_Quest")))->OnClicked.Broadcast();TestTrue(TEXT("Mouse entry button opens details"),UI->bShowingDetails);
        CastChecked<UButton>(Detail->GetWidgetFromName(TEXT("BTN_Back")))->OnClicked.Broadcast();TestFalse(TEXT("Mouse Back button returns to list"),UI->bShowingDetails);
        HUD->ReleaseSlateResources(true);
    }
    Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
