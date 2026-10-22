#include "Inventory/Components/ICInventoryComponent.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Time/ICTimeSubsystem.h"
#include "GameFramework/Actor.h"

UICInventoryComponent::UICInventoryComponent()
{
    IC_TRACE_FUNCTION();
    PrimaryComponentTick.bCanEverTick = false;
}
UICDefinitionSubsystem* UICInventoryComponent::GetDefinitions() const
{
    IC_TRACE_FUNCTION();
    if (DefinitionService) return DefinitionService;
    return GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
}
const FICItemRow* UICInventoryComponent::Find(FName ID) const
{
    IC_TRACE_FUNCTION();
    FICDefinitionResult Result;
    return GetDefinitions() ? GetDefinitions()->Find<FICItemRow>(ID,Result) : nullptr;
}
void UICInventoryComponent::BeginStaging(UICDefinitionSubsystem* Definitions, int32 Class)
{
    IC_TRACE_FUNCTION();
    check(!WorldState && !IsRegistered());
    DefinitionService=Definitions;CurrentClass=Class;StagingState=FICInventoryState();bStaging=true;
}

void UICInventoryComponent::BindWorldState(UICWorldStateSubsystem* InWorldState)
{
    IC_TRACE_FUNCTION();
    if (WorldState) WorldState->OnInventoryChanged.RemoveDynamic(this, &ThisClass::WorldInventoryChanged);
    WorldState = InWorldState;
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Bind] Owner=%s State=%s"),*GetNameSafe(GetOwner()),*GetNameSafe(InWorldState));
    if(WorldState)bStaging=false;
    if (WorldState) WorldState->OnInventoryChanged.AddDynamic(this, &ThisClass::WorldInventoryChanged);
    OnChanged.Broadcast();
}

bool UICInventoryComponent::GetItemDefinition(FName ID,FPlayerItemDefinition& Definition) const
{
    IC_TRACE_FUNCTION();
    if(const auto* Found=Find(ID)){Definition=*Found;return true;}
    Definition=FPlayerItemDefinition();return false;
}

FICInventoryState UICInventoryComponent::GetSnapshot() const { IC_TRACE_FUNCTION(); return WorldState ? WorldState->GetInventorySnapshot() : (bStaging ? StagingState : FICInventoryState()); }
void UICInventoryComponent::WorldInventoryChanged() { IC_TRACE_FUNCTION(); OnChanged.Broadcast(); }

bool UICInventoryComponent::Commit(const FICInventoryState& Snapshot)
{
    IC_TRACE_FUNCTION();
    if (!ValidateSnapshot(Snapshot)) return Fail(*ICText::Get(TEXT("Native_5833b9862e26")).ToString());
    if (WorldState) return WorldState->CommitInventoryState(Snapshot);
    if(!bStaging)return Fail(*ICText::Get(TEXT("Native_33b632698ed4")).ToString());
    StagingState = Snapshot;
    OnChanged.Broadcast();
    return true;
}

void UICInventoryComponent::ResetInventory() { IC_TRACE_FUNCTION(); Commit(FICInventoryState()); }
bool UICInventoryComponent::Fail(const TCHAR* Text) { IC_TRACE_FUNCTION(); UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Rejected] Owner=%s Reason=%s"),*GetNameSafe(GetOwner()),Text);OnMessage.Broadcast(FText::FromString(Text)); return false; }

int32 UICInventoryComponent::Insert(FPlayerContainer& C, FName ID, int32 Count, const FPlayerItemInstance* Original) const
{
    IC_TRACE_FUNCTION();
    const auto* D = Find(ID); if (!D || Count <= 0) return 0;
    const int32 Limit = D->Kind == EPlayerItemKind::Equipment ? 1 : FMath::Clamp(D->StackLimit, 1, 99);
    int32 Left = Count;
    if (Limit > 1) for (auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked && S.Count < Limit)
    { const int32 N = FMath::Min(Left, Limit - S.Count); S.Count += N; Left -= N; if (!Left) return Count; }
    for (auto& S : C.Slots) if (S.IsEmpty())
    {
        S = Original ? *Original : FPlayerItemInstance(); S.ItemID = ID;
        if (!Original) { S.Grade=static_cast<int32>(D->DefaultGrade); if(D->Kind==EPlayerItemKind::Equipment && !RollRandomOptions(S,*D)) {S=FPlayerItemInstance();return 0;} }
        if (!Original || !S.InstanceID.IsValid() || Left!=Count) S.InstanceID = FGuid::NewGuid();
        S.Count = FMath::Min(Left, Limit); Left -= S.Count; if (!Left) break;
    }
    return Count - Left;
}

