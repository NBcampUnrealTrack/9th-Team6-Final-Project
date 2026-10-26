#include "UI/Quest/ICQuestMenuWidget.h"
#include "UI/Quest/ICQuestDisplay.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Quest/ICQuestSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/GameInstance.h"
#include "Internationalization/StringTable.h"

namespace
{
const FLinearColor QuestGlowColor(1,.77f,.32f), QuestButtonColor(.18f,.23f,.32f);
UTextBlock* QuestMenuLabel(UWidgetTree* T,FName Name,FText Value,int32 Font=17,bool Wrap=false)
{
    auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);W->SetText(Value);W->SetAutoWrapText(Wrap);
    W->SetFont(ICFrontendLayout::Font(Font,Font>=24));W->SetColorAndOpacity(FLinearColor(.92f,.94f,.98f));
    W->SetVisibility(ESlateVisibility::HitTestInvisible);return W;
}
void QuestMenuRow(UVerticalBox* P,UWidget* W,float Gap=10){P->AddChildToVerticalBox(W)->SetPadding(FMargin(0,0,0,Gap));}
UButton* QuestMenuButton(UWidgetTree* T,FName Name,FText Text,bool Caption=true)
{
    auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),Name);auto Style=B->GetStyle();
    for(auto* Brush:{&Style.Normal,&Style.Hovered,&Style.Pressed})
    {Brush->DrawAs=ESlateBrushDrawType::Box;Brush->Margin=FMargin(0);Brush->TintColor=FLinearColor::White;}
    B->SetStyle(Style);B->SetBackgroundColor(QuestButtonColor);
    if(Caption)
    {
        auto* TextWidget=QuestMenuLabel(T,FName(*(Name.ToString()+TEXT("_Text"))),Text,18);TextWidget->SetJustification(ETextJustify::Center);
        auto* S=CastChecked<UButtonSlot>(B->SetContent(TextWidget));S->SetPadding(FMargin(12,10));
    }
    return B;
}
FText QuestText(const FString& Key,FText Fallback=FText::GetEmpty())
{
    auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Quest.ST_IC_Quest"));
    const auto Text=UICTextLibrary::ResolveText(Strings,Key);return Text.bSuccess?Text.Text:Fallback;
}
FText DefinitionName(const FString& Key,FName ID)
{
    auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
    const auto Text=UICTextLibrary::ResolveText(Strings,Key);return Text.bSuccess?Text.Text:FText::FromName(ID);
}
void QuestMenuSetText(UUserWidget* W,const TCHAR* Name,FText Text)
{if(auto* T=Cast<UTextBlock>(W->GetWidgetFromName(Name)))T->SetText(Text);}
void SetMessage(UICQuestDetailWidget* W,FText Text)
{QuestMenuSetText(W,TEXT("TXT_Message"),Text);W->GetWidgetFromName(TEXT("TXT_Message"))->SetVisibility(Text.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}
UClass* QuestMenuWidgetClass(const TCHAR* Asset,UClass* Fallback)
{
    const auto Path=FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"),Asset,Asset);
    auto* C=LoadClass<UUserWidget>(nullptr,*Path);return C?C:Fallback;
}
FText RewardText(const FICQuestRow& Quest,UICDefinitionSubsystem* D)
{
    FICRewardRow R;TArray<FText> Lines;
    if(D&&!Quest.RewardID.IsNone()&&D->GetReward(Quest.RewardID,R).bSuccess)
    {
        if(R.Gold>0)Lines.Add(FText::Format(NSLOCTEXT("ICQuestMenu","GoldReward","{0} 골드"),FText::AsNumber(R.Gold)));
        for(const auto& Item:R.Items)
        {
            FICItemRow Definition;const bool Found=D->GetItem(Item.ItemID,Definition).bSuccess;
            const auto Name=Found&&!Definition.DisplayName.IsEmpty()?Definition.DisplayName:DefinitionName(Found?Definition.NameKey:FString(),Item.ItemID);
            Lines.Add(FText::Format(NSLOCTEXT("ICQuestMenu","ItemReward","{0} × {1}"),Name,FText::AsNumber(Item.Count)));
        }
        FICDefinitionResult Result;
        for(const auto& Affinity:R.Affinity)
        {
            const auto* NPC=D->Find<FICNPCRow>(Affinity.NPCID,Result);
            Lines.Add(FText::Format(NSLOCTEXT("ICQuestMenu","AffinityReward","{0} 호감도 +{1}"),DefinitionName(NPC?NPC->NameKey:FString(),Affinity.NPCID),FText::AsNumber(Affinity.Amount)));
        }
        for(FName ID:R.UnlockRecipeIDs)
        {
            FICRecipeRow Recipe;D->GetRecipe(ID,Recipe);
            Lines.Add(FText::Format(NSLOCTEXT("ICQuestMenu","RecipeReward","제작법 해금: {0}"),DefinitionName(Recipe.NameKey,ID)));
        }
    }
    return Lines.IsEmpty()?NSLOCTEXT("ICQuestMenu","NoRewards","없음"):FText::Join(FText::FromString(TEXT("\n")),Lines);
}
}

