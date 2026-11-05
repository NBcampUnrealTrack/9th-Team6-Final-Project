#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "UI/Gameplay/ICGameplayLayout.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "UI/Settings/ICSettingsLayout.h"
#include "UI/Life/ICCraftingStationWidget.h"
#include "UI/Life/ICEnhancementWidget.h"
#include "UI/Life/ICLifeWidgets.h"
#include "UI/Life/ICStorageWidget.h"
#include "UI/Map/ICMapWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/InputKeySelector.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"
#include "UI/CharacterCreate/ICNameKeyButton.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Texture2D.h"

namespace ICFrontendLayout
{
FSlateFontInfo Font(int32 Size, bool Bold)
{
    auto* Asset = LoadObject<UFont>(nullptr, TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI.FONT_IC_UI"));
    return Asset ? FSlateFontInfo(Asset, Size, Bold ? TEXT("Bold") : TEXT("Regular"))
                 : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
}
FText Text(const TCHAR* Key)
{
    return FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI.ST_IC_UI"), Key);
}
static UTextBlock* Label(UWidgetTree* Tree, FName Name, const FText& Value, int32 Size = 20)
{
    auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Widget->SetText(Value);
    Widget->SetFont(Font(Size, Size >= 28));
    Widget->SetColorAndOpacity(FSlateColor(FLinearColor(.92f, .91f, .86f)));
    Widget->SetAutoWrapText(true);
    Widget->SetWrapTextAt(600.f);
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Widget;
}
static void Row(UVerticalBox* Box, UWidget* Widget, float Bottom = 14.f)
{
    auto* Slot = Box->AddChildToVerticalBox(Widget);
    Slot->SetPadding(FMargin(0, 0, 0, Bottom));
    Slot->SetHorizontalAlignment(HAlign_Fill);
}
static UButton* Button(UWidgetTree* Tree, FName Name, const TCHAR* Key,bool WithCaption=true,TSubclassOf<UButton> ButtonClass=UButton::StaticClass())
{
    auto* Widget = Tree->ConstructWidget<UButton>(ButtonClass, Name);
    FButtonStyle Style = Widget->GetStyle();
    auto Brush = [](FLinearColor Color) {
        FSlateBrush Result;
        Result.DrawAs = ESlateBrushDrawType::RoundedBox;
        Result.TintColor = Color;
        Result.OutlineSettings.CornerRadii = FVector4(7, 7, 7, 7);
        Result.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Result;
    };
    Style.Normal = Brush(FLinearColor(.075f, .10f, .15f));
    Style.Hovered = Brush(FLinearColor(.20f, .25f, .32f));
    Style.Pressed = Brush(FLinearColor(.34f, .28f, .17f));
    Style.Disabled = Brush(FLinearColor(.055f, .065f, .085f));
    Style.NormalPadding = FMargin(20, 15);
    Style.PressedPadding = FMargin(20, 15);
    Widget->SetStyle(Style);
    if(WithCaption)Widget->AddChild(Label(Tree, FName(*(Name.ToString() + TEXT("_Text"))), Text(Key)));
    return Widget;
}
static UVerticalBox* Card(UWidgetTree* Tree, float Width = 680.f)
{
    auto* Background = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
    Background->SetBrushColor(FLinearColor(.018f, .026f, .045f, 1.f));
    Background->SetPadding(FMargin(24));
    Tree->RootWidget = Background;
    auto* Safe = Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("SafeZone"));
    Background->SetContent(Safe);
    auto* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("FormScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::DownOnly);
    Safe->SetContent(Scale);
    auto* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FormWidth"));
    Size->SetWidthOverride(Width);
    Scale->SetContent(Size);
    auto* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
    Panel->SetBrushColor(FLinearColor(.035f, .048f, .075f));
    Panel->SetPadding(FMargin(40, 32));
    Size->SetContent(Panel);
    auto* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Form"));
    Panel->SetContent(Box);
    return Box;
}
static void Fill(UOverlay* Parent, UWidget* Child)
{
    if (Child->IsA<UCanvasPanel>() || Child->IsA<UOverlay>())
        Child->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    auto* Slot = Parent->AddChildToOverlay(Child);
    Slot->SetHorizontalAlignment(HAlign_Fill);
    Slot->SetVerticalAlignment(VAlign_Fill);
}
static void Portraits(UWidgetTree* Tree,UVerticalBox* Box,const TCHAR* Prefix,float Width,float Height,float Gap=14)
{
    auto* Line=Tree->ConstructWidget<UHorizontalBox>();
    for(int32 I=0;I<4;++I)
    {
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width);Size->SetHeightOverride(Height);
        auto* Frame=Tree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.08f,.10f,.14f));Frame->SetPadding(FMargin(0));Size->SetContent(Frame);
        auto* Overlay=Tree->ConstructWidget<UOverlay>();Frame->SetContent(Overlay);
        auto* Empty=Label(Tree,FName(*FString::Printf(TEXT("%s%d_Empty"),Prefix,I)),Text(TEXT("Save_EmptyPortrait")),20);
        auto* EmptySlot=Overlay->AddChildToOverlay(Empty);EmptySlot->SetHorizontalAlignment(HAlign_Center);EmptySlot->SetVerticalAlignment(VAlign_Center);
        auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*FString::Printf(TEXT("%s%d"),Prefix,I)));
        Image->SetVisibility(ESlateVisibility::Hidden);
        auto* ImageSlot=Overlay->AddChildToOverlay(Image);ImageSlot->SetHorizontalAlignment(HAlign_Fill);ImageSlot->SetVerticalAlignment(VAlign_Fill);
        Line->AddChildToHorizontalBox(Size)->SetPadding(FMargin(0,0,10,0));
    }
    Row(Box,Line,Gap);
}
static UVerticalBox* SavePanel(UWidgetTree* Tree,float Width,float Height,bool Modal)
{
    auto* Root=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SaveBackground"));
    Root->SetBrushColor(FLinearColor(.018f,.026f,.045f,Modal?.96f:1.f));Root->SetPadding(FMargin(32));
    Tree->RootWidget=Root;
    auto* Safe=Tree->ConstructWidget<USafeZone>();Root->SetContent(Safe);
    auto* Scale=Tree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Scale->SetStretchDirection(EStretchDirection::DownOnly);Safe->SetContent(Scale);
    auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width);Size->SetHeightOverride(Height);Scale->SetContent(Size);
    auto* Card=Tree->ConstructWidget<UBorder>();Card->SetBrushColor(FLinearColor(.035f,.048f,.075f));Card->SetPadding(FMargin(32));Size->SetContent(Card);
    auto* Box=Tree->ConstructWidget<UVerticalBox>();Card->SetContent(Box);return Box;
}
static void SaveActions(UWidgetTree* Tree,UVerticalBox* Box,const TArray<TPair<FName,const TCHAR*>>& Actions,FName Name=NAME_None)
{
    auto* Bar=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),Name);
    for(const auto& Action:Actions)
    {
        auto* Slot=Bar->AddChildToHorizontalBox(Button(Tree,Action.Key,Action.Value));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Slot->SetPadding(FMargin(0,0,12,0));
    }
    Row(Box,Bar,0);
}
static void BuildSave(UWidgetTree* Tree,FName Kind)
{
    if(Kind==TEXT("SaveSlot"))
    {
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("SlotSize"));Size->SetHeightOverride(90);Tree->RootWidget=Size;
        auto* Pick=Button(Tree,TEXT("IC_SaveSlot_Button"),TEXT("Save_EmptySlot"),false,UICSaveRowButton::StaticClass());Size->SetContent(Pick);
        auto* Line=Tree->ConstructWidget<UHorizontalBox>();
        auto* Content=Cast<UButtonSlot>(Pick->SetContent(Line));Content->SetHorizontalAlignment(HAlign_Fill);Content->SetVerticalAlignment(VAlign_Center);Content->SetPadding(FMargin(20,8));
        auto* Summary=Label(Tree,TEXT("IC_SaveSlot_Label"),Text(TEXT("Save_EmptySlot")),18);
        Summary->SetAutoWrapText(false);Summary->SetWrapTextAt(0);Summary->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
        auto* TextSlot=Line->AddChildToHorizontalBox(Summary);TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TextSlot->SetVerticalAlignment(VAlign_Center);TextSlot->SetPadding(FMargin(0,0,16,0));
        auto* Faces=Tree->ConstructWidget<UVerticalBox>();Line->AddChildToHorizontalBox(Faces)->SetVerticalAlignment(VAlign_Center);
        Portraits(Tree,Faces,TEXT("IC_SaveSlot_Party"),50,64,0);
        return;
    }
    if(Kind==TEXT("SaveConfirm"))
    {
        auto* Box=SavePanel(Tree,1000,600,true);
        Row(Box,Label(Tree,TEXT("SaveConfirmTitle"),Text(TEXT("Save_Confirm")),36),20);
        auto* Scroll=Tree->ConstructWidget<UScrollBox>();
        auto* Summary=Label(Tree,TEXT("IC_LoadConfirm_Summary"),FText::GetEmpty(),22);Summary->SetWrapTextAt(890);Scroll->AddChild(Summary);
        auto* SummarySlot=Box->AddChildToVerticalBox(Scroll);SummarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));SummarySlot->SetPadding(FMargin(0,0,20,0));
        SaveActions(Tree,Box,{{TEXT("IC_LoadConfirm_Confirm"),TEXT("Save_Confirm")},{TEXT("IC_LoadConfirm_Cancel"),TEXT("Save_Cancel")}});
        return;
    }
    auto* Box=SavePanel(Tree,1560,760,false);
    Row(Box,Label(Tree,TEXT("IC_SaveLoad_Title"),Text(TEXT("Save")),36),14);
    auto* Caption=Label(Tree,TEXT("SaveListCaption"),NSLOCTEXT("ICSaveUI","Columns","저장 위치 · 플레이어 · 지역 · 플레이 시간 · 저장 시각"),16);
    Caption->SetColorAndOpacity(FLinearColor(.48f,.69f,.77f));Row(Box,Caption,20);
    auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("IC_SaveLoad_Scroll"));
    Scroll->SetScrollbarThickness(FVector2D(3,3));
    auto* MainSlot=Box->AddChildToVerticalBox(Scroll);MainSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));MainSlot->SetPadding(FMargin(0,0,0,14));
    Scroll->AddChild(Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("IC_SaveLoad_SlotList")));
    auto* Notice=Label(Tree,TEXT("IC_SaveLoad_Notice"),FText::GetEmpty(),16);Notice->SetWrapTextAt(1450);Row(Box,Notice,12);
    auto* Help=Label(Tree,TEXT("SaveHelp"),NSLOCTEXT("ICSaveUI","VerticalControls","W/S · ↑/↓  선택     F/G  확인     Delete  삭제     ESC  뒤로"),16);
    Help->SetColorAndOpacity(FLinearColor(.48f,.76f,.84f));Row(Box,Help,0);
    SaveActions(Tree,Box,{{TEXT("IC_SaveLoad_Save"),TEXT("Save")},{TEXT("IC_SaveLoad_Load"),TEXT("Load")},
        {TEXT("IC_SaveLoad_Delete"),TEXT("Delete")},{TEXT("IC_SaveLoad_Close"),TEXT("Close")}},TEXT("SaveActionBar"));

}
void Build(UWidgetTree* Tree, FName Kind)
{
    if(Kind==TEXT("Crosshair"))
    {
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Canvas;
        for(const FVector2D Axis:{FVector2D(1,0),FVector2D(-1,0),FVector2D(0,1),FVector2D(0,-1)})
        for(int32 Layer=0;Layer<2;++Layer)
        {
            auto* Bar=Tree->ConstructWidget<UBorder>();Bar->SetBrushColor(Layer?FLinearColor(1,1,1,.9f):FLinearColor(0,0,0,.85f));
            auto* S=Canvas->AddChildToCanvas(Bar);S->SetAnchors(FAnchors(.5,.5));S->SetAlignment({.5,.5});S->SetPosition(Axis*6);
            S->SetSize(Axis.X?FVector2D(6,Layer?1:3):FVector2D(Layer?1:3,6));
        }
        return;
    }
    if(Kind==TEXT("MapPlayerMarker"))
    {
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(32);Size->SetHeightOverride(32);Tree->RootWidget=Size;
        auto* Text=Label(Tree,TEXT("PlayerArrow"),FText::FromString(TEXT("▲")),26);Text->SetColorAndOpacity(FLinearColor(.4f,1,.4f));Text->SetJustification(ETextJustify::Center);Text->SetAutoWrapText(false);Size->SetContent(Text);return;
    }
    if(Kind==TEXT("MapPortalMarker"))
    {
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(220);Size->SetHeightOverride(30);Tree->RootWidget=Size;
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>();Size->SetContent(Canvas);
        const TCHAR* Names[]={TEXT("PortalUnlocked"),TEXT("PortalLocked"),TEXT("PortalSelected")};
        const FLinearColor Colors[]={FLinearColor(.25f,.85f,1),FLinearColor(.4f,.4f,.45f),FLinearColor(1,.8f,.25f)};
        for(int32 I=0;I<3;++I){auto* Marker=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),Names[I]);Marker->SetBrushColor(Colors[I]);auto* S=Canvas->AddChildToCanvas(Marker);S->SetAlignment({.5,.5});S->SetSize({22,22});}
        auto* Text=Label(Tree,TEXT("TXT_Label"),FText::GetEmpty(),20);auto* S=Canvas->AddChildToCanvas(Text);S->SetPosition({17,-14});S->SetSize({200,30});return;
    }
    if(Kind==TEXT("LoadingMovie"))
    {
        auto* Black=Tree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Tree->RootWidget=Black;
        auto* Body=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MovieBody"));Black->SetContent(Body);Black->SetHorizontalAlignment(HAlign_Center);Black->SetVerticalAlignment(VAlign_Center);
        Row(Body,Label(Tree,TEXT("TXT_Title"),Text(TEXT("Title")),36),28);
        Row(Body,Tree->ConstructWidget<UThrobber>(),18);Row(Body,Label(Tree,TEXT("TXT_Loading"),Text(TEXT("Loading")),20),0);return;
    }
    if(Kind==TEXT("ActionRow"))
    {
        auto* Pick=Button(Tree,TEXT("BTN_Select"),TEXT("Save_Confirm"),false);Tree->RootWidget=Pick;
        auto* Text=Label(Tree,TEXT("TXT_Label"),FText::GetEmpty(),18);Text->SetWrapTextAt(0);
        auto* S=Cast<UButtonSlot>(Pick->SetContent(Text));S->SetPadding(FMargin(14,9));S->SetHorizontalAlignment(HAlign_Fill);return;
    }
    if(Kind==TEXT("TextLine")){Tree->RootWidget=Label(Tree,TEXT("TXT_Label"),FText::GetEmpty(),18);return;}
    if(Kind==TEXT("KeyBindingRow"))
    {
        auto* Frame=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Frame"));Frame->SetPadding(FMargin(22,16));Tree->RootWidget=Frame;
        auto* Line=Tree->ConstructWidget<UHorizontalBox>();Frame->SetContent(Line);
        Line->AddChildToHorizontalBox(Label(Tree,TEXT("TXT_Label"),FText::GetEmpty(),18))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Width=Tree->ConstructWidget<USizeBox>();Width->SetWidthOverride(280);Width->SetHeightOverride(38);Line->AddChildToHorizontalBox(Width);
        auto* Selector=Tree->ConstructWidget<UInputKeySelector>(UInputKeySelector::StaticClass(),TEXT("SEL_Key"));Selector->SetAllowModifierKeys(false);Selector->SetAllowGamepadKeys(false);Selector->SetEscapeKeys({EKeys::Escape});
        auto Style=Selector->GetTextStyle();Style.SetFont(Font(17));Selector->SetTextStyle(Style);Selector->SetKeySelectionText(NSLOCTEXT("ICSettings","PressKey","새 키를 누르세요…"));Width->SetContent(Selector);return;
    }
    if(Kind==TEXT("StatusEffect"))
    {
        auto* Frame=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Frame"));Frame->SetPadding(FMargin(2));Tree->RootWidget=Frame;
        auto* Layer=Tree->ConstructWidget<UOverlay>();Frame->SetContent(Layer);
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(24);Size->SetHeightOverride(24);Layer->AddChildToOverlay(Size);
        auto* Icon=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Icon"));Size->SetContent(Icon);
        auto* Count=Label(Tree,TEXT("TXT_Count"),FText::GetEmpty(),10);Count->SetShadowColorAndOpacity(FLinearColor::Black);Count->SetShadowOffset({1,1});auto* S=Layer->AddChildToOverlay(Count);S->SetHorizontalAlignment(HAlign_Right);S->SetVerticalAlignment(VAlign_Bottom);return;
    }
    if(Kind==TEXT("QuestReward"))
    {
        auto* Frame=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Frame"));Frame->SetPadding(FMargin(12,8));Tree->RootWidget=Frame;
        auto* Line=Tree->ConstructWidget<UHorizontalBox>();Frame->SetContent(Line);
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(36);Size->SetHeightOverride(36);auto* S=Line->AddChildToHorizontalBox(Size);S->SetPadding(FMargin(0,0,12,0));S->SetVerticalAlignment(VAlign_Center);
        auto* Layer=Tree->ConstructWidget<UOverlay>();Size->SetContent(Layer);Fill(Layer,Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Icon")));
        auto* Badge=Label(Tree,TEXT("TXT_Badge"),FText::GetEmpty(),16);auto* BS=Layer->AddChildToOverlay(Badge);BS->SetHorizontalAlignment(HAlign_Center);BS->SetVerticalAlignment(VAlign_Center);
        auto* Text=Label(Tree,TEXT("TXT_Label"),FText::GetEmpty(),17);auto* TS=Line->AddChildToHorizontalBox(Text);TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TS->SetVerticalAlignment(VAlign_Center);return;
    }
    if(Kind==TEXT("Prologue"))
    {
        auto* Black=Tree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Black->SetPadding(FMargin(0));Tree->RootWidget=Black;
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>();Black->SetContent(Canvas);
        auto Place=[&](UWidget* W,FVector2D Anchor,FVector2D Size)
        {auto* S=Canvas->AddChildToCanvas(W);S->SetAnchors(FAnchors(Anchor.X,Anchor.Y));S->SetAlignment(FVector2D(.5,.5));if(Size.IsZero())S->SetAutoSize(true);else S->SetSize(Size);};
        auto* Text=Label(Tree,TEXT("LineText"),FText::GetEmpty(),27);Text->SetJustification(ETextJustify::Center);Text->SetAutoWrapText(false);Place(Text,{.5,.5},{});
        auto* Prompt=Label(Tree,TEXT("Prompt"),FText::GetEmpty(),16);Prompt->SetAutoWrapText(false);Prompt->SetWrapTextAt(0);Place(Prompt,{.5,.72},{});
        auto* Skip=Label(Tree,TEXT("SkipPrompt"),FText::GetEmpty(),16);Skip->SetAutoWrapText(false);Skip->SetWrapTextAt(0);Place(Skip,{.5,.91},{});
        auto* Arc=Tree->ConstructWidget<UICRadialCooldownBar>(UICRadialCooldownBar::StaticClass(),TEXT("SkipGauge"));Arc->StartAngleDegrees=-90;Arc->SweepAngleDegrees=180;Arc->Thickness=5;Arc->FillColor=FLinearColor(.44,.83,.9);Arc->Progress=0;Place(Arc,{.5,.86},{76,76});
        return;
    }
    if(Kind==TEXT("CraftingStation")){UICCraftingStationWidget::BuildLayout(Tree);return;}
    if(Kind==TEXT("Enhancement")){UICEnhancementWidget::BuildLayout(Tree);return;}
    if(Kind==TEXT("Shop")){UICShopWidget::BuildLayout(Tree);return;}
    if(Kind==TEXT("Storage")){UICStorageWidget::BuildLayout(Tree);return;}
    if(Kind==TEXT("Map")||Kind==TEXT("Minimap")){UICMapWidget::BuildLayout(Tree,Kind==TEXT("Minimap"));return;}
    if(Kind==TEXT("StorageTransferPopup")){UICStorageTransferPopup::BuildLayout(Tree);return;}
    if(Kind==TEXT("SeedPicker")){UICSeedPickerWidget::BuildLayout(Tree);return;}
    if(Kind==TEXT("SleepFade")){UICSleepFadeWidget::BuildLayout(Tree);return;}
    if (!Tree || Tree->RootWidget)
        return;
    if (ICGameplayLayout::Build(Tree, Kind))
        return;
    if(Kind==TEXT("SaveLoad")||Kind==TEXT("SaveConfirm")||Kind==TEXT("SaveSlot"))
    {
        BuildSave(Tree,Kind);return;
    }
    if(Kind==TEXT("Settings"))
    {
        ICSettingsLayout::Build(Tree);
        return;
    }
    if(Kind==TEXT("Death"))
    {
        auto* Background=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("DeathBackground"));
        Background->SetBrushColor(FLinearColor::Black);Background->SetPadding(FMargin(0));Tree->RootWidget=Background;
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("DeathCanvas"));Background->SetContent(Canvas);
        auto* Width=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("DeathContent"));Width->SetWidthOverride(440);
        auto* Placement=Canvas->AddChildToCanvas(Width);Placement->SetAnchors(FAnchors(.5f,.5f));Placement->SetAlignment({.5f,.5f});Placement->SetAutoSize(true);
        auto* Content=Tree->ConstructWidget<UVerticalBox>();Width->SetContent(Content);
        auto* Title=Label(Tree,TEXT("TXT_Death"),NSLOCTEXT("ICDeath","Title","사망하였습니다."),46);
        Title->SetJustification(ETextJustify::Center);Title->SetColorAndOpacity(FLinearColor::White);Row(Content,Title,48);
        const TCHAR* Keys[]={TEXT("Load"),TEXT("ToTitle"),TEXT("Quit")};
        const TCHAR* Names[]={TEXT("BTN_Load"),TEXT("BTN_MainMenu"),TEXT("BTN_Quit")};
        const FText Labels[]={NSLOCTEXT("ICDeath","Load","불러오기"),NSLOCTEXT("ICDeath","MainMenu","메인메뉴"),NSLOCTEXT("ICDeath","Quit","그만하기")};
        for(int32 Index=0;Index<3;++Index)
        {
            auto* Action=Button(Tree,Names[Index],Keys[Index]);
            auto* Caption=CastChecked<UTextBlock>(Action->GetContent());Caption->SetText(Labels[Index]);Caption->SetJustification(ETextJustify::Center);Caption->SetFont(Font(24));
            auto* Height=Tree->ConstructWidget<USizeBox>();Height->SetHeightOverride(68);Height->SetContent(Action);Row(Content,Height,Index==2?0:16);
        }
        return;
    }
    if (Kind == TEXT("Root"))
    {
        auto* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
        Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        auto* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("IC_DesignScale"));
        Scale->SetStretch(EStretch::ScaleToFit);
        Scale->SetStretchDirection(EStretchDirection::Both);
        auto* Design = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IC_DesignSize"));
        Design->SetWidthOverride(ICDisplayPolicy::DesignWidth);Design->SetHeightOverride(ICDisplayPolicy::DesignHeight);
        Design->SetContent(Root);
        auto* DesignSlot=Cast<UScaleBoxSlot>(Scale->SetContent(Design));
        DesignSlot->SetHorizontalAlignment(HAlign_Center);DesignSlot->SetVerticalAlignment(VAlign_Center);
        Tree->RootWidget=Scale;
        Fill(Root, Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Layer_HUD")));
        Fill(Root, Tree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(),
                                                                        TEXT("Layer_Screen")));
        Fill(Root, Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Layer_Legacy")));
        Fill(Root, Tree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(),
                                                                        TEXT("Layer_Modal")));
        Fill(Root, Tree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(),
                                                                        TEXT("Layer_Loading")));
        return;
    }
    auto* Box = Card(Tree, Kind == TEXT("Questions") ? 1080.f : 680.f);
    if (Kind == TEXT("Mainmenu"))
    {
        auto* Emblem=Tree->ConstructWidget<UImage>();
        Emblem->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/IsekaiClass/UI/SampleUI/MorpheusGUI/textures/Icons/512/t_Compass_512.t_Compass_512")));
        Emblem->SetColorAndOpacity(FLinearColor(.9f,.72f,.4f));
        auto* EmblemSize=Tree->ConstructWidget<USizeBox>();EmblemSize->SetWidthOverride(52);EmblemSize->SetHeightOverride(52);EmblemSize->SetContent(Emblem);
        Box->AddChildToVerticalBox(EmblemSize)->SetHorizontalAlignment(HAlign_Center);
        auto* Title = Label(Tree, TEXT("TXT_Title"), Text(TEXT("Title")), 46);
        Title->SetJustification(ETextJustify::Center);
        Row(Box, Title, 24);
        auto* Subtitle = Label(Tree, TEXT("TXT_Subtitle"), Text(TEXT("Subtitle")), 16);
        Subtitle->SetJustification(ETextJustify::Center);
        Row(Box, Subtitle, 32);
        Row(Box, Button(Tree, TEXT("BTN_Continue"), TEXT("Continue")));
        Row(Box, Button(Tree, TEXT("BTN_Start"), TEXT("Start")));
        Row(Box, Button(Tree, TEXT("BTN_SaveList"), TEXT("SaveList")));
        Row(Box, Button(Tree, TEXT("BTN_Settings"), TEXT("Settings")));
        Row(Box, Button(Tree, TEXT("BTN_Quit"), TEXT("Quit")), 0);
    }
    else if (Kind == TEXT("Questions"))
    {
        auto* Question = Label(Tree, TEXT("TXT_Question"), Text(TEXT("Question1")), 28);
        Question->SetAutoWrapText(false);
        Question->SetWrapTextAt(0.f);
        Row(Box, Question, 24);
        auto* Name = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("EDIT_Name"));
        Name->SetHintText(Text(TEXT("NameHint")));
        auto NameStyle = Name->GetWidgetStyle();
        NameStyle.SetFont(Font(24));
        NameStyle.SetForegroundColor(FSlateColor(FLinearColor(.06f,.08f,.12f)));
        NameStyle.SetFocusedForegroundColor(FSlateColor(FLinearColor(.06f,.08f,.12f)));
        NameStyle.SetReadOnlyForegroundColor(FSlateColor(FLinearColor(.12f,.14f,.18f)));
        Name->SetWidgetStyle(NameStyle);
        Row(Box, Name, 20);
        auto* Keyboard=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("NameKeyboard"));
        Keyboard->SetMinDesiredSlotWidth(94);Keyboard->SetMinDesiredSlotHeight(58);Keyboard->SetSlotPadding(FMargin(3));
        for(int32 I=0;I<40;++I)
        {
            auto* Key=Tree->ConstructWidget<UICNameKeyButton>(UICNameKeyButton::StaticClass(),FName(*FString::Printf(TEXT("NameKey%d"),I)));
            const FString Jamo=TEXT("ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ");
            const FString Caption=I<26?FString::Chr(Jamo[I]):I<36?FString::FromInt(I-26):I==36?TEXT("한/EN"):I==37?TEXT("Shift"):I==38?TEXT("지우기"):TEXT("공백");
            Key->KeyIndex=I;Key->SetContent(Label(Tree,NAME_None,FText::FromString(Caption),19));
            if(auto* KeyLabel=Cast<UTextBlock>(Key->GetContent())){KeyLabel->SetJustification(ETextJustify::Center);KeyLabel->SetAutoWrapText(false);KeyLabel->SetWrapTextAt(0);}
            auto* KeySlot=Keyboard->AddChildToUniformGrid(Key,I/10,I%10);
            KeySlot->SetHorizontalAlignment(HAlign_Fill);KeySlot->SetVerticalAlignment(VAlign_Fill);
        }
        Row(Box,Keyboard,14);
        auto* Help=Label(Tree,TEXT("TXT_CreationHelp"),NSLOCTEXT("ICCreation","Controls","방향키 / WASD / 패드 이동    G · F / 확인 버튼 선택    LB 한·영    RB Shift"),15);
        Help->SetJustification(ETextJustify::Center);Help->SetAutoWrapText(false);Help->SetWrapTextAt(0);Row(Box,Help,14);
        for (int32 I = 0; I < 4; ++I)
        {
            auto* Choice = Button(Tree, FName(*FString::Printf(TEXT("BTN_Choice%d"), I)), TEXT("Next"));
            auto* Caption = CastChecked<UTextBlock>(Choice->GetContent());
            Caption->SetAutoWrapText(false);
            Caption->SetWrapTextAt(0.f);
            Row(Box, Choice, 10);
        }
        Row(Box, Label(Tree, TEXT("TXT_Error"), FText::GetEmpty(), 16), 14);
        Row(Box, Button(Tree, TEXT("BTN_Next"), TEXT("SetName")), 0);
    }
    else if (Kind == TEXT("Loading"))
    {
        Row(Box, Label(Tree, TEXT("TXT_Title"), Text(TEXT("Title")), 36), 28);
        Row(Box, Tree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), TEXT("LoadingSpinner")), 20);
        Row(Box, Label(Tree, TEXT("TXT_Status"), Text(TEXT("Loading")), 22), 20);
        auto* Retry = Button(Tree, TEXT("BTN_Retry"), TEXT("Retry"));
        Retry->SetVisibility(ESlateVisibility::Collapsed);
        Row(Box, Retry);
        auto* Back = Button(Tree, TEXT("BTN_Title"), TEXT("ToTitle"));
        Back->SetVisibility(ESlateVisibility::Collapsed);
        Row(Box, Back, 0);
    }
}
}
