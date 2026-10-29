#include "UI/Framework/ICLoadingWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Components/Border.h"
#include "UI/Framework/ICPrologueWidget.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Player/Character/ICPlayerAppearance.h"
#include "Components/SkeletalMeshComponent.h"

void UICLoadingWidget::NativeConstruct()
{
    Super::NativeConstruct();UpdateCanTick();
}

void UICLoadingWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!Flow->bPendingNewGame||Flow->Phase!=EICGameFlowPhase::Initializing||!PC||!PC->bCreated||PC->Menu!=TEXT("Loading"))return;
    auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn());auto* Mesh=Player?Player->GetMesh():nullptr;
    if(!ICPlayerAppearance::PrepareFirstPose(Mesh))
        {FirstPoseFrame=MAX_uint64;PreparedAnimation.Reset();return;}
    if(PreparedAnimation.Get()!=Mesh->GetAnimInstance()||FirstPoseFrame==MAX_uint64)
        {PreparedAnimation=Mesh->GetAnimInstance();FirstPoseFrame=GFrameCounter;return;}
    // Keep the cover through subsequent rendered frames, so skinning receives
    // the evaluated pose before the existing world-reveal fade begins.
    if(GFrameCounter-FirstPoseFrame>=2)UICPrologueWidget::Show(PC,true);
}
void UICLoadingWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    SetIsFocusable(true);
    GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->OnChanged.AddUniqueDynamic(this, &ThisClass::Refresh);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Retry"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Retry);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Title"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Title);
    Refresh();
}
void UICLoadingWidget::NativeDestruct()
{
    if (GetGameInstance())
        GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->OnChanged.RemoveDynamic(this, &ThisClass::Refresh);
    Super::NativeDestruct();
}
UWidget* UICLoadingWidget::NativeGetDesiredFocusTarget() const
{
    return GetWidgetFromName(TEXT("BTN_Retry"));
}
void UICLoadingWidget::Refresh()
{
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    const bool Failed = Flow->Phase == EICGameFlowPhase::Failed;
    if(auto* Background=Cast<UBorder>(GetWidgetFromName(TEXT("Background"))))Background->SetBrushColor(FLinearColor(.018f,.026f,.045f));
    if(auto* Card=GetWidgetFromName(TEXT("Card")))Card->SetVisibility(ESlateVisibility::Visible);
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Status"))))
        Text->SetText(Failed ? FText::FromString(Flow->LastError) : ICFrontendLayout::Text(Flow->Phase==EICGameFlowPhase::Loading?TEXT("Loading"):TEXT("Preparing")));
    if (auto* Spinner = GetWidgetFromName(TEXT("LoadingSpinner")))
        Spinner->SetVisibility(Failed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    for (const auto* Name : {TEXT("BTN_Retry"), TEXT("BTN_Title")})
        if (auto* Button = GetWidgetFromName(Name))
            Button->SetVisibility(Failed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (Failed)
        RequestRefreshFocus();
}
void UICLoadingWidget::Retry()
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->BeginInitialization(true);
        PC->RetryNewGame();
    }
}
void UICLoadingWidget::Title()
{
    GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->ReturnToTitle(
        Cast<AICPlayerController>(GetOwningPlayer()));
}