bool UICQuestMenuEntryWidget::BuildLayout(UWidgetTree* T)
{
    auto* Glow=T->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SelectionGlow"));Glow->SetPadding(FMargin(3));T->RootWidget=Glow;
    auto* B=QuestMenuButton(T,TEXT("BTN_Quest"),FText::GetEmpty(),false);Glow->SetContent(B);
    auto* Column=T->ConstructWidget<UVerticalBox>();auto* BS=CastChecked<UButtonSlot>(B->SetContent(Column));BS->SetPadding(FMargin(18,12));BS->SetHorizontalAlignment(HAlign_Fill);
    auto* Title=QuestMenuLabel(T,TEXT("TXT_Title"),FText::GetEmpty(),21);Title->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);QuestMenuRow(Column,Title,6);
    auto* Summary=QuestMenuLabel(T,TEXT("TXT_Summary"),FText::GetEmpty(),16);Summary->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);QuestMenuRow(Column,Summary,0);
    return true;
}
void UICQuestMenuEntryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Quest")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Pick);
}
void UICQuestMenuEntryWidget::Present(FName ID,FText Title,FText Summary,bool Selected)
{
    QuestID=ID;bSelected=Selected;QuestMenuSetText(this,TEXT("TXT_Title"),Title);QuestMenuSetText(this,TEXT("TXT_Summary"),Summary);
    CastChecked<UBorder>(GetWidgetFromName(TEXT("SelectionGlow")))->SetBrushColor(Selected?QuestGlowColor:FLinearColor(.08f,.12f,.19f));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Quest")))->SetBackgroundColor(Selected?FLinearColor(.26f,.29f,.34f):FLinearColor(.10f,.15f,.22f));
}
void UICQuestMenuEntryWidget::NativeTick(const FGeometry& Geometry,float Delta)
{
    Super::NativeTick(Geometry,Delta);if(!bSelected)return;Pulse+=Delta;
    CastChecked<UBorder>(GetWidgetFromName(TEXT("SelectionGlow")))->SetBrushColor(QuestGlowColor*(.8f+.2f*FMath::Sin(Pulse*3.f)));
}
void UICQuestMenuEntryWidget::Pick(){OnPicked.Broadcast(QuestID);}

