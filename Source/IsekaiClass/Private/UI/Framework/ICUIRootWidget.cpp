#include "UI/Framework/ICUIRootWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Framework/ICInputPresentation.h"
#include "Core/Controller/ICPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"

void UICUIRootWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    HUDLayer = Cast<UOverlay>(GetWidgetFromName(TEXT("Layer_HUD")));
    LegacyLayer = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("Layer_Legacy")));
    ScreenLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Screen")));
    ModalLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Modal")));
    LoadingLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Loading")));
    for (auto* Stack : {ScreenLayer.Get(), ModalLayer.Get(), LoadingLayer.Get()})
        if (Stack)
            Stack->SetTransitionDuration(0.f);
    if(auto* Overlay=Cast<UOverlay>(GetWidgetFromName(TEXT("Root"))))
    {
        auto* InputLayer=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("IC_InputHintLayer"));
        InputLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* LayerSlot=Overlay->AddChildToOverlay(InputLayer);LayerSlot->SetHorizontalAlignment(HAlign_Fill);LayerSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Frame=WidgetTree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.008f,.018f,.028f,.94f));Frame->SetPadding(FMargin(16,8));
        auto* HintSlot=InputLayer->AddChildToCanvas(Frame);HintSlot->SetAnchors(FAnchors(.5f,1));HintSlot->SetAlignment({.5f,1});HintSlot->SetPosition({0,-12});HintSlot->SetSize({1450,42});
        auto* Hint=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_InputNavigation"));
        Hint->SetFont(ICFrontendLayout::Font(16));Hint->SetColorAndOpacity(FLinearColor(.68f,.86f,.92f));Hint->SetJustification(ETextJustify::Center);Frame->SetContent(Hint);
        ICInputPresentation::SetHint(Hint,Cast<AICPlayerController>(GetOwningPlayer()),
            NSLOCTEXT("ICInputHints","KeyboardMenuFooter","마우스로 클릭   ·   W/S 또는 방향키 이동   ·   F / Enter 선택   ·   Esc 뒤로"),
            NSLOCTEXT("ICInputHints","GamepadMenuFooter","방향키 / LS 이동   ·   {Accept} 선택   ·   {Back} 뒤로   ·   마우스 클릭 가능"));
        InputLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
}
void UICUIRootWidget::SetPointerUIVisible(bool Visible)
{
    if(auto* Hint=GetWidgetFromName(TEXT("IC_InputHintLayer")))Hint->SetVisibility(Visible?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}

void UICUIRootWidget::Mount(UUserWidget* Widget, int32 ZOrder)
{
    if (!Widget)
        return;
    if (auto* Screen = Cast<UCommonActivatableWidget>(Widget))
    {
        auto* Layer = ZOrder >= 10000 ? LoadingLayer.Get() : ZOrder >= 100 ? ModalLayer.Get() : ScreenLayer.Get();
        if (Layer)
            Layer->AddWidgetInstance(*Screen);
    }
    else if (LegacyLayer)
    {
        auto* PanelSlot = LegacyLayer->AddChildToCanvas(Widget);
        PanelSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        PanelSlot->SetOffsets(FMargin(0));
        PanelSlot->SetZOrder(ZOrder);
    }
}

void UICUIRootWidget::Unmount(UUserWidget* Widget)
{
    if (auto* Screen = Cast<UCommonActivatableWidget>(Widget))
        for (auto* Stack : {ScreenLayer.Get(), ModalLayer.Get(), LoadingLayer.Get()})
            if (Stack)
                Stack->RemoveWidget(*Screen);
    if (Widget)
        Widget->RemoveFromParent();
}

void UICUIRootWidget::SetHUD(UUserWidget* Widget)
{
    if (!HUDLayer)
        return;
    HUDLayer->ClearChildren();
    AddHUDWidget(Widget);
}

void UICUIRootWidget::AddHUDWidget(UUserWidget* Widget)
{
    if(!HUDLayer)return;
    if (Widget)
    {
        auto* PanelSlot = HUDLayer->AddChildToOverlay(Widget);
        PanelSlot->SetHorizontalAlignment(HAlign_Fill);
        PanelSlot->SetVerticalAlignment(VAlign_Fill);
    }
}
