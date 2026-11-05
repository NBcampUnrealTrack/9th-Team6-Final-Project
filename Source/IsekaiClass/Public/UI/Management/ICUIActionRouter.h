#pragma once

#include "CoreMinimal.h"
#include "Input/CommonUIActionRouterBase.h"
#include "ICUIActionRouter.generated.h"

/** Preserve the physical pointer while keyboard/gamepad focus navigates our menus. */
UCLASS()
class ISEKAICLASS_API UICUIActionRouter : public UCommonUIActionRouterBase
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<FCommonAnalogCursor> MakeAnalogCursor() const override;
};