int32 UICInventoryComponent::TryAddItem(FName ID, int32 Count)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.TryAddItem.Request] Item=%s Count=%d"),*ID.ToString(),Count);
    const auto* D = Find(ID);
    if (!D || D->StackLimit<1 || D->StackLimit>99 || Count<=0 || Count>100000) { Fail(*ICText::Get(TEXT("Native_ee6c7695df95")).ToString()); return FMath::Max(0, Count); }
    auto Next = GetSnapshot();
    int32 Added = 0;
    if (D->Kind == EPlayerItemKind::Quest)
    {
        FPlayerContainer C; C.Slots = Next.QuestItems;

        if (Count > 100000) { Fail(*ICText::Get(TEXT("Native_d303952427a0")).ToString()); return Count; }
        C.Slots.SetNum(C.Slots.Num() + FMath::DivideAndRoundUp(Count, FMath::Clamp(D->StackLimit, 1, 99)));
        Added = Insert(C, ID, Count);
        C.Slots.RemoveAll([](const auto& S) { IC_TRACE_FUNCTION(); return S.IsEmpty(); }); Next.QuestItems = MoveTemp(C.Slots);
    }
    else Added = Insert(Next.Bag, ID, Count);
    if ((Added>0 || Count>Added) && !CommitLoot(Next,ID,Count-Added))return Count;
    OnLootResult.Broadcast(ID, Added, Count - Added);
    UE_LOG(LogICInventory, Log, TEXT("[Inventory.Loot] Item=%s Requested=%d Accepted=%d Overflow=%d"),*ID.ToString(),Count,Added,Count-Added);
    return Count - Added;
}

bool UICInventoryComponent::AddItem(FName ID, int32 Count, bool bAllowPartial)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.AddItem.Request] Item=%s Count=%d Partial=%d"),*ID.ToString(),Count,bAllowPartial);
    const auto* D = Find(ID); if (!D || Count <= 0) return Fail(*ICText::Get(TEXT("Native_ee6c7695df95")).ToString());
    if (bAllowPartial || D->Kind == EPlayerItemKind::Quest)
    {
        const int32 Left = TryAddItem(ID, Count);
        return Left == 0;
    }
    auto Next = GetSnapshot();
    const int32 Added = Insert(Next.Bag, ID, Count);
    if (Added != Count) return Fail(*ICText::Get(TEXT("Native_d90ee7cd79e4")).ToString());
    if (!Commit(Next)) return false;
    UE_LOG(LogICInventory, Log, TEXT("[Inventory.Added] Item=%s Count=%d"),*ID.ToString(),Count);OnLootResult.Broadcast(ID, Count, 0); return true;
}

int32 UICInventoryComponent::CountItem(FName ID) const
{
    IC_TRACE_FUNCTION();
    int64 Count = 0;
    for (const auto& S : GetSnapshot().Bag.Slots) if (S.ItemID == ID && !S.bLocked) Count += S.Count;
    return static_cast<int32>(FMath::Min<int64>(Count, MAX_int32));
}

bool UICInventoryComponent::Remove(FPlayerContainer& C, FName ID, int32 Count) const
{
    IC_TRACE_FUNCTION();
    if (Count <= 0 || !Find(ID)) return false;
    int64 Available = 0; for (const auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked) Available += S.Count;
    if (Available < Count) return false;
    for (auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked)
    { const int32 N = FMath::Min(Count, S.Count); S.Count -= N; Count -= N; if (!S.Count) S = FPlayerItemInstance(); if (!Count) break; }
    return true;
}

bool UICInventoryComponent::Consume(FName ID, int32 Count)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Consume.Request] Item=%s Count=%d"),*ID.ToString(),Count);
    auto Next = GetSnapshot();
    if (!Remove(Next.Bag, ID, Count)) return Fail(*ICText::Get(TEXT("Native_86d4e1888edc")).ToString());
    return Commit(Next);
}

