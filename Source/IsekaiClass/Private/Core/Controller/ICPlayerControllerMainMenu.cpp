#include "Core/Controller/ICPlayerController.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Core/ICGameMode.h"
#include "Core/ICGameState.h"
#include "Debug/ICLog.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Engine/World.h"

bool AICPlayerController::IsMainMenuLevel() const
{
    IC_TRACE_FUNCTION();
    const AICGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AICGameMode>() : nullptr;
    return Mode && Mode->IsMainMenuLevel();
}

void AICPlayerController::InitializeMainMenu()
{
    IC_TRACE_FUNCTION();
    Menu = TEXT("MainMenu");
    if (UICTimeSubsystem* Time = GetTime())
    {
        Time->SetRunning(false);
        Time->SetMenuPaused(false);
    }
    if (UICSaveSubsystem* Saves = GetSaves()) Saves->SetAutoSaveSuspended(true);
    if (AICGameState* State = GetWorld()->GetGameState<AICGameState>()) State->SetPhase(EICGamePhase::Menu);
    bShowMouseCursor = true;
    if (!MainMenuWidgetClass)
    {
        UE_LOG(LogICUI, Error, TEXT("[MainMenu.WidgetMissing] Controller=%s: BP_ICPlayerController의 MainMenuWidgetClass를 지정하세요."), *GetNameSafe(this));
        return;
    }
    MainMenuWidget = CreateWidget<UICMainMenuWidget>(this, MainMenuWidgetClass);
    if (!MainMenuWidget)
    {
        UE_LOG(LogICUI, Error, TEXT("[MainMenu.WidgetFailed] Class=%s"), *GetNameSafe(MainMenuWidgetClass.Get()));
        return;
    }
    MainMenuWidget->SetIsFocusable(true);
    MainMenuWidget->AddToViewport();
    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->SetMenuContext(this,MainMenuWidget,true);
    MainMenuWidget->FocusStartButton();
    UE_LOG(LogICUI, Log, TEXT("[MainMenu.Opened] Controller=%s Widget=%s: 공통 컨트롤러에서 메인 메뉴 표시"), *GetNameSafe(this), *GetNameSafe(MainMenuWidget));
}