bool UICQuestDetailWidget::BuildLayout(UWidgetTree* T)
{
    auto* Column=T->ConstructWidget<UVerticalBox>();T->RootWidget=Column;
    QuestMenuRow(Column,QuestMenuLabel(T,TEXT("TXT_Name"),FText::GetEmpty(),30,true),6);
    auto* Status=QuestMenuLabel(T,TEXT("TXT_Status"),FText::GetEmpty(),16);Status->SetColorAndOpacity(QuestGlowColor);QuestMenuRow(Column,Status,12);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("DetailScroll"));
    Column->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Content=T->ConstructWidget<UVerticalBox>();Scroll->AddChild(Content);
    QuestMenuRow(Content,QuestMenuLabel(T,TEXT("TXT_Description"),FText::GetEmpty(),18,true),12);
    QuestMenuRow(Content,QuestMenuLabel(T,TEXT("ObjectivesHeading"),NSLOCTEXT("ICQuestMenu","Objectives","목표"),20),8);
    QuestMenuRow(Content,QuestMenuLabel(T,TEXT("TXT_Objectives"),FText::GetEmpty(),17,true),12);
    QuestMenuRow(Content,QuestMenuLabel(T,TEXT("RewardsHeading"),NSLOCTEXT("ICQuestMenu","Rewards","보상"),20),8);
    QuestMenuRow(Content,QuestMenuLabel(T,TEXT("TXT_Rewards"),FText::GetEmpty(),17,true),12);
    QuestMenuRow(Column,QuestMenuLabel(T,TEXT("TXT_Message"),FText::GetEmpty(),15,true),8);
    T->FindWidget(TEXT("TXT_Message"))->SetVisibility(ESlateVisibility::Collapsed);
    auto* Warning=QuestMenuLabel(T,TEXT("SwitchNotice"),NSLOCTEXT("ICQuestMenu","SwitchNotice","다른 퀘스트를 시작하면 이전 퀘스트의 진행도가 초기화됩니다."),14,true);
    Warning->SetColorAndOpacity(FLinearColor(.8f,.74f,.61f));QuestMenuRow(Column,Warning,10);
    auto* Actions=T->ConstructWidget<UHorizontalBox>();QuestMenuRow(Column,Actions,10);
    auto* Abandon=Actions->AddChildToHorizontalBox(QuestMenuButton(T,TEXT("BTN_Abandon"),NSLOCTEXT("ICQuestMenu","Abandon","포기")));Abandon->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Abandon->SetPadding(FMargin(0,0,8,0));
    Actions->AddChildToHorizontalBox(QuestMenuButton(T,TEXT("BTN_Start"),NSLOCTEXT("ICQuestMenu","Start","시작하기")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    QuestMenuRow(Column,QuestMenuButton(T,TEXT("BTN_Back"),NSLOCTEXT("ICQuestMenu","Back","뒤로가기")),0);return true;
}
void UICQuestDetailWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Start")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Start);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Abandon")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Abandon);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Back")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);
}
void UICQuestDetailWidget::Present(FName ID)
{
    auto* GI=GetGameInstance();auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;auto* Q=GI?GI->GetSubsystem<UICQuestSubsystem>():nullptr;
    FICQuestRow Definition;if(!D||!State||!D->GetQuest(ID,Definition).bSuccess)return;
    if(QuestID!=ID){QuestID=ID;Scroll(-1000000);SetMessage(this,FText::GetEmpty());}
    const auto World=State->ExportState();const auto Quest=World.Quests.FindRef(ID);
    QuestMenuSetText(this,TEXT("TXT_Name"),QuestText(Definition.NameKey,FText::FromName(ID)));
    QuestMenuSetText(this,TEXT("TXT_Description"),QuestText(Definition.DescriptionKey));
    QuestMenuSetText(this,TEXT("TXT_Status"),Quest.bStarted?NSLOCTEXT("ICQuestMenu","Running","진행 중"):Quest.bAccepted?NSLOCTEXT("ICQuestMenu","Waiting","대기 중"):NSLOCTEXT("ICQuestMenu","Available","수락 가능"));
    TArray<FText> Lines;
    for(const auto& Group:Definition.ObjectiveGroups)
    {
        if(Group.Policy==EICConditionPolicy::Any)Lines.Add(NSLOCTEXT("ICQuestMenu","AnyObjective","다음 중 하나 달성"));
        for(const auto& Objective:Group.Objectives)
        {const auto Text=QuestText(Objective.DescriptionKey,FText::FromName(Objective.ObjectiveID));Lines.Add(FICQuestDisplay::FormatObjective(Text,Objective,Quest,World,Q));}
    }
    QuestMenuSetText(this,TEXT("ObjectivesHeading"),Definition.ObjectiveGroupPolicy==EICConditionPolicy::Any?NSLOCTEXT("ICQuestMenu","AnyGroup","목표 (그룹 중 하나 달성)"):NSLOCTEXT("ICQuestMenu","Objectives","목표"));
    QuestMenuSetText(this,TEXT("TXT_Objectives"),FText::Join(FText::FromString(TEXT("\n")),Lines));QuestMenuSetText(this,TEXT("TXT_Rewards"),RewardText(Definition,D));
    GetWidgetFromName(TEXT("BTN_Start"))->SetIsEnabled(Quest.bAccepted&&!Quest.bStarted&&!Quest.bCompleted);
    GetWidgetFromName(TEXT("BTN_Abandon"))->SetIsEnabled(Quest.bAccepted&&!Quest.bCompleted&&Definition.bAllowAbandon);
}
void UICQuestDetailWidget::HighlightAction(int32 Index)
{
    const TCHAR* Names[]={TEXT("BTN_Abandon"),TEXT("BTN_Start"),TEXT("BTN_Back")};
    for(int32 I=0;I<3;++I)CastChecked<UButton>(GetWidgetFromName(Names[I]))->SetBackgroundColor(I==Index?QuestGlowColor:QuestButtonColor);
}
void UICQuestDetailWidget::Scroll(float Delta)
{auto* W=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("DetailScroll")));W->SetScrollOffset(FMath::Clamp(W->GetScrollOffset()+Delta,0.f,W->GetScrollOffsetOfEnd()));}
void UICQuestDetailWidget::Start(){OnStart.Broadcast();}
void UICQuestDetailWidget::Abandon(){OnAbandon.Broadcast();}
void UICQuestDetailWidget::Back(){OnBack.Broadcast();}

