#include "PlayerInventory.h"

UPlayerInventory::UPlayerInventory()
{
    PrimaryComponentTick.bCanEverTick = false;
    auto Add = [this](const TCHAR* ID, const TCHAR* Label, EPlayerItemKind Kind, EPlayerEquipSlot Slot, FName Effect = NAME_None)
    {
        FPlayerItemDefinition D; D.ItemID=ID; D.Name=FText::FromString(Label); D.Kind=Kind; D.EquipSlot=Slot;
        D.StackLimit=Kind==EPlayerItemKind::Equipment ? 1 : 99; D.EffectID=Effect;
        D.bCanDiscard=D.bCanSell=Kind!=EPlayerItemKind::Quest; Definitions.Add(D);
    };
    Add(TEXT("PotionHP"),TEXT("체력 포션"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("RestoreHP"));
    Add(TEXT("Food"),TEXT("채소 수프"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("RestoreSP"));
    Add(TEXT("BookSTR"),TEXT("초급 근력의 서"),EPlayerItemKind::Consumable,EPlayerEquipSlot::None,TEXT("GrowSTR"));
    Add(TEXT("Wood"),TEXT("목재"),EPlayerItemKind::Material,EPlayerEquipSlot::None);
    Add(TEXT("Ticket"),TEXT("가방 확장권"),EPlayerItemKind::Expansion,EPlayerEquipSlot::None);
    Add(TEXT("QuestRing"),TEXT("퀘스트 반지"),EPlayerItemKind::Quest,EPlayerEquipSlot::None);
    Add(TEXT("Sword"),TEXT("초보자 검"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon);
    Add(TEXT("Bow"),TEXT("초보자 활"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon);
    Add(TEXT("Staff"),TEXT("초보자 공격용 지팡이"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon);
    Add(TEXT("HealBook"),TEXT("초보자 치유서"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Weapon);
    Add(TEXT("Clothes"),TEXT("여행자의 옷"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Clothes);
    Add(TEXT("Ring"),TEXT("구리 반지"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Ring);
    Add(TEXT("Necklace"),TEXT("구리 목걸이"),EPlayerItemKind::Equipment,EPlayerEquipSlot::Necklace);
    ResetInventory();
}
const FPlayerItemDefinition* UPlayerInventory::Find(FName ID) const
{
    if (ItemTable) if (const auto* Row=ItemTable->FindRow<FPlayerItemDefinition>(ID,TEXT("PlayerInventory"),false)) return Row;
    return Definitions.FindByPredicate([ID](const auto& D){return D.ItemID==ID;});
}
void UPlayerInventory::ResetInventory()
{
    Bag.Slots.Empty(); Bag.Slots.SetNum(20); QuestItems.Empty(); Equipment.Empty(); Equipment.SetNum(4);
    QuickSlots.Empty(); QuickSlots.SetNum(4); Storage.Empty(); EnsureStorage(TEXT("PlayerHome")); OnChanged.Broadcast();
}
bool UPlayerInventory::Fail(const TCHAR* Text) { OnMessage.Broadcast(FText::FromString(Text)); return false; }
int32 UPlayerInventory::Insert(FPlayerContainer& C, FName ID, int32 Count, const FPlayerItemInstance* Original) const
{
    const auto* D=Find(ID); if (!D || Count<=0) return 0;
    const int32 Limit=D->Kind==EPlayerItemKind::Equipment ? 1 : FMath::Clamp(D->StackLimit,1,99);
    int32 Left=Count;
    if (Limit>1) for (auto& S:C.Slots) if (S.ItemID==ID && !S.bLocked && S.Count<Limit)
    { const int32 N=FMath::Min(Left,Limit-S.Count); S.Count+=N; Left-=N; if (!Left) return Count; }
    for (auto& S:C.Slots) if (S.IsEmpty())
    {
        S=Original ? *Original : FPlayerItemInstance(); S.ItemID=ID;
        if (!Original || !S.InstanceID.IsValid()) S.InstanceID=FGuid::NewGuid();
        S.Count=FMath::Min(Left,Limit); Left-=S.Count; if (!Left) break;
    }
    return Count-Left;
}
bool UPlayerInventory::AddItem(FName ID, int32 Count, bool bDiscardOverflow)
{
    const auto* D=Find(ID); if (!D || Count<=0) return Fail(TEXT("유효하지 않은 아이템 또는 수량입니다."));
    if (D->Kind==EPlayerItemKind::Quest)
    {
        FPlayerContainer C; C.Slots=QuestItems;
        C.Slots.SetNum(C.Slots.Num()+FMath::DivideAndRoundUp(Count,FMath::Clamp(D->StackLimit,1,99)));
        Insert(C,ID,Count); C.Slots.RemoveAll([](const auto& S){return S.IsEmpty();}); QuestItems=MoveTemp(C.Slots);
        OnChanged.Broadcast(); OnLootResult.Broadcast(ID,Count,0); return true;
    }
    FPlayerContainer Next=Bag; const int32 Added=Insert(Next,ID,Count);
    if (!bDiscardOverflow && Added!=Count) return Fail(TEXT("가방 공간이 부족합니다. 변경하지 않았습니다."));
    Bag=MoveTemp(Next); OnChanged.Broadcast(); OnLootResult.Broadcast(ID,Added,Count-Added);
    if (Added<Count) OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("가방 초과분 %d개를 자동으로 버렸습니다."),Count-Added)));
    return Added==Count;
}
int32 UPlayerInventory::CountItem(FName ID) const
{ int32 N=0; for(const auto& S:Bag.Slots) if(S.ItemID==ID && !S.bLocked) N+=S.Count; return N; }
bool UPlayerInventory::Remove(FPlayerContainer& C,FName ID,int32 Count) const
{
    if (Count<=0) return false; int64 Available=0;
    for(const auto& S:C.Slots) if(S.ItemID==ID && !S.bLocked) Available+=S.Count;
    if(Available<Count) return false;
    for(auto& S:C.Slots) if(S.ItemID==ID && !S.bLocked)
    { int32 N=FMath::Min(Count,S.Count); S.Count-=N; Count-=N; if(S.Count==0) S=FPlayerItemInstance(); if(!Count) break; }
    return true;
}
bool UPlayerInventory::Consume(FName ID,int32 Count)
{ if(!Remove(Bag,ID,Count)) return Fail(TEXT("아이템이 없거나 잠겨 있습니다.")); OnChanged.Broadcast(); return true; }
bool UPlayerInventory::MoveSlot(int32 From,int32 To)
{
    if(!Bag.Slots.IsValidIndex(From)||!Bag.Slots.IsValidIndex(To)||From==To) return false;
    auto& A=Bag.Slots[From]; auto& B=Bag.Slots[To];
    if(A.IsEmpty()||A.bLocked||B.bLocked) return Fail(TEXT("빈 칸 또는 잠긴 아이템입니다."));
    const auto* D=Find(A.ItemID);
    if(A.ItemID==B.ItemID && D && D->Kind!=EPlayerItemKind::Equipment)
    { int32 N=FMath::Min(A.Count,FMath::Clamp(D->StackLimit,1,99)-B.Count); A.Count-=N; B.Count+=N; if(!A.Count) A=FPlayerItemInstance(); }
    else Swap(A,B);
    OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::SplitStack(int32 From,int32 Amount)
{
    if(!Bag.Slots.IsValidIndex(From)) return false;
    auto& S=Bag.Slots[From]; const auto* D=Find(S.ItemID);
    if(!D||S.bLocked||D->Kind==EPlayerItemKind::Equipment||Amount<=0||Amount>=S.Count) return Fail(TEXT("이 수량은 분할할 수 없습니다."));
    int32 Empty=Bag.Slots.IndexOfByPredicate([](const auto& X){return X.IsEmpty();});
    if(Empty==INDEX_NONE) return Fail(TEXT("분할할 빈 칸이 없습니다."));
    FPlayerItemInstance Copy=S; Copy.InstanceID=FGuid::NewGuid(); Copy.Count=Amount; S.Count-=Amount; Bag.Slots[Empty]=Copy;
    OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::ToggleLock(int32 I)
{ if(!Bag.Slots.IsValidIndex(I)||Bag.Slots[I].IsEmpty()) return false; Bag.Slots[I].bLocked=!Bag.Slots[I].bLocked; OnChanged.Broadcast(); return true; }
bool UPlayerInventory::Discard(int32 I)
{
    if(!Bag.Slots.IsValidIndex(I)) return false; auto& S=Bag.Slots[I]; const auto* D=Find(S.ItemID);
    if(!D||!D->bCanDiscard||S.bLocked) return Fail(TEXT("버릴 수 없는 아이템입니다.")); S=FPlayerItemInstance(); OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::Equip(int32 I)
{
    if(!Bag.Slots.IsValidIndex(I)) return false; auto& S=Bag.Slots[I]; const auto* D=Find(S.ItemID);
    if(!D||D->Kind!=EPlayerItemKind::Equipment||D->EquipSlot==EPlayerEquipSlot::None||S.bLocked) return Fail(TEXT("장착할 수 없습니다."));
    Swap(S,Equipment[static_cast<int32>(D->EquipSlot)]); OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::Unequip(EPlayerEquipSlot Slot)
{
    int32 I=static_cast<int32>(Slot); if(!Equipment.IsValidIndex(I)||Equipment[I].IsEmpty()) return false;
    int32 Empty=Bag.Slots.IndexOfByPredicate([](const auto& S){return S.IsEmpty();});
    if(Empty==INDEX_NONE) return Fail(TEXT("해제할 가방 공간이 없습니다."));
    Bag.Slots[Empty]=Equipment[I]; Equipment[I]=FPlayerItemInstance(); OnChanged.Broadcast(); return true;
}
void UPlayerInventory::SortBag()
{
    TArray<FPlayerItemInstance> Items; for(const auto& S:Bag.Slots) if(!S.bLocked && !S.IsEmpty()) Items.Add(S);
    Items.StableSort([](const auto& A,const auto& B){return A.ItemID.LexicalLess(B.ItemID);}); int32 N=0;
    for(auto& S:Bag.Slots) if(!S.bLocked) S=Items.IsValidIndex(N)?Items[N++]:FPlayerItemInstance(); OnChanged.Broadcast();
}
bool UPlayerInventory::BindQuickSlot(int32 Q,int32 I)
{
    if(!QuickSlots.IsValidIndex(Q)||!Bag.Slots.IsValidIndex(I)) return false; const auto* D=Find(Bag.Slots[I].ItemID);
    if(!D||(D->Kind!=EPlayerItemKind::Consumable&&D->Kind!=EPlayerItemKind::Expansion)) return Fail(TEXT("소비품 또는 확장권만 등록할 수 있습니다."));
    QuickSlots[Q]=D->ItemID; OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::ExpandUsingTicket()
{
    if(Bag.Slots.Num()>=100) return Fail(TEXT("최대 100칸입니다. 확장권은 소모하지 않았습니다."));
    if(!Remove(Bag,TEXT("Ticket"),1)) return Fail(TEXT("확장권이 없습니다."));
    Bag.Slots.SetNum(FMath::Min(100,Bag.Slots.Num()+10)); OnChanged.Broadcast(); return true;
}
void UPlayerInventory::EnsureStorage(FName ID) { if(!ID.IsNone()&&!Storage.Contains(ID)) Storage.Add(ID).Slots.SetNum(40); }
bool UPlayerInventory::Transfer(FName ID,int32 I,bool ToStorage)
{
    FPlayerContainer* C=Storage.Find(ID); if(!C) return false;
    FPlayerContainer& Source=ToStorage?Bag:*C; FPlayerContainer& Dest=ToStorage?*C:Bag;
    if(!Source.Slots.IsValidIndex(I)||Source.Slots[I].IsEmpty()||Source.Slots[I].bLocked) return false;
    auto S=Source.Slots[I]; FPlayerContainer Next=Dest;
    if(Insert(Next,S.ItemID,S.Count,&S)!=S.Count) return Fail(TEXT("이동할 공간이 부족합니다. 변경하지 않았습니다."));
    Dest=MoveTemp(Next); Source.Slots[I]=FPlayerItemInstance(); OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::ExchangeItems(const TMap<FName,int32>& Costs,FName ID,int32 Count)
{
    const auto* D=Find(ID); if(!D||Count<=0||D->Kind==EPlayerItemKind::Quest) return false;
    FPlayerContainer Next=Bag; for(const auto& C:Costs) if(!Remove(Next,C.Key,C.Value)) return Fail(TEXT("재료가 부족합니다."));
    if(Insert(Next,ID,Count)!=Count) return Fail(TEXT("결과물을 보관할 공간이 없습니다."));
    Bag=MoveTemp(Next); OnChanged.Broadcast(); return true;
}
bool UPlayerInventory::ValidateState() const
{
    if(Bag.Slots.Num()<20||Bag.Slots.Num()>100||Equipment.Num()!=4||QuickSlots.Num()!=4) return false;
    TSet<FGuid> IDs;
    auto Check=[this,&IDs](const FPlayerItemInstance& S,bool Quest)->bool
    {
        if(S.IsEmpty()) return S.ItemID.IsNone()&&S.Count==0;
        const auto* D=Find(S.ItemID); if(!D||!S.InstanceID.IsValid()||IDs.Contains(S.InstanceID)||S.Count<=0) return false;
        if((D->Kind==EPlayerItemKind::Quest)!=Quest||S.Count>(D->Kind==EPlayerItemKind::Equipment?1:FMath::Clamp(D->StackLimit,1,99))) return false;
        IDs.Add(S.InstanceID); return true;
    };
    for(const auto& S:Bag.Slots) if(!Check(S,false)) return false;
    for(const auto& S:QuestItems) if(!Check(S,true)) return false;
    for(int32 I=0;I<Equipment.Num();++I) {const auto& S=Equipment[I]; if(!Check(S,false)) return false; if(!S.IsEmpty()&&static_cast<int32>(Find(S.ItemID)->EquipSlot)!=I) return false;}
    for(const auto& C:Storage) {if(C.Value.Slots.Num()!=40) return false; for(const auto& S:C.Value.Slots) if(!Check(S,false)) return false;}
    for(FName Q:QuickSlots) if(!Q.IsNone()) {const auto* D=Find(Q); if(!D||(D->Kind!=EPlayerItemKind::Consumable&&D->Kind!=EPlayerItemKind::Expansion)) return false;}
    return true;
}
