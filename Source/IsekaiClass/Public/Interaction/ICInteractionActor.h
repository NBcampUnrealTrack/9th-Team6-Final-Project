#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICInteractionActor.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class AICPlayerController;
UCLASS(Blueprintable, meta=(ToolTip="상호작용 대상의 메시·안내 문구·동작·작업 앵커/보관함/지급 아이템 설정입니다. 플레이어 컨트롤러가 거리/시야를 검사한 뒤 TryInteract를 호출합니다.", ShortToolTip="상호작용 대상의 메시·안내 문구·동작·작업 앵커/보관함/지급 아이템 설정입니다."))
class ISEKAICLASS_API AICInteractionActor : public AActor
{
    GENERATED_BODY()
public:
    AICInteractionActor();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="타격 판정 모양 또는 상호작용 메시 컴포넌트입니다. 타입에 따라 범위/충돌 또는 표시 메시를 설정합니다.\n입력 자료형: 객체 참조 (UStaticMeshComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="월드 안내용 TextRender 컴포넌트입니다. 원본 아이템/상호작용 표시 문자열과 구분합니다.\n입력 자료형: 객체 참조 (UTextRenderComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction", meta=(ToolTip="대상 상호작용 안내 FText입니다. String Table을 연결하면 현지화 식별자를 유지합니다.\n입력 자료형: 현지화 가능한 텍스트.")) FText InteractionText;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="현재 지원 동작 키는 Storage/Sleep/Kitchen/Forge/Loot/Quest입니다. Storage는 ContainerID/접근, 작업대는 WorkAnchorID, Loot/Quest는 ItemID/ItemCount를 작성합니다. Quest 동작은 현재 아이템 지급이며 전체 퀘스트 실행기가 아닙니다.\n입력 자료형: 이름/ID.")) FName Action;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction", meta=(ToolTip="작업/서비스 Anchor ID입니다. 생산 시작은 정의 존재와 Production.WorkAnchorIDs 허용 목록을 검사합니다.\n입력 자료형: 이름/ID.")) FName WorkAnchorID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="컨테이너의 안정적인 저장 키입니다. 집/공유 그룹/소유자와 함께 설정하고 여러 물체가 같은 내용물을 공유할 때만 같은 키를 사용합니다.\n입력 자료형: 이름/ID.")) FName ContainerID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="플레이어 집 공유 보관함 상호작용입니다. 체크 시 기본 공유 키 경로를 사용하며 임의 ContainerID/NPC 소유와 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bHome=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="현재 물체 접근 허용 설정입니다. 체크만으로 NPC 관계/소유권 판정이 자동 실행되는 것은 아닙니다. 실제 접근 판정 결과를 연결하세요.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bAccessGranted=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction", meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.")) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction",meta=(ClampMin="1", ToolTip="상호작용 지급 정수 수량입니다. 양수이며 실제 인벤토리/정의/초과 드롭 거래를 검사합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.")) int32 ItemCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction", meta=(ToolTip="상호작용 성공 뒤 표시 액터 제거입니다. 실패에서는 제거하지 않습니다. 저장 가능한 일회성 월드 원본 기록을 자동 만드는 설정은 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bDestroyAfterUse=false;
    UFUNCTION(BlueprintPure,Category="IC|Interaction", meta=(ToolTip="상호작용 대상 위에 표시할 안내 FText를 조회합니다. 자식 BP에서 전용 설명으로 재정의할 수 있습니다.\n@return 현재 상호작용 안내 문구입니다. 자료형: 현지화 가능한 텍스트.")) FText GetInteractionText() const;
    UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IC|Interaction", meta=(ToolTip="실제 ICPlayerController로 상호작용을 수행합니다. 기본 구현은 Storage/Sleep/Kitchen/Forge/Loot/Quest 명령을 처리하며 자식 BP에서 전용 실행을 재정의할 수 있습니다. 성공 후 제거 설정을 사용할 때 지속 상태 처리도 확인하세요.\n@param Controller 실제 플레이어의 ICPlayerController입니다. None이면 상호작용할 수 없습니다. 입력 핀. 자료형: AICPlayerController*.\n@return 상호작용이 성공하면 true입니다. 실패 시 성공 전용 후처리를 하지 마세요. 자료형: 체크박스(true/false).")) bool TryInteract(AICPlayerController* Controller);
};
