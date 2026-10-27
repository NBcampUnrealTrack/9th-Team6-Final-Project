#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Data/Rows/Character/ICCharacterProfileRow.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/SWindow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPartyMenuCheck,"IC.UI.PartyMenu",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPartyMenuCheck::RunTest(const FString& Parameters)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,0);
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("실제 파티 플레이어");Data.Preset=1;Data.FirstSkill=TEXT("SKL_START_SWORD_001");Data.Inventory=Staging->GetSnapshot();
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;P.MainStats.STR=12;Data.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Actual player data initializes"),State->RestoreState(Data));TestTrue(TEXT("Live player initializes"),Player->InitializeState(P.CharacterId));Player->Inventory->BindWorldState(State);
    PC->bCreated=true;PC->Menu=TEXT("Pause");
    auto* Class=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
    auto* HUD=Class?CreateWidget<UICGameplayWidget>(PC,Class):nullptr;
    auto* Menu=HUD?Cast<UICGameMenuWidget>(HUD->GetWidgetFromName(TEXT("GameMenu"))):nullptr;
    auto* Party=Menu?Cast<UICPartyMenuWidget>(Menu->GetWidgetFromName(TEXT("PartyContent"))):nullptr;
    if(TestNotNull(TEXT("Separate party WBP is embedded in the Party tab"),Party))
    {
        const auto Slate=Menu->TakeWidget();Party->RefreshParty();
        for(const auto* Group:{TEXT("ResourcesGroup"),TEXT("AttributesGroup"),TEXT("CombatGroup")})TestNotNull(TEXT("Stats have three distinct groups"),Party->GetWidgetFromName(Group));
        TestEqual(TEXT("CON is labelled stamina constitution in Korean"),CastChecked<UTextBlock>(Party->GetWidgetFromName(TEXT("LABEL_CON")))->GetText().ToString(),FString(TEXT("체력")));
        auto Card=[&](int32 I){return CastChecked<UICPartyPortraitWidget>(Party->GetWidgetFromName(FName(*FString::Printf(TEXT("Member%d"),I))));};
        auto Label=[&](const TCHAR* Key){return CastChecked<UTextBlock>(Party->GetWidgetFromName(Key))->GetText().ToString();};
        TestEqual(TEXT("Without companions the roster contains only the player"),Party->Members.Num(),1);
        TestEqual(TEXT("Right pane uses actual player name"),Label(TEXT("TXT_Name")),Data.PlayerName);
        TestEqual(TEXT("The player main setting comes from the profile DT"),Label(TEXT("TXT_MainSetting")),FString(TEXT("게임 속에 떨어진 수험생")));
        TestTrue(TEXT("The exact supplied story is present"),Label(TEXT("TXT_Story")).Contains(TEXT("3년 전 게임 속으로 떨어졌다."))&&Label(TEXT("TXT_Story")).Contains(TEXT("일단 모든 친구들을 만나러 가자")));
        FICDefinitionResult R;const auto* Preset=D->Find<FICPlayerPresetRow>(TEXT("PRESET_PLAYER_F"),R);
        TestEqual(TEXT("Actual selected preset portrait"),CastChecked<UImage>(Card(0)->GetWidgetFromName(TEXT("IMG_Portrait")))->GetBrush().GetResourceObject(),static_cast<UObject*>(Preset?Preset->Portrait.LoadSynchronous():nullptr));
        TestEqual(TEXT("Stats read live GAS including derived bonuses"),Label(TEXT("STAT_STR")),FString::FromInt(FMath::RoundToInt(Player->GetICAbilitySystem()->Stat(EICStat::STR))));
        for(int32 I=1;I<4;++I)TestEqual(TEXT("Unfilled portrait cells collapse"),Card(I)->GetParent()->GetVisibility(),ESlateVisibility::Collapsed);
        const FName IDs[]={TEXT("NPC_R01_002"),TEXT("NPC_R01_008"),TEXT("NPC_R01_011")};
        for(int32 I=0;I<3;++I)
        {FICCharacterState NPC;NPC.CharacterId=IDs[I];NPC.Affinity=300;State->SetCharacterState(NPC);}
        Party->RefreshParty();TestEqual(TEXT("Unrecruited NPCs stay out of the party"),Party->Members.Num(),1);
        TestTrue(TEXT("Recruit into third slot"),State->SetPartySlot(IDs[2],2));TestTrue(TEXT("Recruit into first slot"),State->SetPartySlot(IDs[0],0));TestTrue(TEXT("Recruit into middle slot"),State->SetPartySlot(IDs[1],1));
        Party->RefreshParty();TestEqual(TEXT("Player plus three companions"),Party->Members.Num(),4);
        Menu->SelectTab(TEXT("Party"));Menu->HandleMenuKey(EKeys::S);
        TestEqual(TEXT("S chooses the first companion immediately"),Party->SelectedCharacter,IDs[0]);
        TestTrue(TEXT("Selected portrait grows"),CastChecked<USizeBox>(Card(1)->GetParent())->GetWidthOverride()>CastChecked<USizeBox>(Card(0)->GetParent())->GetWidthOverride());
        Menu->HandleMenuKey(EKeys::S);Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("Keyboard traverses actual party order"),Party->SelectedCharacter,IDs[2]);
        Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("S stops at the last real companion"),Party->SelectedCharacter,IDs[2]);
        Menu->HandleMenuKey(EKeys::W);TestEqual(TEXT("W moves back up"),Party->SelectedCharacter,IDs[1]);
        auto* NPC=D->Find<FICNPCRow>(IDs[1],R);
        TestEqual(TEXT("Companion uses its own actual portrait"),CastChecked<UImage>(Card(2)->GetWidgetFromName(TEXT("IMG_Portrait")))->GetBrush().GetResourceObject(),static_cast<UObject*>(NPC?NPC->Portrait.LoadSynchronous():nullptr));
        auto* Profiles=DuplicateObject<UDataTable>(Party->ProfileTable.LoadSynchronous(),GI);Party->ProfileTable=Profiles;
        auto* Profile=Profiles->FindRow<FICCharacterProfileRow>(IDs[1],TEXT("PartyTest"));
        if(TestNotNull(TEXT("NPC profile has its own DT row"),Profile))
        {Profile->MainSetting=FText::FromString(TEXT("테스트 주 설정"));Profile->Story=FText::FromString(TEXT("테스트 이야기"));Profile->PetPortrait=NPC->Portrait;Party->RefreshParty();
         TestEqual(TEXT("NPC main setting uses its own row"),Label(TEXT("TXT_MainSetting")),FString(TEXT("테스트 주 설정")));
         TestEqual(TEXT("Configured pet image appears"),CastChecked<UImage>(Party->GetWidgetFromName(TEXT("IMG_Pet")))->GetBrush().GetResourceObject(),static_cast<UObject*>(NPC->Portrait.LoadSynchronous()));}
        Menu->HandleMenuKey(EKeys::D);TestEqual(TEXT("D enters story scrolling"),Menu->NavigationArea,2);
        Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("Story navigation preserves selected character"),Party->SelectedCharacter,IDs[1]);
        Menu->HandleMenuKey(EKeys::A);Menu->HandleMenuKey(EKeys::A);TestEqual(TEXT("A returns to top tabs"),Menu->NavigationArea,0);
        Menu->HandleMenuKey(EKeys::A);TestEqual(TEXT("A moves to Settings without a mouse"),Menu->GetSelectedTab(),FName(TEXT("Settings")));
        Menu->HandleMenuKey(EKeys::S);Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("W/S selects Settings actions"),Menu->SettingsActionIndex,1);
        PC->Screen=nullptr;Menu->HandleMenuKey(EKeys::Enter);TestEqual(TEXT("Keyboard activates the actual Save button"),PC->Menu,FString(TEXT("Save")));PC->CloseSessionMenu();
        Menu->SelectTab(TEXT("Party"));Party->SelectMember(2);State->SetPartySlot(IDs[1],INDEX_NONE);Party->RefreshParty();
        TestEqual(TEXT("Dismissed member is removed immediately"),Party->Members.Num(),3);TestFalse(TEXT("Selection cannot retain a dismissed character"),Party->SelectedCharacter==IDs[1]);
        State->RestoreState(Data);Party->RefreshParty();TestEqual(TEXT("Loading a different party rebuilds the roster"),Party->Members.Num(),1);
        TestEqual(TEXT("Missing pet image clears previous character's pet"),CastChecked<UImage>(Party->GetWidgetFromName(TEXT("IMG_Pet")))->GetVisibility(),ESlateVisibility::Hidden);
        // Register the menu through the real UI manager, then verify shared WASD rules.
        PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->SetHUD(Menu);
        const auto Config=FSlateApplication::Get().GetNavigationConfig();
        TestEqual(TEXT("Shared menus understand WASD"),Config->GetNavigationDirectionFromKey(FKeyEvent(EKeys::S,FModifierKeysState(),0,false,0,0)),EUINavigation::Down);
        auto Window=SNew(SWindow).ClientSize(FVector2D(300,70)).CreateTitleBar(false);auto Edit=SNew(SEditableText);Window->SetContent(Edit);
        FSlateApplication::Get().AddWindow(Window,false);FSlateApplication::Get().SetKeyboardFocus(Edit);
        TestEqual(TEXT("WASD remains text inside name fields"),Config->GetNavigationDirectionFromKey(FKeyEvent(EKeys::W,FModifierKeysState(),0,false,0,0)),EUINavigation::Invalid);
        FSlateApplication::Get().RequestDestroyWindow(Window);
        Menu->ReleaseSlateResources(true);
    }
    Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
