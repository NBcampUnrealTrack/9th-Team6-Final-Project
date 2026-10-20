#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"

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
static UButton* Button(UWidgetTree* Tree, FName Name, const TCHAR* Key)
{
    auto* Widget = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
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
    Widget->AddChild(Label(Tree, FName(*(Name.ToString() + TEXT("_Text"))), Text(Key)));
    return Widget;
}
static UVerticalBox* Card(UWidgetTree* Tree, bool Modal = false)
{
    auto* Background = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
    Background->SetBrushColor(FLinearColor(.018f, .026f, .045f, Modal ? .96f : 1.f));
    Background->SetPadding(FMargin(24));
    Tree->RootWidget = Background;
    auto* Safe = Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("SafeZone"));
    Background->SetContent(Safe);
    auto* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("FormScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::DownOnly);
    Safe->SetContent(Scale);
    auto* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FormWidth"));
    Size->SetWidthOverride(680);
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
void Build(UWidgetTree* Tree, FName Kind)
{
    if (!Tree || Tree->RootWidget)
        return;
    if (Kind == TEXT("Root"))
    {
        auto* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
        Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Tree->RootWidget = Root;
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
    if (Kind == TEXT("HUD"))
    {
        auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
        Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Tree->RootWidget = Root;
        auto* Status = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Status"));
        auto* StatusSlot = Root->AddChildToCanvas(Status);
        StatusSlot->SetOffsets(FMargin(32, 28, 330, 220));
        Row(Status, Label(Tree, TEXT("TXT_Player"), Text(TEXT("Title")), 24));
        const TCHAR* Names[] = {TEXT("HP"), TEXT("MP"), TEXT("SP")};
        const FLinearColor Colors[] = {FLinearColor(.7f, .13f, .12f), FLinearColor(.15f, .4f, .8f),
                                       FLinearColor(.65f, .55f, .16f)};
        for (int32 I = 0; I < 3; ++I)
        {
            Row(Status, Label(Tree, FName(*FString::Printf(TEXT("TXT_%s"), Names[I])), FText::FromString(Names[I]), 16),
                3);
            auto* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),
                                                            FName(*FString::Printf(TEXT("BAR_%s"), Names[I])));
            Bar->SetFillColorAndOpacity(Colors[I]);
            Row(Status, Bar, 10);
        }
        auto* HintSlot = Root->AddChildToCanvas(Label(Tree, TEXT("TXT_Hint"), FText::GetEmpty(), 24));
        HintSlot->SetAnchors(FAnchors(.5f, .76f));
        HintSlot->SetAlignment(FVector2D(.5f, .5f));
        HintSlot->SetOffsets(FMargin(0, 0, 700, 70));
        auto* FooterSlot = Root->AddChildToCanvas(Label(Tree, TEXT("TXT_Controls"), Text(TEXT("Controls")), 16));
        FooterSlot->SetAnchors(FAnchors(0, 1, 1, 1));
        FooterSlot->SetOffsets(FMargin(32, -64, 32, 50));
        auto* NoticeSlot = Root->AddChildToCanvas(Label(Tree, TEXT("TXT_Notice"), FText::GetEmpty(), 18));
        NoticeSlot->SetAnchors(FAnchors(.5f, .9f));
        NoticeSlot->SetAlignment(FVector2D(.5f, .5f));
        NoticeSlot->SetOffsets(FMargin(0, 0, 850, 80));
        auto* Menu = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
        Menu->SetBrushColor(FLinearColor(.02f, .03f, .05f, .98f));
        Menu->SetPadding(FMargin(28));
        auto* MenuSlot = Root->AddChildToCanvas(Menu);
        MenuSlot->SetAnchors(FAnchors(.5f, .5f));
        MenuSlot->SetAlignment(FVector2D(.5f, .5f));
        MenuSlot->SetOffsets(FMargin(0, 0, 680, 650));
        auto* MenuBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
        Menu->SetContent(MenuBox);
        Row(MenuBox, Label(Tree, TEXT("TXT_Menu"), Text(TEXT("Paused")), 28));
        auto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MenuScroll"));
        Scroll->AddChild(Label(Tree, TEXT("TXT_MenuBody"), FText::GetEmpty(), 17));
        auto* ScrollSlot = MenuBox->AddChildToVerticalBox(Scroll);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        for (const auto* Key : {TEXT("Resume"), TEXT("Save"), TEXT("Load"), TEXT("Settings"), TEXT("Quit")})
            Row(MenuBox, Button(Tree, FName(*FString::Printf(TEXT("BTN_%s"), Key)), Key), 8);
        Menu->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    auto* Box = Card(Tree, Kind == TEXT("Confirm"));
    if (Kind == TEXT("Mainmenu"))
    {
        auto* Title = Label(Tree, TEXT("TXT_Title"), Text(TEXT("Title")), 46);
        Title->SetJustification(ETextJustify::Center);
        Row(Box, Title, 24);
        auto* Subtitle = Label(Tree, TEXT("TXT_Subtitle"), Text(TEXT("Subtitle")), 16);
        Subtitle->SetJustification(ETextJustify::Center);
        Row(Box, Subtitle, 32);
        Row(Box, Button(Tree, TEXT("BTN_Start"), TEXT("Start")));
        auto* Continue = Button(Tree, TEXT("BTN_Continue"), TEXT("Continue"));
        Continue->SetIsEnabled(false);
        Row(Box, Continue);
        auto* Settings = Button(Tree, TEXT("BTN_Settings"), TEXT("Settings"));
        Settings->SetIsEnabled(false);
        Row(Box, Settings);
        Row(Box, Button(Tree, TEXT("BTN_Quit"), TEXT("Quit")), 0);
    }
    else if (Kind == TEXT("Questions"))
    {
        Row(Box, Label(Tree, TEXT("TXT_Step"), FText::GetEmpty(), 16), 10);
        Row(Box, Label(Tree, TEXT("TXT_Question"), Text(TEXT("Question1")), 28), 24);
        auto* Name = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("EDIT_Name"));
        Name->SetHintText(Text(TEXT("NameHint")));
        auto NameStyle = Name->GetWidgetStyle();
        NameStyle.SetFont(Font(24));
        NameStyle.SetForegroundColor(FSlateColor(FLinearColor(.06f,.08f,.12f)));
        NameStyle.SetFocusedForegroundColor(FSlateColor(FLinearColor(.06f,.08f,.12f)));
        NameStyle.SetReadOnlyForegroundColor(FSlateColor(FLinearColor(.12f,.14f,.18f)));
        Name->SetWidgetStyle(NameStyle);
        Row(Box, Name, 20);
        for (int32 I = 0; I < 4; ++I)
            Row(Box, Button(Tree, FName(*FString::Printf(TEXT("BTN_Choice%d"), I)), TEXT("Next")), 10);
        Row(Box, Label(Tree, TEXT("TXT_Detail"), FText::GetEmpty(), 17), 20);
        Row(Box, Label(Tree, TEXT("TXT_Error"), FText::GetEmpty(), 16), 14);
        Row(Box, Button(Tree, TEXT("BTN_Next"), TEXT("Next")), 10);
        Row(Box, Button(Tree, TEXT("BTN_Back"), TEXT("Back")), 0);
    }
    else if (Kind == TEXT("Confirm"))
    {
        Row(Box, Label(Tree, TEXT("TXT_Title"), Text(TEXT("ConfirmTitle")), 28), 22);
        Row(Box, Label(Tree, TEXT("TXT_Message"), FText::GetEmpty(), 20), 24);
        Row(Box, Button(Tree, TEXT("BTN_Accept"), TEXT("Begin")), 12);
        Row(Box, Button(Tree, TEXT("BTN_Cancel"), TEXT("Back")), 0);
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
