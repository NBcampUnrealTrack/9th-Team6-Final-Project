#include "PlayerInventory.h"
#include "UObject/ConstructorHelpers.h"
#include "../Core/ICWorldStateSubsystem.h"

UPlayerInventory::UPlayerInventory()
{
    PrimaryComponentTick.bCanEverTick = false;
    auto Add = [this](const TCHAR* ID, const TCHAR* Label, EPlayerItemKind Kind, EPlayerEquipSlot Slot, FName Effect = NAME_None)
    {
        FPlayerItemDefinition D; D.ItemID=ID; D.DisplayName=FText::FromString(Label); D.Kind=Kind; D.EquipSlot=Slot;
        D.StackLimit=Kind==EPlayerItemKind::Equipment ? 1 : 99; D.EffectID=Effect;
        D.bCanDiscard=D.bCanSell=Kind!=EPlayerItemKind::Quest; Definitions.Add(D);
        auto& Added=Definitions.Last();const FString Code(ID);
        if(Code.StartsWith(TEXT("ITM_CONS_")))Added.Category=EPlayerItemCategory::Consumable;
        else if(Code.StartsWith(TEXT("ITM_FOOD_")))Added.Category=EPlayerItemCategory::Food;
        else if(Code.StartsWith(TEXT("ITM_BOOK_")))Added.Category=EPlayerItemCategory::Book;
        else if(Code.StartsWith(TEXT("ITM_MAT_")))Added.Category=EPlayerItemCategory::Material;
        else if(Code.StartsWith(TEXT("ITM_GATH_")))Added.Category=EPlayerItemCategory::Gathering;
        else if(Code.StartsWith(TEXT("ITM_FISH_")))Added.Category=EPlayerItemCategory::Fish;
        else if(Code.StartsWith(TEXT("ITM_CRAFT_")))Added.Category=EPlayerItemCategory::Crafted;
        else if(Code.StartsWith(TEXT("ITM_QST_")))Added.Category=EPlayerItemCategory::Quest;
        else if(Code.StartsWith(TEXT("ITM_TKT_")))Added.Category=EPlayerItemCategory::Ticket;
        else if(Code.StartsWith(TEXT("EQP_")))Added.Category=EPlayerItemCategory::Equipment;
        Added.Tab=Kind==EPlayerItemKind::Quest?EPlayerItemTab::Quest:EPlayerItemTab::Normal;
        Added.bCanGift=Added.Category==EPlayerItemCategory::Food||Added.Category==EPlayerItemCategory::Book||Added.Category==EPlayerItemCategory::Consumable;
        if(Kind==EPlayerItemKind::Equipment)
        {
            Added.MaxEnhanceLevel=10;Added.MaxRandomOptions=4;Added.EnhanceBaseCost=100;Added.EnhanceBaseSuccessRate=1.f;
            Added.OptionPoolID=Slot==EPlayerEquipSlot::Weapon?TEXT("OPT_WPN"):Slot==EPlayerEquipSlot::Clothes?TEXT("OPT_ARM"):TEXT("OPT_ACC");
        }
    };
    Add(TEXT("ITM_CONS_001"),TEXT("체력 포션"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("RestoreHP")); Definitions.Last().RestoreHP=25.f;
    Add(TEXT("ITM_FOOD_001"),TEXT("채소 수프"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("RestoreSP")); Definitions.Last().RestoreSP=25.f;
    Add(TEXT("ITM_FOOD_002"),TEXT("빵"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("RestoreSP")); Definitions.Last().RestoreSP=15.f;
    auto AddBook=[&](const TCHAR* ID,const TCHAR* Label,EPlayerMainStat Stat,int32 Amount)
    {
        Add(ID,Label,EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("GrowMainStat"));Definitions.Last().GrowthStat=Stat;Definitions.Last().GrowthAmount=Amount;
    };
    AddBook(TEXT("ITM_BOOK_001"),TEXT("초급 근력의 서"),EPlayerMainStat::Strength,1);
    AddBook(TEXT("ITM_BOOK_002"),TEXT("중급 근력의 서"),EPlayerMainStat::Strength,3);
    AddBook(TEXT("ITM_BOOK_003"),TEXT("고급 근력의 서"),EPlayerMainStat::Strength,5);
    AddBook(TEXT("ITM_BOOK_004"),TEXT("초급 민첩의 서"),EPlayerMainStat::Dexterity,1);
    AddBook(TEXT("ITM_BOOK_005"),TEXT("중급 민첩의 서"),EPlayerMainStat::Dexterity,3);
    AddBook(TEXT("ITM_BOOK_006"),TEXT("고급 민첩의 서"),EPlayerMainStat::Dexterity,5);
    AddBook(TEXT("ITM_BOOK_007"),TEXT("초급 지력의 서"),EPlayerMainStat::Intelligence,1);
    AddBook(TEXT("ITM_BOOK_008"),TEXT("중급 지력의 서"),EPlayerMainStat::Intelligence,3);
    AddBook(TEXT("ITM_BOOK_009"),TEXT("고급 지력의 서"),EPlayerMainStat::Intelligence,5);
    AddBook(TEXT("ITM_BOOK_010"),TEXT("초급 체력의 서"),EPlayerMainStat::Constitution,1);
    AddBook(TEXT("ITM_BOOK_011"),TEXT("중급 체력의 서"),EPlayerMainStat::Constitution,3);
    AddBook(TEXT("ITM_BOOK_012"),TEXT("고급 체력의 서"),EPlayerMainStat::Constitution,5);
    AddBook(TEXT("ITM_BOOK_013"),TEXT("초급 매력의 서"),EPlayerMainStat::Charisma,1);
    AddBook(TEXT("ITM_BOOK_014"),TEXT("중급 매력의 서"),EPlayerMainStat::Charisma,3);
    AddBook(TEXT("ITM_BOOK_015"),TEXT("고급 매력의 서"),EPlayerMainStat::Charisma,5);
    AddBook(TEXT("ITM_BOOK_016"),TEXT("초급 운의 서"),EPlayerMainStat::Luck,1);
    AddBook(TEXT("ITM_BOOK_017"),TEXT("중급 운의 서"),EPlayerMainStat::Luck,3);
    AddBook(TEXT("ITM_BOOK_018"),TEXT("고급 운의 서"),EPlayerMainStat::Luck,5);
    Add(TEXT("ITM_MAT_001"),TEXT("목재"),EPlayerItemKind::Material,EPlayerEquipSlot::None);
    Add(TEXT("ITM_MAT_002"),TEXT("밀가루"),EPlayerItemKind::Material,EPlayerEquipSlot::None);
    Add(TEXT("ITM_MAT_003"),TEXT("철 주괴"),EPlayerItemKind::Material,EPlayerEquipSlot::None);
    Add(TEXT("ITM_TKT_001"),TEXT("가방 확장권"),EPlayerItemKind::Expansion,EPlayerEquipSlot::None);
    Add(TEXT("ITM_QST_001"),TEXT("퀘스트 반지"),EPlayerItemKind::Quest,EPlayerEquipSlot::None);
    Add(TEXT("EQP_WPN_SWORD_001"),TEXT("초보자 검"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon); Definitions.Last().RequiredClass=0;Definitions.Last().AttachSocket=TEXT("weapon_r");Definitions.Last().BaseAttack=10.f;Definitions.Last().EnhanceAttackPerLevel=2.f;
    Add(TEXT("EQP_WPN_BOW_001"),TEXT("초보자 활"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon); Definitions.Last().RequiredClass=1;Definitions.Last().AttachSocket=TEXT("weapon_r");Definitions.Last().BaseAttack=10.f;Definitions.Last().EnhanceAttackPerLevel=2.f;
    Add(TEXT("EQP_WPN_STAFF_001"),TEXT("초보자 마법스태프"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon); Definitions.Last().RequiredClass=2;Definitions.Last().AttachSocket=TEXT("weapon_r");Definitions.Last().BaseAttack=10.f;Definitions.Last().EnhanceAttackPerLevel=2.f;
    Add(TEXT("EQP_WPN_WAND_001"),TEXT("초보자 치유완드"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon); Definitions.Last().RequiredClass=3;Definitions.Last().AttachSocket=TEXT("weapon_r");Definitions.Last().BaseAttack=10.f;Definitions.Last().EnhanceAttackPerLevel=2.f;
    Add(TEXT("EQP_ARM_001"),TEXT("여행자의 옷"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Clothes);Definitions.Last().BaseDefense=6.f;Definitions.Last().EnhanceDefensePerLevel=1.5f;
    Add(TEXT("EQP_RNG_001"),TEXT("구리 반지"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Ring);Definitions.Last().BaseAttack=3.f;Definitions.Last().EnhanceAttackPerLevel=1.f;
    Add(TEXT("EQP_NCK_001"),TEXT("구리 목걸이"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Necklace);Definitions.Last().BaseDefense=2.f;Definitions.Last().EnhanceDefensePerLevel=1.f;
    static ConstructorHelpers::FObjectFinder<UDataTable> ItemTableAsset(TEXT("/Game/IsekaiClass/Data/Items/DT_IC_Item.DT_IC_Item"));
    if(ItemTableAsset.Succeeded())ItemTable=ItemTableAsset.Object;
    ResetInventory();
}
const FPlayerItemDefinition* UPlayerInventory::Find(FName ID) const
{
    if (ItemTable) if (const auto* Row=ItemTable->FindRow<FPlayerItemDefinition>(ID,TEXT("PlayerInventory"),false)) return Row;
    return Definitions.FindByPredicate([ID](const auto& D){return D.ItemID==ID;});
}

void UPlayerInventory::BindWorldState(UICWorldStateSubsystem* InWorldState)
{
    if (WorldState) WorldState->OnInventoryChanged.RemoveDynamic(this, &ThisClass::WorldInventoryChanged);
    WorldState = InWorldState;
    if (WorldState) WorldState->OnInventoryChanged.AddDynamic(this, &ThisClass::WorldInventoryChanged);
    OnChanged.Broadcast();
}

FICInventoryState UPlayerInventory::GetSnapshot() const { return WorldState ? WorldState->GetInventorySnapshot() : StandaloneState; }
void UPlayerInventory::WorldInventoryChanged() { OnChanged.Broadcast(); }

bool UPlayerInventory::Commit(const FICInventoryState& Snapshot)
{
    if (!ValidateSnapshot(Snapshot)) return Fail(TEXT("인벤토리 상태 검증 실패. 변경하지 않았습니다."));
    if (WorldState) return WorldState->CommitInventoryState(Snapshot);
    StandaloneState = Snapshot;
    OnChanged.Broadcast();
    return true;
}

void UPlayerInventory::ResetInventory() { Commit(FICInventoryState()); }
bool UPlayerInventory::Fail(const TCHAR* Text) { OnMessage.Broadcast(FText::FromString(Text)); return false; }

int32 UPlayerInventory::Insert(FPlayerContainer& C, FName ID, int32 Count, const FPlayerItemInstance* Original) const
{
    const auto* D = Find(ID); if (!D || Count <= 0) return 0;
    const int32 Limit = D->Kind == EPlayerItemKind::Equipment ? 1 : FMath::Clamp(D->StackLimit, 1, 99);
    int32 Left = Count;
    if (Limit > 1) for (auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked && S.Count < Limit)
    { const int32 N = FMath::Min(Left, Limit - S.Count); S.Count += N; Left -= N; if (!Left) return Count; }
    for (auto& S : C.Slots) if (S.IsEmpty())
    {
        S=Original ? *Original : FPlayerItemInstance(); S.ItemID=ID;
        if(!Original){S.Grade=D->DefaultGrade;if(D->Kind==EPlayerItemKind::Equipment)RollRandomOptions(S,*D);}
        if (!Original || !S.InstanceID.IsValid()) S.InstanceID=FGuid::NewGuid();
        S.Count=FMath::Min(Left,Limit); Left-=S.Count; if (!Left) break;
        S = Original ? *Original : FPlayerItemInstance(); S.ItemID = ID;
        if (!Original || !S.InstanceID.IsValid()) S.InstanceID = FGuid::NewGuid();
        S.Count = FMath::Min(Left, Limit); Left -= S.Count; if (!Left) break;
    }
    return Count - Left;
}

int32 UPlayerInventory::TryAddItem(FName ID, int32 Count)
{
    const auto* D = Find(ID);
    if (!D || Count <= 0) { Fail(TEXT("유효하지 않은 아이템 또는 수량입니다.")); return FMath::Max(0, Count); }
    auto Next = GetSnapshot();
    int32 Added = 0;
    if (D->Kind == EPlayerItemKind::Quest)
    {
        FPlayerContainer C; C.Slots = Next.QuestItems;
        // Avoid oversized allocation from malformed requests.
        if (Count > 100000) { Fail(TEXT("한 번에 추가할 수 있는 수량을 초과했습니다.")); return Count; }
        C.Slots.SetNum(C.Slots.Num() + FMath::DivideAndRoundUp(Count, FMath::Clamp(D->StackLimit, 1, 99)));
        Added = Insert(C, ID, Count);
        C.Slots.RemoveAll([](const auto& S) { return S.IsEmpty(); }); Next.QuestItems = MoveTemp(C.Slots);
    }
    FPlayerContainer Next=Bag; const int32 Added=Insert(Next,ID,Count);
    if (!bDiscardOverflow && Added!=Count) return Fail(TEXT("가방 공간이 부족합니다. 변경하지 않았습니다."));
    Bag=MoveTemp(Next); OnChanged.Broadcast(); OnLootResult.Broadcast(ID,Added,Count-Added);
    if (Added<Count) OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("가방 초과분 %d개가 바닥에 떨어졌습니다."),Count-Added)));
    return Added==Count;
}
int32 UPlayerInventory::TryAddItem(FName ID,int32 Count)
{
    const auto* D=Find(ID);if(!D||Count<=0)return Count;
    if(D->Kind==EPlayerItemKind::Quest)return AddItem(ID,Count,false)?0:Count;
    const int32 Added=Insert(Bag,ID,Count);if(Added>0){OnChanged.Broadcast();OnLootResult.Broadcast(ID,Added,0);}return Count-Added;
}
int32 UPlayerInventory::TryAddItemInstance(const FPlayerItemInstance& Item)
{
    const auto* Definition=Find(Item.ItemID);if(!Definition||Item.IsEmpty())return Item.Count;
    if(Definition->Kind!=EPlayerItemKind::Equipment)return TryAddItem(Item.ItemID,Item.Count);
    const int32 Added=Insert(Bag,Item.ItemID,Item.Count,&Item);if(Added>0){OnChanged.Broadcast();OnLootResult.Broadcast(Item.ItemID,Added,0);}return Item.Count-Added;
}
    else Added = Insert(Next.Bag, ID, Count);
    if (Added > 0 && !Commit(Next)) return Count;
    OnLootResult.Broadcast(ID, Added, Count - Added);
    return Count - Added;
}

bool UPlayerInventory::AddItem(FName ID, int32 Count, bool bAllowPartial)
{
    const auto* D = Find(ID); if (!D || Count <= 0) return Fail(TEXT("유효하지 않은 아이템 또는 수량입니다."));
    if (bAllowPartial || D->Kind == EPlayerItemKind::Quest)
    {
        const int32 Left = TryAddItem(ID, Count);
        if (Left) OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("가방 초과분 %d개: 드롭 담당 연결이 필요합니다."), Left)));
        return Left == 0;
    }
    auto Next = GetSnapshot();
    const int32 Added = Insert(Next.Bag, ID, Count);
    if (Added != Count) return Fail(TEXT("가방 공간이 부족합니다. 변경하지 않았습니다."));
    if (!Commit(Next)) return false;
    OnLootResult.Broadcast(ID, Count, 0); return true;
}

int32 UPlayerInventory::CountItem(FName ID) const
{
    int64 Count = 0;
    for (const auto& S : GetSnapshot().Bag.Slots) if (S.ItemID == ID && !S.bLocked) Count += S.Count;
    return static_cast<int32>(FMath::Min<int64>(Count, MAX_int32));
}

bool UPlayerInventory::Remove(FPlayerContainer& C, FName ID, int32 Count) const
{
    if (Count <= 0) return false;
    int64 Available = 0; for (const auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked) Available += S.Count;
    if (Available < Count) return false;
    for (auto& S : C.Slots) if (S.ItemID == ID && !S.bLocked)
    { const int32 N = FMath::Min(Count, S.Count); S.Count -= N; Count -= N; if (!S.Count) S = FPlayerItemInstance(); if (!Count) break; }
    return true;
}

bool UPlayerInventory::Consume(FName ID, int32 Count)
{
    auto Next = GetSnapshot();
    if (!Remove(Next.Bag, ID, Count)) return Fail(TEXT("아이템이 없거나 잠겨 있습니다."));
    return Commit(Next);
}

bool UPlayerInventory::BuildConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const
{
    OutSnapshot = GetSnapshot();
    return Remove(OutSnapshot.Bag, ID, Count) && ValidateSnapshot(OutSnapshot);
}

bool UPlayerInventory::MoveSlot(int32 From, int32 To)
{
    if(!Bag.Slots.IsValidIndex(I)) return false; auto& S=Bag.Slots[I]; const auto* D=Find(S.ItemID);
    if(!D||!D->bCanDiscard||S.bLocked) return Fail(TEXT("버릴 수 없는 아이템입니다."));const FPlayerItemInstance Dropped=S;S=FPlayerItemInstance();OnChanged.Broadcast();OnDropRequested.Broadcast(Dropped);return true;
    auto Next = GetSnapshot();
    if (!Next.Bag.Slots.IsValidIndex(From) || !Next.Bag.Slots.IsValidIndex(To) || From == To) return false;
    auto& A = Next.Bag.Slots[From]; auto& B = Next.Bag.Slots[To];
    if (A.IsEmpty() || A.bLocked || B.bLocked) return Fail(TEXT("빈 칸 또는 잠긴 아이템입니다."));
    const auto* D = Find(A.ItemID);
    if (A.ItemID == B.ItemID && D && D->Kind != EPlayerItemKind::Equipment)
    { const int32 N = FMath::Min(A.Count, FMath::Clamp(D->StackLimit, 1, 99) - B.Count); A.Count -= N; B.Count += N; if (!A.Count) A = FPlayerItemInstance(); }
    else Swap(A, B);
    return Commit(Next);
}

bool UPlayerInventory::SplitStack(int32 From, int32 Amount)
{
    if(!Bag.Slots.IsValidIndex(I)) return false; auto& S=Bag.Slots[I]; const auto* D=Find(S.ItemID);
    if(!D||D->Kind!=EPlayerItemKind::Equipment||D->EquipSlot==EPlayerEquipSlot::None||S.bLocked) return Fail(TEXT("장착할 수 없습니다."));
    if(D->EquipSlot==EPlayerEquipSlot::Weapon&&D->RequiredClass!=INDEX_NONE&&D->RequiredClass!=CurrentClass)return Fail(TEXT("현재 직업이 사용할 수 없는 무기입니다."));
    Swap(S,Equipment[static_cast<int32>(D->EquipSlot)]); OnChanged.Broadcast(); return true;
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(From)) return false;
    auto& S = Next.Bag.Slots[From]; const auto* D = Find(S.ItemID);
    if (!D || S.bLocked || D->Kind == EPlayerItemKind::Equipment || Amount <= 0 || Amount >= S.Count) return Fail(TEXT("이 수량은 분할할 수 없습니다."));
    const int32 Empty = Next.Bag.Slots.IndexOfByPredicate([](const auto& X) { return X.IsEmpty(); });
    if (Empty == INDEX_NONE) return Fail(TEXT("분할할 빈 칸이 없습니다."));
    FPlayerItemInstance Copy = S; Copy.InstanceID = FGuid::NewGuid(); Copy.Count = Amount;
    S.Count -= Amount; Next.Bag.Slots[Empty] = Copy;
    return Commit(Next);
}

bool UPlayerInventory::ToggleLock(int32 Index)
{
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index) || Next.Bag.Slots[Index].IsEmpty()) return false;
    Next.Bag.Slots[Index].bLocked = !Next.Bag.Slots[Index].bLocked; return Commit(Next);
}

