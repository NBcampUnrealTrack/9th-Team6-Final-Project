#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICStateParticipant.generated.h"

class UICWorldStateSubsystem;

UINTERFACE(BlueprintType)
class ISEKAICLASS_API UICStateParticipant : public UInterface
{
    GENERATED_BODY()
};

class ISEKAICLASS_API IICStateParticipant
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State")
    bool ReadLiveState(FName CharacterId, FICCharacterState& Snapshot) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State")
    bool FlushToWorldState(UICWorldStateSubsystem* WorldState);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State")
    bool CanRestoreWorldState(const FICWorldState& Candidate) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IC|State")
    void RestoreFromWorldState(const FICWorldState& Candidate);
};