bool UICInventoryComponent::BuildConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const
{
    IC_TRACE_FUNCTION();
    OutSnapshot = GetSnapshot();
    return Remove(OutSnapshot.Bag, ID, Count) && ValidateSnapshot(OutSnapshot);
}

bool UICInventoryComponent::BuildQuestItemConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const
{
    IC_TRACE_FUNCTION();
    OutSnapshot = GetSnapshot();
    const FICItemRow* Item = Find(ID);
    if (!Item)
    {
        return false;
    }
    if (Item->Kind != EPlayerItemKind::Quest)
    {
        return Remove(OutSnapshot.Bag, ID, Count) && ValidateSnapshot(OutSnapshot);
    }
    FPlayerContainer QuestContainer;
    QuestContainer.Slots = OutSnapshot.QuestItems;
    if (!Remove(QuestContainer, ID, Count))
    {
        return false;
    }
    QuestContainer.Slots.RemoveAll([](const FPlayerItemInstance& Entry)
    {
        return Entry.IsEmpty();
    });
    OutSnapshot.QuestItems = MoveTemp(QuestContainer.Slots);
    return ValidateSnapshot(OutSnapshot);
}

bool UICInventoryComponent::MoveSlot(int32 From, int32 To)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.MoveSlot.Request] From=%d To=%d"),From,To);
    auto Next = GetSnapshot();
    if (!Next.Bag.Slots.IsValidIndex(From) || !Next.Bag.Slots.IsValidIndex(To) || From == To) return false;
    auto& A = Next.Bag.Slots[From]; auto& B = Next.Bag.Slots[To];
    if (A.IsEmpty() || A.bLocked || B.bLocked) return Fail(*ICText::Get(TEXT("Native_bf45b65a47cf")).ToString());
    const auto* D = Find(A.ItemID);
    if (A.ItemID == B.ItemID && D && D->Kind != EPlayerItemKind::Equipment)
    { const int32 N = FMath::Min(A.Count, FMath::Clamp(D->StackLimit, 1, 99) - B.Count); A.Count -= N; B.Count += N; if (!A.Count) A = FPlayerItemInstance(); }
    else Swap(A, B);
    return Commit(Next);
}

bool UICInventoryComponent::SplitStack(int32 From, int32 Amount)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.SplitStack.Request] Slot=%d Amount=%d"),From,Amount);
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(From)) return false;
    auto& S = Next.Bag.Slots[From]; const auto* D = Find(S.ItemID);
    if (!D || S.bLocked || D->Kind == EPlayerItemKind::Equipment || Amount <= 0 || Amount >= S.Count) return Fail(*ICText::Get(TEXT("Native_0995f10806e5")).ToString());
    const int32 Empty = Next.Bag.Slots.IndexOfByPredicate([](const auto& X) { IC_TRACE_FUNCTION(); return X.IsEmpty(); });
    if (Empty == INDEX_NONE) return Fail(*ICText::Get(TEXT("Native_c3223af5a8df")).ToString());
    FPlayerItemInstance Copy = S; Copy.InstanceID = FGuid::NewGuid(); Copy.Count = Amount;
    S.Count -= Amount; Next.Bag.Slots[Empty] = Copy;
    return Commit(Next);
}

bool UICInventoryComponent::ToggleLock(int32 Index)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.ToggleLock.Request] Slot=%d"),Index);
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index) || Next.Bag.Slots[Index].IsEmpty()) return false;
    Next.Bag.Slots[Index].bLocked = !Next.Bag.Slots[Index].bLocked; return Commit(Next);
}

bool UICInventoryComponent::Discard(int32 Index)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Discard.Request] Slot=%d"),Index);
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index)) return false;
    auto& S = Next.Bag.Slots[Index]; const auto* D = Find(S.ItemID);
    if (!D || !D->bCanDiscard || S.bLocked) return Fail(*ICText::Get(TEXT("Native_e092954ef79e")).ToString());
    if(!WorldState || !GetWorld() || !GetWorld()->GetGameInstance())return Fail(*ICText::Get(TEXT("Native_f2d3b72a6db7")).ToString());
    auto* Clock=GetWorld()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>();if(!Clock)return false;
    FICDropState Drop;Drop.ID=FGuid::NewGuid();Drop.Contents.Add(S);
    Drop.Position=GetOwner()?GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*100.f:WorldState->ExportState().PlayerPosition;
    Drop.CreatedPlaySecond=Clock->GetClock().PlaySecond;
    S=FPlayerItemInstance();FICWorldState Candidate=WorldState->ExportState();Candidate.Inventory=Next;Candidate.Drops.Add(Drop);
    return ValidateSnapshot(Next)&&WorldState->CommitGameplayTransaction(Candidate,FGuid::NewGuid());
}