bool UPlayerInventory::Discard(int32 Index)
{
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index)) return false;
    auto& S = Next.Bag.Slots[Index]; const auto* D = Find(S.ItemID);
    if (!D || !D->bCanDiscard || S.bLocked) return Fail(TEXT("버릴 수 없는 아이템입니다."));
    S = FPlayerItemInstance(); return Commit(Next);
}

bool UPlayerInventory::Equip(int32 Index)
{
    auto Next = GetSnapshot(); if (!Next.Bag.Slots.IsValidIndex(Index)) return false;
    auto& S = Next.Bag.Slots[Index]; const auto* D = Find(S.ItemID);
    if (!D || D->Kind != EPlayerItemKind::Equipment || D->EquipSlot == EPlayerEquipSlot::None || S.bLocked) return Fail(TEXT("장착할 수 없습니다."));
    Swap(S, Next.Equipment[static_cast<int32>(D->EquipSlot)]); return Commit(Next);
}

bool UPlayerInventory::Unequip(EPlayerEquipSlot Slot)
{
    auto Next = GetSnapshot(); const int32 Index = static_cast<int32>(Slot);
    if (!Next.Equipment.IsValidIndex(Index) || Next.Equipment[Index].IsEmpty()) return false;
    const int32 Empty = Next.Bag.Slots.IndexOfByPredicate([](const auto& S) { return S.IsEmpty(); });
    if (Empty == INDEX_NONE) return Fail(TEXT("해제할 가방 공간이 없습니다."));
    Next.Bag.Slots[Empty] = Next.Equipment[Index]; Next.Equipment[Index] = FPlayerItemInstance(); return Commit(Next);
}

