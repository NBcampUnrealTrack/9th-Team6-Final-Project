#include "Inventory/Components/ICInventoryComponent.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Time/ICTimeSubsystem.h"
#include "GameFramework/Actor.h"
#include "Player/Character/ICPlayerCharacter.h"

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
    if (Limit > 1) for (auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked && S.Count < Limit
        &&(!Original||(S.Grade==Original->Grade&&S.Upgrade==Original->Upgrade&&S.OwnerID==Original->OwnerID&&S.Options.OrderIndependentCompareEqual(Original->Options))))
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
    if (!D || D->Kind != EPlayerItemKind::Equipment || D->EquipSlot != EPlayerEquipSlot::Weapon || S.bLocked) return Fail(*ICText::Get(TEXT("Native_cfa5c06f1ded")).ToString());
    FICDefinitionResult Lookup;
    const auto* Gear=GetDefinitions()?GetDefinitions()->Find<FICEquipmentRow>(D->EquipmentID,Lookup):nullptr;
    if(!Gear || Gear->Slot!=D->EquipSlot)return Fail(*ICText::Get(TEXT("Native_6eec833069e7")).ToString());
    FText Requirement;if(!MeetsEquipmentRequirements(UICWorldStateSubsystem::PlayerId(),S.ItemID,Requirement))return Fail(*Requirement.ToString());
    if (D->EquipSlot == EPlayerEquipSlot::Weapon)
    {
        if (const auto* Player=Cast<AICPlayerCharacter>(GetOwner());Player&&Player->IsSwitchingWeapon()) return false;
        const int32 FamilySlot=static_cast<int32>(Gear->WeaponFamily)-1;
        if (!Next.WeaponSlots.IsValidIndex(FamilySlot)) return false;
        auto& Selected=Next.Equipment[static_cast<int32>(EPlayerEquipSlot::Weapon)];
        if ((Selected.IsEmpty() && Next.WeaponSlots[FamilySlot].IsEmpty()) || GetItemWeaponFamily(Selected)==Gear->WeaponFamily) Swap(S,Selected);
        else Swap(S,Next.WeaponSlots[FamilySlot]);
    }
    return Commit(Next);
}

bool UICInventoryComponent::EquipForCharacter(FName ID,int32 Index)
{
    if(ID==UICWorldStateSubsystem::PlayerId())return Equip(Index);
    if(!WorldState)return false;
    auto Character=WorldState->GetSnapshot(ID);auto Next=GetSnapshot();
    if(Character.Kind!=EICCharacterKind::Recruitable||Character.Mode!=EICCharacterMode::Companion||Character.PartySlot<0
        ||!Next.Bag.Slots.IsValidIndex(Index))return false;
    auto& Item=Next.Bag.Slots[Index];const auto* Definition=Find(Item.ItemID);FICDefinitionResult R;
    auto* D=GetDefinitions();
    const auto* Gear=Definition&&D?D->Find<FICEquipmentRow>(Definition->EquipmentID,R):nullptr;
    const auto* NPC=D?D->Find<FICNPCRow>(ID,R):nullptr;
    const auto* Recruit=NPC?D->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,R):nullptr;
    const auto* Class=Recruit?D->Find<FICClassRow>(Recruit->ClassID,R):nullptr;
    if(Item.IsEmpty()||Item.bLocked||!Definition||Definition->Kind!=EPlayerItemKind::Equipment||!Gear||!Class
        ||Gear->Slot!=Definition->EquipSlot||Gear->Slot!=EPlayerEquipSlot::Weapon
        ||(Gear->Slot==EPlayerEquipSlot::Weapon&&Gear->WeaponFamily!=Recruit->WeaponFamily)
        ||(Gear->RequiredClass!=EICCharacterClass::None&&Gear->RequiredClass!=Class->CharacterClass))return false;
    FText Requirement;if(!MeetsEquipmentRequirements(ID,Item.ItemID,Requirement))return Fail(*Requirement.ToString());
    Character.Equipment.SetNum(1);Swap(Item,Character.Equipment[0]);
    return ValidateSnapshot(Next)&&WorldState->CommitCharacterAndInventory(Character,Next);
}

