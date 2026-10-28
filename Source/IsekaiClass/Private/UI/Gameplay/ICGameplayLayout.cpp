#include "UI/Gameplay/ICGameplayLayout.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "UI/Quest/ICQuestMenuWidget.h"
#include "UI/Codex/ICCodexMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/GridPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Texture2D.h"

namespace ICGameplayLayout
{
namespace
{
const FLinearColor Ink(.018f, .027f, .044f, .96f);
const FLinearColor Paper(.9f, .92f, .95f);

UTextBlock* Label(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize = 18)
{
    auto* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Result->SetText(Text);
    Result->SetFont(ICFrontendLayout::Font(FontSize, FontSize >= 24));
    Result->SetColorAndOpacity(Paper);
    Result->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Result;
}

UBorder* Frame(UWidgetTree* Tree, FName Name, FMargin Padding = FMargin(16))
{
    auto* Result = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    Result->SetBrushColor(Ink);
    Result->SetPadding(Padding);
    return Result;
}

USizeBox* Size(UWidgetTree* Tree, UWidget* Child, float Width, float Height)
{
    auto* Result = Tree->ConstructWidget<USizeBox>();
    Result->SetWidthOverride(Width);
    Result->SetHeightOverride(Height);
    Result->SetContent(Child);
    return Result;
}

void Row(UVerticalBox* Parent, UWidget* Child, float Bottom = 8)
{
    auto* Slot = Parent->AddChildToVerticalBox(Child);
    Slot->SetPadding(FMargin(0, 0, 0, Bottom));
    Slot->SetHorizontalAlignment(HAlign_Fill);
}

UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Anchor, FVector2D Alignment,
    FVector2D Position, FVector2D Extent)
{
    auto* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
    Slot->SetAlignment(Alignment);
    Slot->SetPosition(Position);
    Slot->SetSize(Extent);
    return Slot;
}

UUserWidget* Child(UWidgetTree* Tree, const TCHAR* Asset, FName Name)
{
    const FString Path = FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"), Asset, Asset);
    UClass* Class = LoadClass<UUserWidget>(nullptr, *Path);
    return Class ? Tree->ConstructWidget<UUserWidget>(Class, Name) : nullptr;
}

UButton* Button(UWidgetTree* Tree, FName Name, const FText& Caption, int32 FontSize = 18)
{
    auto* Result = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    FButtonStyle Style = Result->GetStyle();
    auto Brush = [](FLinearColor Color)
    {
        FSlateBrush Value;
        Value.DrawAs = ESlateBrushDrawType::RoundedBox;
        Value.TintColor = Color;
        Value.OutlineSettings.CornerRadii = FVector4(5, 5, 5, 5);
        Value.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return Value;
    };
    Style.Normal = Brush(FLinearColor(.28f, .34f, .44f));
    Style.Hovered = Brush(FLinearColor(.5f, .58f, .7f));
    Style.Pressed = Brush(FLinearColor(.65f, .51f, .28f));
    Result->SetStyle(Style);
    auto* CaptionWidget = Label(Tree, FName(*(Name.ToString() + TEXT("_Text"))), Caption, FontSize);
    CaptionWidget->SetJustification(ETextJustify::Center);
    auto* Slot = Cast<UButtonSlot>(Result->SetContent(CaptionWidget));
    Slot->SetPadding(FMargin(10, 12));
    Slot->SetHorizontalAlignment(HAlign_Center);
    Slot->SetVerticalAlignment(VAlign_Center);
    return Result;
}

void Resource(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Key, FLinearColor Color)
{
    auto* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*FString::Printf(TEXT("BAR_%s"), Key)));
    Bar->SetPercent(0.f);
    Bar->SetFillColorAndOpacity(Color);
    FProgressBarStyle Style = Bar->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FLinearColor(.055f, .07f, .1f);
    Bar->SetWidgetStyle(Style);
    auto* Height = Tree->ConstructWidget<USizeBox>();
    Height->SetHeightOverride(12);
    Height->SetContent(Bar);
    Row(Parent, Height, 8);
}

