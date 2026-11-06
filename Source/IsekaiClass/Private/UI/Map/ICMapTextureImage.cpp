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
    FVector2D UVMin=FVector2D::ZeroVector;
    FVector2D UVMax=FVector2D(1.0,1.0);
    virtual FVector2D ComputeDesiredSize(float) const override{return {256,128};}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool) const override
    {
        if(!Brush.GetResourceObject())return L;
        FVector2D LocalMin,LocalMax;
        if(!UICMapTextureImage::ClipUVRegion(UVMin,UVMax,LocalMin,LocalMax))return L;
        const int32 Columns=bFade?32:1,Rows=bFade?16:1;
        TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
        const auto Transform=G.GetAccumulatedRenderTransform();const FVector2D Size=G.GetLocalSize();
        for(int32 Y=0;Y<=Rows;++Y)for(int32 X=0;X<=Columns;++X)
        {
            const FVector2D LocalUV(FMath::Lerp(LocalMin.X,LocalMax.X,double(X)/Columns),FMath::Lerp(LocalMin.Y,LocalMax.Y,double(Y)/Rows));
            const FVector2D TextureUV(
                FMath::Lerp(UVMin.X,UVMax.X,LocalUV.X),
                FMath::Lerp(UVMin.Y,UVMax.Y,LocalUV.Y));
            auto Color=Style.GetColorAndOpacityTint();if(bFade)Color.A*=UICMapWidget::MinimapOpacity(LocalUV);
            Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,FVector2f(LocalUV*Size),FVector2f(TextureUV),Color.ToFColor(true)));
        }
        for(int32 Y=0;Y<Rows;++Y)for(int32 X=0;X<Columns;++X)
        {const SlateIndex I=Y*(Columns+1)+X;Indices.Append({I,SlateIndex(I+1),SlateIndex(I+Columns+1),SlateIndex(I+1),SlateIndex(I+Columns+2),SlateIndex(I+Columns+1)});}
        FSlateDrawElement::MakeCustomVerts(E,L,FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush),Vertices,Indices,nullptr,0,0);return L;
    }
};
TSharedRef<SWidget> UICMapTextureImage::RebuildWidget(){Image=SNew(SICMapTextureImage);SynchronizeProperties();return Image.ToSharedRef();}
void UICMapTextureImage::SynchronizeProperties()
{Super::SynchronizeProperties();if(Image){Image->Brush.SetResourceObject(Texture);Image->Brush.DrawAs=ESlateBrushDrawType::Image;Image->bFade=bFadeEdges;Image->UVMin=UVMin;Image->UVMax=UVMax;Image->Invalidate(EInvalidateWidgetReason::Paint);}}
void UICMapTextureImage::SetTexture(UTexture2D* Value){if(Texture!=Value){Texture=Value;SynchronizeProperties();}}
void UICMapTextureImage::SetUVRegion(FVector2D Min,FVector2D Max)
{
    if(!UVMin.Equals(Min)||!UVMax.Equals(Max)){UVMin=Min;UVMax=Max;SynchronizeProperties();}
}
bool UICMapTextureImage::ClipUVRegion(FVector2D Min,FVector2D Max,FVector2D& LocalMin,FVector2D& LocalMax)
{
    const FVector2D Span=Max-Min;
    if(Span.X<=UE_SMALL_NUMBER||Span.Y<=UE_SMALL_NUMBER)return false;
    LocalMin={FMath::Clamp(-Min.X/Span.X,0.,1.),FMath::Clamp(-Min.Y/Span.Y,0.,1.)};
    LocalMax={FMath::Clamp((1.-Min.X)/Span.X,0.,1.),FMath::Clamp((1.-Min.Y)/Span.Y,0.,1.)};
    return LocalMax.X>LocalMin.X&&LocalMax.Y>LocalMin.Y;
}
void UICMapTextureImage::ReleaseSlateResources(bool Children){Super::ReleaseSlateResources(Children);Image.Reset();}
