#include "Settings/ICGameUserSettings.h"
#include "Data/Text/ICLocalization.h"
#include "Settings/ICDisplaySettings.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "InputMappingContext.h"
#include "InputAction.h"

UICGameUserSettings::UICGameUserSettings()
{
    UserSoundMix=FSoftObjectPath(TEXT("/Game/IsekaiClass/Audio/Settings/SM_UserSettings.SM_UserSettings"));
    for(const TCHAR* Name:{TEXT("Master"),TEXT("Music"),TEXT("Effects"),TEXT("Ambient"),TEXT("Voice"),TEXT("UI")})
        UserSoundClasses.Add(TSoftObjectPtr<USoundClass>(FSoftObjectPath(FString::Printf(TEXT("/Game/IsekaiClass/Audio/Settings/SC_%s.SC_%s"),Name,Name))));
}
UICGameUserSettings* UICGameUserSettings::Get()
{
    if(auto* Settings=Cast<UICGameUserSettings>(UGameUserSettings::GetGameUserSettings()))return Settings;
    return GetMutableDefault<UICGameUserSettings>();
}
void UICGameUserSettings::LoadSettings(bool bForceReload)
{
    Super::LoadSettings(bForceReload);
    ICLocalization::ApplyLanguage(LanguageIndex);
    float Value;
    if(!GConfig->GetFloat(TEXT("/Script/IsekaiClass.ICGameUserSettings"),TEXT("CameraSensitivity"),Value,GGameUserSettingsIni))
        if(GConfig->GetFloat(TEXT("/Script/IsekaiClass.Input"),TEXT("CameraSensitivity"),Value,GGameUserSettingsIni))CameraSensitivity=Value;
}
void UICGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    CameraSensitivity=1;bInvertCameraX=bInvertCameraY=false;CameraFOV=90;CameraZoomStep=50;
    bAutoSelectTarget=bCombatFacing=bProjectileAimAssist=bDamageNumbers=bEnemyHealthBars=true;
    MasterVolume=MusicVolume=EffectsVolume=AmbientVolume=VoiceVolume=UIVolume=1;bMuteUnfocused=true;
    bShowFPS=false;UIScale=1;DisplayGamma=2.2f;AntiAliasingMethod=4;
    bMotionBlur=bBloom=bDepthOfField=bAmbientOcclusion=true;
    MonitorDevice.Empty();MonitorRefreshRate=0;KeyOverrides.Reset();
    LanguageIndex=InputDeviceMode=0;GamepadLayout=1;
}
void UICGameUserSettings::ValidateSettings()
{
    Super::ValidateSettings();
    LanguageIndex=FMath::Clamp(LanguageIndex,0,1);InputDeviceMode=FMath::Clamp(InputDeviceMode,0,2);GamepadLayout=FMath::Clamp(GamepadLayout,0,1);
    auto Clamp=[](float Value,float Min,float Max,float Default){return FMath::IsFinite(Value)?FMath::Clamp(Value,Min,Max):Default;};
    CameraSensitivity=Clamp(CameraSensitivity,.1f,3,1);CameraFOV=Clamp(CameraFOV,60,110,90);CameraZoomStep=Clamp(CameraZoomStep,10,100,50);
    for(float* Value:{&MasterVolume,&MusicVolume,&EffectsVolume,&AmbientVolume,&VoiceVolume,&UIVolume})*Value=Clamp(*Value,0,1,1);
    UIScale=Clamp(UIScale,.75f,1.3f,1);DisplayGamma=Clamp(DisplayGamma,1.8f,2.6f,2.2f);AntiAliasingMethod=FMath::Clamp(AntiAliasingMethod,0,4);
}
FKey UICGameUserSettings::GamepadFaceKey(int32 Semantic)
{
    const bool Nintendo=Get()->GamepadLayout==0;
    const FKey Keys[]={Nintendo?EKeys::Gamepad_FaceButton_Top:EKeys::Gamepad_FaceButton_Left,
        Nintendo?EKeys::Gamepad_FaceButton_Left:EKeys::Gamepad_FaceButton_Top,
        Nintendo?EKeys::Gamepad_FaceButton_Bottom:EKeys::Gamepad_FaceButton_Right,
        Nintendo?EKeys::Gamepad_FaceButton_Right:EKeys::Gamepad_FaceButton_Bottom};
    return Keys[FMath::Clamp(Semantic,0,3)];
}
bool UICGameUserSettings::IsGamepadAcceptKey(FKey Key){return Key==GamepadFaceKey(3);}
bool UICGameUserSettings::IsGamepadBackKey(FKey Key){return Key==GamepadFaceKey(2);}
FKey UICGameUserSettings::MenuKey(FKey Key)
{
    if(Key==EKeys::G||Key==EKeys::F||IsGamepadAcceptKey(Key))return EKeys::Enter;if(IsGamepadBackKey(Key))return EKeys::Escape;
    if(Key==EKeys::Gamepad_DPad_Up)return EKeys::Up;if(Key==EKeys::Gamepad_DPad_Down)return EKeys::Down;
    if(Key==EKeys::Gamepad_DPad_Left)return EKeys::Left;if(Key==EKeys::Gamepad_DPad_Right)return EKeys::Right;return Key;
}
double UICGameUserSettings::ReadOption(FName ID) const
{
#define IC_QUALITY_READ(Name) if(ID==TEXT(#Name))return Get##Name##Quality();
    IC_QUALITY_READ(ViewDistance) IC_QUALITY_READ(AntiAliasing) IC_QUALITY_READ(Shadow) IC_QUALITY_READ(GlobalIllumination)
    IC_QUALITY_READ(Reflection) IC_QUALITY_READ(PostProcessing) IC_QUALITY_READ(Texture) IC_QUALITY_READ(VisualEffect)
    IC_QUALITY_READ(Foliage) IC_QUALITY_READ(Shading) IC_QUALITY_READ(Landscape)
#undef IC_QUALITY_READ
    if(ID==TEXT("Quality"))return GetOverallScalabilityLevel();
    if(ID==TEXT("FrameLimit"))return GetFrameRateLimit();
    if(ID==TEXT("VSync"))return IsVSyncEnabled();
    if(ID==TEXT("DynamicResolution"))return IsDynamicResolutionEnabled();
    if(ID==TEXT("ResolutionScale")){float Normalized,Value,Min,Max;GetResolutionScaleInformationEx(Normalized,Value,Min,Max);return Value;}
    if(const auto* Property=FindFProperty<FProperty>(GetClass(),ID))
    {
        if(const auto* Bool=CastField<FBoolProperty>(Property))return Bool->GetPropertyValue_InContainer(this);
        if(const auto* Float=CastField<FFloatProperty>(Property))return Float->GetPropertyValue_InContainer(this);
        if(const auto* Int=CastField<FIntProperty>(Property))return Int->GetPropertyValue_InContainer(this);
    }
    return 0;
}
void UICGameUserSettings::WriteOption(FName ID,double Value)
{
    if(!FMath::IsFinite(Value))return;
#define IC_QUALITY_WRITE(Name) if(ID==TEXT(#Name)){Set##Name##Quality(FMath::Clamp(FMath::RoundToInt(Value),0,3));return;}
    IC_QUALITY_WRITE(ViewDistance) IC_QUALITY_WRITE(AntiAliasing) IC_QUALITY_WRITE(Shadow) IC_QUALITY_WRITE(GlobalIllumination)
    IC_QUALITY_WRITE(Reflection) IC_QUALITY_WRITE(PostProcessing) IC_QUALITY_WRITE(Texture) IC_QUALITY_WRITE(VisualEffect)
    IC_QUALITY_WRITE(Foliage) IC_QUALITY_WRITE(Shading) IC_QUALITY_WRITE(Landscape)
#undef IC_QUALITY_WRITE
    if(ID==TEXT("Quality")){if(Value>=0)SetOverallScalabilityLevel(FMath::Clamp(FMath::RoundToInt(Value),0,3));return;}
    if(ID==TEXT("FrameLimit")){SetFrameRateLimit(FMath::Clamp(Value,0.,360.));return;}
    if(ID==TEXT("VSync")){SetVSyncEnabled(Value!=0);return;}
    if(ID==TEXT("DynamicResolution")){SetDynamicResolutionEnabled(Value!=0);return;}
    if(ID==TEXT("ResolutionScale")){SetResolutionScaleValueEx(FMath::Clamp(Value,50.,100.));return;}
    if(auto* Property=FindFProperty<FProperty>(GetClass(),ID))
    {
        if(const auto* Bool=CastField<FBoolProperty>(Property))Bool->SetPropertyValue_InContainer(this,Value!=0);
        else if(const auto* Float=CastField<FFloatProperty>(Property))Float->SetPropertyValue_InContainer(this,Value);
        else if(const auto* Int=CastField<FIntProperty>(Property))Int->SetPropertyValue_InContainer(this,FMath::RoundToInt(Value));
    }
}
void UICGameUserSettings::ApplyNonResolutionSettings()
{
    ValidateSettings();Super::ApplyNonResolutionSettings();
    ICLocalization::ApplyLanguage(LanguageIndex);
    auto Set=[](const TCHAR* Name,int32 Value){if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(Name))Variable->Set(Value,ECVF_SetByGameOverride);};
    Set(TEXT("r.AntiAliasingMethod"),AntiAliasingMethod);Set(TEXT("r.MotionBlurQuality"),bMotionBlur?4:0);
    Set(TEXT("r.DefaultFeature.MotionBlur"),bMotionBlur?1:0);Set(TEXT("r.DefaultFeature.Bloom"),bBloom?1:0);
    Set(TEXT("r.DefaultFeature.AmbientOcclusion"),bAmbientOcclusion?1:0);
    Set(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"),bAmbientOcclusion?1:0);
    Set(TEXT("r.DynamicRes.OperationMode"),IsDynamicResolutionEnabled()?1:0);
    Set(TEXT("r.BloomQuality"),bBloom?5:0);Set(TEXT("r.DepthOfFieldQuality"),bDepthOfField?2:0);Set(TEXT("r.AmbientOcclusionLevels"),bAmbientOcclusion?-1:0);
    if(GEngine)GEngine->DisplayGamma=DisplayGamma;
    FApp::SetUnfocusedVolumeMultiplier(bMuteUnfocused?0.f:1.f);
}
UInputMappingContext* UICGameUserSettings::MakeInputMapping(UInputMappingContext* Source,UObject* Outer,const UInputAction* Move,const UInputAction* Roll) const
{
    if(!Source)return nullptr;
    auto* Copy=DuplicateObject<UInputMappingContext>(Source,Outer);
    for(int32 I=0;I<Copy->GetMappings().Num();++I)
    {
        auto& Mapping=Copy->GetMapping(I);if(!Mapping.Action||Mapping.Key==EKeys::Escape)continue;
        const UInputAction* Identity=Mapping.Action==Roll?Move:Mapping.Action.Get();if(!Identity)continue;
        const FString ID=Identity->GetName()+TEXT(":")+Mapping.Key.GetFName().ToString();
        if(const FString* Override=KeyOverrides.Find(ID))
        {const FKey Key(**Override);if(Key.IsValid()&&Key!=EKeys::Escape&&!Key.IsGamepadKey()&&!Key.IsAxis1D()&&!Key.IsAxis2D())Mapping.Key=Key;}
    }
    return Copy;
}
void UICGameUserSettings::ApplyRuntime(AICPlayerController* Controller)
{
    if(!Controller)return;
    if(!bDisplayInitialized&&!GIsEditor&&Controller->GetWorld())
    {
        bDisplayInitialized=true;
        const auto Displays=ICDisplaySettings::Monitors();
        if(const auto* Display=Displays.FindByPredicate([&](const FICMonitor& M){return M.Device==MonitorDevice;}))
        {
            const FIntPoint Resolution=GetScreenResolution();
            const FICDisplayMode Mode{GetFullscreenMode()==EWindowMode::Fullscreen?Resolution.X:Display->Current.Width,
                GetFullscreenMode()==EWindowMode::Fullscreen?Resolution.Y:Display->Current.Height,MonitorRefreshRate>0?MonitorRefreshRate:Display->Current.Hz};
            FString Error;if(Mode==Display->Current||ICDisplaySettings::Apply(Display->Device,Mode,Error))
            {ICDisplaySettings::MoveGameWindow(*Display);ApplyResolutionSettings(false);}
        }
    }
    if(auto* Player=Cast<AICPlayerCharacter>(Controller->GetPawn()))
    {Player->CameraSensitivity=CameraSensitivity;Player->CameraZoomStep=CameraZoomStep;if(Player->Camera)Player->Camera->SetFieldOfView(CameraFOV);}
    auto* World=Controller->GetWorld();
    if(auto* Mix=UserSoundMix.LoadSynchronous())
    {
        if(AudioWorld.Get()!=World){UGameplayStatics::PushSoundMixModifier(World,Mix);AudioWorld=World;}
        const float Volumes[]={MasterVolume,MusicVolume,EffectsVolume,AmbientVolume,VoiceVolume,UIVolume};
        for(int32 I=0;I<UserSoundClasses.Num();++I)if(auto* Class=UserSoundClasses[I].LoadSynchronous())
            UGameplayStatics::SetSoundMixClassOverride(World,Mix,Class,Volumes[I],1,.05f,I==0);
    }
    if(GEngine&&GEngine->GameViewport)
    {
        const bool Current=GEngine->GameViewport->IsStatEnabled(TEXT("FPS"));
        if(Current!=bShowFPS)Controller->ConsoleCommand(TEXT("stat FPS"),false);
    }
}
