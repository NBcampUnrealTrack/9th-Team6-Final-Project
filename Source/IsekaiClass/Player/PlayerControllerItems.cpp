#include "PlayerPrototypeController.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"
#include "../Core/ICWorldStateSubsystem.h"

void APlayerPrototypeController::BeginItem(FName ID,int32 Quick)
{
    if(bEffectWaiting||bDead)return;
    auto* Inventory=GetInventory();const auto* Definition=Inventory?Inventory->Find(ID):nullptr;
    if(!Definition||Inventory->CountItem(ID)<=0){Show(TEXT("사용할 아이템이 없습니다."));return;}
    if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}
    if(Definition->Kind!=EPlayerItemKind::Consumable){Show(TEXT("사용 가능한 소비품이 아닙니다."));return;}
    PendingItem=ID;PendingQuick=Quick;Show(FString::Printf(TEXT("대상 선택: 같은 숫자 키 = 자신 / Z X C V = 동료 / %s = 취소"),*MenuCancelLabel()));
}
void APlayerPrototypeController::ConfirmTarget(FName Target)
{
    auto* Inventory=GetInventory();auto* State=GetWorldState();
    if(PendingItem.IsNone()||bEffectWaiting||!Inventory||!State||Inventory->CountItem(PendingItem)<=0)return;
    if(Target==TEXT("Player"))Target=UICWorldStateSubsystem::PlayerId();
    if(Target.ToString().StartsWith(TEXT("Companion")))
    {
        const int32 Slot=FCString::Atoi(*Target.ToString().RightChop(9))-1;
        if(Slot<0||Slot>3){Show(TEXT("올바르지 않은 동료 슬롯입니다."));return;}
        Target=NAME_None;
        for(const auto& Entry:State->ExportState().Characters)if(Entry.Value.PartySlot==Slot){Target=Entry.Key;break;}
    }
    const auto* Stored=State->GetStoredCharacter(Target);
    if(!Stored||!Stored->HasStats()||Stored->Mode==EICCharacterMode::Downed){Show(TEXT("사용 가능한 대상이 없습니다. 아이템은 소모하지 않았습니다."));return;}
    if(bUseTestEffects)
    {
        FICCharacterState TargetCharacter=*Stored;
        if(PendingItem==TEXT("PotionHP"))
        {if(TargetCharacter.HP>=TargetCharacter.MaxHP){Show(TEXT("이미 최대 HP입니다."));return;}TargetCharacter.HP=FMath::Min(TargetCharacter.MaxHP,TargetCharacter.HP+25);}
        else if(PendingItem==TEXT("Food"))
        {if(TargetCharacter.SP>=TargetCharacter.MaxSP){Show(TEXT("이미 최대 SP입니다."));return;}TargetCharacter.SP=FMath::Min(TargetCharacter.MaxSP,TargetCharacter.SP+25);}
        else if(PendingItem==TEXT("BookSTR"))
        {if(TargetCharacter.MainStats.STR>=10000){Show(TEXT("이미 최대 스탯입니다."));return;}++TargetCharacter.MainStats.STR;}
        else{Show(TEXT("시험 효과가 없는 아이템입니다. 전투 담당 연결이 필요합니다."));return;}
        FICInventoryState NextInventory;
        if(!Inventory->BuildConsumption(PendingItem,1,NextInventory)||!State->CommitCharacterAndInventory(TargetCharacter,NextInventory))
        {Show(TEXT("상태 검증 실패. 아이템과 효과를 변경하지 않았습니다."));return;}
        CancelTarget();Show(TEXT("시험 효과와 아이템 소모를 1회 적용했습니다."));return;
    }
    PendingRequest=FGuid::NewGuid();bEffectWaiting=true;SyncSaveSafety();
    Show(TEXT("전투 파트의 효과 응답 대기 중"));
    OnEffectRequest.Broadcast(PendingRequest,PendingItem,Target);
}
void APlayerPrototypeController::ResolveEffect(FGuid ID,bool Success)
{
    if(!bEffectWaiting||PendingRequest!=ID)return;
    const bool Consumed=Success&&GetInventory()&&GetInventory()->Consume(PendingItem,1);
    bEffectWaiting=false;CancelTarget();SyncSaveSafety();
    Show(Success?(Consumed?TEXT("아이템 사용 완료"):TEXT("효과 처리 후 아이템 소모 실패. 담당 연동을 확인하세요.")):TEXT("효과가 거절되었습니다. 아이템 미소모"));
}
void APlayerPrototypeController::PlayerDied()
{
    if(bEffectWaiting)ResolveEffect(PendingRequest,false);
    bDead=true;SetMenu(TEXT("Load"));Show(TEXT("사망했습니다. 수동 또는 오토 저장을 선택해 불러오세요."));
}
void APlayerPrototypeController::OpenContainer(FName ID,bool Home,bool Access)
{
    if(!Access){Show(TEXT("이 보관함의 접근 권한이 없습니다."));return;}
    auto* State=GetWorld()?GetWorld()->GetGameState<APlayerPrototypeGameState>():nullptr;
    OpenStorage=Home&&State?State->SharedHomeStorageID:ID;
    if(OpenStorage.IsNone())return;
    GetInventory()->EnsureStorage(OpenStorage);SetMenu(TEXT("Storage"));SelectedStorage=INDEX_NONE;
}