bool UICInventoryComponent::UnequipForCharacter(FName ID,EPlayerEquipSlot Slot)
{
    if(Slot!=EPlayerEquipSlot::Weapon)return false;
    if(ID==UICWorldStateSubsystem::PlayerId())return Unequip(Slot);
    if(!WorldState)return false;
    auto Character=WorldState->GetSnapshot(ID);auto Next=GetSnapshot();const int32 Index=static_cast<int32>(Slot);
    const int32 Empty=Next.Bag.Slots.IndexOfByPredicate([](const auto& Item){return Item.IsEmpty();});
    if(Character.Kind!=EICCharacterKind::Recruitable||Character.Mode!=EICCharacterMode::Companion||Character.PartySlot<0
        ||!Character.Equipment.IsValidIndex(Index)||Character.Equipment[Index].IsEmpty()||Empty==INDEX_NONE)return false;
    Next.Bag.Slots[Empty]=Character.Equipment[Index];Character.Equipment[Index]=FPlayerItemInstance();
    return ValidateSnapshot(Next)&&WorldState->CommitCharacterAndInventory(Character,Next);
}

bool UICInventoryComponent::TrashItem(int32 Index)
{
    auto Next=GetSnapshot();if(!Next.Bag.Slots.IsValidIndex(Index))return false;
    const auto Item=Next.Bag.Slots[Index];const auto* D=Find(Item.ItemID);
    if(Item.IsEmpty()||Item.bLocked||!D||!D->bCanDiscard||D->Kind==EPlayerItemKind::Quest)return false;
    Next.Bag.Slots[Index]=FPlayerItemInstance();
    if(!Next.Bag.Slots.ContainsByPredicate([&](const auto& Other){return !Other.IsEmpty()&&Other.ItemID==Item.ItemID;}))
        for(auto& Quick:Next.QuickSlots)if(Quick==Item.ItemID)Quick=NAME_None;
    return Commit(Next);
}

EICWeaponFamily UICInventoryComponent::GetEquippedWeaponFamily() const
{
    return IsWeaponSheathed() ? EICWeaponFamily::None : GetSelectedWeaponFamily();
}

bool UICInventoryComponent::IsWeaponSheathed() const
{
    return GetSnapshot().bWeaponSheathed;
}

EICWeaponFamily UICInventoryComponent::GetSelectedWeaponFamily() const
{
    const auto Snapshot = GetSnapshot();
    const int32 Index = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Snapshot.Equipment.IsValidIndex(Index) || Snapshot.Equipment[Index].IsEmpty()) return EICWeaponFamily::None;
    return GetItemWeaponFamily(Snapshot.Equipment[Index]);
}

EICWeaponFamily UICInventoryComponent::GetItemWeaponFamily(const FPlayerItemInstance& Instance) const
{
    const auto* Item = Instance.IsEmpty() ? nullptr : Find(Instance.ItemID);
    FICDefinitionResult Result;
    const auto* Gear = Item && GetDefinitions() ? GetDefinitions()->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
    return Gear && Gear->Slot==EPlayerEquipSlot::Weapon ? Gear->WeaponFamily : EICWeaponFamily::None;
}

TArray<FPlayerItemInstance> UICInventoryComponent::GetWeaponLoadout() const
{
    const auto Snapshot=GetSnapshot();
    auto Slots=Snapshot.WeaponSlots;
    Slots.SetNum(4);
    const int32 WeaponSlot=static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (Snapshot.Equipment.IsValidIndex(WeaponSlot))
    {
        const int32 SelectedSlot=static_cast<int32>(GetItemWeaponFamily(Snapshot.Equipment[WeaponSlot]))-1;
        if (Slots.IsValidIndex(SelectedSlot)) Slots[SelectedSlot]=Snapshot.Equipment[WeaponSlot];
    }
    return Slots;
}

bool UICInventoryComponent::SelectLoadoutWeapon(FICInventoryState& Snapshot,int32 Slot) const
{
    const int32 WeaponSlot=static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Snapshot.WeaponSlots.IsValidIndex(Slot) || Snapshot.WeaponSlots[Slot].IsEmpty()) return false;
    auto& Selected=Snapshot.Equipment[WeaponSlot];
    auto Incoming=Snapshot.WeaponSlots[Slot];
    Snapshot.WeaponSlots[Slot]=FPlayerItemInstance();
    if (!Selected.IsEmpty())
    {
        const int32 Previous=static_cast<int32>(GetItemWeaponFamily(Selected))-1;
        if (!Snapshot.WeaponSlots.IsValidIndex(Previous) || !Snapshot.WeaponSlots[Previous].IsEmpty()) return false;
        Snapshot.WeaponSlots[Previous]=Selected;
    }
    Selected=Incoming;
    return true;
}

