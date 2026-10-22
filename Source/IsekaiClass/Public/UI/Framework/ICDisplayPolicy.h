#pragma once
#include "CoreMinimal.h"

namespace ICDisplayPolicy
{
    inline constexpr float DesignWidth = 1920.f;
    inline constexpr float DesignHeight = 1080.f;
    inline constexpr float AspectRatio = DesignWidth / DesignHeight;
    ISEKAICLASS_API FIntRect ContentRect(FIntPoint ViewportSize);
}

