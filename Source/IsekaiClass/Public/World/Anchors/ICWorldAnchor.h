#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICWorldAnchor.generated.h"
UCLASS(Blueprintable,meta=(ToolTip="로드된 셀의 앵커를 스폰 시스템에 등록하는 월드 액터입니다. BeginPlay 등록·EndPlay 해제와 실제 배치 좌표를 제공하며 이동 캐릭터 상태 원본은 보관하지 않습니다."))
class ISEKAICLASS_API AICWorldAnchor : public AActor
{
    GENERATED_BODY()
public:
    AICWorldAnchor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|Anchor",meta=(ToolTip="Anchor DT의 안정적인 ID입니다. 같은 월드에서 중복 등록하면 실패합니다. 좌표는 실제 액터 배치를 사용하고 종류·안전 조건은 DT에서 읽습니다.")) FName AnchorID;
};
