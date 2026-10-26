#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "Data/Rows/Items/ICItemRows.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInventoryMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerLootResult, FName, ItemID, int32, Accepted, int32, Remainder);

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICInventoryComponent();
    class UICDefinitionSubsystem* GetDefinitions() const;
    void BindDefinitions(class UICDefinitionSubsystem* Service) { DefinitionService=Service; }
    void BeginStaging(class UICDefinitionSubsystem* Service,int32 Class);
    void BindWorldState(class UICWorldStateSubsystem* InWorldState);
    UFUNCTION(BlueprintPure) FICInventoryState GetSnapshot() const;
    bool ApplySnapshot(const FICInventoryState& Snapshot) { IC_TRACE_FUNCTION(); return Commit(Snapshot); }
    bool ValidateSnapshot(const FICInventoryState& Snapshot) const;
    bool BuildConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const;
    bool BuildQuestItemConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryChanged OnChanged;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryMessage OnMessage;
    UPROPERTY(BlueprintAssignable) FPlayerLootResult OnLootResult;
    UPROPERTY(BlueprintReadOnly) int32 CurrentClass = 0;
    const struct FICItemRow* Find(FName ID) const;
    UFUNCTION(BlueprintPure,Category="IC|Items") bool GetItemDefinition(FName ID,FPlayerItemDefinition& Definition) const;
    UFUNCTION(BlueprintCallable) void ResetInventory();
    UFUNCTION(BlueprintCallable) int32 TryAddItem(FName ID, int32 Count);
    int32 TryAddItemInstance(const FPlayerItemInstance& Item);
    UFUNCTION(BlueprintCallable) bool AddItem(FName ID, int32 Count, bool bAllowPartial = true);
    UFUNCTION(BlueprintPure) int32 CountItem(FName ID) const;
    UFUNCTION(BlueprintCallable) bool Consume(FName ID, int32 Count);
    UFUNCTION(BlueprintCallable) bool MoveSlot(int32 From, int32 To);
    UFUNCTION(BlueprintCallable) bool SplitStack(int32 From, int32 Amount);
    UFUNCTION(BlueprintCallable) bool ToggleLock(int32 Index);
    UFUNCTION(BlueprintCallable) bool Discard(int32 Index);
    UFUNCTION(BlueprintCallable) bool Equip(int32 Index);
    UFUNCTION(BlueprintCallable) bool EquipForCharacter(FName CharacterID, int32 BagIndex);
    UFUNCTION(BlueprintCallable) bool UnequipForCharacter(FName CharacterID, EPlayerEquipSlot Slot);
    /** Permanently removes the selected stack. Quest and locked items are protected. */
    UFUNCTION(BlueprintCallable) bool TrashItem(int32 BagIndex);
    UFUNCTION(BlueprintCallable, Category="IC|Weapons") bool CycleWeapon();
    UFUNCTION(BlueprintPure, Category="IC|Weapons") EICWeaponFamily GetEquippedWeaponFamily() const;
    UFUNCTION(BlueprintPure, Category="IC|Weapons") EICWeaponFamily GetSelectedWeaponFamily() const;
    UFUNCTION(BlueprintPure, Category="IC|Weapons") bool IsWeaponSheathed() const;
    /** Sword, bow, magic staff, healing wand. Includes the currently held weapon. */
    UFUNCTION(BlueprintPure, Category="IC|Weapons") TArray<FPlayerItemInstance> GetWeaponLoadout() const;
    UFUNCTION(BlueprintCallable, Category="IC|Weapons") bool UnequipWeapon(EICWeaponFamily Family);
    bool BuildNextWeaponState(FICInventoryState& Next) const;
    bool SelectWeaponForTransition(FGuid InstanceID);
    bool SetWeaponSheathed(bool bSheathed);
    UFUNCTION(BlueprintCallable) bool Unequip(EPlayerEquipSlot Slot);
    UFUNCTION(BlueprintCallable) bool EnhanceEquipment(int32 BagIndex,UPARAM(ref) int32& Gold);
    UFUNCTION(BlueprintPure) FPlayerEquipmentStats GetEquipmentStats() const;
    UFUNCTION(BlueprintCallable) void SortBag();
    UFUNCTION(BlueprintCallable) bool BindQuickSlot(int32 QuickIndex, int32 BagIndex);
    UFUNCTION(BlueprintCallable) bool ExpandUsingTicket();
    UFUNCTION(BlueprintCallable) void EnsureStorage(FName ContainerID);
    /** Called by an approved home upgrade transaction; never shrinks occupied legacy slots. */
    UFUNCTION(BlueprintCallable, Category="IC|Home") bool SetHomeStorageStage(int32 Stage);
    UFUNCTION(BlueprintCallable) bool Transfer(FName ContainerID, int32 Index, bool bToStorage);
    UFUNCTION(BlueprintCallable) bool ExchangeItems(const TMap<FName, int32>& Costs, FName ResultID, int32 ResultCount);
    UFUNCTION(BlueprintPure) bool ValidateDefinitions() const;
    bool ValidateState() const;
private:
    UPROPERTY() TObjectPtr<class UICWorldStateSubsystem> WorldState;
    UPROPERTY() TObjectPtr<class UICDefinitionSubsystem> DefinitionService;
    FICInventoryState StagingState;
    bool bStaging=false;
    bool Commit(const FICInventoryState& Snapshot);
    UFUNCTION() void WorldInventoryChanged();
    int32 Insert(FPlayerContainer& Container, FName ID, int32 Count, const FPlayerItemInstance* Original = nullptr) const;
    bool Remove(FPlayerContainer& Container, FName ID, int32 Count) const;
    bool RollRandomOptions(FPlayerItemInstance& Item,const FICItemRow& Definition) const;
    EICWeaponFamily GetItemWeaponFamily(const FPlayerItemInstance& Item) const;
    bool SelectLoadoutWeapon(FICInventoryState& Snapshot, int32 Slot) const;
    bool Fail(const TCHAR* Text);
    friend class UICProductionSubsystem;
    friend class FICRewardService;
    friend class AICDroppedItem;
    bool CommitLoot(const FICInventoryState& Inventory,FName ItemID,int32 Overflow);
};
