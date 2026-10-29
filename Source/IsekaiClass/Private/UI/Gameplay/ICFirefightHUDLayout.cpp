#include "UI/Gameplay/ICFirefightHUDLayout.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace ICFirefightHUDLayout
{
namespace
{
UWidgetTree* Source()
{
    auto* Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/IsekaiClass/UI/SampleUI/FirefightUI/Blueprints/UserInterface/BP_Firefight_HUD.BP_Firefight_HUD_C"));
    auto* Generated = Cast<UWidgetBlueprintGeneratedClass>(Class);
    return Generated ? Generated->GetWidgetTreeArchetype() : nullptr;
}
UCanvasPanelSlot* At(UCanvasPanel* Parent, UWidget* Widget, FVector2D Position, FVector2D Size)
{
    auto* Slot = Parent->AddChildToCanvas(Widget);
    Slot->SetPosition(Position); Slot->SetSize(Size);
    return Slot;
}
void Layout(UCanvasPanelSlot* To, const UCanvasPanelSlot* From)
{
    To->SetLayout(From->GetLayout()); To->SetAutoSize(From->GetAutoSize()); To->SetZOrder(From->GetZOrder());
}
// Copy designer widgets and their slots; none of the sample HUD's gameplay graphs are imported.
UWidget* Copy(UWidgetTree* Tree, UWidget* From, FName Name = NAME_None)
{
    if (!From) return nullptr;
    if (Name.IsNone()) Name = From->GetFName();
    if (auto* Canvas = Cast<UCanvasPanel>(From))
    {
        auto* Result = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Name);
        Result->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Result->SetRenderTransform(Canvas->GetRenderTransform());
        for (auto* Child : Canvas->GetAllChildren())
            if (auto* NewChild = Copy(Tree, Child))
                Layout(Result->AddChildToCanvas(NewChild), CastChecked<UCanvasPanelSlot>(Child->Slot));
        return Result;
    }
    auto* Result = DuplicateObject<UWidget>(From, Tree, Name);
    Result->Slot = nullptr;
    Result->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (auto* Image = Cast<UImage>(Result))
    {
        auto Brush = Image->GetBrush();
        if (auto* Texture = Cast<UTexture2D>(Brush.GetResourceObject()); Texture && Brush.ImageSize.IsZero())
        { Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()); Image->SetBrush(Brush); }
    }
    return Result;
}
UUserWidget* Child(UWidgetTree* Tree, const TCHAR* Asset, FName Name)
{
    const FString Path = FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"), Asset, Asset);
    auto* Class = LoadClass<UUserWidget>(nullptr, *Path);
    return Class ? Tree->ConstructWidget<UUserWidget>(Class, Name) : nullptr;
}
UTextBlock* Text(UWidgetTree* Tree, FName Name, int32 Size)
{
    auto* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Result->SetFont(ICFrontendLayout::Font(Size, true));
    Result->SetColorAndOpacity(FLinearColor::White);
    Result->SetShadowColorAndOpacity(FLinearColor::Black); Result->SetShadowOffset({1, 1});
    Result->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Result;
}
}

