#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ICMainMenuPlayerController.generated.h"

class UICMainMenuWidget;

UCLASS(Blueprintable)
class ISEKAICLASS_API AICMainMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
    TSubclassOf<UICMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Main Menu")
    TObjectPtr<UICMainMenuWidget> MainMenuWidget;
};
