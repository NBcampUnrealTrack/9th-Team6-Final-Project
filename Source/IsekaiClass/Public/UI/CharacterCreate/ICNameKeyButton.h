#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "ICNameKeyButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNameKeyChosen,int32,Index);
UCLASS()
class ISEKAICLASS_API UICNameKeyButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 KeyIndex=0;
    UPROPERTY() FICNameKeyChosen OnKeyChosen;
    void BindKey() { OnClicked.AddUniqueDynamic(this,&ThisClass::Choose); }
private:
    UFUNCTION() void Choose() { OnKeyChosen.Broadcast(KeyIndex); }
};