bool Status(UWidgetTree* Tree, bool Player, bool Alternate)
{
    auto* Sample = Source();
    auto* Panel = Sample ? Cast<UCanvasPanel>(Sample->FindWidget(Player ? TEXT("CanvasPanel_21") : Alternate ? TEXT("CanvasPanel_23") : TEXT("CanvasPanel_22"))) : nullptr;
    if (!Panel) return false;
    auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Panel->GetFName());
    Tree->RootWidget = Root;
    // The original sample companion card was only as wide as its portrait.
    // Reserve a real information rail on the right for name, resources and
    // status effects so those widgets never need to paint over the portrait.
    const float Width = Player ? 280.f : 220.f;
    const float Height = Player ? 194.f : 126.f;
    const float Shift = Player ? 174.f : 125.66188f;
    const FName PortraitName = Player ? TEXT("Image_70") : Alternate ? TEXT("Image_77") : TEXT("Image_74");
    const FName HPName = Player ? TEXT("ProgressBar_15") : Alternate ? TEXT("ProgressBar") : TEXT("ProgressBar_17");
    const FName NameLabel = Player ? TEXT("TextBlock_51") : Alternate ? TEXT("TextBlock_55") : TEXT("TextBlock_54");
    const FName BuffName = Player ? TEXT("Image_69") : Alternate ? TEXT("Image_79") : TEXT("Image_75");
    auto* Backdrop=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("StatusBackdrop"));
    Backdrop->SetBrushColor(FLinearColor(.01f,.018f,.03f,.62f));Backdrop->SetPadding(FMargin(0));
    At(Root,Backdrop,{0,0},{Width,Height})->SetZOrder(-10);
    int32 MPIndex = 0;
    for (auto* Original : Panel->GetAllChildren())
    {
        const auto Key = Original->GetFName();
        const FString KeyString = Key.ToString();
        if (Key == TEXT("Image_73") || Key == TEXT("Image_76") || Key == TEXT("Image_80")) continue;
        if (Cast<UTextBlock>(Original) && Key != NameLabel) continue;
        auto* OriginalSlot = CastChecked<UCanvasPanelSlot>(Original->Slot);
        auto Position = OriginalSlot->GetPosition();
        Position.Y += Shift; // Original cards anchor every child to their bottom edge.
        UWidget* Widget = nullptr;
        if (Key == PortraitName)
        {
            auto* Image = CastChecked<UImage>(Copy(Tree, Original, TEXT("IMG_Portrait")));
            Image->SetBrushFromTexture(nullptr);
            auto* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitBox"));
            const float Extent = Player ? 128.f : 96.f;
            Box->SetWidthOverride(Extent); Box->SetHeightOverride(Extent); Box->SetContent(Image);
            Widget = Box;
            At(Root, Widget, Position, {Extent, Extent});
            continue;
        }
        if (Key == BuffName)
        {
            auto* Prototype = Copy(Tree, Original, TEXT("StatusEffectPrototype"));
            Prototype->SetVisibility(ESlateVisibility::Collapsed);
            At(Root, Prototype, Position, {24, 24});
            auto* EffectsPanel=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("StatusEffectsPanel"));
            EffectsPanel->SetBrushColor(FLinearColor(.01f,.015f,.025f,.82f));EffectsPanel->SetPadding(FMargin(3));
            auto* Effects = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusEffects"));
            EffectsPanel->SetContent(Effects);
            auto* Slot=At(Root,EffectsPanel,Player?FVector2D(132,8):FVector2D(106,5),Player?FVector2D(140,32):FVector2D(108,32));
            Slot->SetZOrder(20);
            auto* Overflow=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("StatusOverflow"));
            Overflow->SetBrushColor(FLinearColor(.18f,.22f,.28f,.95f));Overflow->SetPadding(FMargin(2));Overflow->SetVisibility(ESlateVisibility::Collapsed);
            auto* OverflowText=Text(Tree,TEXT("TXT_StatusOverflow"),11);OverflowText->SetText(FText::FromString(TEXT("+0")));OverflowText->SetJustification(ETextJustify::Center);Overflow->SetContent(OverflowText);
            Effects->AddChildToHorizontalBox(Overflow)->SetPadding(FMargin(2,0,0,0));
            continue;
        }
        if (KeyString.StartsWith(TEXT("ImageArmor")) && !KeyString.StartsWith(TEXT("ImageArmorBg")))
        {
            auto* Image = CastChecked<UImage>(Original);
            auto* MP = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*FString::Printf(TEXT("BAR_MP%d"), ++MPIndex)));
            FProgressBarStyle Style;
            Style.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;
            Style.FillImage = Image->GetBrush(); Style.EnableFillAnimation = false;
            if (auto* Texture = Cast<UTexture2D>(Style.FillImage.GetResourceObject()))
                Style.FillImage.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
            MP->SetWidgetStyle(Style); MP->SetFillColorAndOpacity(FLinearColor::White);
            MP->SetBorderPadding({0, 0}); MP->SetBarFillStyle(EProgressBarFillStyle::Mask); MP->SetPercent(0);
            At(Root, MP, Position, Player ? FVector2D(70, 12) : FVector2D(30, 7));
            continue;
        }
        Widget = Copy(Tree, Original, Key == HPName ? FName(TEXT("BAR_HP")) : Key == NameLabel ? FName(TEXT("TXT_Name")) : Key == TEXT("ProgressBar_14") ? FName(TEXT("BAR_SP")) : Key);
        auto* Slot = At(Root, Widget, Position, OriginalSlot->GetSize());
        Slot->SetAutoSize(OriginalSlot->GetAutoSize()); Slot->SetZOrder(OriginalSlot->GetZOrder());
        if (auto* Label = Cast<UTextBlock>(Widget))
        {
            Label->SetText(FText::GetEmpty()); Label->SetFont(ICFrontendLayout::Font(Player ? 16 : 13));
            Label->SetRenderScale({1, 1});
            Slot->SetPosition({3, Player ? 174.f : 108.f}); Slot->SetSize({Player ? Width : 180.f, 22});
        }
        if (Key == TEXT("ProgressBar_14"))
        { Slot->SetPosition({3, 159}); Slot->SetSize({213, 10}); }
    }
    // Retain the aggregate value for bindings; the three visible segments present it.
    auto* Aggregate = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BAR_MP"));
    Aggregate->SetVisibility(ESlateVisibility::Collapsed); At(Root, Aggregate, {0, 0}, {0, 0});
    auto* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StatusSize"));
    Size->SetWidthOverride(Width); Size->SetHeightOverride(Height); Size->SetContent(Root); Tree->RootWidget = Size;
    UICCharacterStatusWidget::BuildSkillCooldownOverlay(Tree);
    return true;
}

