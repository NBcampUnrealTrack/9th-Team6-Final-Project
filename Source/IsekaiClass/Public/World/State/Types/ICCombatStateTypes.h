#pragma once
#include "CoreMinimal.h"
#include "ICCombatStateTypes.generated.h"

// Persistent values, never live GAS handles or actor pointers. Deadlines use PlayClock.
USTRUCT(BlueprintType, meta=(ToolTip="활성 상태 효과 하나의 저장/전투 캐시입니다. 효과 정의 ID와 인스턴스 GUID·원본·남은 시간·틱·중첩·보호막을 보존합니다. 실제 GAS 복원/실행 연결은 별도입니다.", ShortToolTip="활성 상태 효과 하나의 저장/전투 캐시입니다."))
struct FICActiveEffectState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="보유 아이템/효과의 고유 GUID입니다. 같은 Item/Effect 정의의 여러 인스턴스를 구분하며 저장 때 유지합니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) FGuid InstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 활성 효과의 StatusEffect DT ID입니다. 정의 조회와 인스턴스 GUID를 구분해 저장/GAS 복원에 연결합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) FName EffectID;
    // Character ID or stable monster spawn-slot ID; the source actor may already be gone.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="활성 효과를 부여한 원본 캐릭터/엔티티 ID입니다. 효과량 캡처/해제/소유 규칙에 연결하며 정의 ID와 구분합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) FName SourceEntityID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="효과 현재 중첩 수입니다. 1 이상/정의 최대값을 확인하고 실제 GAS 복원에 사용합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) int32 Stacks = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="활성 효과 시간 무한 기록입니다. 만료 시각보다 명시적 종료 조건/제거 실행을 사용합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) bool bInfinite = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="유해 상태 효과 여부입니다. 실제 아이템 해제는 인스턴스/정의 모두 유해인지 검사합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) bool bHarmful = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="비영구 효과 만료 절대 PlayClock 초입니다. bInfinite 효과와 구분하며 실제 효과 종료 실행을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) double ExpiresAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="활성 효과 다음 틱 절대 PlayClock 초입니다. 로드 때 유지하고 중복 틱/만료 순서는 실제 GAS 실행기에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) double NextTickPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="활성 효과의 남은 보호막 수치입니다. 0 이상이며 실제 흡수/GAS 복원 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) double RemainingShield = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="활성 효과의 적용 시 계산된 수치 캐시입니다. 키→유한 수치를 저장하고 복원 때 재추첨/중복 보정을 피하는 GAS 실행 연결이 필요합니다.\n입력 자료형: 이름/ID → 배정밀도 소수 맵.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 실제 GAS 복원/실행 연결은 별도입니다.")) TMap<FName, double> CapturedMagnitudes;
};

USTRUCT(BlueprintType, meta=(ToolTip="전투 효과·쿨타임·캐시 캡처 시각입니다. 활성 액터 캐시를 반영한 뒤 저장/디스폰에 사용하며 상태 효과와 쿨타임 실행/복원 연결은 별도입니다.", ShortToolTip="전투 효과·쿨타임·캐시 캡처 시각입니다."))
struct FICCombatState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 활성 상태 효과 캐시 목록입니다. Effect ID/인스턴스 GUID/남은 시간·틱·중첩·보호막을 보존하며 실제 GAS 실행 복원은 별도입니다.\n입력 자료형: 구조/객체 (FICActiveEffectState) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FICActiveEffectState> Effects;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Skill ID→재사용 가능 절대 PlayClock 초입니다. 저장/복원 때 남은 대기를 유지하며 실제 GAS 쿨타임 실행 연결은 별도입니다.\n입력 자료형: 이름/ID → 배정밀도 소수 맵.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TMap<FName, double> SkillReadyAtPlaySecond;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="전투 캐시를 마지막으로 캡처한 PlayClock 초입니다. 저장 시계보다 미래 값이면 검사가 실패합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double LastCapturedPlaySecond = 0;
};
