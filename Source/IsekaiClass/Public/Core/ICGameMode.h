#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ICGameMode.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API AICGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AICGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Main Menu")
    TSoftObjectPtr<UWorld> MainMenuMap;

    UFUNCTION(BlueprintPure, Category="IC|Main Menu")
    bool IsMainMenuLevel() const;
};