bool UICInventoryComponent::Equip(int32 Index)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Equip.Request] Slot=%d Class=%d"),Index,CurrentClass);
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index)) return false;
    auto& S = Next.Bag.Slots[Index]; const auto* D = Find(S.ItemID);
    if (!D || D->Kind != EPlayerItemKind::Equipment || D->EquipSlot == EPlayerEquipSlot::None || S.bLocked) return Fail(*ICText::Get(TEXT("Native_cfa5c06f1ded")).ToString());
    FICDefinitionResult Lookup;
    const auto* Gear=GetDefinitions()?GetDefinitions()->Find<FICEquipmentRow>(D->EquipmentID,Lookup):nullptr;
    if(!Gear || Gear->Slot!=D->EquipSlot)return Fail(*ICText::Get(TEXT("Native_6eec833069e7")).ToString());
    if(Gear->Slot!=EPlayerEquipSlot::Weapon && Gear->RequiredClass!=EICCharacterClass::None && static_cast<int32>(Gear->RequiredClass)!=CurrentClass+1)
        return Fail(*ICText::Get(TEXT("Native_5256ca0c34f2")).ToString());
    Swap(S, Next.Equipment[static_cast<int32>(D->EquipSlot)]); return Commit(Next);
}

EICWeaponFamily UICInventoryComponent::GetEquippedWeaponFamily() const
{
    const auto Snapshot = GetSnapshot();
    const int32 Index = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Snapshot.Equipment.IsValidIndex(Index) || Snapshot.Equipment[Index].IsEmpty()) return EICWeaponFamily::None;
    const auto* Item = Find(Snapshot.Equipment[Index].ItemID);
    FICDefinitionResult Result;
    const auto* Gear = Item && GetDefinitions() ? GetDefinitions()->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
    return Gear ? Gear->WeaponFamily : EICWeaponFamily::None;
}

bool UICInventoryComponent::CycleWeapon()
{
    const auto Snapshot = GetSnapshot();
    const auto Current = GetEquippedWeaponFamily();
    TArray<int32> Candidates;
    for (int32 Index = 0; Index < Snapshot.Bag.Slots.Num(); ++Index)
    {
        const auto& Instance = Snapshot.Bag.Slots[Index];
        const auto* Item = Instance.IsEmpty() || Instance.bLocked ? nullptr : Find(Instance.ItemID);
        if (!Item || Item->Kind != EPlayerItemKind::Equipment || Item->EquipSlot != EPlayerEquipSlot::Weapon) continue;
        FICDefinitionResult Result;
        const auto* Gear = GetDefinitions() ? GetDefinitions()->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
        if (Gear && Gear->WeaponFamily != EICWeaponFamily::None) Candidates.Add(Index);
    }
    if (Candidates.IsEmpty()) return Fail(*ICText::Get(TEXT("Native_92e884b0dc6b")).ToString());
    auto Family = [&](int32 Index)
    {
        FICDefinitionResult Result;
        const auto* Item = Find(Snapshot.Bag.Slots[Index].ItemID);
        const auto* Gear = GetDefinitions()->Find<FICEquipmentRow>(Item->EquipmentID, Result);
        return static_cast<int32>(Gear->WeaponFamily);
    };
    Candidates.Sort([&](int32 A, int32 B)
    {
        const int32 RankA = (Family(A) - static_cast<int32>(Current) + 4) % 4;
        const int32 RankB = (Family(B) - static_cast<int32>(Current) + 4) % 4;
        const int32 PriorityA = RankA == 0 ? 4 : RankA;
        const int32 PriorityB = RankB == 0 ? 4 : RankB;
        return PriorityA == PriorityB ? A < B : PriorityA < PriorityB;
    });
    return Equip(Candidates[0]);
}

