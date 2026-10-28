#include "Settings/ICGameUserSettings.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "UI/Quest/ICQuestMenuWidget.h"
#include "UI/Codex/ICCodexMenuWidget.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

const TArray<FName>& UICGameMenuWidget::Tabs()
{
    static const TArray<FName> Names = {TEXT("Party"), TEXT("Inventory"), TEXT("Quest"), TEXT("Codex"), TEXT("Settings")};
    return Names;
}

void UICGameMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget) ICFrontendLayout::Build(WidgetTree, TEXT("GameMenu"));
    SetIsFocusable(true);
    for (int32 Index = 0; Index < Tabs().Num(); ++Index)
        if (auto* Button = Cast<UButton>(GetWidgetFromName(FName(*(TEXT("TAB_") + Tabs()[Index].ToString())))))
        {
            FScriptDelegate Click;
            Click.BindUFunction(this, Tabs()[Index]);
            Button->OnClicked.AddUnique(Click);
        }
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Resume")))) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Resume);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Save")))) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Save);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Load")))) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Load);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_GameSettings")))) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::GameSettings);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit")))) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Quit);
    if(auto* Content=Cast<UICCodexMenuWidget>(GetWidgetFromName(TEXT("CodexContent"))))Content->OnNavigationChanged.AddUniqueDynamic(this,&ThisClass::CodexNavigationChanged);
    if(auto* Content=Cast<UICInventoryMenuWidget>(GetWidgetFromName(TEXT("InventoryContent"))))Content->OnNavigationChanged.AddUniqueDynamic(this,&ThisClass::InventoryNavigationChanged);
    SelectTab(SelectedTab);
}

bool UICGameMenuWidget::SelectTab(FName Tab)
{
    if(auto* Inventory=Cast<UICInventoryMenuWidget>(GetWidgetFromName(TEXT("InventoryContent")));Inventory&&Inventory->IsBusy())return false;
    if(auto* Content=Cast<UICCodexMenuWidget>(GetWidgetFromName(TEXT("CodexContent")));Content&&Content->IsModalOpen())return false;
    const int32 Index = Tabs().IndexOfByKey(Tab);
    auto* Pages = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("MenuPages")));
    if (Index == INDEX_NONE || !Pages || Index >= Pages->GetNumWidgets()) return false;
    SelectedTab = Tab;
    NavigationArea=1;SettingsActionIndex=0;
    Pages->SetActiveWidgetIndex(Index);
    for (const FName Name : Tabs())
        if (auto* Button = Cast<UButton>(GetWidgetFromName(FName(*(TEXT("TAB_") + Name.ToString())))))
            Button->SetBackgroundColor(Name == Tab ? FLinearColor(.72f, .57f, .31f) : FLinearColor(.29f, .36f, .47f));
    if(auto* Party=Cast<UICPartyMenuWidget>(GetWidgetFromName(TEXT("PartyContent"))))Party->RefreshParty();
    if(Tab==TEXT("Inventory"))if(auto* Inventory=Cast<UICInventoryMenuWidget>(GetWidgetFromName(TEXT("InventoryContent"))))Inventory->Activate();
    if(Tab==TEXT("Quest"))if(auto* Quests=Cast<UICQuestMenuWidget>(GetWidgetFromName(TEXT("QuestContent"))))Quests->Activate();
    if(Tab==TEXT("Codex"))if(auto* Content=Cast<UICCodexMenuWidget>(GetWidgetFromName(TEXT("CodexContent"))))Content->Activate();
    RefreshNavigation();
    if(GetOwningPlayer()&&(HasAnyUserFocus()||HasFocusedDescendants()))SetUserFocus(GetOwningPlayer());
    return true;
}

