#include "Settings/ICDisplaySettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

TArray<FICMonitor> ICDisplaySettings::Monitors()
{
    TArray<FICMonitor> Result;
#if PLATFORM_WINDOWS
    DISPLAY_DEVICEW Device{};Device.cb=sizeof(Device);
    for(DWORD I=0;EnumDisplayDevicesW(nullptr,I,&Device,0);++I)
    {
        if(!(Device.StateFlags&DISPLAY_DEVICE_ACTIVE)||(Device.StateFlags&DISPLAY_DEVICE_MIRRORING_DRIVER))continue;
        DEVMODEW Mode{};Mode.dmSize=sizeof(Mode);
        if(!EnumDisplaySettingsExW(Device.DeviceName,ENUM_CURRENT_SETTINGS,&Mode,0))continue;
        DISPLAY_DEVICEW Panel{};Panel.cb=sizeof(Panel);EnumDisplayDevicesW(Device.DeviceName,0,&Panel,0);
        FICMonitor Monitor;Monitor.Device=Device.DeviceName;
        Monitor.Current={int32(Mode.dmPelsWidth),int32(Mode.dmPelsHeight),int32(Mode.dmDisplayFrequency)};
        Monitor.Position=FIntPoint(Mode.dmPosition.x,Mode.dmPosition.y);Monitor.bPrimary=(Device.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE)!=0;
        Monitor.Label=FString::Printf(TEXT("%d  ·  %s"),Result.Num()+1,Panel.DeviceString[0]?Panel.DeviceString:Device.DeviceString);
        Result.Add(Monitor);
    }
#endif
    return Result;
}
TArray<FICDisplayMode> ICDisplaySettings::Modes(const FString& Device)
{
    TArray<FICDisplayMode> Result;
#if PLATFORM_WINDOWS
    DEVMODEW Mode{};Mode.dmSize=sizeof(Mode);
    for(DWORD I=0;EnumDisplaySettingsExW(*Device,I,&Mode,0);++I)
        if(Mode.dmBitsPerPel>=24&&Mode.dmPelsWidth>=1280&&Mode.dmPelsHeight>=720&&Mode.dmDisplayFrequency>=24)
            Result.AddUnique({int32(Mode.dmPelsWidth),int32(Mode.dmPelsHeight),int32(Mode.dmDisplayFrequency)});
    Result.Sort([](const FICDisplayMode& A,const FICDisplayMode& B){return A.Width!=B.Width?A.Width<B.Width:A.Height!=B.Height?A.Height<B.Height:A.Hz<B.Hz;});
#endif
    return Result;
}
bool ICDisplaySettings::Apply(const FString& Device,const FICDisplayMode& Value,FString& Error)
{
#if PLATFORM_WINDOWS
    if(!Modes(Device).Contains(Value)){Error=NSLOCTEXT("ICSettings","UnsupportedMode","선택한 모니터에서 지원하지 않는 해상도 또는 주사율입니다.").ToString();return false;}
    DEVMODEW Mode{};Mode.dmSize=sizeof(Mode);
    if(!EnumDisplaySettingsExW(*Device,ENUM_CURRENT_SETTINGS,&Mode,0))return false;
    Mode.dmPelsWidth=Value.Width;Mode.dmPelsHeight=Value.Height;Mode.dmDisplayFrequency=Value.Hz;
    Mode.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFREQUENCY;
    if(ChangeDisplaySettingsExW(*Device,&Mode,nullptr,CDS_TEST,nullptr)!=DISP_CHANGE_SUCCESSFUL
        ||ChangeDisplaySettingsExW(*Device,&Mode,nullptr,CDS_FULLSCREEN,nullptr)!=DISP_CHANGE_SUCCESSFUL)
    {Error=NSLOCTEXT("ICSettings","DisplayApplyFailed","화면 설정을 적용할 수 없습니다. 이전 설정으로 돌아갑니다.").ToString();return false;}
    return true;
#else
    Error=NSLOCTEXT("ICSettings","NoDisplayControl","이 플랫폼에서는 모니터 주사율을 변경할 수 없습니다.").ToString();return false;
#endif
}
void ICDisplaySettings::MoveGameWindow(const FICMonitor& Monitor)
{
    if(GEngine&&GEngine->GameViewport)if(auto Window=GEngine->GameViewport->GetWindow())
        Window->MoveWindowTo(FVector2D(Monitor.Position.X,Monitor.Position.Y));
}
