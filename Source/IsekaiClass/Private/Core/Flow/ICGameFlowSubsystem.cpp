#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICLoadingScreenSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

bool FICCreationAnswers::IsNameValid(const FString& Value)
{
    const FString Clean = Value.TrimStartAndEnd();
    if (Clean.IsEmpty() || Clean.Len() > 12)
        return false;
    for (TCHAR Ch : Value)
        if (Ch < TEXT(' ') || Ch == 0x2028 || Ch == 0x2029)
            return false;
    return true;
}

bool FICCreationAnswers::IsComplete() const
{
    return IsNameValid(Name) && Gender >= 0 && Gender < 2 && Story >= 0 && Story < 4 && Music >= 0 && Music < 4 &&
           Happiness >= 0 && Happiness < 4;
}

UDataTable* UICGameFlowSubsystem::GetOptions() const
{
    auto* Self = const_cast<UICGameFlowSubsystem*>(this);
    if (!Self->Options)
        Self->Options = GetDefault<UICGameFlowSettings>()->CreationOptions.LoadSynchronous();
    return Self->Options;
}

const FICCreationOptionRow* UICGameFlowSubsystem::FindOption(int32 Question, int32 Option) const
{
    const UDataTable* Table = GetOptions();
    if (!Table || Table->GetRowStruct() != FICCreationOptionRow::StaticStruct())
        return nullptr;
    return Table->FindRow<FICCreationOptionRow>(FName(*FString::Printf(TEXT("Q%d_%d"), Question + 1, Option + 1)),
                                                TEXT("Creation"), false);
}

FICCreationOptionRow UICGameFlowSubsystem::GetOption(int32 Question, int32 Option) const
{
    const auto* Row = FindOption(Question, Option);
    return Row ? *Row : FICCreationOptionRow();
}

void UICGameFlowSubsystem::BeginQuestions()
{
    Answers = FICCreationAnswers();
    LastError.Empty();
    bPendingNewGame = false;
    Phase = EICGameFlowPhase::Questions;
    OnChanged.Broadcast();
}

void UICGameFlowSubsystem::SetAnswers(const FICCreationAnswers& Value)
{
    Answers = Value;
}

bool UICGameFlowSubsystem::BeginNewGame(AICPlayerController* Controller)
{
    if (!Controller || !Answers.IsComplete() || bPendingNewGame)
        return false;
    const auto* Reward = FindOption(3, Answers.Music);
    if (!Reward || Reward->WeaponItemID.IsNone() || Reward->SkillID.IsNone())
    {
        Fail(TEXT("시작 보상 데이터가 없습니다. DT_IC_CreationOptions를 확인하세요."));
        return false;
    }
    const FString Map = GetDefault<UICGameFlowSettings>()->PlayMap.GetLongPackageName();
    if (!FPackageName::DoesPackageExist(Map))
    {
        Fail(TEXT("플레이 맵을 찾을 수 없습니다."));
        return false;
    }
    auto* Saves = GetGameInstance()->GetSubsystem<UICSaveSubsystem>();
    auto* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    auto* Time = GetGameInstance()->GetSubsystem<UICTimeSubsystem>();
    if (!Saves || !State || !Time || Saves->bSaving)
    {
        Fail(TEXT("저장 시스템이 준비되지 않았습니다."));
        return false;
    }
    if (!State->RestoreState(FICWorldState()))
    {
        Fail(TEXT("새 게임 상태를 초기화할 수 없습니다."));
        return false;
    }
    Saves->ResetSession();
    Saves->SetAutoSaveSuspended(true);
    Time->ResetClock();
    bPendingNewGame = true;
    Phase = EICGameFlowPhase::Loading;
    LastError.Empty();
    OnChanged.Broadcast();
    if (Controller->GetLocalPlayer())
        if (auto* UI = Controller->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())
            UI->CloseAllScreens();
    Controller->SetMenu(TEXT("Loading"));
    Time->PrepareForTravel();
    GetGameInstance()->GetSubsystem<UICLoadingScreenSubsystem>()->Arm();
    UGameplayStatics::SetGamePaused(Controller, false);
    UGameplayStatics::OpenLevel(Controller, FName(*Map));
    return true;
}

void UICGameFlowSubsystem::BeginInitialization(bool bRetry)
{
    if (!bPendingNewGame || (Phase == EICGameFlowPhase::Failed && !bRetry))
        return;
    LastError.Empty();
    Phase = EICGameFlowPhase::Initializing;
    OnChanged.Broadcast();
}

void UICGameFlowSubsystem::CompleteNewGame()
{
    bPendingNewGame = false;
    Phase = EICGameFlowPhase::Playing;
    LastError.Empty();
    OnChanged.Broadcast();
}

void UICGameFlowSubsystem::Fail(const FString& Reason)
{
    LastError = Reason;
    Phase = EICGameFlowPhase::Failed;
    OnChanged.Broadcast();
}

void UICGameFlowSubsystem::ReturnToTitle(AICPlayerController* Controller)
{
    if (!Controller)
        return;
    auto* Saves = GetGameInstance()->GetSubsystem<UICSaveSubsystem>();
    if (Saves && Saves->bSaving)
        return;
    bPendingNewGame = false;
    Phase = EICGameFlowPhase::Title;
    LastError.Empty();
    if (Controller->IsMainMenuLevel())
        Controller->SetMenu(TEXT("MainMenu"));
    else
    {
        if (Controller->GetLocalPlayer())
            if (auto* UI = Controller->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())
                UI->CloseAllScreens();
        GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->PrepareForTravel();
        UGameplayStatics::OpenLevelBySoftObjectPtr(Controller, GetDefault<UICGameFlowSettings>()->TitleMap);
    }
    OnChanged.Broadcast();
}
