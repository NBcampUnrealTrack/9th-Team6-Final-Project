#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICLifeStateTypes.generated.h"

UENUM(BlueprintType, meta=(ToolTip="저장 가능한 생산 작업의 단계입니다. 같은 JobID를 중복 완료하지 않도록 상태와 거래 기록을 검사합니다."))
enum class EICProductionJobStatus : uint8 { Running UMETA(ToolTip="PlayClock 시간이 진행 중입니다. 예약 재료는 다른 거래에서 사용할 수 없습니다.\n저장 가능한 생산 작업의 단계입니다. 같은 JobID를 중복 완료하지 않도록 상태와 거래 기록을 검사합니다."), Ready UMETA(ToolTip="필요 시간이 끝났습니다. 공간 부족 등의 완료 실패에서는 이 상태로 보존하고 재시도합니다.\n저장 가능한 생산 작업의 단계입니다. 같은 JobID를 중복 완료하지 않도록 상태와 거래 기록을 검사합니다."), Completed UMETA(ToolTip="재료 소모·결과 지급이 완료된 작업입니다.\n저장 가능한 생산 작업의 단계입니다. 같은 JobID를 중복 완료하지 않도록 상태와 거래 기록을 검사합니다."), Cancelled UMETA(ToolTip="취소한 작업입니다. 예약 재료를 해제하며 이미 지불한 SP는 현재 구현에서 환급하지 않습니다.\n저장 가능한 생산 작업의 단계입니다. 같은 JobID를 중복 완료하지 않도록 상태와 거래 기록을 검사합니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="제작 작업이 확보한 정확한 아이템 인스턴스와 수량·소모 시점입니다. 활성 예약은 일반 소비/판매/장비 이동 후보를 검사할 때 보호합니다.", ShortToolTip="제작 작업이 확보한 정확한 아이템 인스턴스와 수량·소모 시점입니다."))
struct FICMaterialReservationState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="예약한 정확한 가방 아이템 GUID입니다. ItemID만 같은 다른 묶음을 임의로 소모하지 않으며 개수/잠금을 재검사합니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FGuid ItemInstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="정수 수량입니다. 요구/지급 정의에서는 양수, 보유 인스턴스에서는 0이 빈 슬롯입니다. 음수 상태는 거부합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Count = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="예약 재료 소모 시점입니다. 현재 Recipe 시작은 성공 소비로 예약하며 완료/취소 검증에 사용합니다.\n입력 자료형: 선택 목록 (EICProductionMaterialTiming).\nC++ 초기값: ConsumeOnSuccess — 성공 완료 거래에서 재료를 소모하는 규칙입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICProductionMaterialTiming Timing = EICProductionMaterialTiming::ConsumeOnSuccess;
};

// Costs/results are committed separately by the future production transaction API.
// A saved deadline continues from the saved PlayClock; UTC/offline time is not used.
USTRUCT(BlueprintType, meta=(ToolTip="저장 가능한 제작 작업입니다. 시작/완료 PlayClock·소유자·예약 비용·재료·상태·완료 영수증을 저장합니다. 완료/취소/결과 확정이 중복되지 않도록 검사합니다.", ShortToolTip="저장 가능한 제작 작업입니다."))
struct FICProductionJobState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="실제 제작 작업 GUID입니다. Recipe ID와 다르며 시작 성공 때 반환하고 완료/취소에서 같은 값을 사용합니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FGuid JobID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 소유 캐릭터 ID입니다. 플레이어 CHR_PLAYER 또는 실제 NPC ID를 사용하며 소유권 원본을 저장합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Production DT ID입니다. 현재 레시피 실행은 제작 시간/SP/결과 종류를 연결 규칙과 비교합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName ProductionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Recipe DT ID입니다. 예: RCP_COOK_001. 재료·결과·ProductionID를 조회해 제작합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName RecipeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="작업/서비스 Anchor ID입니다. 생산 시작은 정의 존재와 Production.WorkAnchorIDs 허용 목록을 검사합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName WorkAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작 Running/Ready/Completed/Cancelled 상태입니다. Ready는 시간이 끝났지만 정의/공간 문제로 결과 확정이 대기 중인 상태입니다.\n입력 자료형: 선택 목록 (EICProductionJobStatus).\nC++ 초기값: Running — PlayClock 시간이 진행 중입니다. 예약 재료는 다른 거래에서 사용할 수 없습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICProductionJobStatus Status = EICProductionJobStatus::Running;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작 시작 절대 PlayClock 초입니다. 완료 시각/저장 시계와 일치해야 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double StartedAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작 완료 목표의 절대 PlayClock 초입니다. 남은 시간=max(0,목표-현재)이며 저장/메뉴 정지/로드에도 기준을 유지합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double CompletesAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="실제 제작 예약 재료 목록 또는 정적 생산 재료 목록입니다. 예약은 정확한 GUID/수량을 보호하며 현재 Recipe 실행은 Ingredients를 성공 소비로 예약합니다.\n입력 자료형: 구조/객체 (FICMaterialReservationState) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FICMaterialReservationState> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작에서 예약/확정할 정수 골드입니다. 실제 남은 원본/다른 예약을 재검사합니다. 현재 플레이어 레시피 시작은 별도 골드 비용을 추가하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 ReservedGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작 SP 비용 기록입니다. 현재 시작 때 차감되고 bCostsCommitted로 중복 차감을 막습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double ReservedSP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="작업 비용 확정 영수증 상태입니다. 현재 시작 SP를 이미 차감했음을 기록해 완료/로드에서 중복 차감하지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) bool bCostsCommitted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="작업 결과 확정 기록입니다. Completed와 완료 거래 GUID를 함께 검사해 중복 지급을 막습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) bool bResultsCommitted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="제작 결과 확정 거래 GUID입니다. 완료 결과와 WorldState 영수증을 함께 기록해 중복 지급을 막습니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FGuid CompletionTransactionID;
};

USTRUCT(BlueprintType, meta=(ToolTip="작물 정의 ID·소유 캐릭터 ID·심은 시각·수확 가능 시각을 보존합니다. 두 시각은 GameClock 누적 분입니다. 실제 작물 액터의 성장과 수확 거래 실행은 별도 연결이 필요합니다.", ShortToolTip="작물 정의 ID·소유 캐릭터 ID·심은 시각·수확 가능 시각을 보존합니다."))
struct FICCropState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Crop DT 정의 ID입니다. 씨앗 Item.CropID 또는 실제 심어진 CropState에서 정적 성장 규칙을 찾습니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 작물 액터의 성장과 수확 거래 실행은 별도 연결이 필요합니다.")) FName CropID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 소유 캐릭터 ID입니다. 플레이어 CHR_PLAYER 또는 실제 NPC ID를 사용하며 소유권 원본을 저장합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 작물 액터의 성장과 수확 거래 실행은 별도 연결이 필요합니다.")) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="작물 심은 누적 GameClock 분입니다. 수면/경과 성장 기준이며 오프라인 UTC와 구분합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 작물 액터의 성장과 수확 거래 실행은 별도 연결이 필요합니다.")) double PlantedAtGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="작물 수확 가능 누적 GameClock 분입니다. 실제 성장/수확 실행은 별도이며 저장 기준으로 유지합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 작물 액터의 성장과 수확 거래 실행은 별도 연결이 필요합니다.")) double HarvestableAtGameMinute = 0;
};
