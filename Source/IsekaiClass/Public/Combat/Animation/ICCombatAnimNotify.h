#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ICCombatAnimNotify.generated.h"
UCLASS()
class ISEKAICLASS_API UICCombatAnimNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Combat") FName Event=TEXT("AN_EffectCommit");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Combat",meta=(ClampMin="0")) int32 HitIndex=0;
};
