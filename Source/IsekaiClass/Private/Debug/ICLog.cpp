#include "Debug/ICLog.h"
#include "HAL/PlatformTLS.h"
#include "HAL/PlatformTime.h"
#include <atomic>

DEFINE_LOG_CATEGORY(LogICCore);
DEFINE_LOG_CATEGORY(LogICInput);
DEFINE_LOG_CATEGORY(LogICData);
DEFINE_LOG_CATEGORY(LogICInventory);
DEFINE_LOG_CATEGORY(LogICProduction);
DEFINE_LOG_CATEGORY(LogICWorldState);
DEFINE_LOG_CATEGORY(LogICTime);
DEFINE_LOG_CATEGORY(LogICSave);
DEFINE_LOG_CATEGORY(LogICUI);
DEFINE_LOG_CATEGORY(LogICInteraction);
DEFINE_LOG_CATEGORY(LogICTrace);

namespace
{
std::atomic<uint64> NextFunctionCallID{1};
thread_local uint32 FunctionTraceDepth = 0;
}

FICFunctionLogScope::FICFunctionLogScope(const ANSICHAR* InFunction, const ANSICHAR* InFile, int32 InLine)
    : Function(InFunction), File(InFile), Line(InLine)
{
    bTracing = UE_LOG_ACTIVE(LogICTrace, VeryVerbose);
    if (!bTracing) return;

    ThreadID = FPlatformTLS::GetCurrentThreadId();
    Depth = FunctionTraceDepth++;
    CallID = NextFunctionCallID.fetch_add(1, std::memory_order_relaxed);
    StartedAt = FPlatformTime::Seconds();
    UE_LOG(LogICTrace, VeryVerbose, TEXT("[Function.Enter] Call=%llu Thread=%u Depth=%u Name=%s File=%s Line=%d"),
        static_cast<unsigned long long>(CallID), ThreadID, Depth, ANSI_TO_TCHAR(Function), ANSI_TO_TCHAR(File), Line);
}

FICFunctionLogScope::~FICFunctionLogScope()
{
    if (!bTracing) return;

    --FunctionTraceDepth;
    const double DurationMs = (FPlatformTime::Seconds() - StartedAt) * 1000.0;
    UE_LOG(LogICTrace, VeryVerbose, TEXT("[Function.Exit] Call=%llu Thread=%u Depth=%u Name=%s DurationMs=%.3f"),
        static_cast<unsigned long long>(CallID), ThreadID, Depth, ANSI_TO_TCHAR(Function), DurationMs);
}
