#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "ICPlayerCharacter.generated.h"
class UCameraComponent;
class UICInventoryComponent;
class UICCraftingComponent;
class USpringArmComponent;
class USkeletalMesh;
USTRUCT(BlueprintType, meta=(ToolTip="BP 플레이어의 실제 외형 프리셋 설정입니다. Mesh와 AnimClass의 스켈레톤이 맞아야 하고 MeshTransform으로 캡슐 기준 위치/방향을 조정합니다.", ShortToolTip="BP 플레이어의 실제 외형 프리셋 설정입니다."))
struct FICPlayerAppearancePreset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: 객체 참조 (USkeletalMesh).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance", meta=(ToolTip="프리셋 Anim BP 클래스입니다. Mesh 스켈레톤과 일치하는 AnimInstance 자식을 지정합니다.\n입력 자료형: 클래스 선택 (UAnimInstance).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSubclassOf<UAnimInstance> AnimClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance", meta=(ToolTip="캡슐 기준 메시 상대 위치(cm)·회전(도)·배율입니다. 프리셋 적용 시 메시를 보정합니다.\n입력 자료형: 위치·회전·배율.\nC++ 초기값: FTransform(FRotator(0,-90,0),FVector(0,0,-88)). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FTransform MeshTransform=FTransform(FRotator(0,-90,0),FVector(0,0,-88));
};
UCLASS(Blueprintable, meta=(ToolTip="실제 이동·카메라·인벤토리·제작 컴포넌트를 가진 플레이어 Pawn입니다. BP에서 외형·IA 연결 컨트롤러와 속도 등을 설정합니다. 지속 상태 원본은 WorldState입니다.", ShortToolTip="실제 이동·카메라·인벤토리·제작 컴포넌트를 가진 플레이어 Pawn입니다."))
class ISEKAICLASS_API AICPlayerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AICPlayerCharacter();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="캐릭터의 인벤토리 컴포넌트 또는 WorldState 인벤토리 후보/원본입니다. 실제 컴포넌트는 지속 원본을 조회하며 WBP에 별도 보관 원본을 만들지 않습니다.\n입력 자료형: 객체 참조 (UICInventoryComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UICInventoryComponent> Inventory;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="플레이어의 제작 조회/시작 컴포넌트입니다. 네이티브 캐릭터가 생성하며 BP에 중복 컴포넌트를 추가하지 않습니다.\n입력 자료형: 객체 참조 (UICCraftingComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UICCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="카메라 거리/충돌/회전을 담당하는 SpringArm 컴포넌트입니다. 실제 BP 기본값에서 카메라 배치를 조정합니다.\n입력 자료형: 객체 참조 (USpringArmComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(ToolTip="플레이 카메라 컴포넌트입니다. CameraArm에 부착하고 시점/상호작용 추적에 사용합니다.\n입력 자료형: 객체 참조 (UCameraComponent).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UCameraComponent> Camera;
    // Empty presets retain the Mesh and Anim Blueprint authored directly on the Blueprint.
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance", meta=(ToolTip="실제 플레이어 외형 프리셋 배열입니다. ApplyPreset이 인덱스로 Mesh/AnimClass/Transform을 적용합니다. 비면 BP에 직접 설정한 메시/애니메이션을 유지합니다.\n입력 자료형: 구조/객체 (FICPlayerAppearancePreset) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICPlayerAppearancePreset> AppearancePresets;
    UPROPERTY(BlueprintReadOnly,Category="IC|Appearance", meta=(ToolTip="현재 적용한 외형 프리셋 인덱스입니다. 0부터 시작하고 AppearancePresets 범위와 맞아야 합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 CurrentPreset=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement", meta=(ToolTip="마우스 Look 입력에 곱할 감도입니다. 1은 입력 그대로, 0.5는 절반이며 실제 Look 처리에서 사용합니다.\n입력 자료형: 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float CameraSensitivity=1.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement", meta=(ToolTip="실제 플레이어 달리기 cm/s 속도입니다. 500=5m/s이며 일반 이동은 WalkSpeedMultiplier를 곱합니다. CombatSettings DT와 자동 동기화되지 않습니다.\n입력 자료형: 소수.\nC++ 초기값: 500.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float FinalMoveSpeed=500.f;
    // FinalMoveSpeed is the run speed; normal movement uses the walking multiplier.
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Movement",meta=(ClampMin="0",ClampMax="1", ToolTip="일반 이동의 달리기 속도 대비 배율입니다. 실제 Pawn은 FinalMoveSpeed×이 값을 사용합니다. 0.5는 절반, Shift 유지 시 달리기 속도로 전환합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.5. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float WalkSpeedMultiplier=0.5f;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="BP AppearancePresets 배열의 외형 인덱스를 플레이어 메시·애니메이션·초상화에 적용합니다. 현재 PlayerPreset DT를 자동 조회하는 함수는 아닙니다.\n@param Preset BP AppearancePresets 배열의 0 기반 외형 인덱스입니다. 입력 핀. 자료형: 정수.")) void ApplyPreset(int32 Preset);
};
