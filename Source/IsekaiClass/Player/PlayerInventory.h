#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Core/ICInventoryTypes.h"
#include "../Core/ICWorldStateTypes.h"
#include "PlayerInventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInventoryMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerLootResult, FName, ItemID, int32, Accepted, int32, Remainder);

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UPlayerInventory : public UActorComponent
{
    GENERATED_BODY()
public:
    UPlayerInventory();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items") TObjectPtr<UDataTable> ItemTable;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items") TArray<FPlayerItemDefinition> Definitions;
    void BindWorldState(class UICWorldStateSubsystem* InWorldState);
    UFUNCTION(BlueprintPure) FICInventoryState GetSnapshot() const;
    UFUNCTION(BlueprintCallable) int32 TryAddItem(FName ID, int32 Count);
    bool ValidateSnapshot(const FICInventoryState& Snapshot) const;
    bool BuildConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryChanged OnChanged;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryMessage OnMessage;
    UPROPERTY(BlueprintAssignable) FPlayerLootResult OnLootResult;
    const FPlayerItemDefinition* Find(FName ID) const;
    UFUNCTION(BlueprintCallable) void ResetInventory();
    UFUNCTION(BlueprintCallable) bool AddItem(FName ID, int32 Count, bool bAllowPartial = true);
    UFUNCTION(BlueprintPure) int32 CountItem(FName ID) const;
    UFUNCTION(BlueprintCallable) bool Consume(FName ID, int32 Count);
    UFUNCTION(BlueprintCallable) bool MoveSlot(int32 From, int32 To);
    UFUNCTION(BlueprintCallable) bool SplitStack(int32 From, int32 Amount);
    UFUNCTION(BlueprintCallable) bool ToggleLock(int32 Index);
    UFUNCTION(BlueprintCallable) bool Discard(int32 Index);
    UFUNCTION(BlueprintCallable) bool Equip(int32 Index);
    UFUNCTION(BlueprintCallable) bool Unequip(EPlayerEquipSlot Slot);
    UFUNCTION(BlueprintCallable) void SortBag();
    UFUNCTION(BlueprintCallable) bool BindQuickSlot(int32 QuickIndex, int32 BagIndex);
    UFUNCTION(BlueprintCallable) bool ExpandUsingTicket();
    UFUNCTION(BlueprintCallable) void EnsureStorage(FName ContainerID);
    UFUNCTION(BlueprintCallable) bool Transfer(FName ContainerID, int32 Index, bool bToStorage);
    UFUNCTION(BlueprintCallable) bool ExchangeItems(const TMap<FName, int32>& Costs, FName ResultID, int32 ResultCount);
    bool ValidateState() const;
private:
    UPROPERTY() TObjectPtr<class UICWorldStateSubsystem> WorldState;
    // Standalone objects used by tests have no GameInstance. Gameplay binds WorldState.
    FICInventoryState StandaloneState;
    bool Commit(const FICInventoryState& Snapshot);
    UFUNCTION() void WorldInventoryChanged();
    int32 Insert(FPlayerContainer& Container, FName ID, int32 Count, const FPlayerItemInstance* Original = nullptr) const;
    bool Remove(FPlayerContainer& Container, FName ID, int32 Count) const;
    bool Fail(const TCHAR* Text);
};
