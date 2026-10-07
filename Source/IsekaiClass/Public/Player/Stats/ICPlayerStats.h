#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "ICPlayerStats.generated.h"

UENUM(BlueprintType, meta=(ToolTip="아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."))
enum class EPlayerMainStat : uint8
{
    Strength UMETA(ToolTip="힘입니다. WorldState STR과 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    Dexterity UMETA(ToolTip="민첩입니다. WorldState DEX와 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    Intelligence UMETA(ToolTip="지능입니다. WorldState INT와 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    Constitution UMETA(ToolTip="체력입니다. WorldState CON과 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    Charisma UMETA(ToolTip="매력입니다. WorldState CHA와 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    Luck UMETA(ToolTip="행운입니다. WorldState LUK과 대응합니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다."),
    None UMETA(ToolTip="기본 능력치를 지정하지 않습니다.\n아이템/장비 표시 경로의 기본 능력치 선택입니다. None은 능력치 변경이 없는 값입니다.")
};

USTRUCT(BlueprintType, meta=(ToolTip="플레이어 UI/기존 API에 제공하는 기본 능력치 6종입니다. WorldState의 MainStats를 표시하는 값이며 장비 효과와 혼합된 최종 능력치가 아닙니다.", ShortToolTip="플레이어 UI/기존 API에 제공하는 기본 능력치 6종입니다."))
struct FPlayerMainStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 힘 기본 능력치의 표시/호환 값입니다. WorldState STR와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Strength=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 민첩 기본 능력치의 표시/호환 값입니다. WorldState DEX와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Dexterity=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 지능 기본 능력치의 표시/호환 값입니다. WorldState INT와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Intelligence=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 체력 기본 능력치의 표시/호환 값입니다. WorldState CON와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Constitution=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 매력 기본 능력치의 표시/호환 값입니다. WorldState CHA와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Charisma=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="플레이어 행운 기본 능력치의 표시/호환 값입니다. WorldState LUK와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Luck=1;

    int32 Get(EPlayerMainStat Stat) const
    {
        IC_TRACE_FUNCTION();
        switch(Stat)
        {
        case EPlayerMainStat::Strength:return Strength;
        case EPlayerMainStat::Dexterity:return Dexterity;
        case EPlayerMainStat::Intelligence:return Intelligence;
        case EPlayerMainStat::Constitution:return Constitution;
        case EPlayerMainStat::Charisma:return Charisma;
        case EPlayerMainStat::Luck:return Luck;
        default:return 0;
        }
    }

    int32 Add(EPlayerMainStat Stat,int32 Amount)
    {
        IC_TRACE_FUNCTION();
        int32* Value=nullptr;
        switch(Stat)
        {
        case EPlayerMainStat::Strength:Value=&Strength;break;
        case EPlayerMainStat::Dexterity:Value=&Dexterity;break;
        case EPlayerMainStat::Intelligence:Value=&Intelligence;break;
        case EPlayerMainStat::Constitution:Value=&Constitution;break;
        case EPlayerMainStat::Charisma:Value=&Charisma;break;
        case EPlayerMainStat::Luck:Value=&Luck;break;
        default:return 0;
        }
        const int32 Before=*Value;*Value=FMath::Clamp(Before+FMath::Max(0,Amount),1,10000);return *Value-Before;
    }

    bool IsValid() const
    {
        IC_TRACE_FUNCTION();
        return Strength>=1&&Strength<=10000&&Dexterity>=1&&Dexterity<=10000&&Intelligence>=1&&Intelligence<=10000&&Constitution>=1&&Constitution<=10000&&Charisma>=1&&Charisma<=10000&&Luck>=1&&Luck<=10000;
    }
};