void UPlayerInventory::SortBag()
{
    auto Next = GetSnapshot(); TArray<FPlayerItemInstance> Items;
    for (const auto& S : Next.Bag.Slots) if (!S.bLocked && !S.IsEmpty()) Items.Add(S);
    Items.StableSort([](const auto& A, const auto& B) { return A.ItemID.LexicalLess(B.ItemID); });
    int32 N = 0; for (auto& S : Next.Bag.Slots) if (!S.bLocked) S = Items.IsValidIndex(N) ? Items[N++] : FPlayerItemInstance();
    Commit(Next);
}

bool UPlayerInventory::BindQuickSlot(int32 Quick, int32 Index)
{
    auto Next = GetSnapshot(); if (!Next.QuickSlots.IsValidIndex(Quick) || !Next.Bag.Slots.IsValidIndex(Index)) return false;
    const auto* D = Find(Next.Bag.Slots[Index].ItemID);
    if (!D || (D->Kind != EPlayerItemKind::Consumable && D->Kind != EPlayerItemKind::Expansion)) return Fail(TEXT("소비품 또는 확장권만 등록할 수 있습니다."));
    Next.QuickSlots[Quick] = D->ItemID; return Commit(Next);
}

bool UPlayerInventory::ExpandUsingTicket()
{
    if(Bag.Slots.Num()>=100) return Fail(TEXT("최대 100칸입니다. 확장권은 소모하지 않았습니다."));
    if(!Remove(Bag,TEXT("ITM_TKT_001"),1)) return Fail(TEXT("확장권이 없습니다."));
    Bag.Slots.SetNum(FMath::Min(100,Bag.Slots.Num()+10)); OnChanged.Broadcast(); return true;
    auto Next = GetSnapshot(); if (Next.Bag.Slots.Num() >= 100) return Fail(TEXT("최대 100칸입니다. 확장권은 소모하지 않았습니다."));
    if (!Remove(Next.Bag, TEXT("Ticket"), 1)) return Fail(TEXT("확장권이 없습니다."));
    Next.Bag.Slots.SetNum(Next.Bag.Slots.Num() + 10); return Commit(Next);
}

