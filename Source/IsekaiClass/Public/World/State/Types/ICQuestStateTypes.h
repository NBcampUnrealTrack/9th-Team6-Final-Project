#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICQuestStateTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="수락·완료·보상 수령·목표별 진행을 보존하는 퀘스트 상태입니다. 보상 거래 ID와 완료 횟수로 수령 이력을 기록합니다.", ShortToolTip="퀘스트의 수락·완료·목표 진행 상태입니다."))
struct FICQuestState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 수락 원본입니다. 완료 상태는 반드시 수락을 포함해야 하며 수락품/시각 거래와 함께 갱신합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) bool bAccepted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 완료 원본입니다. 목표 검사/보고/보상 영수증을 실제 완료 실행에서 함께 확정해야 합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) bool bCompleted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="그룹의 목표 목록 또는 저장 목표별 카운터 맵입니다. 정적 조건과 현재 정수 진행을 구분합니다.\n입력 자료형: 이름/ID → 정수 맵.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) TMap<FName, int32> Objectives;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 보상 지급 완료 원본입니다. RewardTransactionID/전역 완료 거래 기록 없이 체크하면 저장 검사가 실패합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) bool bRewardClaimed = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 보상 지급 GUID 영수증입니다. bRewardClaimed와 전역 완료 거래 집합을 함께 기록해 중복 지급을 막습니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) FGuid RewardTransactionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 완료 누적 정수 횟수입니다. 음수 금지이며 반복 회차 실행 원본으로 사용하도록 정의합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) int32 CompletionCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 수락 누적 GameClock 분입니다. 로드 시 유지하며 상태 판정/반복 기준과 실제 실행에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) double AcceptedAtGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="퀘스트 마지막 완료 누적 GameClock 분입니다. 수락 시각/반복 시각보다 앞뒤 관계를 검사합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 일반 퀘스트 실행과 조건 판정 연결은 아직 필요합니다.")) double CompletedAtGameMinute = 0;
};
