#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICEndingRow.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="엔딩 시네마틱·크레딧/결과 WBP·String Table 키의 정적 참조입니다. 결과 플래그·보상 지급 원본은 WorldState입니다. 엔딩 표시 실행은 아직 연결 대상입니다.", ShortToolTip="엔딩 시네마틱·크레딧/결과 WBP·String Table 키의 정적 참조입니다."))
struct ISEKAICLASS_API FICEndingPresentation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(AllowedClasses="/Script/LevelSequence.LevelSequence", ToolTip="엔딩 LevelSequence 등의 소프트 경로입니다. 실제 파일/연출 재생과 월드 귀환을 연결해야 합니다.\n입력 자료형: FSoftObjectPath.\n허용 에셋 종류: /Script/LevelSequence.LevelSequence.\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    FSoftObjectPath CinematicSequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="기존/작성된 엔딩 크레딧 UUserWidget 자식 WBP 참조입니다. 실제 엔딩 표시 실행에서 생성해야 합니다.\n입력 자료형: Soft Class 에셋 참조 (UUserWidget).\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    TSoftClassPtr<UUserWidget> CreditsWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="엔딩 결과 UUserWidget 자식 WBP 참조입니다. 실제 표시 실행에서 생성해야 합니다.\n입력 자료형: Soft Class 에셋 참조 (UUserWidget).\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    TSoftClassPtr<UUserWidget> ResultWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="엔딩 크레딧 String Table 키입니다. 실제 원문과 크레딧 UI 해석을 연결합니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    FString CreditsTextKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="엔딩 결과 제목 String Table 키입니다. 해당 원문과 실제 결과 UI를 연결합니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    FString ResultTitleKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="엔딩 결과 설명 String Table 키입니다. 실제 결과 WBP에서 해석합니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 엔딩 표시 실행은 아직 연결 대상입니다."))
    FString ResultDescriptionKey;
};

/** Only static criteria/presentation. First-decision snapshot and completion receipts belong to WorldState. */
USTRUCT(BlueprintType, meta=(ToolTip="최종 보스·엔딩 퀘스트·진행 정의·숨겨진 엔딩 관계 대상·보상·귀환 위치 정의입니다. 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다.", ShortToolTip="최종 보스·엔딩 퀘스트·진행 정의·숨겨진 엔딩 관계 대상·보상·귀환 위치 정의입니다."))
struct ISEKAICLASS_API FICEndingRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="최종 보스 Monster ID입니다. 종류·엔딩/진행 참조를 확인합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FName FinalBossID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="엔딩 Quest ID입니다. 진행/보스/엔딩 참조를 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FName EndingQuestID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="Progression DT ID입니다. 개별 Quest와 구분하고 단계/엔딩 연결을 확인합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FName ProgressionID;

    /** Exactly ten distinct recruitable/resident NPCs. Current affinity 100 for all is required. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HiddenEnding", meta=(ToolTip="숨겨진 엔딩 관계 대상 NPC ID 목록입니다. 현재 관계는 WorldState로 조회하며 인원/조건 규격을 검사합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    TArray<FName> HiddenAffinityNPCIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding", meta=(ToolTip="일반 엔딩 Reward ID입니다. 엔딩 완료 영수증으로 한 번 확정해야 합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FName NormalEndingRewardID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding", meta=(ToolTip="엔딩 뒤 월드 귀환 Anchor ID입니다. 실제 이동·상태 복원 실행과 연결해야 합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FName WorldReturnAnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding", meta=(ToolTip="일반 엔딩 연출/크레딧/결과 참조입니다. 실제 완료 판정/표시 실행과 연결합니다.\n입력 자료형: 구조/객체 (FICEndingPresentation).\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FICEndingPresentation NormalPresentation;

    /** Hidden ending always suppresses common/first/quest rewards and writes only clear record 5. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HiddenEnding", meta=(ToolTip="숨겨진 엔딩 연출/크레딧/결과 참조입니다. 실제 숨겨진 판정과 표시 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICEndingPresentation).\n소속 정의/시스템: 실제 엔딩 판정·보상·전환 실행은 아직 연결 대상입니다."))
    FICEndingPresentation HiddenPresentation;
};