void Status(UWidgetTree* Tree, bool Player)
{
    auto* Card = Frame(Tree, TEXT("StatusCard"), FMargin(Player ? 16 : 12));
    Tree->RootWidget = Card;
    auto* Content = Tree->ConstructWidget<UVerticalBox>();
    Card->SetContent(Content);
    auto* Header = Tree->ConstructWidget<UHorizontalBox>();
    auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_Portrait"));
    Image->SetVisibility(ESlateVisibility::Hidden);
    Header->AddChildToHorizontalBox(Size(Tree, Image, Player ? 72 : 64, Player ? 72 : 64))->SetPadding(FMargin(0, 0, 14, 0));
    auto* Details = Tree->ConstructWidget<UVerticalBox>();
    auto* DetailSlot = Header->AddChildToHorizontalBox(Details);
    DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DetailSlot->SetVerticalAlignment(VAlign_Center);
    auto* Name = Label(Tree, TEXT("TXT_Name"), FText::GetEmpty(), Player ? 24 : 18);
    Name->SetAutoWrapText(Player);
    Row(Details, Name, Player ? 0 : 4);
    Row(Content, Header, Player ? 10 : 0);
    UVerticalBox* Bars = Player ? Content : Details;
    Resource(Tree, Bars, TEXT("HP"), FLinearColor(.76f, .17f, .18f));
    Resource(Tree, Bars, TEXT("MP"), FLinearColor(.15f, .42f, .83f));
    if (Player) Resource(Tree, Bars, TEXT("SP"), FLinearColor(.76f, .63f, .18f));
}

void CompanionGroup(UWidgetTree* Tree)
{
    auto* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CompanionStack"));
    Tree->RootWidget = Stack;
    for (int32 Index = 0; Index < 3; ++Index)
        if (auto* Widget = Cast<UICCharacterStatusWidget>(Child(Tree, TEXT("WBP_IC_CompanionStatus"), FName(*FString::Printf(TEXT("Companion%d"), Index + 1)))))
        {
            Widget->DisplayName = FText::GetEmpty();
            Widget->Portrait = nullptr;
            Widget->SetVisibility(ESlateVisibility::Collapsed);
            auto* Wrapper = Size(Tree, Widget, 350, 88);
            Wrapper->SetVisibility(ESlateVisibility::Collapsed);
            Row(Stack, Wrapper, 0);
        }
}

void QuickSlot(UWidgetTree* Tree)
{
    auto* Square = Frame(Tree, TEXT("QuickSlotFrame"), FMargin(3));
    Tree->RootWidget = Size(Tree, Square, 64, 64);
    auto* Content = Tree->ConstructWidget<UOverlay>();
    Square->SetContent(Content);
    auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_ItemIcon"));
    auto* ImageSlot = Content->AddChildToOverlay(Image);
    ImageSlot->SetHorizontalAlignment(HAlign_Fill);
    ImageSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Count = Label(Tree, TEXT("TXT_Count"), FText::GetEmpty(), 18);
    Count->SetShadowColorAndOpacity(FLinearColor::Black);
    Count->SetShadowOffset(FVector2D(1, 1));
    auto* CountSlot = Content->AddChildToOverlay(Count);
    CountSlot->SetHorizontalAlignment(HAlign_Right);
    CountSlot->SetVerticalAlignment(VAlign_Bottom);
    auto* Key=Label(Tree,TEXT("TXT_SlotKey"),FText::FromString(TEXT("1")),14);
    Key->SetShadowColorAndOpacity(FLinearColor::Black);Key->SetShadowOffset(FVector2D(1,1));
    auto* KeySlot=Content->AddChildToOverlay(Key);KeySlot->SetHorizontalAlignment(HAlign_Left);KeySlot->SetVerticalAlignment(VAlign_Top);
}

void QuickSlots(UWidgetTree* Tree)
{
    auto* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuickSlotBar"));
    Tree->RootWidget = Bar;
    for (int32 Index = 0; Index < 4; ++Index)
        if (auto* Widget = Child(Tree, TEXT("WBP_IC_QuickSlot"), FName(*FString::Printf(TEXT("QuickSlot%d"), Index + 1))))
        {
            Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
            // Transparent cells preserve each registered slot's position when another slot is empty.
            Bar->AddChildToHorizontalBox(Size(Tree, Widget, 64, 64))->SetPadding(FMargin(Index > 0 ? 10 : 0, 0, 0, 0));
        }
}

