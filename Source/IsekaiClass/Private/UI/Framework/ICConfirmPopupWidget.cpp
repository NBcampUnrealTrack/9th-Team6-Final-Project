#include "UI/Framework/ICConfirmPopupWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
UICConfirmPopupWidget::UICConfirmPopupWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
    bIsModal = true;
}
void UICConfirmPopupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("Confirm"));
    SetIsFocusable(true);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Accept"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Accept);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Cancel"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Cancel);
}
void UICConfirmPopupWidget::Configure(const FText& Message, FSimpleDelegate InAccept)
{
    Accepted = InAccept;
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Message"))))
        Text->SetText(Message);
}
UWidget* UICConfirmPopupWidget::NativeGetDesiredFocusTarget() const
{
    return GetWidgetFromName(TEXT("BTN_Cancel"));
}
void UICConfirmPopupWidget::Accept()
{
    const auto Callback = Accepted;
    Cancel();
    Callback.ExecuteIfBound();
}
void UICConfirmPopupWidget::Cancel()
{
    Accepted.Unbind();
    if (GetOwningLocalPlayer())
        GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(this);
}
void UICConfirmPopupWidget::GoBack()
{
    Cancel();
}
