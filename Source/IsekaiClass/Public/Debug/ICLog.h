#pragma once

#include "CoreMinimal.h"

// Output Log에서 LogIC로 프로젝트 로그 전체를, 카테고리 이름으로 기능별 로그를 찾습니다.
// Log: 주요 상태 전환/처리 완료, Warning: 복구가 필요한 실패, Error: 필수 설정 누락.
// Verbose: 요청 거절/거래 상세, VeryVerbose: 반복 조회 및 함수 진입/종료 추적.
// 콘솔 예: Log LogICSave Verbose / Log LogICTrace VeryVerbose / Log LogICTrace off
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICCore, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICInput, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICData, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICInventory, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICProduction, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICWorldState, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICTime, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICSave, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICUI, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICInteraction, Log, All);
ISEKAICLASS_API DECLARE_LOG_CATEGORY_EXTERN(LogICTrace, Log, All);

// 각 함수의 첫 줄에 IC_TRACE_FUNCTION();을 넣습니다. 조기 return도 소멸자가 종료를 기록합니다.
// LogICTrace VeryVerbose를 활성화하면 함수명, 소스 위치, 호출 ID, 스레드, 깊이, 실행 시간이 출력됩니다.
// 로그가 비활성화된 동안 문자열 변환/시간 측정/호출 ID 발급을 하지 않으며 Shipping/Test에서는 제거됩니다.
// 이 클래스 자체에는 추적 매크로를 넣지 않습니다. 로그가 다시 로그를 호출하는 재귀를 방지합니다.
class ISEKAICLASS_API FICFunctionLogScope final
{
public:
    FICFunctionLogScope(const ANSICHAR* InFunction, const ANSICHAR* InFile, int32 InLine);
    ~FICFunctionLogScope();

    FICFunctionLogScope(const FICFunctionLogScope&) = delete;
    FICFunctionLogScope& operator=(const FICFunctionLogScope&) = delete;
    FICFunctionLogScope(FICFunctionLogScope&&) = delete;
    FICFunctionLogScope& operator=(FICFunctionLogScope&&) = delete;

private:
    const ANSICHAR* Function;
    const ANSICHAR* File;
    int32 Line;
    uint32 ThreadID = 0;
    uint32 Depth = 0;
    uint64 CallID = 0;
    double StartedAt = 0;
    bool bTracing = false;
};

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#define IC_TRACE_JOIN_INNER(A, B) A##B
#define IC_TRACE_JOIN(A, B) IC_TRACE_JOIN_INNER(A, B)
#define IC_TRACE_FUNCTION() FICFunctionLogScope IC_TRACE_JOIN(ICFunctionTraceScope_, __COUNTER__)(__FUNCTION__, __FILE__, __LINE__)
#else
#define IC_TRACE_FUNCTION() do {} while (false)
#endif
