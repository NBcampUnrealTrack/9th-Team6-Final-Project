#pragma once
#include "Life/Crafting/ICCraftingComponent.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICRecipeRow.generated.h"

// Structured requirements and results are shared by authoring and runtime production.
USTRUCT(BlueprintType, meta=(ToolTip="레시피의 실제 재료·도구·결과와 연결 Production ID입니다. 재료는 인스턴스 GUID로 예약하고 PlayClock 시간 후 완료 거래에서 소모/지급합니다. Production의 시간·SP와 일치해야 합니다.", ShortToolTip="레시피의 실제 재료·도구·결과와 연결 Production ID입니다."))
struct ISEKAICLASS_API FICRecipeRow : public FPlayerRecipeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Production DT ID입니다. 현재 레시피 실행은 제작 시간/SP/결과 종류를 연결 규칙과 비교합니다.\n입력 자료형: 이름/ID.")) FName ProductionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="실제 레시피 소모 재료 Item ID/수량 목록입니다. 중복 ID 수량은 합산해 검사하고 정확한 인스턴스 GUID를 예약합니다. 시작은 소모하지 않으며 성공 완료 거래에서 소모합니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.")) TArray<FICItemAmount> Ingredients;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="필수 도구 Item ID/수량 목록입니다. 현재 레시피 시작은 Recipe/Production 두 목록 보유를 검사하며 도구 자체는 소모하지 않습니다.\n입력 자료형: 구조/객체 (FICProductionToolRequirement) 배열.")) TArray<FICProductionToolRequirement> Tools;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="Recipe 제작 PlayClock 초입니다. 현재 연결 Production.DurationPlaySeconds와 같아야 하며 메뉴 정지 중 진행하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 5. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double WorkDurationPlaySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="생산 결과 종류입니다. 현재 레시피 가방 아이템만 지원하며 설치/농사/낚시/채집은 전용 경로 필요로 실패합니다.\n입력 자료형: 선택 목록 (EICProductionResultKind).\nC++ 초기값: BagItems — 결과 아이템을 가방에 지급합니다. 공간 부족 시 작업을 Ready 상태로 보존합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICProductionResultKind ResultKind = EICProductionResultKind::BagItems;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="완료 지급 Item ID/수량 목록입니다. 현재 레시피 완료는 모든 결과를 후보 가방에 넣고 재료 소모/영수증과 함께 확정합니다. 공간/정의 오류면 부분 지급하지 않습니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.")) TArray<FICItemAmount> Results;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="해금 조건입니다. 현재 레시피 경로에서는 비어 있지 않으면 조건 판정기 미연결로 시작을 거부합니다. 다른 정의도 실제 평가 연결을 확인합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열.")) TArray<FICConditionGroup> UnlockConditions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="설치 허용 고정 Anchor ID 목록입니다. 자유 배치가 아니라 주거 지정 지점입니다. 설치 생산 실행은 별도 연결 대상입니다.\n입력 자료형: 이름/ID 배열.")) TArray<FName> InstallationAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="설치 AActor 자식 BP입니다. 고정 주거 앵커/설치 원본과 연결하며 현재 설치 실행은 미연결입니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).")) TSoftClassPtr<AActor> InstalledActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="설치 보관함 용량입니다. 현재 규격 40칸이며 실제 설치 실행은 아직 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 40. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) int32 StorageCapacityAdded = 40;
};
