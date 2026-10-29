#include "Core/Flow/ICLoadingScreenSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UI/Framework/ICText.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MoviePlayer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SThrobber.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "Engine/Font.h"

void UICLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeforeMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::AfterMap);
    if (GEngine)
        GEngine->OnTravelFailure().AddUObject(this, &ThisClass::TravelFailed);
}

void UICLoadingScreenSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    if (GEngine)
        GEngine->OnTravelFailure().RemoveAll(this);
    Super::Deinitialize();
}

void UICLoadingScreenSubsystem::Arm()
{
    bArmed = true;
}

void UICLoadingScreenSubsystem::BeforeMap(const FString& MapName)
{
    if (!bArmed || IsRunningDedicatedServer() || IsRunningCommandlet())
        return;
    if (!FSlateApplication::IsInitialized() || !IsMoviePlayerEnabled() ||
        (GetWorld() && GetWorld()->WorldType == EWorldType::PIE))
        return;
    FLoadingScreenAttributes Screen;
    auto* Type=LoadClass<UUserWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_LoadingMovie.WBP_IC_LoadingMovie_C"));
    if(!ensureMsgf(Type,TEXT("WBP_IC_LoadingMovie is required")))return;
    // The GameInstance owns the saved WBP throughout map travel.
    MovieWidget=CreateWidget<UUserWidget>(GetGameInstance(),Type);if(!MovieWidget)return;
    if(GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->bPendingNewGame)
        if(auto* Body=MovieWidget->GetWidgetFromName(TEXT("MovieBody")))Body->SetVisibility(ESlateVisibility::Collapsed);
    Screen.bAutoCompleteWhenLoadingCompletes=true;Screen.bMoviesAreSkippable=false;Screen.MinimumLoadingScreenDisplayTime=.4f;
    Screen.WidgetLoadingScreen=MovieWidget->TakeWidget();
    GetMoviePlayer()->SetupLoadingScreen(Screen);
}

void UICLoadingScreenSubsystem::AfterMap(UWorld* World)
{
    if (!bArmed || !World || World->GetGameInstance() != GetGameInstance())
        return;
    bArmed = false;
    GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->BeginInitialization();
}

void UICLoadingScreenSubsystem::TravelFailed(UWorld* World, ETravelFailure::Type Failure, const FString& Reason)
{
    if (!bArmed || (World && World->GetGameInstance() != GetGameInstance()))
        return;
    bArmed = false;
    if (IsMoviePlayerEnabled())
        GetMoviePlayer()->StopMovie();
    GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->Fail(*ICText::Get(TEXT("Native_43628d6993c8")).ToString() + Reason);
}
