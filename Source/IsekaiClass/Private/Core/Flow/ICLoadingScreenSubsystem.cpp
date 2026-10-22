#include "Core/Flow/ICLoadingScreenSubsystem.h"
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
    // Slate's loading thread outlives the old world's GC; keep its UObject font alive.
    LoadingFont=LoadObject<UFont>(nullptr,TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI.FONT_IC_UI"));
    Screen.bAutoCompleteWhenLoadingCompletes = true;
    Screen.bMoviesAreSkippable = false;
    Screen.MinimumLoadingScreenDisplayTime = 0.4f;
    Screen.WidgetLoadingScreen =
        SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor::Black)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(ICDisplayPolicy::DesignWidth).HeightOverride(ICDisplayPolicy::DesignHeight)
        [
        SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.06f))
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
                [SNew(SVerticalBox) +
                 SVerticalBox::Slot()
                     .AutoHeight()
                     .HAlign(HAlign_Center)
                     .Padding(0, 0, 0, 28)[SNew(STextBlock)
                                               .Text(ICText::Get(TEXT("Title")))
                                               .Font(ICFrontendLayout::Font(36, true))] +
                 SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 18)[SNew(SThrobber)] +
                 SVerticalBox::Slot().AutoHeight().HAlign(
                     HAlign_Center)[SNew(STextBlock)
                                        .Text(ICText::Get(TEXT("Loading")))
                                        .Font(ICFrontendLayout::Font(20))]]]]];
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
