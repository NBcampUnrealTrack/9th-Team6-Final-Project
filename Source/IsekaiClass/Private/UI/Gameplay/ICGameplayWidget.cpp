#include "UI/Gameplay/ICGameplayWidget.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"

AICPlayerController* UICGameplayWidget::GetICPlayerController() const
{
    IC_TRACE_FUNCTION();
    return Cast<AICPlayerController>(GetOwningPlayer());
}

void UICGameplayWidget::RefreshCurrentScreen() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, VeryVerbose, TEXT("[GameplayUI.Refresh] Widget=%s Controller=%s"),*GetNameSafe(this),*GetNameSafe(GetOwningPlayer()));OnRefreshGameplayUI(); }
