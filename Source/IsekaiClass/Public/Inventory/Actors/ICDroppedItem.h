#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "World/State/Types/ICWorldProgressStateTypes.h"
#include "ICDroppedItem.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class AICPlayerController;

UCLASS(Blueprintable, meta=(ToolTip="WorldState의 지속 드롭 레코드를 화면에 표시하고 줍기를 처리하는 액터입니다. 액터 자체가 원본이 아니므로 제거/레벨 전환과 데이터 삭제를 구분합니다. Controller가 드롭 목록과 동기화합니다.", ShortToolTip="WorldState의 지속 드롭 레코드를 화면에 표시하고 줍기를 처리하는 액터입니다."))
class ISEKAICLASS_API AICDroppedItem : public AActor
{
    GENERATED_BODY()
public:
    AICDroppedItem();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="표시 액터가 참조하는 지속 드롭 GUID입니다. WorldState.Drops의 같은 레코드를 줍기/수명 처리에서 찾습니다.\n입력 자료형: 인스턴스 GUID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FGuid RecordID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="드롭 레코드 GUID를 표시하는 파생 이름입니다. 실제 원본은 RecordID/WorldState.Drops이며 임의로 편집하지 않습니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName DropID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="재료 묶음 또는 드롭 표시 아이템입니다. Production에서는 Item ID/수량, DropActor에서는 실제 인스턴스 표시 값이며 원본은 WorldState입니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FPlayerItemInstance Item;
    UPROPERTY(VisibleAnywhere, meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: 객체 참조 (UStaticMeshComponent).")) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, meta=(ToolTip="월드 안내용 TextRender 컴포넌트입니다. 원본 아이템/상호작용 표시 문자열과 구분합니다.\n입력 자료형: 객체 참조 (UTextRenderComponent).")) TObjectPtr<UTextRenderComponent> Label;
    void Initialize(const FICDropState& Record);
    void TryPickup(AICPlayerController* Controller);
};