void MovementCooldown(UWidgetTree* Tree)
{
    auto* Ring=Tree->ConstructWidget<UICRadialCooldownBar>(UICRadialCooldownBar::StaticClass(),TEXT("RING_Cooldown"));
    Ring->SetVisibility(ESlateVisibility::Hidden);
    Tree->RootWidget=Size(Tree,Ring,UICRadialCooldownBar::Diameter,UICRadialCooldownBar::Diameter);
    Tree->RootWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void AffinityHearts(UWidgetTree* Tree)
{
    auto* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AffinityHeartRow"));
    Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
    Tree->RootWidget = Bar;
    const TCHAR* Folder = TEXT("/Game/IsekaiClass/UI/SampleUI/MorpheusGUI/textures/Icons/512/");
    auto* Outline = LoadObject<UTexture2D>(nullptr, *(FString(Folder) + TEXT("t_HeartOutline_512.t_HeartOutline_512")));
    auto* Filled = LoadObject<UTexture2D>(nullptr, *(FString(Folder) + TEXT("t_Heart_512.t_Heart_512")));
    for (int32 Index = 0; Index < 5; ++Index)
    {
        auto* Heart = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*FString::Printf(TEXT("AffinityHeart%d"), Index + 1)));
        FProgressBarStyle Style;
        Style.BackgroundImage.SetResourceObject(Outline);
        Style.FillImage.SetResourceObject(Filled);
        for (auto* Brush : {&Style.BackgroundImage, &Style.FillImage})
        {
            Brush->DrawAs = ESlateBrushDrawType::Image;
            Brush->ImageSize = FVector2D(32, 32);
            Brush->TintColor = FLinearColor::White;
        }
        Style.EnableFillAnimation = false;
        Heart->SetWidgetStyle(Style);
        Heart->SetFillColorAndOpacity(FLinearColor::White);
        Heart->SetBorderPadding(FVector2D::ZeroVector);
        Heart->SetBarFillType(EProgressBarFillType::LeftToRight);
        Heart->SetBarFillStyle(EProgressBarFillStyle::Mask);
        Heart->SetPercent(0.f);
        Bar->AddChildToHorizontalBox(Size(Tree, Heart, 32, 32))->SetPadding(FMargin(Index ? 4 : 0, 0, 0, 0));
    }
}

