#include "Settings/ICGameUserSettings.h"
#include "UI/Framework/ICScreenWidget.h"

FReply UICScreenWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape || UICGameUserSettings::IsGamepadBackKey(Event.GetKey()))
    {
        if (!Event.IsRepeat())
            GoBack();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

bool UICScreenWidget::NativeOnHandleBackAction()
{
    GoBack();
    return true;
}
void UICScreenWidget::GoBack()
{
}
