#include "UI/Map/ICMapPlayerMarkerShape.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

class SICMapPlayerMarkerShape : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SICMapPlayerMarkerShape){} SLATE_END_ARGS()
    void Construct(const FArguments&){}
    FLinearColor FillColor=FLinearColor(.4f,1.f,.4f,1.f);
    FLinearColor OutlineColor=FLinearColor(.02f,.08f,.03f,1.f);
    virtual FVector2D ComputeDesiredSize(float) const override{return {20,36};}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geometry,const FSlateRect&,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool) const override
    {
        const FVector2D Size=Geometry.GetLocalSize();
        // (0.04 + 0.73 + 0.73) / 3 = 0.5, so the triangle centroid is
        // exactly the widget/player centre while its point faces upward.
        const TArray<FVector2D> Points=UICMapPlayerMarkerShape::TrianglePoints(Size);
        const auto Transform=Geometry.GetAccumulatedRenderTransform();
        const auto Tint=FillColor*Style.GetColorAndOpacityTint();
        TArray<FSlateVertex> Vertices;
        for(const auto& Point:Points)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,FVector2f(Point),FVector2f::ZeroVector,Tint.ToFColor(true)));
        const FSlateBrush* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        FSlateDrawElement::MakeCustomVerts(Elements,Layer,FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush),Vertices,{0,1,2},nullptr,0,0);
        TArray<FVector2D> Outline=Points;Outline.Add(Points[0]);
        FSlateDrawElement::MakeLines(Elements,Layer+1,Geometry.ToPaintGeometry(),Outline,ESlateDrawEffect::None,OutlineColor*Style.GetColorAndOpacityTint(),true,1.5f);
        return Layer+1;
    }
};

TArray<FVector2D> UICMapPlayerMarkerShape::TrianglePoints(FVector2D Size)
{return {{Size.X*.5,Size.Y*.04},{Size.X*.22,Size.Y*.73},{Size.X*.78,Size.Y*.73}};}
TSharedRef<SWidget> UICMapPlayerMarkerShape::RebuildWidget(){Shape=SNew(SICMapPlayerMarkerShape);SynchronizeProperties();return Shape.ToSharedRef();}
void UICMapPlayerMarkerShape::SynchronizeProperties(){Super::SynchronizeProperties();if(Shape){Shape->FillColor=FillColor;Shape->OutlineColor=OutlineColor;Shape->Invalidate(EInvalidateWidgetReason::Paint);}}
void UICMapPlayerMarkerShape::ReleaseSlateResources(bool Children){Super::ReleaseSlateResources(Children);Shape.Reset();}
