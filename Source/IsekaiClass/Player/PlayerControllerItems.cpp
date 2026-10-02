#include "PlayerPrototypeController.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"

void APlayerPrototypeController::BeginItem(FName ID,int32 Quick)
{
    if(bEffectWaiting||bDead)return;auto* Inventory=GetInventory();const auto* Definition=Inventory->Find(ID);
    if(!Definition||Inventory->CountItem(ID)<=0){Show(TEXT("사용할 아이템이 없습니다."));return;}if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}if(Definition->Kind!=EPlayerItemKind::Consumable){Show(TEXT("사용 가능한 소비품이 아닙니다."));return;}
    PendingItem=ID;PendingQuick=Quick;Show(TEXT("대상 선택: 같은 숫자 키 = 자신 / Z X C V = 동료 / 우클릭·ESC = 취소"));
}

void APlayerPrototypeController::ConfirmTarget(FName Target)
{
    if(PendingItem.IsNone()||bEffectWaiting||GetInventory()->CountItem(PendingItem)<=0)return;
    if(bUseTestEffects)
    {
        if(Target!=TEXT("Player")){Show(TEXT("이 데모에는 동료가 없습니다. 아이템은 소모하지 않았습니다."));return;}
        if((PendingItem==TEXT("PotionHP")&&TestHP>=100)||(PendingItem==TEXT("Food")&&TestSP>=100)||(PendingItem==TEXT("BookSTR")&&TestStrength>=10000)){Show(TEXT("이미 최대치입니다. 소모하지 않았습니다."));return;}
        const FName ID=PendingItem;if(!GetInventory()->Consume(ID,1))return;if(ID==TEXT("PotionHP"))TestHP=FMath::Min(100.f,TestHP+25);if(ID==TEXT("Food"))TestSP=FMath::Min(100.f,TestSP+25);if(ID==TEXT("BookSTR"))TestStrength=FMath::Min(10000,TestStrength+1);CancelTarget();Show(TEXT("시험 효과를 1회 적용했습니다."));return;
    }
    PendingRequest=FGuid::NewGuid();bEffectWaiting=true;OnEffectRequest.Broadcast(PendingRequest,PendingItem,Target);Show(TEXT("전투 파트의 효과 승인 대기 중"));
}

void APlayerPrototypeController::ResolveEffect(FGuid ID,bool Success)
{
    if(!bEffectWaiting||PendingRequest!=ID)return;bEffectWaiting=false;if(Success)GetInventory()->Consume(PendingItem,1);CancelTarget();Show(Success?TEXT("아이템 사용 완료"):TEXT("효과가 거절되었습니다. 아이템 미소모"));
}

void APlayerPrototypeController::PlayerDied()
{
    if(bEffectWaiting)ResolveEffect(PendingRequest,false);bDead=true;SetMenu(TEXT("Load"));Show(TEXT("사망했습니다. 저장 파일을 불러오세요."));UpdateGamePhase();
}

void APlayerPrototypeController::OpenContainer(FName ID,bool Home,bool Access)
{
    if(!Access){Show(TEXT("이 보관함의 접근 권한이 없습니다."));return;}
    auto* State=GetWorld()?GetWorld()->GetGameState<APlayerPrototypeGameState>():nullptr;OpenStorage=Home&&State?State->SharedHomeStorageID:ID;
    if(OpenStorage.IsNone())return;GetInventory()->EnsureStorage(OpenStorage);SetMenu(TEXT("Storage"));SelectedStorage=INDEX_NONE;
}
