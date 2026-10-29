#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICPrologueRow.generated.h"

class USoundBase;
UENUM(BlueprintType)
enum class EICPrologueMusicAction : uint8 { Keep, Play, Stop };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICPrologueRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Order=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText Text;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EICPrologueMusicAction MusicAction=EICPrologueMusicAction::Keep;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName MusicRow;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<USoundBase> Sound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float SoundVolume=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float SoundDelay=0.f;
    /** Zero uses the presentation table's character interval. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float CharacterInterval=0.f;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProloguePresentationRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText NextPrompt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText GamepadNextPrompt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText SkipPrompt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText GamepadSkipPrompt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.001")) float CharacterInterval=.045f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.1")) float FadeSeconds=1.2f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.1")) float SkipHoldSeconds=1.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName InitialMusicRow=TEXT("BGM_Prologue");
};
