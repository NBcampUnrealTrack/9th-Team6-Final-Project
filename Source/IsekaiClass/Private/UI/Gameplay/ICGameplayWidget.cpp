#include "UI/Gameplay/ICGameplayWidget.h"
#include "Core/Controller/ICPlayerController.h"

AICPlayerController* UICGameplayWidget::GetICPlayerController() const
{
    return Cast<AICPlayerController>(GetOwningPlayer());
}

void UICGameplayWidget::RefreshCurrentScreen() { OnRefreshGameplayUI(); }