bool UICInventoryComponent::Unequip(EPlayerEquipSlot Slot)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Unequip.Request] EquipSlot=%d"),static_cast<int32>(Slot));
    auto Next = GetSnapshot(); const int32 Index = static_cast<int32>(Slot);
    if (!Next.Equipment.IsValidIndex(Index) || Next.Equipment[Index].IsEmpty()) return false;
    const int32 Empty = Next.Bag.Slots.IndexOfByPredicate([](const auto& S) { IC_TRACE_FUNCTION(); return S.IsEmpty(); });
    if (Empty == INDEX_NONE) return Fail(*ICText::Get(TEXT("Native_89e1471e83a4")).ToString());
    Next.Bag.Slots[Empty] = Next.Equipment[Index]; Next.Equipment[Index] = FPlayerItemInstance(); return Commit(Next);
}

void UICInventoryComponent::SortBag()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.SortBag.Request] Slots=%d"),GetSnapshot().Bag.Slots.Num());
    auto Next = GetSnapshot(); TArray<FPlayerItemInstance> Items;
    for (const auto& S : Next.Bag.Slots) if (!S.bLocked && !S.IsEmpty()) Items.Add(S);
    Items.StableSort([](const auto& A, const auto& B) { IC_TRACE_FUNCTION(); return A.ItemID.LexicalLess(B.ItemID); });
    int32 N = 0; for (auto& S : Next.Bag.Slots) if (!S.bLocked) S = Items.IsValidIndex(N) ? Items[N++] : FPlayerItemInstance();
    Commit(Next);
}

bool UICInventoryComponent::BindQuickSlot(int32 Quick, int32 Index)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.BindQuickSlot.Request] Quick=%d BagSlot=%d"),Quick,Index);
    auto Next = GetSnapshot(); if (!Next.QuickSlots.IsValidIndex(Quick) || !Next.Bag.Slots.IsValidIndex(Index)) return false;
    const auto* D = Find(Next.Bag.Slots[Index].ItemID);
    if (!D || (D->Kind != EPlayerItemKind::Consumable && D->Kind != EPlayerItemKind::Expansion)) return Fail(*ICText::Get(TEXT("Native_8f821e2cc767")).ToString());
    Next.QuickSlots[Quick] = D->ItemID; return Commit(Next);
}

bool UICInventoryComponent::ExpandUsingTicket()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.ExpandUsingTicket.Request] BeforeSlots=%d"),GetSnapshot().Bag.Slots.Num());
    auto Next = GetSnapshot(); if (Next.Bag.Slots.Num() >= 100) return Fail(*ICText::Get(TEXT("Native_ad88655591b8")).ToString());
    if (!Remove(Next.Bag, TEXT("ITM_TKT_001"), 1)) return Fail(*ICText::Get(TEXT("Native_d1e4a13152f9")).ToString());
    Next.Bag.Slots.SetNum(Next.Bag.Slots.Num() + 10); return Commit(Next);
}

void UICInventoryComponent::EnsureStorage(FName ID)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.EnsureStorage.Request] Container=%s"),*ID.ToString());
    auto Next = GetSnapshot(); if (!ID.IsNone() && !Next.Storage.Contains(ID)) { Next.Storage.Add(ID).Slots.SetNum(40); Commit(Next); }
}

bool UICInventoryComponent::Transfer(FName ID, int32 Index, bool bToStorage)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Transfer.Request] Container=%s Slot=%d ToStorage=%d"),*ID.ToString(),Index,bToStorage);
    auto Next = GetSnapshot(); auto* Container = Next.Storage.Find(ID); if (!Container) return false;
    auto& Source = bToStorage ? Next.Bag : *Container; auto& Destination = bToStorage ? *Container : Next.Bag;
    if (!Source.Slots.IsValidIndex(Index) || Source.Slots[Index].IsEmpty() || Source.Slots[Index].bLocked) return false;
    const auto Item = Source.Slots[Index]; FPlayerContainer NewDestination = Destination;
    // Legacy 160-slot saves stay readable/withdrawable; new deposits use at most 120 home slots.
    const bool LegacyHome=bToStorage&&ID==TEXT("PlayerHome")&&NewDestination.Slots.Num()>120;
    FPlayerContainer Deposit=NewDestination;
    if(LegacyHome)Deposit.Slots.SetNum(120);
    if (Insert(Deposit, Item.ItemID, Item.Count, &Item) != Item.Count) return Fail(*ICText::Get(TEXT("Native_ce19791ea019")).ToString());
    if(LegacyHome){for(int32 Slot=0;Slot<120;++Slot)NewDestination.Slots[Slot]=Deposit.Slots[Slot];}
    else NewDestination=MoveTemp(Deposit);
    Destination = MoveTemp(NewDestination); Source.Slots[Index] = FPlayerItemInstance(); return Commit(Next);
}

