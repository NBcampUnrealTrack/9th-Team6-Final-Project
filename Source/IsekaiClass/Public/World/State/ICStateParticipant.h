#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICStateParticipant.generated.h"

class UICWorldStateSubsystem;

UINTERFACE(BlueprintType, meta=(ToolTip="현재 스폰된 액터·컴포넌트의 실행 캐시를 WorldState 원본과 연결하는 BP 구현 가능 인터페이스입니다. 읽기·저장 전 반영·로드 사전 검사·로드 후 복원 네 함수를 구현합니다. 등록과 해제는 WorldStateSubsystem에서 처리하고 동일 원본을 액터에 중복 보관하지 마세요.", ShortToolTip="현재 스폰된 액터·컴포넌트의 실행 캐시를 WorldState 원본과 연결하는 BP 구현 가능 인터페이스입니다."))
class ISEKAICLASS_API UICStateParticipant : public UInterface
{
    GENERATED_BODY()
};

class ISEKAICLASS_API IICStateParticipant
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State", meta=(ToolTip="등록된 캐릭터 ID의 활성 액터/GAS 실행 캐시를 읽는 BP 구현 함수입니다. 원본 ID가 다르거나 캐시를 제공할 수 없으면 false를 반환합니다. 여기서 영구 원본을 별도로 만들지 마세요.\n@param CharacterId WorldState에서 조회할 캐릭터 ID입니다. 플레이어는 CHR_PLAYER입니다. 입력 핀. 자료형: 이름/ID.\n@param Snapshot 활성 실행 캐시에서 읽은 캐릭터 상태의 출력 복사본입니다. 성공 여부와 요청 ID 일치를 확인하세요. 입력/출력 핀. 자료형: 구조/객체 (FICCharacterState).\n@return 해당 캐릭터의 유효 실행 스냅샷을 반환했으면 true입니다. 자료형: 체크박스(true/false)."))
    bool ReadLiveState(FName CharacterId, FICCharacterState& Snapshot) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State", meta=(ToolTip="저장·디스폰·모드 전환 전에 활성 실행 캐시를 WorldState에 반영하는 BP 구현 함수입니다. 자원·효과·쿨타임을 실제 확정 경로로 반영하고 반영 실패를 숨기지 마세요.\n@param WorldState 캐시를 반영할 GameInstance의 WorldState 원본 서브시스템입니다. 입력 핀. 자료형: UICWorldStateSubsystem*.\n@return 캐시 반영에 성공하면 true입니다. 실패하면 관련 저장/전환을 진행하지 않아야 합니다. 자료형: 체크박스(true/false)."))
    bool FlushToWorldState(UICWorldStateSubsystem* WorldState);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State", meta=(ToolTip="로드 후보를 실제 액터/GAS가 복원할 수 있는지 미리 검사하는 BP 구현 함수입니다. 상태를 변경하지 않는 사전 검사로 구현하세요.\n@param Candidate 검사 또는 복원할 전체 WorldState 후보입니다. 사전 검사에서 원본을 변경하지 마세요. 입력 핀. 자료형: 구조/객체 (FICWorldState).\n@return 후보를 안전하게 복원할 수 있으면 true입니다. 자료형: 체크박스(true/false)."))
    bool CanRestoreWorldState(const FICWorldState& Candidate) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State", meta=(ToolTip="검사를 통과한 WorldState를 실제 액터/GAS에 복원하는 BP 구현 함수입니다. 자원·파생값·효과·쿨타임 복원 후 AI를 재개하는 순서를 지키세요.\n@param Candidate 검사 또는 복원할 전체 WorldState 후보입니다. 사전 검사에서 원본을 변경하지 마세요. 입력 핀. 자료형: 구조/객체 (FICWorldState)."))
    void RestoreFromWorldState(const FICWorldState& Candidate);
};
