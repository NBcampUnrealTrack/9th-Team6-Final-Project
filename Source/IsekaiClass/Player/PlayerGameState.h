#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PlayerGameState.generated.h"
UENUM(BlueprintType)
enum class EPlayerGamePhase : uint8 { CharacterCreation, Playing, Menu, Dead };
UCLASS()
class ISEKAICLASS_API APlayerPrototypeGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) EPlayerGamePhase Phase=EPlayerGamePhase::CharacterCreation;
    UPROPERTY(BlueprintReadOnly) FName SharedHomeStorageID=TEXT("PlayerHome");
    UFUNCTION(BlueprintCallable) void SetPhase(EPlayerGamePhase NewPhase);
};
