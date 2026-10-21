#pragma once

#include "CoreMinimal.h"


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
