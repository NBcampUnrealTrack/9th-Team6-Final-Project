#include "UI/Framework/ICDisplayPolicy.h"

FIntRect ICDisplayPolicy::ContentRect(FIntPoint Size)
{
    const int32 Width=FMath::Max(0,Size.X),Height=FMath::Max(0,Size.Y);
    FIntPoint Content(Width,Height);
    if(Width>Height*AspectRatio)Content.X=FMath::RoundToInt(Height*AspectRatio);
    else Content.Y=FMath::RoundToInt(Width/AspectRatio);
    const FIntPoint Origin((Width-Content.X)/2,(Height-Content.Y)/2);
    return FIntRect(Origin,Origin+Content);
}

