#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICBGMZone.generated.h"
class UBoxComponent;

/** Place a box over a village/dungeon/etc. No collision or overlap event setup is needed. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICBGMZone : public AActor
{
    GENERATED_BODY()
public:
    AICBGMZone();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="BGM",meta=(GetOptions="GetMusicRows")) FName MusicRowID=TEXT("BGM_Village");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="BGM") int32 Priority=10;
    UFUNCTION(BlueprintPure,Category="BGM") bool ContainsPosition(FVector Position) const;
    UFUNCTION() TArray<FName> GetMusicRows() const;
};