bool UICQuestMenuWidget::BuildLayout(UWidgetTree* T)
{
    auto* Pages=T->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("QuestPages"));T->RootWidget=Pages;
    auto* List=T->ConstructWidget<UVerticalBox>();Pages->AddChild(List);
    QuestMenuRow(List,QuestMenuLabel(T,TEXT("ListHeading"),NSLOCTEXT("ICQuestMenu","Journal","퀘스트 목록"),26),14);
    QuestMenuRow(List,QuestMenuLabel(T,TEXT("TXT_Empty"),NSLOCTEXT("ICQuestMenu","Empty","수락한 퀘스트가 없습니다."),18),0);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("QuestScroll"));
    List->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Scroll->AddChild(T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("QuestEntries")));
    auto* Detail=T->ConstructWidget<UICQuestDetailWidget>(QuestMenuWidgetClass(TEXT("WBP_IC_QuestDetail"),UICQuestDetailWidget::StaticClass()),TEXT("QuestDetail"));Pages->AddChild(Detail);
    for(int32 I=0;I<Pages->GetNumWidgets();++I){auto* S=CastChecked<UWidgetSwitcherSlot>(Pages->GetWidgetAtIndex(I)->Slot);S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);}
    return true;
}
void UICQuestMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);SetIsFocusable(true);
    auto* Detail=CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail")));
    Detail->OnStart.AddUniqueDynamic(this,&ThisClass::StartAction);Detail->OnAbandon.AddUniqueDynamic(this,&ThisClass::AbandonAction);Detail->OnBack.AddUniqueDynamic(this,&ThisClass::BackToList);
    RefreshQuests();
}
void UICQuestMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState){BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RefreshQuests);BoundState->OnInventoryChanged.AddUniqueDynamic(this,&ThisClass::RefreshQuests);BoundState->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);}
    RefreshQuests();
}
void UICQuestMenuWidget::NativeDestruct()
{
    if(BoundState){BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::RefreshQuests);BoundState->OnInventoryChanged.RemoveDynamic(this,&ThisClass::RefreshQuests);BoundState->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);BoundState=nullptr;}
    Super::NativeDestruct();
}
void UICQuestMenuWidget::CharacterChanged(FName){RefreshQuests();}
void UICQuestMenuWidget::Activate(){bShowingDetails=false;bReadingDetails=false;RefreshQuests();}
void UICQuestMenuWidget::RefreshQuests()
{
    if(bRefreshing)return;TGuardValue<bool> Guard(bRefreshing,true);
    auto* GI=GetGameInstance();auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    const auto World=State?State->ExportState():FICWorldState();QuestIDs.Reset();
    for(const auto& Pair:World.Quests)if(Pair.Value.bAccepted&&!Pair.Value.bCompleted&&!Pair.Value.bAbandoned){FICQuestRow Definition;if(D&&D->GetQuest(Pair.Key,Definition).bSuccess)QuestIDs.Add(Pair.Key);}
    QuestIDs.Sort(FNameLexicalLess());const int32 Previous=QuestIDs.IndexOfByKey(SelectedQuest);
    if(Previous==INDEX_NONE)bShowingDetails=false;
    SelectedIndex=Previous!=INDEX_NONE?Previous:FMath::Clamp(SelectedIndex,0,FMath::Max(0,QuestIDs.Num()-1));SelectedQuest=QuestIDs.IsValidIndex(SelectedIndex)?QuestIDs[SelectedIndex]:NAME_None;
    auto* Entries=CastChecked<UVerticalBox>(GetWidgetFromName(TEXT("QuestEntries")));
    if(Entries->GetChildrenCount()!=QuestIDs.Num())
    {
        Entries->ClearChildren();auto* C=QuestMenuWidgetClass(TEXT("WBP_IC_QuestMenuEntry"),UICQuestMenuEntryWidget::StaticClass());
        for(int32 I=0;I<QuestIDs.Num();++I){auto* Entry=WidgetTree->ConstructWidget<UICQuestMenuEntryWidget>(C);Entry->OnPicked.AddUniqueDynamic(this,&ThisClass::QuestPicked);QuestMenuRow(Entries,Entry,12);}
    }
    for(int32 I=0;I<QuestIDs.Num();++I)
    {
        FICQuestRow Definition;D->GetQuest(QuestIDs[I],Definition);FString Summary;int32 Count=0;
        for(const auto& Group:Definition.ObjectiveGroups)for(const auto& Objective:Group.Objectives)
        {if(Count++==0)Summary=QuestText(Objective.DescriptionKey,FText::FromName(Objective.ObjectiveID)).ToString();}
        Summary.ReplaceInline(TEXT("\r"),TEXT(" "));Summary.ReplaceInline(TEXT("\n"),TEXT(" "));
        if(Count>1)Summary+=FString::Printf(TEXT("  외 %d개"),Count-1);
        auto Title=QuestText(Definition.NameKey,FText::FromName(Definition.ID));
        if(World.Quests.FindRef(QuestIDs[I]).bStarted)Title=FText::Format(NSLOCTEXT("ICQuestMenu","RunningTitle","{0} (진행중)"),Title);
        CastChecked<UICQuestMenuEntryWidget>(Entries->GetChildAt(I))->Present(QuestIDs[I],Title,FText::FromString(Summary),I==SelectedIndex);
    }
    GetWidgetFromName(TEXT("TXT_Empty"))->SetVisibility(QuestIDs.IsEmpty()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    CastChecked<UWidgetSwitcher>(GetWidgetFromName(TEXT("QuestPages")))->SetActiveWidgetIndex(bShowingDetails?1:0);
    if(bShowingDetails)CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail")))->Present(SelectedQuest);
    RefreshSelection();
}
void UICQuestMenuWidget::RefreshSelection()
{
    CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail")))->HighlightAction(bReadingDetails?INDEX_NONE:ActionIndex);
}
bool UICQuestMenuWidget::SelectQuest(int32 Index)
{
    if(!QuestIDs.IsValidIndex(Index))return false;SelectedIndex=Index;SelectedQuest=QuestIDs[Index];RefreshQuests();
    auto* Entries=CastChecked<UVerticalBox>(GetWidgetFromName(TEXT("QuestEntries")));
    CastChecked<UScrollBox>(GetWidgetFromName(TEXT("QuestScroll")))->ScrollWidgetIntoView(Entries->GetChildAt(Index),true,EDescendantScrollDestination::IntoView);return true;
}
bool UICQuestMenuWidget::OpenQuest(FName ID)
{
    const int32 Index=QuestIDs.IndexOfByKey(ID);if(Index==INDEX_NONE)return false;
    SelectedQuest=ID;SelectedIndex=Index;bShowingDetails=true;bReadingDetails=false;ActionIndex=1;RefreshQuests();return true;
}
void UICQuestMenuWidget::QuestPicked(FName ID){OpenQuest(ID);}
void UICQuestMenuWidget::BackToList(){bShowingDetails=false;bReadingDetails=false;RefreshQuests();if(GetOwningPlayer())SetUserFocus(GetOwningPlayer());}
bool UICQuestMenuWidget::StartSelectedQuest()
{
    auto* Q=GetGameInstance()?GetGameInstance()->GetSubsystem<UICQuestSubsystem>():nullptr;
    const bool Done=Q&&Q->StartQuest(SelectedQuest);RefreshQuests();
    if(bShowingDetails)SetMessage(CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail"))),Done?FText::GetEmpty():NSLOCTEXT("ICQuestMenu","StartFailed","이 퀘스트를 시작할 수 없습니다."));return Done;
}
bool UICQuestMenuWidget::AbandonSelectedQuest()
{
    auto* Q=GetGameInstance()?GetGameInstance()->GetSubsystem<UICQuestSubsystem>():nullptr;const bool Done=Q&&Q->AbandonQuest(SelectedQuest);
    if(Done)BackToList();else SetMessage(CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail"))),NSLOCTEXT("ICQuestMenu","AbandonFailed","이 퀘스트는 포기할 수 없습니다."));return Done;
}
void UICQuestMenuWidget::StartAction(){StartSelectedQuest();}
void UICQuestMenuWidget::AbandonAction(){AbandonSelectedQuest();}
FText UICQuestMenuWidget::NavigationHint() const
{
    if(!bShowingDetails)return NSLOCTEXT("ICQuestMenu","ListHint","W / S  퀘스트 선택     Enter / D  상세 보기     A  상단 탭     ESC  닫기");
    if(bReadingDetails)return NSLOCTEXT("ICQuestMenu","ReadHint","W / S  설명 스크롤     D / Enter  버튼 이동     A / ESC  목록으로");
    return NSLOCTEXT("ICQuestMenu","DetailHint","A / D  포기 / 시작하기     S  뒤로가기     W  설명 읽기     Enter  선택     ESC  목록으로");
}
bool UICQuestMenuWidget::HandleMenuKey(FKey Key,bool Repeat)
{
    const bool Up=Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up,Down=Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down;
    const bool Left=Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left,Right=Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right;
    const bool Accept=Key==EKeys::Enter||Key==EKeys::SpaceBar||Key==EKeys::Gamepad_FaceButton_Bottom;
    const bool Back=Key==EKeys::Escape||Key==EKeys::Equals||Key==EKeys::Gamepad_FaceButton_Right;
    if(!bShowingDetails)
    {
        if(Left||(Up&&SelectedIndex==0))return false;
        if(Up||Down)SelectQuest(FMath::Clamp(SelectedIndex+(Down?1:-1),0,FMath::Max(0,QuestIDs.Num()-1)));
        else if((Accept||Right)&&!Repeat)OpenQuest(SelectedQuest);else if(!Accept&&!Right)return false;
        return true;
    }
    auto* Detail=CastChecked<UICQuestDetailWidget>(GetWidgetFromName(TEXT("QuestDetail")));
    if(Back){if(!Repeat)BackToList();return true;}
    if(bReadingDetails)
    {if(Up||Down)Detail->Scroll(Down?72:-72);else if(Right||Accept)bReadingDetails=false;else if(Left)BackToList();else return false;}
    else if(Up){if(ActionIndex==2)ActionIndex=1;else bReadingDetails=true;}
    else if(Down)ActionIndex=2;
    else if(Left||Right)ActionIndex=Right?1:0;
    else if(Accept&&!Repeat)
    {
        const TCHAR* Names[]={TEXT("BTN_Abandon"),TEXT("BTN_Start"),TEXT("BTN_Back")};auto* B=CastChecked<UButton>(Detail->GetWidgetFromName(Names[ActionIndex]));if(B->GetIsEnabled())B->OnClicked.Broadcast();
    }
    else if(!Accept)return false;
    RefreshSelection();return true;
}
