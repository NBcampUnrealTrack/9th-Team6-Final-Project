#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "PlayerInventory.generated.h"

UENUM(BlueprintType)
enum class EPlayerItemKind : uint8 { Consumable, Material, Equipment, Quest, Expansion };
UENUM(BlueprintType)
enum class EPlayerEquipSlot : uint8 { Weapon, Clothes, Ring, Necklace, None };

USTRUCT(BlueprintType)
struct FPlayerItemDefinition : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerItemKind Kind = EPlayerItemKind::Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerEquipSlot EquipSlot = EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StackLimit = 99;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanDiscard = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanSell = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EffectID;
};

USTRUCT(BlueprintType)
struct FPlayerItemInstance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid InstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Count = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Grade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Upgrade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, float> Options;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerID = TEXT("Player");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bLocked = false;
    bool IsEmpty() const { return ItemID.IsNone() || Count <= 0; }
};

USTRUCT(BlueprintType)
struct FPlayerContainer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Slots;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInventoryMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerLootResult, FName, ItemID, int32, Accepted, int32, Discarded);

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UPlayerInventory : public UActorComponent
{
    GENERATED_BODY()
public:
    UPlayerInventory();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items") TObjectPtr<UDataTable> ItemTable;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items") TArray<FPlayerItemDefinition> Definitions;
    UPROPERTY(BlueprintReadOnly, SaveGame) FPlayerContainer Bag;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FPlayerContainer> Storage;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryChanged OnChanged;
    UPROPERTY(BlueprintAssignable) FPlayerInventoryMessage OnMessage;
    UPROPERTY(BlueprintAssignable) FPlayerLootResult OnLootResult;
    const FPlayerItemDefinition* Find(FName ID) const;
    UFUNCTION(BlueprintCallable) void ResetInventory();
    UFUNCTION(BlueprintCallable) bool AddItem(FName ID, int32 Count, bool bDiscardOverflow = true);
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
    int32 Insert(FPlayerContainer& Container, FName ID, int32 Count, const FPlayerItemInstance* Original = nullptr) const;
    bool Remove(FPlayerContainer& Container, FName ID, int32 Count) const;
    bool Fail(const TCHAR* Text);
};