bool UICInventoryComponent::SetHomeStorageStage(int32 Stage)
{
    if(Stage<1||Stage>3)return false;
    if(WorldState&&!WorldState->FlushParticipants())return false;
    auto Next=GetSnapshot();auto& Storage=Next.Storage.FindOrAdd(TEXT("PlayerHome"));
    const int32 Capacity=Stage*40;
    // Do not reduce capacity or overwrite items from older saves.
    if(Storage.Slots.Num()>Capacity)return false;
    Storage.Slots.SetNum(Capacity);
    if(!WorldState)return Commit(Next);
    auto Candidate=WorldState->ExportState();Candidate.Inventory=Next;
    Candidate.ProgressionStages.Add(TEXT("Home.Storage"),Stage);
    return ValidateSnapshot(Next)&&WorldState->CommitGameplayTransaction(Candidate,FGuid::NewGuid());
}

bool UICInventoryComponent::ExchangeItems(const TMap<FName, int32>& Costs, FName ID, int32 Count)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.ExchangeItems.Request] Result=%s Count=%d CostKinds=%d"),*ID.ToString(),Count,Costs.Num());
    const auto* D = Find(ID); if (!D || Count <= 0 || D->Kind == EPlayerItemKind::Quest) return false;
    auto Next = GetSnapshot(); for (const auto& Cost : Costs) if (!Remove(Next.Bag, Cost.Key, Cost.Value)) return Fail(*ICText::Get(TEXT("Native_ae11dc64bfc4")).ToString());
    if (Insert(Next.Bag, ID, Count) != Count) return Fail(*ICText::Get(TEXT("Native_eaa99b9549f6")).ToString());
    return Commit(Next);
}

bool UICInventoryComponent::ValidateSnapshot(const FICInventoryState& Snapshot) const
{
    IC_TRACE_FUNCTION();
    if (!UICWorldStateSubsystem::ValidateInventoryShape(Snapshot)) return false;
    auto Check = [this](const FPlayerItemInstance& Item, bool bQuest)
    {
        IC_TRACE_FUNCTION();
        if (Item.IsEmpty()) return true;
        const auto* D = Find(Item.ItemID);if(!D)return false;
        auto* Service=GetDefinitions();FICDefinitionResult R;
        if(D->Kind==EPlayerItemKind::Equipment)
        {
            const auto* Gear=Service?Service->Find<FICEquipmentRow>(D->EquipmentID,R):nullptr;
            const auto* Rules=Gear?Service->Find<FICEnhancementRow>(Gear->EnhancementID,R):nullptr;
            if(!Gear || !Rules || Gear->ItemID!=D->ID || Gear->Slot!=D->EquipSlot || Item.Upgrade>Rules->MaximumLevel)return false;
            for(const auto& O:Item.Options)
            {
                const auto* Rule=Service->Find<FICOptionRow>(O.Key,R);
                if(!Rule || !FMath::IsFinite(O.Value) || O.Value<Rule->MinimumValue || O.Value>Rule->MaximumValue
                    || (!Rule->AllowedSlots.IsEmpty()&&!Rule->AllowedSlots.Contains(Gear->Slot))
                    || (!Rule->AllowedWeaponFamilies.IsEmpty()&&!Rule->AllowedWeaponFamilies.Contains(Gear->WeaponFamily)))return false;
            }
        }
        else if(!Item.Options.IsEmpty()||Item.Upgrade!=0)return false;
        return D && Item.Grade>=0 && Item.Grade<=static_cast<int32>(EPlayerItemGrade::Legendary)
            && Item.Upgrade>=0 && Item.Upgrade<=D->MaxEnhanceLevel && Item.Options.Num()<=D->MaxRandomOptions
            && (D->Kind == EPlayerItemKind::Quest) == bQuest && Item.Count <= (D->Kind == EPlayerItemKind::Equipment ? 1 : FMath::Clamp(D->StackLimit, 1, 99));
    };
    for (const auto& S : Snapshot.Bag.Slots) if (!Check(S, false)) return false;
    for (const auto& S : Snapshot.QuestItems) if (!Check(S, true)) return false;
    for (int32 Index = 0; Index < Snapshot.Equipment.Num(); ++Index)
    {
        const auto& S = Snapshot.Equipment[Index];
        if (!Check(S, false) || (!S.IsEmpty() && static_cast<int32>(Find(S.ItemID)->EquipSlot) != Index)) return false;
    }
    for (const auto& C : Snapshot.Storage) for (const auto& S : C.Value.Slots) if (!Check(S, false)) return false;
    for (FName ID : Snapshot.QuickSlots) if (!ID.IsNone())
    { const auto* D = Find(ID); if (!D || (D->Kind != EPlayerItemKind::Consumable && D->Kind != EPlayerItemKind::Expansion)) return false; }
    return true;
}

