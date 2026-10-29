#include "UI/Map/ICMapTextureImage.h"
#include "UI/Map/ICMapWidget.h"
#include "Engine/Texture2D.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

class SICMapTextureImage : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SICMapTextureImage){} SLATE_END_ARGS()
    void Construct(const FArguments&){}
    FSlateBrush Brush;
    bool bFade=false;
    virtual FVector2D ComputeDesiredSize(float) const override{return {256,128};}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool) const override
    {
        if(!Brush.GetResourceObject())return L;
        if(!bFade){FSlateDrawElement::MakeBox(E,L,G.ToPaintGeometry(),&Brush,ESlateDrawEffect::None,Style.GetColorAndOpacityTint());return L;}
        constexpr int32 Columns=32,Rows=16;
        TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
        const auto Transform=G.GetAccumulatedRenderTransform();const FVector2D Size=G.GetLocalSize();
        for(int32 Y=0;Y<=Rows;++Y)for(int32 X=0;X<=Columns;++X)
        {
            const FVector2D UV(double(X)/Columns,double(Y)/Rows);
            auto Color=Style.GetColorAndOpacityTint();Color.A*=UICMapWidget::MinimapOpacity(UV);
            Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,FVector2f(UV*Size),FVector2f(UV),Color.ToFColor(true)));
        }
        for(int32 Y=0;Y<Rows;++Y)for(int32 X=0;X<Columns;++X)
        {const SlateIndex I=Y*(Columns+1)+X;Indices.Append({I,SlateIndex(I+1),SlateIndex(I+Columns+1),SlateIndex(I+1),SlateIndex(I+Columns+2),SlateIndex(I+Columns+1)});}
        FSlateDrawElement::MakeCustomVerts(E,L,FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush),Vertices,Indices,nullptr,0,0);return L;
    }
};
TSharedRef<SWidget> UICMapTextureImage::RebuildWidget(){Image=SNew(SICMapTextureImage);SynchronizeProperties();return Image.ToSharedRef();}
void UICMapTextureImage::SynchronizeProperties()
{Super::SynchronizeProperties();if(Image){Image->Brush.SetResourceObject(Texture);Image->Brush.DrawAs=ESlateBrushDrawType::Image;Image->bFade=bFadeEdges;Image->Invalidate(EInvalidateWidgetReason::Paint);}}
void UICMapTextureImage::SetTexture(UTexture2D* Value){if(Texture!=Value){Texture=Value;SynchronizeProperties();}}
void UICMapTextureImage::ReleaseSlateResources(bool Children){Super::ReleaseSlateResources(Children);Image.Reset();}