bool UICInventoryComponent::CycleWeapon()
{
    FICInventoryState Next;
    return BuildNextWeaponState(Next) && Commit(Next);
}

bool UICInventoryComponent::BuildNextWeaponState(FICInventoryState& Next) const
{
    Next = GetSnapshot();
    const int32 WeaponSlot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Next.Equipment.IsValidIndex(WeaponSlot)) return false;
    const int32 Current = static_cast<int32>(GetEquippedWeaponFamily());
    int32 BestFamily = MAX_int32, BestSlot = INDEX_NONE;
    auto Consider = [&](const FPlayerItemInstance& Instance, int32 Slot)
    {
        const int32 Family = static_cast<int32>(GetItemWeaponFamily(Instance));
        if (Family > Current && Family < BestFamily)
        { BestFamily = Family; BestSlot = Slot; }
    };
    // Only the four equipped family slots participate. Bag contents never change.
    Consider(Next.Equipment[WeaponSlot], INDEX_NONE);
    for (int32 Index = 0; Index < Next.WeaponSlots.Num(); ++Index) Consider(Next.WeaponSlots[Index], Index);
    if (BestFamily == MAX_int32)
    {
        if (Current == 0) return false;
        Next.bWeaponSheathed = true;
    }
    else
    {
        if (BestSlot != INDEX_NONE && !SelectLoadoutWeapon(Next,BestSlot)) return false;
        Next.bWeaponSheathed = false;
    }
    return true;
}

bool UICInventoryComponent::SetWeaponSheathed(bool bSheathed)
{
    auto Next = GetSnapshot();
    if (Next.bWeaponSheathed == bSheathed) return true;
    Next.bWeaponSheathed = bSheathed;
    return Commit(Next);
}

bool UICInventoryComponent::DrawWeaponForTransition(FGuid InstanceID)
{
    if (!InstanceID.IsValid()) return false;
    auto Next = GetSnapshot();
    const int32 WeaponSlot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Next.Equipment.IsValidIndex(WeaponSlot)) return false;
    if (Next.Equipment[WeaponSlot].InstanceID == InstanceID && !Next.bWeaponSheathed) return true;
    if (Next.Equipment[WeaponSlot].InstanceID != InstanceID)
    {
        const int32 Slot = Next.WeaponSlots.IndexOfByPredicate([&](const auto& Item) { return Item.InstanceID == InstanceID; });
        if (!SelectLoadoutWeapon(Next,Slot)) return false;
    }
    // Keep the previous weapon drawn until the new weapon replaces it. Never
    // publish an empty-hand state between two occupied equipment slots.
    Next.bWeaponSheathed = false;
    return Commit(Next);
}

bool UICInventoryComponent::Unequip(EPlayerEquipSlot Slot)
{
    return Slot==EPlayerEquipSlot::Weapon&&UnequipWeapon(GetSelectedWeaponFamily());
}

