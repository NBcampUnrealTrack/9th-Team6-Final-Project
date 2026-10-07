#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICPlayerState.generated.h"

class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICPlayerSnapshotChanged);

// Blueprint-facing view. Persistent data remains in WorldState.
UCLASS(Blueprintable, meta=(ToolTip="플레이어의 WorldState 스냅샷과 골드를 BP/UI에 제공하는 조회 창구입니다. 액터에 별도 저장 원본을 만들지 않으며 변경 이벤트를 구독해 화면을 갱신합니다.", ShortToolTip="플레이어의 WorldState 스냅샷과 골드를 BP/UI에 제공하는 조회 창구입니다."))
class ISEKAICLASS_API AICPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure, Category="IC|Player", meta=(ToolTip="플레이어 CHR_PLAYER의 표시용 상태 복사본을 조회합니다. 활성 실행 캐시와 현재 장착 정보를 조회 경로에서 반영합니다. 복사본을 수정해도 원본이 바뀌지 않습니다.\n@return 현재 플레이어 상태의 스냅샷입니다. 원본이 없으면 비어 있는 상태입니다. 자료형: 구조/객체 (FICCharacterState).")) FICCharacterState GetPlayerSnapshot() const;
    UFUNCTION(BlueprintPure, Category="IC|Player", meta=(ToolTip="WorldState 인벤토리 원본의 복사본을 조회합니다. 복사본 수정은 원본에 반영되지 않으며 변경 API를 사용해야 합니다.\n@return 현재 인벤토리 스냅샷입니다. 자료형: 구조/객체 (FICInventoryState).")) FICInventoryState GetInventorySnapshot() const;
    UFUNCTION(BlueprintPure, Category="IC|Player", meta=(ToolTip="WorldState의 현재 골드를 조회합니다. UI에 별도 골드 원본을 보관하지 마세요.\n@return 현재 보유 골드 정수입니다. 자료형: 정수.")) int32 GetGold() const;
    UFUNCTION(BlueprintPure, Category="IC|Player", meta=(ToolTip="실제 플레이어 생성 확정 여부를 조회합니다. Pawn 존재만으로 생성 완료를 판단하지 마세요.\n@return 플레이어 생성이 확정되어 있으면 true입니다. 자료형: 체크박스(true/false).")) bool IsPlayerCreated() const;
    UPROPERTY(BlueprintAssignable, Category="IC|Player", meta=(ToolTip="플레이어 스냅샷 갱신 이벤트입니다. PlayerState 조회 함수를 다시 읽어 UI를 갱신합니다.\n입력 자료형: FICPlayerSnapshotChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICPlayerSnapshotChanged OnSnapshotChanged;
private:
    UICWorldStateSubsystem* GetWorldState() const;
    UFUNCTION(meta=(ToolTip="WorldState 캐릭터 변경 이벤트의 내부 콜백입니다. 조회/표시용 플레이어 상태를 갱신합니다.\n@param CharacterId WorldState에서 조회할 캐릭터 ID입니다. 플레이어는 CHR_PLAYER입니다. 입력 핀. 자료형: 이름/ID.")) void HandleCharacterChanged(FName CharacterId);
    UFUNCTION(meta=(ToolTip="WorldState 전체 스냅샷 변경 이벤트의 내부 콜백입니다. 로드·생성 후 표시를 원본에 맞춥니다.")) void HandleSnapshotChanged();
};
