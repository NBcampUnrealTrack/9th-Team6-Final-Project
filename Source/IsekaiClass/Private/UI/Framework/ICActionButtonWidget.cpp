#include "UI/Framework/ICActionButtonWidget.h"
#include "UI/Framework/ICUITheme.h"
#include "Settings/ICGameUserSettings.h"
#include "Components/TextBlock.h"

void UICActionButtonWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    if(BTN_Select)BTN_Select->OnClicked.AddUniqueDynamic(this,&ThisClass::Clicked);
}
void UICActionButtonWidget::SetCaption(const FText& Text){if(TXT_Label)TXT_Label->SetText(Text);}
void UICActionButtonWidget::SetSelected(bool Value){ICUITheme::SetSelected(BTN_Select,Value);}
void UICActionButtonWidget::NativeOnAddedToFocusPath(const FFocusEvent& Event)
{Super::NativeOnAddedToFocusPath(Event);ICUITheme::SetSelected(BTN_Select,true);}
void UICActionButtonWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& Event)
{Super::NativeOnRemovedFromFocusPath(Event);ICUITheme::SetSelected(BTN_Select,false);}
FReply UICActionButtonWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(UICGameUserSettings::MenuKey(Event.GetKey())==EKeys::Enter)
    {if(!Event.IsRepeat())Clicked();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
