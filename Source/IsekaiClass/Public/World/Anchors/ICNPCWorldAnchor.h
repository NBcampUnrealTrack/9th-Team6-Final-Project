#pragma once

#include "CoreMinimal.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "ICNPCWorldAnchor.generated.h"

class UArrowComponent;
class UTextRenderComponent;

/**
 * NPC 일과 앵커용 월드 액터입니다.
 *
 * AICWorldAnchor의 등록/해제 동작을 그대로 사용하며, 에디터에서 배치 방향과
 * AnchorID를 쉽게 확인할 수 있도록 화살표와 라벨만 추가합니다. 두 표시 컴포넌트는
 * 게임 중에는 숨겨집니다.
 */
UCLASS(Blueprintable, meta=(ToolTip="NPC 일과 앵커입니다. 기존 월드 앵커 등록 기능에 에디터용 방향 화살표와 AnchorID 라벨을 더합니다."))
class ISEKAICLASS_API AICNPCWorldAnchor : public AICWorldAnchor
{
    GENERATED_BODY()

public:
    AICNPCWorldAnchor();
    virtual void OnConstruction(const FTransform& Transform) override;

    /** NPC가 도착 후 바라볼 기준 방향입니다. 앵커 액터 자체의 회전과 맞춰 배치하세요. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|NPC Anchor")
    TObjectPtr<UArrowComponent> FacingArrow;

    /** 에디터에서 확인하는 현재 AnchorID 라벨입니다. 게임 중에는 표시하지 않습니다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|NPC Anchor")
    TObjectPtr<UTextRenderComponent> AnchorLabel;

private:
    void RefreshEditorLabel();
};