void GameMenu(UWidgetTree* Tree)
{
    auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuCanvas"));
    Tree->RootWidget = Root;
    Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    auto* Dim = Frame(Tree, TEXT("MenuDim"), FMargin(0));
    Dim->SetBrushColor(FLinearColor(0, 0, 0, .45f));
    auto* DimSlot = Root->AddChildToCanvas(Dim);
    DimSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    DimSlot->SetOffsets(FMargin(0));
    auto* Card = Frame(Tree, TEXT("MenuCard"), FMargin(28));
    Place(Root, Card, {.5f, .5f}, {.5f, .5f}, {0, 0}, {1120, 680});
    auto* Column = Tree->ConstructWidget<UVerticalBox>();
    Card->SetContent(Column);
    Row(Column, Label(Tree, TEXT("TXT_MenuTitle"), ICFrontendLayout::Text(TEXT("Menu")), 28), 20);
    auto* Tabs = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MenuTabs"));
    const FText Captions[] = {ICFrontendLayout::Text(TEXT("Party")), ICFrontendLayout::Text(TEXT("Inventory")),
        ICFrontendLayout::Text(TEXT("Quest")), ICFrontendLayout::Text(TEXT("Codex")), ICFrontendLayout::Text(TEXT("Settings"))};
    for (int32 Index = 0; Index < UICGameMenuWidget::Tabs().Num(); ++Index)
    {
        const FName Name(*(TEXT("TAB_") + UICGameMenuWidget::Tabs()[Index].ToString()));
        auto* Tab = Button(Tree, Name, Captions[Index], 16);
        Tab->SetBackgroundColor(Index == 0 ? FLinearColor(.72f, .57f, .31f) : FLinearColor(.29f, .36f, .47f));
        auto* Slot = Tabs->AddChildToHorizontalBox(Tab);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(Index > 0 ? 4 : 0, 0, 0, 0));
    }
    Row(Column, Tabs, 16);
    auto* Body = Frame(Tree, TEXT("MenuContent"), FMargin(20));
    Body->SetBrushColor(FLinearColor(.025f, .038f, .06f));
    auto* BodySlot = Column->AddChildToVerticalBox(Body);
    BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* NavigationHint=Label(Tree,TEXT("TXT_Navigation"),FText::GetEmpty(),14);
    auto* NavigationSlot=Column->AddChildToVerticalBox(NavigationHint);NavigationSlot->SetPadding(FMargin(0,10,0,0));
    auto* Pages = Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("MenuPages"));
    Body->SetContent(Pages);
    for (const FName Tab : UICGameMenuWidget::Tabs())
    {
        auto* Page = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(*(TEXT("Page_") + Tab.ToString())));
        auto* Slot = Cast<UWidgetSwitcherSlot>(Pages->AddChild(Page));
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Fill);
        if(Tab==TEXT("Party"))if(auto* Party=Child(Tree,TEXT("WBP_IC_PartyMenu"),TEXT("PartyContent")))
        {
            auto* Placement=Page->AddChildToCanvas(Party);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));
        }
        if(Tab==TEXT("Inventory"))if(auto* Inventory=Child(Tree,TEXT("WBP_IC_InventoryMenu"),TEXT("InventoryContent")))
        {
            auto* Placement=Page->AddChildToCanvas(Inventory);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));
        }
        if(Tab==TEXT("Quest"))if(auto* Quests=Child(Tree,TEXT("WBP_IC_QuestMenu"),TEXT("QuestContent")))
        {
            auto* Placement=Page->AddChildToCanvas(Quests);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));
        }
        if(Tab==TEXT("Codex"))if(auto* CodexContent=Child(Tree,TEXT("WBP_IC_CodexMenu"),TEXT("CodexContent")))
        {
            auto* Placement=Page->AddChildToCanvas(CodexContent);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));
        }
        if (Tab == TEXT("Settings"))
        {
            auto* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
            auto* Width = Tree->ConstructWidget<USizeBox>();
            Width->SetWidthOverride(400);
            Width->SetContent(Content);
            Place(Page, Width, {.5f, .5f}, {.5f, .5f}, {0, 0}, {400, 320})->SetAutoSize(true);
            auto* Message = Label(Tree, TEXT("TXT_SessionMessage"), FText::GetEmpty(), 22);
            Message->SetAutoWrapText(true);
            Message->SetVisibility(ESlateVisibility::Collapsed);
            Row(Content, Message, 16);
            auto* Actions = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuActions"));
            for (const auto* Key : {TEXT("Resume"), TEXT("Save"), TEXT("Load"), TEXT("GameSettings"), TEXT("Quit")})
                Row(Actions, Button(Tree, FName(*FString::Printf(TEXT("BTN_%s"), Key)), ICFrontendLayout::Text(Key)), 12);
            Row(Content, Actions, 0);
        }
    }
}

void HUD(UWidgetTree* Tree)
{
    auto* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Tree->RootWidget = Canvas;
    const auto Add = [&](const TCHAR* Asset, FName Name, FVector2D Anchor, FVector2D Alignment, FVector2D Position, FVector2D Extent)
    {
        if (auto* Widget = Child(Tree, Asset, Name))
        {
            Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(Canvas, Widget, Anchor, Alignment, Position, Extent);
        }
    };
    Add(TEXT("WBP_IC_Minimap"), TEXT("HUD_Minimap"), {0, 0}, {0, 0}, {32, 32}, {240, 140});
    Add(TEXT("WBP_IC_PlayerStatus"), TEXT("HUD_PlayerStatus"), {0, 1}, {0, 1}, {32, -32}, {350, 200});
    Add(TEXT("WBP_IC_CompanionGroup"), TEXT("HUD_Companions"), {0, 1}, {0, 1}, {32, -248}, {350, 280});
    if (auto* Companions = Tree->FindWidget(TEXT("HUD_Companions")))
        if (auto* Slot = Cast<UCanvasPanelSlot>(Companions->Slot)) Slot->SetAutoSize(true);
    Add(TEXT("WBP_IC_QuestTracker"), TEXT("HUD_QuestTracker"), {1, 0}, {1, 0}, {-32, 32}, {350, 240});
    Add(TEXT("WBP_IC_QuickSlots"), TEXT("HUD_QuickSlots"), {1, 1}, {1, 1}, {-32, -32}, {286, 64});
    Add(TEXT("WBP_IC_MovementCooldown"), TEXT("HUD_MovementCooldown"), {0, 0}, {.5f, .5f}, {0, 0},
        FVector2D(UICRadialCooldownBar::Diameter));
    if(auto* Feedback=Child(Tree,TEXT("WBP_IC_CombatFeedback"),TEXT("HUD_CombatFeedback")))
    {
        Feedback->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Placement=Canvas->AddChildToCanvas(Feedback);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));Placement->SetZOrder(5);
    }
    Place(Canvas, Label(Tree, TEXT("TXT_Hint"), FText::GetEmpty(), 22), {.5f, .76f}, {.5f, .5f}, {0, 0}, {680, 50});
    Place(Canvas, Label(Tree, TEXT("TXT_Notice"), FText::GetEmpty(), 18), {.5f, .9f}, {.5f, .5f}, {0, 0}, {700, 50});
    if (auto* Menu = Child(Tree, TEXT("WBP_IC_GameMenu"), TEXT("GameMenu")))
    {
        auto* Slot = Canvas->AddChildToCanvas(Menu);
        Slot->SetAnchors(FAnchors(0, 0, 1, 1));
        Slot->SetOffsets(FMargin(0));
        Slot->SetZOrder(10);
        Menu->SetVisibility(ESlateVisibility::Collapsed);
    }
}
}

