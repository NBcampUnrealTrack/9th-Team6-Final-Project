#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICSpawnerRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="스폰 원점에서 허용할 이동 경계의 모양입니다. 실제 AI 이탈 판정 연결이 필요합니다."))
enum class EICLeashShape : uint8 { Radius UMETA(ToolTip="원점에서 반경으로 제한합니다. 반경 단위는 cm입니다.\n스폰 원점에서 허용할 이동 경계의 모양입니다. 실제 AI 이탈 판정 연결이 필요합니다."), Box UMETA(ToolTip="원점 주변의 상자 범위로 제한합니다. 범위 크기 단위는 cm입니다.\n스폰 원점에서 허용할 이동 경계의 모양입니다. 실제 AI 이탈 판정 연결이 필요합니다.") };

/** Persistent creature key is (spawner row ID, SpawnSlotIndex). Never renumber an existing slot. */
USTRUCT(BlueprintType, meta=(ToolTip="스포너의 고유 슬롯 키·몬스터·원점·조건·이탈 경계·재생 시간을 정의합니다. 스폰 상태와 처치 시각은 WorldState에 저장합니다. 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.", ShortToolTip="스포너의 고유 슬롯 키·몬스터·원점·조건·이탈 경계·재생 시간을 정의합니다."))
struct ISEKAICLASS_API FICSpawnSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ClampMin="0", ToolTip="같은 스포너 안의 고유 0부터 시작하는 슬롯 번호입니다. 저장 키에 사용하므로 중복/사용 중 변경을 피합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) int32 SpawnSlotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ToolTip="Monster DT의 정의 ID입니다. 스폰 슬롯의 개별 인스턴스 키와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ToolTip="스폰 기준점 Anchor ID입니다. 복귀 위치와 이탈 경계의 원점으로 사용하도록 정의합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) FName OriginAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ToolTip="대사/서비스/스폰 등 이용 조건입니다. 참조 검사는 제공되지만 모든 런타임 조건 평가가 구현된 것은 아닙니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash", meta=(ToolTip="이탈 경계 Radius/Box 선택입니다. 반지름 또는 원점 상대 박스 중 해당 필드를 작성합니다.\n입력 자료형: 선택 목록 (EICLeashShape).\nC++ 초기값: Radius — 원점에서 반경으로 제한합니다. 반경 단위는 cm입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) EICLeashShape LeashShape = EICLeashShape::Radius;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash", meta=(ClampMin="0", Units="cm", ToolTip="원점에서 허용하는 몬스터 이동 반지름 cm입니다. 3000=30m이며 Radius 모드의 실제 AI 복귀 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 3000.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) double LeashRadiusCm = 3000.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash", meta=(Units="cm", ToolTip="스폰 원점 상대 이탈 박스 cm입니다. Box 모드에서 사용하며 월드 절대 좌표와 구분합니다.\n입력 자료형: FBox.\nC++ 초기값: FBox(ForceInit). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) FBox OriginRelativeLeashBoundsCm = FBox(ForceInit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn", meta=(ToolTip="스폰 슬롯 재생 시간을 몬스터 기본값 대신 사용합니다. 해제하면 몬스터 규칙을 쓰도록 정의합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) bool bOverrideRespawnTime = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn", meta=(ClampMin="0", Units="s", ToolTip="몬스터 재생 PlayClock 초입니다. 메뉴 중 경과하지 않으며 슬롯 덮어쓰기 설정/실제 스폰 원본과 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 90.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 실제 셀/스폰 제어 실행은 별도 연결 대상입니다.")) double RespawnPlaySeconds = 90.0;
};

USTRUCT(BlueprintType, meta=(ToolTip="지역의 몬스터 스폰 슬롯 목록입니다. 같은 스포너 안에서 SpawnSlotIndex를 중복시키면 안 됩니다. 액터 스폰 실행은 아직 별도로 연결해야 합니다.", ShortToolTip="지역의 몬스터 스폰 슬롯 목록입니다."))
struct ISEKAICLASS_API FICSpawnerRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 액터 스폰 실행은 아직 별도로 연결해야 합니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ToolTip="스포너 정의 슬롯 목록 또는 컨테이너 인스턴스 슬롯 배열입니다. 빈 컨테이너 칸도 용량에 포함하며 스폰 인덱스는 중복시키지 않습니다.\n입력 자료형: 구조/객체 (FICSpawnSlotDefinition) 배열.\n소속 정의/시스템: 액터 스폰 실행은 아직 별도로 연결해야 합니다.")) TArray<FICSpawnSlotDefinition> Slots;
};