void UPlayerInventory::EnsureStorage(FName ID)
{
    auto Next = GetSnapshot(); if (!ID.IsNone() && !Next.Storage.Contains(ID)) { Next.Storage.Add(ID).Slots.SetNum(40); Commit(Next); }
}

bool UPlayerInventory::Transfer(FName ID, int32 Index, bool bToStorage)
{
    auto Next = GetSnapshot(); auto* Container = Next.Storage.Find(ID); if (!Container) return false;
    auto& Source = bToStorage ? Next.Bag : *Container; auto& Destination = bToStorage ? *Container : Next.Bag;
    if (!Source.Slots.IsValidIndex(Index) || Source.Slots[Index].IsEmpty() || Source.Slots[Index].bLocked) return false;
    const auto Item = Source.Slots[Index]; FPlayerContainer NewDestination = Destination;
    if (Insert(NewDestination, Item.ItemID, Item.Count, &Item) != Item.Count) return Fail(TEXT("이동할 공간이 부족합니다. 변경하지 않았습니다."));
    Destination = MoveTemp(NewDestination); Source.Slots[Index] = FPlayerItemInstance(); return Commit(Next);
}

bool UPlayerInventory::ExchangeItems(const TMap<FName, int32>& Costs, FName ID, int32 Count)
{
    const auto* D = Find(ID); if (!D || Count <= 0 || D->Kind == EPlayerItemKind::Quest) return false;
    auto Next = GetSnapshot(); for (const auto& Cost : Costs) if (!Remove(Next.Bag, Cost.Key, Cost.Value)) return Fail(TEXT("재료가 부족합니다."));
    if (Insert(Next.Bag, ID, Count) != Count) return Fail(TEXT("결과물을 보관할 공간이 없습니다."));
    return Commit(Next);
}

bool UPlayerInventory::ValidateSnapshot(const FICInventoryState& Snapshot) const
{
    if (!UICWorldStateSubsystem::ValidateInventoryShape(Snapshot)) return false;
    auto Check = [this](const FPlayerItemInstance& Item, bool bQuest)
    {
        if (Item.IsEmpty()) return true;
        const auto* D = Find(Item.ItemID);
        return D && (D->Kind == EPlayerItemKind::Quest) == bQuest && Item.Count <= (D->Kind == EPlayerItemKind::Equipment ? 1 : FMath::Clamp(D->StackLimit, 1, 99));
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

bool UPlayerInventory::ValidateState() const { return ValidateSnapshot(GetSnapshot()); }
