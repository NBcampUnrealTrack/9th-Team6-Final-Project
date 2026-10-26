#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "UI/Gameplay/ICGameplayLayout.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "Widgets/SLeafWidget.h"

class SICRadialCooldown : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SICRadialCooldown) {} SLATE_END_ARGS()
    void Construct(const FArguments&) {}
    float Progress = 1, Thickness = 14;
    FLinearColor Fill;
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(UICRadialCooldownBar::Diameter); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        const FVector2D Size=Geometry.GetLocalSize();
        const float Width=FMath::Clamp(Thickness,1.f,float(FMath::Min(Size.X,Size.Y)*.45));
        const float Radius=FMath::Max(0.,FMath::Min(Size.X,Size.Y)*.5-Width*.5-1);
        const auto DrawArc=[&](float Fraction,FLinearColor Color,int32 Z)
        {
            if(Fraction<=0||Radius<=0)return;
            const int32 Segments=FMath::Max(1,FMath::CeilToInt(128*Fraction));
            TArray<FVector2D> Points;Points.Reserve(Segments+1);
            for(int32 Index=0;Index<=Segments;++Index)
                Points.Add(Size*.5+UICRadialCooldownBar::ClockwisePoint(Fraction*Index/Segments)*Radius);
            FSlateDrawElement::MakeLines(Elements,Z,Geometry.ToPaintGeometry(),Points,
                ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,Width);
        };
        // The arc itself grows clockwise into empty space; there is no full-ring track.
        DrawArc(Progress,Fill,Layer);
        return Layer;
    }
};

FVector2D UICRadialCooldownBar::ClockwisePoint(float Fraction)
{
    const double Angle=2*PI*Fraction;
    return {FMath::Sin(Angle),-FMath::Cos(Angle)};
}

TSharedRef<SWidget> UICRadialCooldownBar::RebuildWidget()
{
    Ring=SNew(SICRadialCooldown);
    SynchronizeProperties();
    return Ring.ToSharedRef();
}

void UICRadialCooldownBar::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if(!Ring)return;
    Ring->Progress=FMath::IsFinite(Progress)?FMath::Clamp(Progress,0.f,1.f):0.f;
    Ring->Thickness=Thickness;Ring->Fill=FillColor;
    Ring->Invalidate(EInvalidateWidgetReason::Paint);
}

void UICRadialCooldownBar::SetProgress(float Value)
{
    const float Clamped=FMath::IsFinite(Value)?FMath::Clamp(Value,0.f,1.f):0.f;
    if(Progress==Clamped)return;
    Progress=Clamped;SynchronizeProperties();
}

void UICRadialCooldownBar::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);Ring.Reset();
}

void UICMovementCooldownWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(WidgetTree&&!WidgetTree->RootWidget)ICGameplayLayout::Build(WidgetTree,TEXT("MovementCooldown"));
    RefreshCooldown();
}

void UICMovementCooldownWidget::NativePreConstruct()
{
    Super::NativePreConstruct();RefreshCooldown();
}

void UICMovementCooldownWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);RefreshCooldown();
}

void UICMovementCooldownWidget::RefreshCooldown()
{
    const auto* Player=Cast<AICPlayerCharacter>(GetOwningPlayerPawn());
    const auto* ASC=Player?Player->GetICAbilitySystem():nullptr;
    RemainingSeconds=ASC?ASC->GetMovementCooldownRemaining():0;
    Progress=ASC?ASC->GetMovementCooldownProgress():1.f;
    bReady=RemainingSeconds<=0;
    bool bShow=ASC&&ASC->IsAlive()&&!bReady;
    if(bShow)
    {
        if(auto* Placement=Cast<UCanvasPanelSlot>(Slot))
        {
            FVector2D Screen;
            bShow=GetParent()&&UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
                GetOwningPlayer(),Player->GetActorLocation(),Screen,false);
            if(bShow)
            {
                const auto ViewportGeometry=UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
                Placement->SetPosition(GetParent()->GetCachedGeometry().AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(Screen)));
            }
        }
    }
    if(auto* Bar=Cast<UICRadialCooldownBar>(GetWidgetFromName(TEXT("RING_Cooldown"))))
    {
        Bar->SetProgress(IsDesignTime()?.6f:Progress);
        // Keep the transparent owner ticking so a later dash can reveal the arc again.
        Bar->SetVisibility(bShow||IsDesignTime()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    }
}
