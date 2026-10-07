#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ICCombatAnimNotify.generated.h"
UCLASS(meta=(ToolTip="GAS 행동의 몽타주 인스턴스 ID와 행동 GUID를 검사하는 공통 애니메이션 알림입니다. 같은 타격 번호는 한 번만 처리하며 중단된 이전 몽타주 알림은 거부합니다."))
class ISEKAICLASS_API UICCombatAnimNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Combat",meta=(ToolTip="AN_HitStart·AN_HitEnd·AN_ProjectileFire·AN_EffectCommit·AN_IFrameStart·AN_IFrameEnd·AN_CancelWindowStart·AN_CancelWindowEnd 중 실행 사건입니다. 반드시 행동을 재생하는 몽타주에 배치합니다.")) FName Event=TEXT("AN_EffectCommit");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Combat",meta=(ClampMin="0",ToolTip="Skill.Hits의 반복 횟수를 펼친 0 기반 타격 번호입니다. 같은 행동 GUID·타격 번호·대상에는 피해를 중복 적용하지 않습니다.")) int32 HitIndex=0;
};