bool QuickSlot(UWidgetTree* Tree)
{
    auto* Sample = Source();
    auto* SourcePanel = Sample ? Cast<UCanvasPanel>(Sample->FindWidget(TEXT("CanvasPanel_1"))) : nullptr;
    if (!SourcePanel) return false;
    auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuickSlotCanvas"));
    Tree->RootWidget = Root;
    for (auto* Original : SourcePanel->GetAllChildren())
    {
        const FName Key = Original->GetFName();
        auto* Widget = Copy(Tree, Original, Key == TEXT("ImageIcon_1") ? FName(TEXT("IMG_ItemIcon")) : Key == TEXT("TextBlock") ? FName(TEXT("TXT_SlotKey")) : Key == TEXT("ProgressBar_4") ? FName(TEXT("BAR_ItemCooldown")) : Key);
        auto* Slot = Root->AddChildToCanvas(Widget); Layout(Slot, CastChecked<UCanvasPanelSlot>(Original->Slot));
        if (Key == TEXT("ImageIcon_1"))
        { Slot->SetAutoSize(false); Slot->SetPosition({-33, -113}); Slot->SetSize({72, 80}); CastChecked<UImage>(Widget)->SetBrushFromTexture(nullptr); }
        if (Key == TEXT("TextBlock")) { CastChecked<UTextBlock>(Widget)->SetFont(ICFrontendLayout::Font(16, true)); CastChecked<UTextBlock>(Widget)->SetText(FText::GetEmpty()); }
        if (Key == TEXT("ProgressBar_4")) { CastChecked<UProgressBar>(Widget)->SetPercent(0); Widget->SetVisibility(ESlateVisibility::Hidden); }
    }
    auto* Count = Text(Tree, TEXT("TXT_Count"), 16);
    auto* CountSlot = At(Root, Count, {-15, -39}, {50, 22}); CountSlot->SetAnchors(FAnchors(.5f, 1)); CountSlot->SetZOrder(4);
    auto* Cooldown = Text(Tree, TEXT("TXT_ItemCooldown"), 26); Cooldown->SetJustification(ETextJustify::Center); Cooldown->SetVisibility(ESlateVisibility::Hidden);
    auto* CDSlot = At(Root, Cooldown, {-38, -95}, {82, 38}); CDSlot->SetAnchors(FAnchors(.5f, 1)); CDSlot->SetZOrder(5);
    return true;
}

bool QuickSlots(UWidgetTree* Tree)
{
    auto* Sample = Source();
    if (!Sample || !Sample->FindWidget(TEXT("CanvasPanel_94"))) return false;
    auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_94"));
    Tree->RootWidget = Root;
    // The original root contains only three cell canvases; the reusable cell copies their art.
    for (int32 Index = 0; Index < 4; ++Index)
        if (auto* Cell = Child(Tree, TEXT("WBP_IC_QuickSlot"), FName(*FString::Printf(TEXT("QuickSlot%d"), Index + 1))))
        {
            auto* Slot = At(Root, Cell, {-244.f + Index * 128.f, -70}, {105, 127});
            Slot->SetAnchors(FAnchors(.5f, .5f));
        }
    return true;
}

void Target(UWidgetTree* Tree, UCanvasPanel* HUD)
{
    auto* Sample = Source();
    auto* Panel = Sample ? Cast<UCanvasPanel>(Sample->FindWidget(TEXT("CanvasPanel"))) : nullptr;
    if (!Panel) return;
    auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUD_TargetInfo"));
    for (const TCHAR* Key : {TEXT("Image_65"), TEXT("Image_66"), TEXT("TextBlock_49"), TEXT("ProgressBar_13")})
        if (auto* Original = Sample->FindWidget(Key))
        {
            auto* Widget = Copy(Tree, Original);
            Layout(Root->AddChildToCanvas(Widget), CastChecked<UCanvasPanelSlot>(Original->Slot));
            if (auto* TextWidget = Cast<UTextBlock>(Widget)) { TextWidget->SetFont(ICFrontendLayout::Font(13)); TextWidget->SetText(FText::GetEmpty()); }
        }
    Root->SetVisibility(ESlateVisibility::Collapsed);
    auto* Slot = At(HUD, Root, {0, 72}, {85, 32}); Slot->SetAnchors(FAnchors(.5f, 0)); Slot->SetAlignment({.5f, 0});
    Root->SetRenderScale({2, 2}); Slot->SetZOrder(4);
}
}