void UICGameMenuWidget::SetSessionPresentation(const FString& Menu, const FText& Message)
{
    // Keep the selected tab and action while the separate options screen is open.
    if(Menu==TEXT("Settings"))return;
    if (LastSessionMenu != Menu)
    {
        SelectTab(Menu == TEXT("Confirm") || Menu == TEXT("Quit") || Menu == TEXT("MenuSettings")
            ? FName(TEXT("Settings")) : FName(*Menu));
        LastSessionMenu = Menu;
    }
    const bool Confirm = Menu == TEXT("Confirm");
    const bool Exit = Menu == TEXT("Quit");
    if (auto* TabsWidget = GetWidgetFromName(TEXT("MenuTabs")))
        TabsWidget->SetVisibility(Confirm || Exit ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SessionMessage"))))
    {
        Text->SetText(Message);
        Text->SetVisibility(Confirm || Exit ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    for (const auto* Name : {TEXT("BTN_Save"), TEXT("BTN_Load"), TEXT("BTN_GameSettings")})
        if (auto* Button = GetWidgetFromName(Name)) Button->SetVisibility(Confirm ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Resume"))))
        if (auto* Caption = Cast<UTextBlock>(Button->GetContent()))
            Caption->SetText(ICFrontendLayout::Text(Confirm ? TEXT("Accept") : Exit ? TEXT("Back") : TEXT("Resume")));
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit"))))
        if (auto* Caption = Cast<UTextBlock>(Button->GetContent()))
            Caption->SetText(ICFrontendLayout::Text(Confirm ? TEXT("Back") : Exit ? TEXT("QuitWithoutSave") : TEXT("Quit")));
    RefreshNavigation();
}

void UICGameMenuWidget::RefreshNavigation()
{
    auto* Inventory=SelectedTab==TEXT("Inventory")?Cast<UICInventoryMenuWidget>(GetWidgetFromName(TEXT("InventoryContent"))):nullptr;
    auto* CodexContent=SelectedTab==TEXT("Codex")?Cast<UICCodexMenuWidget>(GetWidgetFromName(TEXT("CodexContent"))):nullptr;
    if(auto* TabsWidget=GetWidgetFromName(TEXT("MenuTabs")))TabsWidget->SetIsEnabled((!Inventory||!Inventory->IsBusy())&&(!CodexContent||!CodexContent->IsModalOpen()));
    TArray<UButton*> Actions;
    for(const auto* Name:{TEXT("BTN_Resume"),TEXT("BTN_Save"),TEXT("BTN_Load"),TEXT("BTN_GameSettings"),TEXT("BTN_Quit")})
        if(auto* Button=Cast<UButton>(GetWidgetFromName(Name));Button&&Button->IsVisible()&&Button->GetIsEnabled())Actions.Add(Button);
    SettingsActionIndex=FMath::Clamp(SettingsActionIndex,0,FMath::Max(0,Actions.Num()-1));
    for(int32 Index=0;Index<Actions.Num();++Index)Actions[Index]->SetBackgroundColor(
        SelectedTab==TEXT("Settings")&&NavigationArea==1&&Index==SettingsActionIndex?FLinearColor(.72f,.57f,.31f):FLinearColor(.29f,.36f,.47f));
    FText Hint;
    if(NavigationArea==0)Hint=NSLOCTEXT("ICParty","TabsHint","A / D  탭 이동     S / Enter  열기     ESC  닫기");
    else if(SelectedTab==TEXT("Party")&&NavigationArea==2)Hint=NSLOCTEXT("ICParty","StoryHint","W / S  이야기 스크롤     A  인물 목록     D  상단 탭     ESC  닫기");
    else if(SelectedTab==TEXT("Party"))Hint=NSLOCTEXT("ICParty","PartyHint","W / S  인물 선택     D  이야기 읽기     A  상단 탭     ESC  닫기");
    else if(SelectedTab==TEXT("Settings"))Hint=NSLOCTEXT("ICParty","SettingsHint","W / S  버튼 이동     Enter / Space  선택     A / D  상단 탭     ESC  닫기");
    else if(Inventory)Hint=Inventory->NavigationHint();
    else if(SelectedTab==TEXT("Quest")){if(auto* Quests=Cast<UICQuestMenuWidget>(GetWidgetFromName(TEXT("QuestContent"))))Hint=Quests->NavigationHint();}
    else if(CodexContent)Hint=CodexContent->NavigationHint();
    else Hint=NSLOCTEXT("ICParty","EmptyHint","A / D  상단 탭     ESC  닫기");
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Navigation"))))Text->SetText(Hint);
    if(auto* Body=Cast<UBorder>(GetWidgetFromName(TEXT("MenuContent"))))Body->SetBrushColor(
        NavigationArea==2?FLinearColor(.055f,.067f,.085f):FLinearColor(.025f,.038f,.06f));
}

void UICGameMenuWidget::CodexNavigationChanged()
{if(SelectedTab==TEXT("Codex")){NavigationArea=1;RefreshNavigation();}}
void UICGameMenuWidget::InventoryNavigationChanged()
{if(SelectedTab==TEXT("Inventory")){NavigationArea=1;RefreshNavigation();}}

bool UICGameMenuWidget::HandleMenuKey(FKey Key,bool bRepeat)
{
    auto* CodexContent=SelectedTab==TEXT("Codex")?Cast<UICCodexMenuWidget>(GetWidgetFromName(TEXT("CodexContent"))):nullptr;
    if(CodexContent&&(NavigationArea!=0||CodexContent->IsModalOpen()))
    {
        if(CodexContent->IsModalOpen())NavigationArea=1;
        if(CodexContent->HandleMenuKey(Key,bRepeat)){RefreshNavigation();return true;}
    }
    auto* Inventory=SelectedTab==TEXT("Inventory")?Cast<UICInventoryMenuWidget>(GetWidgetFromName(TEXT("InventoryContent"))):nullptr;
    if(Inventory&&Inventory->IsSearchFocused())return false;
    if(Inventory&&(NavigationArea!=0||Inventory->IsBusy()))
    {
        if(Inventory->HandleMenuKey(Key,bRepeat)){RefreshNavigation();return true;}
    }
    auto* Quests=SelectedTab==TEXT("Quest")?Cast<UICQuestMenuWidget>(GetWidgetFromName(TEXT("QuestContent"))):nullptr;
    if(Quests&&(NavigationArea!=0||Quests->bShowingDetails))
    {
        if(Quests->bShowingDetails)NavigationArea=1;
        if(Quests->HandleMenuKey(Key,bRepeat)){RefreshNavigation();return true;}
    }
    const bool Up=Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up;
    const bool Down=Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down;
    const bool Left=Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left;
    const bool Right=Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right;
    const bool Accept=Key==EKeys::Enter||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key);
    if(!Up&&!Down&&!Left&&!Right&&!Accept)return false;
    auto* TabsWidget=GetWidgetFromName(TEXT("MenuTabs"));const bool TabsVisible=!TabsWidget||TabsWidget->IsVisible();
    if(!TabsVisible)NavigationArea=1;
    if(NavigationArea==0)
    {
        if(Left||Right){const int32 Index=(Tabs().IndexOfByKey(SelectedTab)+(Right?1:Tabs().Num()-1))%Tabs().Num();SelectTab(Tabs()[Index]);NavigationArea=0;}
        else if(Down||Accept)NavigationArea=1;
    }
    else if(SelectedTab==TEXT("Party"))
    {
        auto* Party=Cast<UICPartyMenuWidget>(GetWidgetFromName(TEXT("PartyContent")));
        if(NavigationArea==2)
        {if(Left)NavigationArea=1;else if(Right)NavigationArea=0;else if(Up||Down){if(Party)Party->ScrollStory(Down?72:-72);}}
        else if(Left||(Up&&Party&&Party->SelectedIndex<=0))NavigationArea=0;
        else if(Right)NavigationArea=2;
        else if(Party&&(Up||Down))Party->MoveSelection(Down?1:-1);
    }
    else if(SelectedTab==TEXT("Settings"))
    {
        TArray<UButton*> Actions;
        for(const auto* Name:{TEXT("BTN_Resume"),TEXT("BTN_Save"),TEXT("BTN_Load"),TEXT("BTN_GameSettings"),TEXT("BTN_Quit")})
            if(auto* Button=Cast<UButton>(GetWidgetFromName(Name));Button&&Button->IsVisible()&&Button->GetIsEnabled())Actions.Add(Button);
        if(TabsVisible&&(Left||Right||(Up&&SettingsActionIndex==0)))NavigationArea=0;
        else if(Up||Down)SettingsActionIndex=FMath::Clamp(SettingsActionIndex+(Down?1:-1),0,FMath::Max(0,Actions.Num()-1));
        else if(Accept&&!bRepeat&&Actions.IsValidIndex(SettingsActionIndex))Actions[SettingsActionIndex]->OnClicked.Broadcast();
    }
    else if(Inventory)NavigationArea=0;
    else if(Quests)NavigationArea=0;
    else if(CodexContent)NavigationArea=0;
    else if(Left||Right||Up)NavigationArea=0;
    RefreshNavigation();return true;
}

FReply UICGameMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    return HandleMenuKey(Event.GetKey(),Event.IsRepeat())?FReply::Handled():Super::NativeOnPreviewKeyDown(Geometry,Event);
}

FReply UICGameMenuWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if(HandleMenuKey(Event.GetKey(),Event.IsRepeat()))return FReply::Handled();
    if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Equals || UICGameUserSettings::IsGamepadBackKey(Event.GetKey()))
    {
        if (!Event.IsRepeat())
        {
            if (OnBackRequested.IsBound()) OnBackRequested.Broadcast();
            else Resume();
        }
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UICGameMenuWidget::Party() { SelectTab(TEXT("Party")); }
void UICGameMenuWidget::Inventory() { SelectTab(TEXT("Inventory")); }
void UICGameMenuWidget::Quest() { SelectTab(TEXT("Quest")); }
void UICGameMenuWidget::Codex() { SelectTab(TEXT("Codex")); }
void UICGameMenuWidget::Settings() { SelectTab(TEXT("Settings")); }
void UICGameMenuWidget::Resume() { if (OnResumeRequested.IsBound()) OnResumeRequested.Broadcast(); else SetVisibility(ESlateVisibility::Collapsed); }
void UICGameMenuWidget::Save() { OnSaveRequested.Broadcast(); }
void UICGameMenuWidget::Load() { OnLoadRequested.Broadcast(); }
void UICGameMenuWidget::GameSettings() { OnGameSettingsRequested.Broadcast(); }
void UICGameMenuWidget::Quit() { OnQuitRequested.Broadcast(); }
