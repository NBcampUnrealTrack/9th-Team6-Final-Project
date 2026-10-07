#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICProductionRow.generated.h"

/** Static work rules only. Reservations, remaining time and progress belong to runtime state. */
USTRUCT(BlueprintType, meta=(ToolTip="생산 작업의 정적 도구·재료·위치·비용·시간·결과 정의입니다. 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다.", ShortToolTip="생산 작업의 정적 도구·재료·위치·비용·시간·결과 정의입니다."))
struct ISEKAICLASS_API FICProductionRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production", meta=(ToolTip="생활 생산/일과 활동 분류입니다. 상위 필드 타입에 맞춰 제작·요리·심기·수확·낚시·채집 또는 대기·이동·작업·수면을 선택합니다.\n입력 자료형: 선택 목록 (EICLifeActivity).\nC++ 초기값: Crafting — 제작 활동입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    EICLifeActivity Activity = EICLifeActivity::Crafting;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production", meta=(ToolTip="생산 결과 종류입니다. 현재 레시피 가방 아이템만 지원하며 설치/농사/낚시/채집은 전용 경로 필요로 실패합니다.\n입력 자료형: 선택 목록 (EICProductionResultKind).\nC++ 초기값: BagItems — 결과 아이템을 가방에 지급합니다. 공간 부족 시 작업을 Ready 상태로 보존합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    EICProductionResultKind ResultKind = EICProductionResultKind::BagItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="필수 도구 Item ID/수량 목록입니다. 현재 레시피 시작은 Recipe/Production 두 목록 보유를 검사하며 도구 자체는 소모하지 않습니다.\n입력 자료형: 구조/객체 (FICProductionToolRequirement) 배열.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FICProductionToolRequirement> Tools;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="실제 제작 예약 재료 목록 또는 정적 생산 재료 목록입니다. 예약은 정확한 GUID/수량을 보호하며 현재 Recipe 실행은 Ingredients를 성공 소비로 예약합니다.\n입력 자료형: 구조/객체 (FICProductionMaterialRequirement) 배열.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FICProductionMaterialRequirement> Materials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="허용 작업 Anchor ID 목록입니다. 현재 제작에서 비면 앵커 존재만 검사하고, 값이 있으면 선택 앵커가 포함되어야 합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FName> WorkAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ToolTip="생산 시작 조건입니다. 현재 레시피는 일반 판정기가 미연결이므로 비어 있지 않으면 소모 없이 시작을 거부합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FICConditionGroup> StartConditions;

    /** Work advances exclusively with PlayClock and pauses with the world. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cost", meta=(ClampMin="0.0", Units="s", ToolTip="작업 시작부터 완료까지 PlayClock 초입니다. 5이면 실제 월드 진행 5초 뒤 완료를 시도합니다. 메뉴 정지에는 진행하지 않고 GameClock만 움직이는 수면으로 채워지지 않습니다. 현재 Recipe.WorkDurationPlaySeconds와 같아야 시작합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 5.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    double DurationPlaySeconds = 5.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cost", meta=(ClampMin="0.0", ToolTip="제작 SP 비용입니다. 현재 시작 시 한 번 차감하며 Recipe/Production 값이 같아야 합니다. 취소에도 시작 SP는 환불하지 않고 재료는 성공 완료 시 소모합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 5.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    double SPCost = 5.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production", meta=(ToolTip="생산 반복 허용 정의입니다. 현재 StartRecipe는 이 필드를 검사하지 않고 진행 중 플레이어 작업이 있으면 시작을 거부합니다. 완료 후 반복 금지 실행 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    bool bAllowRepeat = true;

    /** Leaving this radius cancels work; zero means the work-anchor provider decides. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ClampMin="0.0", Units="cm", ToolTip="작업 앵커 이탈 허용 cm 규격입니다. 300=3m, 0=제공 측 결정입니다. 현재 생산 실행에는 거리 검사/자동 취소가 연결되지 않아 값 변경만으로 취소되지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    double MaximumWorkDistanceCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result", meta=(ToolTip="완료 지급 Item ID/수량 목록입니다. 현재 레시피 완료는 모든 결과를 후보 가방에 넣고 재료 소모/영수증과 함께 확정합니다. 공간/정의 오류면 부분 지급하지 않습니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FICItemAmount> Results;

    /** Crop/resource/fishing-point definition chosen by the static action or its target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result", meta=(ToolTip="가방 아이템 외 생산 결과의 정의 종류+ID입니다. 작물/자원/낚시터 등에 맞춰 작성합니다. 전용 실행은 미연결입니다.\n입력 자료형: 구조/객체 (FICDefinitionRef).\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    FICDefinitionRef ResultDefinition;

    /** Only fixed home-storage anchors may be used; no free placement or recovery. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(EditCondition="ResultKind == EICProductionResultKind::InstallStorage", ToolTip="설치 허용 고정 Anchor ID 목록입니다. 자유 배치가 아니라 주거 지정 지점입니다. 설치 생산 실행은 별도 연결 대상입니다.\n입력 자료형: 이름/ID 배열.\n편집 활성 조건: ResultKind == EICProductionResultKind::InstallStorage.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TArray<FName> InstallationAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(EditCondition="ResultKind == EICProductionResultKind::InstallStorage", ToolTip="설치 AActor 자식 BP입니다. 고정 주거 앵커/설치 원본과 연결하며 현재 설치 실행은 미연결입니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).\n편집 활성 조건: ResultKind == EICProductionResultKind::InstallStorage.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TSoftClassPtr<AActor> InstalledActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(ClampMin="1", EditCondition="ResultKind == EICProductionResultKind::InstallStorage", ToolTip="설치 보관함 용량입니다. 현재 규격 40칸이며 실제 설치 실행은 아직 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 40. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n편집 활성 조건: ResultKind == EICProductionResultKind::InstallStorage.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    int32 StorageCapacityAdded = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="생산 AnimMontage입니다. 현재 제작 시간/결과 처리는 구현되었지만 이 에셋의 자동 재생 연결은 별도입니다.\n입력 자료형: Soft Object 에셋 참조 (UAnimMontage).\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TSoftObjectPtr<UAnimMontage> WorkMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="제작 완료 SoundBase입니다. 현재 완료 이벤트에 재생 실행을 연결해야 합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    TSoftObjectPtr<USoundBase> CompletionSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="생산 VFX 소프트 경로입니다. 실제 파일/종류를 검증하고 표시 실행을 연결합니다.\n입력 자료형: FSoftObjectPath.\n소속 정의/시스템: 현재 가방 아이템 레시피 생산은 지원하고 설치·농사·낚시·채집·일반 조건 판정은 별도 실행 연결이 필요합니다."))
    FSoftObjectPath WorkVFX;
};
