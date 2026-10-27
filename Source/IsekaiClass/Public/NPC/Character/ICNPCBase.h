#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "ICNPCBase.generated.h"

class AICNPCBase;
class AICPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FICNPCInteractionStarted, AICNPCBase*, NPC, AICPlayerController*, Requester, FName, ServiceID);

UENUM(BlueprintType)
enum class EICNPCInteractionOption : uint8
{
    Talk ,
    Gift ,
    Trade ,
    End,
    Recruit,
    Dismiss
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCInteractionOption
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC") EICNPCInteractionOption Option = EICNPCInteractionOption::Talk;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC") FText Label;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC") FName ServiceID;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC") bool bEnabled = true;
};
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCBase : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICNPCBase();
    virtual void BeginPlay() override;
    virtual void HandleZeroHP() override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
    bool InitializeNPC(FName ID);
    void RefreshExecution();
    void HoldDownedActor();
    const FICNPCRow& GetNPCDefinition() const { return Definition; }

    UFUNCTION(BlueprintPure,Category="IC|NPC")
    TArray<FName> GetAvailableServices() const;

    UFUNCTION(BlueprintCallable,Category="IC|NPC")
    bool TryInteract(AICPlayerController* Requester);

    UFUNCTION(BlueprintCallable,Category="IC|NPC")
    void EndInteraction();
    UFUNCTION(BlueprintCallable, Category="IC|NPC") bool TryGiveGift(AICPlayerController* Requester, FName ItemID);
    UFUNCTION(BlueprintPure, Category="IC|NPC") bool CanRecruit() const;
    UFUNCTION(BlueprintCallable, Category="IC|NPC") bool TryRecruit(AICPlayerController* Requester);
    UFUNCTION(BlueprintPure, Category="IC|NPC") bool CanDismiss() const;
    UFUNCTION(BlueprintCallable, Category="IC|NPC") bool TryDismiss(AICPlayerController* Requester);

    UFUNCTION(BlueprintPure,Category="IC|NPC")
    FText GetInteractionText() const;

    UFUNCTION(BlueprintPure,Category="IC|NPC")
    TArray<FICNPCInteractionOption> GetInteractionOptions() const;

    UFUNCTION(BlueprintPure,Category="IC|NPC")
    bool IsInteracting() const { return bInteracting; }

    UPROPERTY(BlueprintAssignable,Category="IC|NPC")
    FICNPCInteractionStarted OnInteractionStarted;

    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="IC|NPC")
    FName NPCDefinitionID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|Quest") TObjectPtr<class UWidgetComponent> QuestMarker;

protected:
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|NPC|Movement",meta=(ClampMin="0"))
    float ScheduleMoveSpeed=300.f;
    virtual void StoredCharacterChanged(FName ID) override;

private:
    FICNPCRow Definition;
    bool bInitialized=false,bInteracting=false;
    FTimerHandle DefeatTimer;
};
