#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICInventoryPreview.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class AICPlayerCharacter;

UCLASS(Transient, Blueprintable, meta=(ToolTip="기존 인벤토리 인물 미리보기 액터입니다. 플레이어 외형을 복사해 RenderTarget으로 촬영합니다. 게임 상태 원본을 변경하지 않습니다.", ShortToolTip="기존 인벤토리 인물 미리보기 액터입니다."))
class ISEKAICLASS_API AICInventoryPreview : public AActor
{
    GENERATED_BODY()
public:
    AICInventoryPreview();
    UFUNCTION(BlueprintCallable,Category="IC|UI", meta=(ToolTip="현재 플레이어 외형과 장비 표시를 인벤토리 미리보기 액터에 반영합니다. RenderTarget 촬영용이며 원본 캐릭터의 상태를 바꾸지 않습니다.\n@param Character 반영하거나 표시할 캐릭터 상태/액터입니다. 이 함수의 입력 자료형을 확인하세요. 입력 핀. 자료형: AICPlayerCharacter*.")) void UpdateFromCharacter(AICPlayerCharacter* Character);
    UPROPERTY(meta=(ToolTip="미리보기 인물 SkeletalMesh 컴포넌트입니다. 실제 Pawn 외형을 복사하며 게임 상태를 변경하지 않습니다.\n입력 자료형: 객체 참조 (USkeletalMeshComponent).")) TObjectPtr<USkeletalMeshComponent> PortraitMesh;
    UPROPERTY(meta=(ToolTip="인물 미리보기 SceneCapture2D입니다. RenderTarget과 표시할 메시를 설정하고 실제 캡처 갱신에 사용합니다.\n입력 자료형: 객체 참조 (USceneCaptureComponent2D).")) TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(meta=(ToolTip="미리보기 인물 조명 컴포넌트입니다. 화면 표시만 담당하며 실제 게임 상태를 변경하지 않습니다.\n입력 자료형: 객체 참조 (UPointLightComponent).")) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="인물 미리보기 TextureRenderTarget2D입니다. SceneCapture 출력과 UI 머티리얼/이미지 표시를 함께 연결하며 저장 썸네일과 구분합니다.\n입력 자료형: 객체 참조 (UTextureRenderTarget2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextureRenderTarget2D> Target;
};
