#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICProgressionRow.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="진행 단계의 진입/완료 조건·포함 퀘스트·다음 단계입니다. StageID는 같은 Progression 안의 로컬 키입니다. 실제 진행 상태는 WorldState에 저장합니다.", ShortToolTip="진행 단계의 진입/완료 조건·포함 퀘스트·다음 단계입니다."))
struct ISEKAICLASS_API FICProgressionStageDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="같은 작물/진행 그래프 안의 고유 로컬 단계 키입니다. 전역 DT ID와 구분합니다.\n입력 자료형: 이름/ID."))
    FName StageID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="상세 설명의 String Table 키입니다. 문장 원문 대신 키를 작성하고 해당 테이블에 원문을 등록합니다. 사용하는 화면/노드의 키 해석 연결이 필요합니다.\n입력 자료형: 문자열."))
    FString DescriptionKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="단계/페이즈 진입 조건입니다. 상태·영수증으로 같은 진입을 중복 처리하지 않아야 합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열."))
    TArray<FICConditionGroup> EntryConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="단계의 Quest ID 목록입니다. 현재 카운터/완료 여부는 WorldState로 판정하도록 연결합니다.\n입력 자료형: 이름/ID 배열."))
    TArray<FName> QuestIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="진행 완료 조건 목록입니다. 실제 상태/카운터 판정 실행을 연결합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열."))
    TArray<FICConditionGroup> CompletionConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="같은 진행 그래프의 다음 Stage ID 목록입니다. 존재/순환을 확인하며 실제 선택/진입 실행을 연결합니다.\n입력 자료형: 이름/ID 배열."))
    TArray<FName> NextStageIDs;
};

/** Final-boss access is separately authored; all eight regional bosses are not an implicit prerequisite. */
USTRUCT(BlueprintType, meta=(ToolTip="진행 단계 그래프와 최종 보스 해금·엔딩 퀘스트 정의입니다. Quest DT는 개별 퀘스트, 이 DT는 여러 단계의 흐름을 설명합니다. 실제 진행 실행은 아직 연결 대상입니다.", ShortToolTip="진행 단계 그래프와 최종 보스 해금·엔딩 퀘스트 정의입니다."))
struct ISEKAICLASS_API FICProgressionRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression", meta=(ToolTip="진행 그래프 시작 로컬 Stage ID입니다. Stages 안에 존재해야 하며 다른 DT의 전역 정의 ID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 진행 실행은 아직 연결 대상입니다."))
    FName InitialStageID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression", meta=(ToolTip="진행 단계 목록입니다. 시작/다음 단계 참조와 조건/순환을 검사하고 실제 실행에 연결합니다.\n입력 자료형: 구조/객체 (FICProgressionStageDefinition) 배열.\n소속 정의/시스템: 실제 진행 실행은 아직 연결 대상입니다."))
    TArray<FICProgressionStageDefinition> Stages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss", meta=(ToolTip="최종 보스 Monster ID입니다. 종류·엔딩/진행 참조를 확인합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 진행 실행은 아직 연결 대상입니다."))
    FName FinalBossID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss", meta=(ToolTip="엔딩 Quest ID입니다. 진행/보스/엔딩 참조를 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 진행 실행은 아직 연결 대상입니다."))
    FName EndingQuestID;

    /** Groups are ANDed; any optional alternatives are explicitly represented inside a group. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss", meta=(ToolTip="최종 보스 해금 조건입니다. 진행/퀘스트/보스 참조와 실제 실행을 연결합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열.\n소속 정의/시스템: 실제 진행 실행은 아직 연결 대상입니다."))
    TArray<FICConditionGroup> FinalBossUnlockConditions;
};
