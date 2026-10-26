#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ICWeaponAnimNotify.generated.h"

/** Reveals/hides only the weapon belonging to the active switch montage instance. */
UCLASS()
class ISEKAICLASS_API UICWeaponAnimNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Weapons") bool bDraw = true;
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;
};
