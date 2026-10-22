#include "UI/Framework/ICUIRootWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"

void UICUIRootWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("Root"));
    HUDLayer = Cast<UOverlay>(GetWidgetFromName(TEXT("Layer_HUD")));
    LegacyLayer = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("Layer_Legacy")));
    ScreenLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Screen")));
    ModalLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Modal")));
    LoadingLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Layer_Loading")));
    for (auto* Stack : {ScreenLayer.Get(), ModalLayer.Get(), LoadingLayer.Get()})
        if (Stack)
            Stack->SetTransitionDuration(0.f);
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
