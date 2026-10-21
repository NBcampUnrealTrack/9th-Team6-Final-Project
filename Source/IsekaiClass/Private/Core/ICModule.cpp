

#include "Core/ICModule.h"
#include "Debug/ICLog.h"
#include "Modules/ModuleManager.h"

class FICGameModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        IC_TRACE_FUNCTION();
        FDefaultGameModuleImpl::StartupModule();
        UE_LOG(LogICCore, Log, TEXT("[Module.Start] IsekaiClass 모듈 시작. 기능별 로그는 LogIC 카테고리로 확인하세요."));
    }
    virtual void ShutdownModule() override
    {
        IC_TRACE_FUNCTION();
        UE_LOG(LogICCore, Log, TEXT("[Module.Stop] IsekaiClass 모듈 종료"));
        FDefaultGameModuleImpl::ShutdownModule();
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FICGameModule, IsekaiClass, "IsekaiClass");