bool UICInventoryComponent::ValidateState() const { IC_TRACE_FUNCTION(); return ValidateDefinitions() && ValidateSnapshot(GetSnapshot()); }

int32 UICInventoryComponent::TryAddItemInstance(const FPlayerItemInstance& Item)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.TryAddItemInstance.Request] Item=%s Instance=%s Count=%d"),*Item.ItemID.ToString(),*Item.InstanceID.ToString(),Item.Count);
    const auto* Definition=Find(Item.ItemID);
    if(!Definition||Item.IsEmpty())return FMath::Max(0,Item.Count);
    if(Definition->Kind==EPlayerItemKind::Quest)
    {

        auto Next=GetSnapshot(); FPlayerContainer Container; Container.Slots=Next.QuestItems;
        if(Item.Count>100000)return Item.Count;
        Container.Slots.SetNum(Container.Slots.Num()+FMath::DivideAndRoundUp(Item.Count,FMath::Clamp(Definition->StackLimit,1,99)));
        const int32 Added=Insert(Container,Item.ItemID,Item.Count,&Item);
        Container.Slots.RemoveAll([](const auto& S){ IC_TRACE_FUNCTION();return S.IsEmpty();});Next.QuestItems=MoveTemp(Container.Slots);
        return Added>0&&Commit(Next)?Item.Count-Added:Item.Count;
    }
    auto Next=GetSnapshot(); const int32 Added=Insert(Next.Bag,Item.ItemID,Item.Count,&Item);
    if(Added>0&&!Commit(Next))return Item.Count;
    return Item.Count-Added;
}

bool UICInventoryComponent::CommitLoot(const FICInventoryState& Inventory,FName ItemID,int32 Overflow)
{
    IC_TRACE_FUNCTION();
    if(Overflow<=0)return Commit(Inventory);
    if(!WorldState || !GetWorld() || !GetWorld()->GetGameInstance() || Overflow>100000)return Fail(*ICText::Get(TEXT("Native_d93ebfd6d58e")).ToString());
    auto* Clock=GetWorld()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>();const auto* D=Find(ItemID);
    if(!Clock||!D||D->StackLimit<1||D->StackLimit>99)return false;
    FICWorldState Candidate=WorldState->ExportState();Candidate.Inventory=Inventory;
    for(int32 Remaining=Overflow;Remaining>0;)
    {
        FPlayerItemInstance Item;Item.ItemID=ItemID;Item.InstanceID=FGuid::NewGuid();Item.Count=FMath::Min(Remaining,D->Kind==EPlayerItemKind::Equipment?1:D->StackLimit);
        Item.Grade=static_cast<int32>(D->DefaultGrade);
        if(D->Kind==EPlayerItemKind::Equipment&&!RollRandomOptions(Item,*D))return false;
        Remaining-=Item.Count;FICDropState Drop;Drop.ID=FGuid::NewGuid();Drop.Contents.Add(Item);
        Drop.Position=GetOwner()?GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*100.f:Candidate.PlayerPosition;
        Drop.CreatedPlaySecond=Clock->GetClock().PlaySecond;Candidate.Drops.Add(Drop);
    }
    return ValidateSnapshot(Inventory)&&WorldState->CommitGameplayTransaction(Candidate,FGuid::NewGuid());
}
