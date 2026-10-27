#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ICGameUserSettings.generated.h"

class AICPlayerController;
class USoundMix;
class USoundClass;
class UInputMappingContext;
class UInputAction;

/** User preferences live outside save games and survive map travel/new characters. */
UCLASS(Config=GameUserSettings)
class ISEKAICLASS_API UICGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()
public:
    UICGameUserSettings();
    static UICGameUserSettings* Get();
    virtual void SetToDefaults() override;
    virtual void ValidateSettings() override;
    virtual void LoadSettings(bool bForceReload=false) override;
    virtual void ApplyNonResolutionSettings() override;
    void ApplyRuntime(AICPlayerController* Controller);
    double ReadOption(FName ID) const;
    void WriteOption(FName ID,double Value);
    UInputMappingContext* MakeInputMapping(UInputMappingContext* Source,UObject* Outer,const UInputAction* Move,const UInputAction* Roll) const;

    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float CameraSensitivity=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bInvertCameraX=false;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bInvertCameraY=false;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float CameraFOV=90;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float CameraZoomStep=50;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bAutoSelectTarget=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bCombatFacing=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bProjectileAimAssist=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bDamageNumbers=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bEnemyHealthBars=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float MasterVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float MusicVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float EffectsVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float AmbientVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float VoiceVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float UIVolume=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bMuteUnfocused=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bShowFPS=false;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float UIScale=1;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") float DisplayGamma=2.2f;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") int32 AntiAliasingMethod=4;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bMotionBlur=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bBloom=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bDepthOfField=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") bool bAmbientOcclusion=true;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") FString MonitorDevice;
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") int32 MonitorRefreshRate=0;
    /** Stable mapping identity: action name plus the original key, preserving axis modifiers. */
    UPROPERTY(Config,EditAnywhere,BlueprintReadWrite,Category="User Settings") TMap<FString,FString> KeyOverrides;
private:
    UPROPERTY() TSoftObjectPtr<USoundMix> UserSoundMix;
    UPROPERTY() TArray<TSoftObjectPtr<USoundClass>> UserSoundClasses;
    TWeakObjectPtr<UWorld> AudioWorld;
    bool bDisplayInitialized=false;
};