bool Build(UWidgetTree* Tree, FName Kind)
{
    if (Kind == TEXT("PlayerStatus")) Status(Tree, true);
    else if (Kind == TEXT("CompanionStatus")) Status(Tree, false);
    else if (Kind == TEXT("CompanionGroup")) CompanionGroup(Tree);
    else if (Kind == TEXT("QuickSlot")) QuickSlot(Tree);
    else if (Kind == TEXT("QuickSlots")) QuickSlots(Tree);
    else if (Kind == TEXT("MovementCooldown")) MovementCooldown(Tree);
    else if (Kind == TEXT("PartyPortrait")) return UICPartyPortraitWidget::BuildLayout(Tree);
    else if (Kind == TEXT("PartyMenu")) return UICPartyMenuWidget::BuildLayout(Tree);
    else if (Kind == TEXT("InventorySlot")) return UICInventorySlotWidget::BuildLayout(Tree);
    else if (Kind == TEXT("ItemConfirm")) return UICItemConfirmWidget::BuildLayout(Tree);
    else if (Kind == TEXT("InventoryItemPopup")) return UICInventoryItemPopupWidget::BuildLayout(Tree);
    else if (Kind == TEXT("InventoryMenu")) return UICInventoryMenuWidget::BuildLayout(Tree);
    else if (Kind == TEXT("QuestMenuEntry")) return UICQuestMenuEntryWidget::BuildLayout(Tree);
    else if (Kind == TEXT("QuestDetail")) return UICQuestDetailWidget::BuildLayout(Tree);
    else if (Kind == TEXT("QuestMenu")) return UICQuestMenuWidget::BuildLayout(Tree);
    else if (Kind == TEXT("CodexCell")) return UICCodexCellWidget::BuildLayout(Tree);
    else if (Kind == TEXT("CodexPopup")) return UICCodexPopupWidget::BuildLayout(Tree);
    else if (Kind == TEXT("CodexTargets")) return UICCodexTargetsWidget::BuildLayout(Tree);
    else if (Kind == TEXT("CodexMenu")) return UICCodexMenuWidget::BuildLayout(Tree);
    else if (Kind == TEXT("AffinityHearts")) AffinityHearts(Tree);
    else if (Kind == TEXT("CombatFeedback"))
    {
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeedbackCanvas"));
        Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);Tree->RootWidget=Canvas;
    }
    else if (Kind == TEXT("PlayerDamage") || Kind == TEXT("MonsterDamage"))
    {
        auto* Text=Label(Tree,TEXT("TXT_Damage"),FText::FromString(TEXT("0")),30);
        Text->SetJustification(ETextJustify::Center);
        auto Style=Text->GetFont();Style.OutlineSettings.OutlineSize=2;Style.OutlineSettings.OutlineColor=FLinearColor(.04f,.02f,.01f,1);Text->SetFont(Style);
        Text->SetColorAndOpacity(Kind==TEXT("PlayerDamage")?FLinearColor(1,.19f,.16f):FLinearColor(1,.88f,.46f));
        Tree->RootWidget=Size(Tree,Text,140,60);
    }
    else if (Kind == TEXT("NPCGiftPicker"))
    {
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Canvas;
        auto* Shade=Frame(Tree,TEXT("GiftDim"),FMargin(0));Shade->SetBrushColor(FLinearColor(0,0,0,.65f));
        auto* ShadeSlot=Canvas->AddChildToCanvas(Shade);ShadeSlot->SetAnchors(FAnchors(0,0,1,1));ShadeSlot->SetOffsets(FMargin(0));
        auto* Card=Frame(Tree,TEXT("GiftCard"),FMargin(24));Place(Canvas,Card,{.5f,.5f},{.5f,.5f},{0,0},{820,650});
        auto* Content=Tree->ConstructWidget<UVerticalBox>();Card->SetContent(Content);
        Row(Content,Label(Tree,TEXT("GiftTitle"),FText::FromString(TEXT("선물 인벤토리")),26),16);
        auto* Status=Label(Tree,TEXT("GiftStatus"),FText::FromString(TEXT("선물할 아이템을 선택하세요.")),18);
        Status->SetAutoWrapText(true);Row(Content,Status,14);
        auto* Body=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("GiftBody"));
        auto* BodySlot=Content->AddChildToVerticalBox(Body);BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));BodySlot->SetPadding(FMargin(0,0,0,14));
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("GiftScroll"));
        Scroll->AddChild(Tree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(),TEXT("GiftGrid")));
        Body->AddChildToHorizontalBox(Size(Tree,Scroll,420,420))->SetPadding(FMargin(0,0,18,0));
        auto* Detail=Tree->ConstructWidget<UVerticalBox>();auto* DetailSlot=Body->AddChildToHorizontalBox(Detail);DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("GiftSelectedIcon"));Row(Detail,Size(Tree,Image,112,112),16);
        auto* Name=Label(Tree,TEXT("GiftSelectedName"),FText::GetEmpty(),22);Name->SetAutoWrapText(true);Row(Detail,Name,12);
        Row(Detail,Label(Tree,TEXT("GiftSelectedCount"),FText::GetEmpty(),18),16);
        auto* Description=Label(Tree,TEXT("GiftDescription"),FText::GetEmpty(),17);Description->SetAutoWrapText(true);
        auto* DescriptionScroll=Tree->ConstructWidget<UScrollBox>();DescriptionScroll->AddChild(Description);Row(Detail,Size(Tree,DescriptionScroll,290,180),0);
        Row(Content,Button(Tree,TEXT("BTN_GiftBack"),FText::FromString(TEXT("돌아가기"))),12);
        Row(Content,Label(Tree,TEXT("GiftHelp"),FText::FromString(TEXT("WASD  아이템 이동     Enter  선택     ESC  돌아가기")),16),0);
        if(auto* Confirm=Child(Tree,TEXT("WBP_IC_ItemConfirm"),TEXT("GiftConfirm")))
        {auto* Placement=Canvas->AddChildToCanvas(Confirm);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));Placement->SetZOrder(10);Confirm->SetVisibility(ESlateVisibility::Collapsed);}
    }
    else if (Kind == TEXT("GatherProgress"))
    {
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Canvas;
        auto* Card=Frame(Tree,TEXT("GatherProgressCard"),FMargin(20,14));
        Place(Canvas,Card,{.5f,.68f},{.5f,.5f},{0,0},{380,114});
        auto* Content=Tree->ConstructWidget<UVerticalBox>();Card->SetContent(Content);
        auto* Name=Label(Tree,TEXT("TXT_GatherName"),FText::FromString(TEXT("채집 중")),20);
        Name->SetJustification(ETextJustify::Center);Row(Content,Name,10);
        auto* Bar=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("BAR_GatherProgress"));
        auto Style=Bar->GetWidgetStyle();Style.BackgroundImage.TintColor=FLinearColor(.035f,.055f,.07f,1);
        Bar->SetWidgetStyle(Style);Bar->SetFillColorAndOpacity(FLinearColor(.3f,.9f,.55f));
        Bar->SetBarFillType(EProgressBarFillType::LeftToRight);Bar->SetPercent(0);Row(Content,Size(Tree,Bar,340,20),8);
        auto* Hint=Label(Tree,TEXT("TXT_GatherCancel"),FText::FromString(TEXT("이동 / F / ESC  취소")),14);
        Hint->SetJustification(ETextJustify::Center);Hint->SetColorAndOpacity(FLinearColor(.55f,.63f,.7f));Row(Content,Hint,0);
    }
    else if (Kind == TEXT("LifeLabel"))
    {
        auto* Text=Label(Tree,TEXT("LifeLabelText"),FText::FromString(TEXT("물고기 랜덤")),24);
        Text->SetJustification(ETextJustify::Center);
        auto Font=Text->GetFont();Font.OutlineSettings.OutlineSize=2;
        Font.OutlineSettings.OutlineColor=FLinearColor::Black;Text->SetFont(Font);
        Tree->RootWidget=Size(Tree,Text,320,60);
    }
    else if (Kind == TEXT("NPCQuestMarker"))
    {
        auto* Text=Label(Tree,TEXT("QuestMarkerText"),FText::FromString(TEXT("!")),52);
        Text->SetJustification(ETextJustify::Center);
        Text->SetColorAndOpacity(FLinearColor(.08f,1,.22f));
        auto Font=Text->GetFont();Font.OutlineSettings.OutlineSize=3;
        Font.OutlineSettings.OutlineColor=FLinearColor(.01f,.02f,.01f,1);Text->SetFont(Font);
        Tree->RootWidget=Size(Tree,Text,70,80);
    }
    else if (Kind == TEXT("MonsterHealthBar"))
    {
        auto* Bar=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("BAR_MonsterHP"));
        auto Style=Bar->GetWidgetStyle();Style.BackgroundImage.TintColor=FLinearColor(.015f,.015f,.025f,.9f);Bar->SetWidgetStyle(Style);
        Bar->SetFillColorAndOpacity(FLinearColor(.8f,.13f,.12f));Bar->SetPercent(1);
        Bar->SetBorderPadding({2,2});Tree->RootWidget=Size(Tree,Bar,180,14);
    }
    else if (Kind == TEXT("RewardNotice"))
    {
        auto* Stack=Tree->ConstructWidget<UVerticalBox>();Tree->RootWidget=Stack;
        auto* Gold=Label(Tree,TEXT("TXT_Gold"),FText::GetEmpty(),24);Gold->SetColorAndOpacity(FLinearColor(1,.8f,.2f));Row(Stack,Gold,4);
        auto* Items=Label(Tree,TEXT("TXT_Items"),FText::GetEmpty(),22);Items->SetAutoWrapText(true);Row(Stack,Items,0);
        auto* Essence=Label(Tree,TEXT("TXT_Essence"),FText::GetEmpty(),22);Essence->SetColorAndOpacity(FLinearColor(.55f,.85f,1));Row(Stack,Essence,0);
    }
    else if (Kind == TEXT("Minimap"))
    {
        auto* Black = Frame(Tree, TEXT("MinimapPlaceholder"), FMargin(0));
        Black->SetBrushColor(FLinearColor::Black);
        Tree->RootWidget = Size(Tree, Black, 240, 240);
    }
    else if (Kind == TEXT("QuestTracker"))
    {
        auto* Card = Frame(Tree, TEXT("QuestTrackerCard"));
        Tree->RootWidget = Card;
        auto* Content = Tree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);
        Row(Content, Label(Tree, TEXT("QuestHeading"), ICFrontendLayout::Text(TEXT("Quest")), 22), 12);
        auto* Text = Label(Tree, TEXT("QuestText"), FText::GetEmpty(), 17);
        Text->SetAutoWrapText(true);
        Row(Content, Text, 0);
    }
    else if (Kind == TEXT("GameMenu")) GameMenu(Tree);
    else if (Kind == TEXT("HUD")) {HUD(Tree);EnsureGameClock(Tree);}
    else return false;
    return true;
}
bool EnsureGameClock(UWidgetTree* Tree)
{
    auto* Canvas=Tree?Cast<UCanvasPanel>(Tree->FindWidget(TEXT("HUDCanvas"))):nullptr;if(!Canvas)return false;
    if(!Tree->FindWidget(TEXT("TXT_GameClock")))
    {
        auto* Clock=Label(Tree,TEXT("TXT_GameClock"),FText::GetEmpty(),18);
        Clock->SetJustification(ETextJustify::Center);Clock->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Canvas,Clock,{0,0},{0,0},{32,20},{240,30});
    }
    if(auto* Map=Tree->FindWidget(TEXT("HUD_Minimap")))if(auto* Slot=Cast<UCanvasPanelSlot>(Map->Slot)){Slot->SetPosition(FVector2D(32,58));Slot->SetSize(FVector2D(240,140));}
    return true;
}
}
