#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataTable.h"
#include "ICBGMSubsystem.generated.h"
class USoundBase;
class UAudioComponent;
class USoundMix;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBGMRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<USoundBase> Music;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ClampMax="2")) float Volume=0.6f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float FadeInSeconds=1.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float FadeOutSeconds=1.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bLoop=true;
};

/** Area music is chosen by zone priority; combat temporarily overrides it. */
UCLASS()
class ISEKAICLASS_API UICBGMSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    UICBGMSubsystem();
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableWhenPaused() const override{return true;}
    virtual void Deinitialize() override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="BGM") TSoftObjectPtr<UDataTable> MusicTable;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="BGM") FName DefaultAreaRow=TEXT("BGM_Outside");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="BGM") FName CombatRow=TEXT("BGM_Combat");
    UFUNCTION(BlueprintPure,Category="BGM") FName ResolveMusicRow(FVector Position,bool InCombat) const;
    UFUNCTION(BlueprintPure,Category="BGM") FName GetCurrentMusicRow() const{return CurrentRow;}
    UFUNCTION(BlueprintPure,Category="BGM") UAudioComponent* GetMusicComponent() const{return Current;}
    UFUNCTION(BlueprintCallable,Category="BGM") void SetMusicOverride(FName Row){OverrideRow=Row;RefreshIn=0;}
    UFUNCTION(BlueprintCallable,Category="BGM") void ClearMusicOverride(){OverrideRow=NAME_None;RefreshIn=0;}
    UFUNCTION(BlueprintCallable,Category="BGM") void SetPrologueMusic(FName Row){PrologueRow=Row;RefreshIn=0;}
    void SilenceForDeath();
    void ResetForRestore();
    static bool CanPlayEffects(const UWorld* World);
private:
    UPROPERTY(Transient) TObjectPtr<USoundMix> DeathMute;
    bool bDeathMuted=false;
    double SilentUntil=0;
    void StopMusic();
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Current;
    UPROPERTY(Transient) TObjectPtr<USoundBase> CurrentSource;
    FName CurrentRow,OverrideRow;
    FName PrologueRow=TEXT("BGM_Prologue");
    bool bCurrentLoop=true;
    float CurrentFadeOut=1.5f,RefreshIn=0;
    void ChangeMusic(FName Row);
    UFUNCTION() void MusicFinished();
};