bool UICInventoryComponent::UnequipWeapon(EICWeaponFamily Family)
{
    if (const auto* Player=Cast<AICPlayerCharacter>(GetOwner());Player&&Player->IsSwitchingWeapon()) return false;
    const int32 FamilySlot=static_cast<int32>(Family)-1;
    auto Next=GetSnapshot();
    if (!Next.WeaponSlots.IsValidIndex(FamilySlot)) return false;
    auto& Selected=Next.Equipment[static_cast<int32>(EPlayerEquipSlot::Weapon)];
    const bool bSelected=GetItemWeaponFamily(Selected)==Family;
    auto& Item=bSelected ? Selected : Next.WeaponSlots[FamilySlot];
    if (Item.IsEmpty()) return false;
    const int32 Empty=Next.Bag.Slots.IndexOfByPredicate([](const auto& Value){return Value.IsEmpty();});
    if (Empty==INDEX_NONE) return Fail(*ICText::Get(TEXT("Native_89e1471e83a4")).ToString());
    Next.Bag.Slots[Empty]=Item;Item=FPlayerItemInstance();
    if (bSelected) Next.bWeaponSheathed=true;
    return Commit(Next);
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

bool UICInventoryComponent::ClearQuickSlot(int32 QuickIndex)
{
    auto Next=GetSnapshot();if(!Next.QuickSlots.IsValidIndex(QuickIndex))return false;
    Next.QuickSlots[QuickIndex]=NAME_None;return Commit(Next);
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
    FText Reason;return TransferAmount(ID,Index,GetTransferableCount(ID,Index,bToStorage),bToStorage,Reason);
}
int32 UICInventoryComponent::GetTransferableCount(FName ID,int32 Index,bool bToStorage) const
{
    const auto Snapshot=GetSnapshot();const auto* Storage=Snapshot.Storage.Find(ID);if(!Storage||Storage->Slots.Num()!=40)return 0;
    const auto& Source=bToStorage?Snapshot.Bag:*Storage;if(!Source.Slots.IsValidIndex(Index))return 0;
    const auto& Item=Source.Slots[Index];const auto* Definition=Find(Item.ItemID);
    if(Item.IsEmpty()||Item.bLocked||!Definition||Definition->Kind==EPlayerItemKind::Quest)return 0;
    int32 Reserved=0;
    if(bToStorage&&WorldState)for(const auto& Pair:WorldState->ExportState().ProductionJobs)
        if(Pair.Value.Status==EICProductionJobStatus::Running||Pair.Value.Status==EICProductionJobStatus::Ready)
            for(const auto& Material:Pair.Value.Materials)if(Material.ItemInstanceID==Item.InstanceID&&!(Material.Timing==EICProductionMaterialTiming::ConsumeAtStart&&Pair.Value.bCostsCommitted))Reserved+=Material.Count;
    return FMath::Max(0,Item.Count-Reserved);
}
bool UICInventoryComponent::TransferAmount(FName ID,int32 Index,int32 Count,bool bToStorage,FText& Reason)
{
    Reason=FText::GetEmpty();
    if(WorldState&&!WorldState->FlushParticipants())return false;
    const int32 Available=GetTransferableCount(ID,Index,bToStorage);
    if(Count<=0||Count>Available){Reason=NSLOCTEXT("ICStorage","Unavailable","이동할 수량이 부족하거나 잠금·제작 예약된 아이템입니다.");return false;}
    auto Next=GetSnapshot();auto* Storage=Next.Storage.Find(ID);if(!Storage||Storage->Slots.Num()!=40)return false;
    auto& Source=bToStorage?Next.Bag:*Storage;auto& Destination=bToStorage?*Storage:Next.Bag;
    auto& Item=Source.Slots[Index];auto Moving=Item;Moving.Count=Count;
    if(Count<Item.Count)Moving.InstanceID=FGuid::NewGuid();
    auto NewDestination=Destination;
    if(Insert(NewDestination,Moving.ItemID,Count,&Moving)!=Count){Reason=NSLOCTEXT("ICStorage","NoSpace","선택한 수량을 넣을 공간이 부족합니다.");return false;}
    Destination=MoveTemp(NewDestination);Item.Count-=Count;if(Item.Count==0)Item=FPlayerItemInstance();
    for(auto& Quick:Next.QuickSlots)if(!Quick.IsNone()&&!Next.Bag.Slots.ContainsByPredicate([&](const auto& Value){return Value.ItemID==Quick&&!Value.IsEmpty();}))Quick=NAME_None;
    if(!Commit(Next)){Reason=NSLOCTEXT("ICStorage","Changed","아이템 상태가 변경되었습니다. 다시 선택하세요.");return false;}return true;
}
bool UICInventoryComponent::SetHomeStorageStage(int32 Stage)
{return Stage==1&&GetSnapshot().Storage.Contains(TEXT("PlayerHome"))&&GetSnapshot().Storage.FindChecked(TEXT("PlayerHome")).Slots.Num()==40;}

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
    const auto SelectedFamily=GetItemWeaponFamily(Snapshot.Equipment[static_cast<int32>(EPlayerEquipSlot::Weapon)]);
    for (int32 Index=0;Index<Snapshot.WeaponSlots.Num();++Index)
    {
        const auto& Item=Snapshot.WeaponSlots[Index];
        if (!Item.IsEmpty() && (!Check(Item,false) || GetItemWeaponFamily(Item)!=static_cast<EICWeaponFamily>(Index+1)
            || SelectedFamily==static_cast<EICWeaponFamily>(Index+1))) return false;
    }
    for (const auto& C : Snapshot.Storage){if(C.Value.Slots.Num()!=40)return false;for (const auto& S : C.Value.Slots) if (!Check(S, false)) return false;}
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
