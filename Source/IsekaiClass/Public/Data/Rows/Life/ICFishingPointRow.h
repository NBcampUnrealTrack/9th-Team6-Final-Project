#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICFishingPointRow.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="낚시터에서 선택할 물고기 후보와 수량·가중치·미끼 제한입니다. FishID는 Fish DT, 미끼는 Item DT 참조입니다.", ShortToolTip="낚시터에서 선택할 물고기 후보와 수량·가중치·미끼 제한입니다."))
struct ISEKAICLASS_API FICFishingSelectionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="Fish DT ID입니다. 가방에 지급할 아이템은 그 Fish.ItemID를 조회합니다.\n입력 자료형: 이름/ID."))
    FName FishID;

    /** Relative weight, normalized only among eligible species after bait filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ClampMin="0.0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0."))
    double Weight = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ClampMin="1", ToolTip="낚시 후보 획득 수량입니다. 양수 정수이며 미끼 소모와 같은 거래로 확정하도록 정의합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1."))
    int32 CatchCount = 1;

    /** Optional entry-specific bait restriction, combined with point and species restrictions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="허용 미끼 Item ID 목록입니다. 빈 목록의 제한 해제 규칙은 낚시 실행 측에서 확정해야 하며 현재 전용 실행은 미연결입니다.\n입력 자료형: 이름/ID 배열."))
    TArray<FName> AcceptedBaitItemIDs;
};

/** Success is rolled after PlayClock work, then one eligible species is chosen by weight. */
USTRUCT(BlueprintType, meta=(ToolTip="낚시터 위치·생산·낚싯대·미끼·성공 확률·물고기 후보 정의입니다. 낚시 전용 실행 경로는 아직 연결되지 않았습니다.", ShortToolTip="낚시터 위치·생산·낚싯대·미끼·성공 확률·물고기 후보 정의입니다."))
struct ISEKAICLASS_API FICFishingPointRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="Anchor DT의 위치 ID입니다. 좌표·지역·종류를 참조하며 액터 표시 이름을 넣지 않습니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    FName AnchorID;

    /** Work duration/SP/montage are owned by this production definition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="Production DT ID입니다. 현재 레시피 실행은 제작 시간/SP/결과 종류를 연결 규칙과 비교합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    FName ProductionID;

    /** Must be a nonconsumed bag tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="낚싯대 Item ID입니다. 실제 낚시 실행에서 보유를 검사하도록 정의하며 전용 경로 연결이 필요합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    FName FishingRodItemID;

    /** Exactly one selected bait is consumed at start, including cancellation/failure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="허용 미끼 Item ID 목록입니다. 빈 목록의 제한 해제 규칙은 낚시 실행 측에서 확정해야 하며 현재 전용 실행은 미연결입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    TArray<FName> AcceptedBaitItemIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="낚시 성공 확률입니다. 0.5=50%이며 현재 전용 실행 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n최댓값: 1.0.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    double SuccessChance = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ToolTip="낚시 물고기 후보/수량/가중치/미끼 목록입니다. 실제 낚시 선택 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICFishingSelectionEntry) 배열.\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    TArray<FICFishingSelectionEntry> FishSelections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="낚시터 표시 AActor 자식 BP입니다. 배치·낚시 실행과 연결합니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).\n소속 정의/시스템: 낚시 전용 실행 경로는 아직 연결되지 않았습니다."))
    TSoftClassPtr<AActor> PointActorClass;
};
