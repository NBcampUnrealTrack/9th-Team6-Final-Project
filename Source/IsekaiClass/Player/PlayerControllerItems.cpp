#include "PlayerPrototypeController.h"
#include "PlayerGameState.h"
#include "PlayerInventory.h"
#include "PlayerCrafting.h"
#include "PlayerTimeSubsystem.h"
#include "PlayerWorldStateSubsystem.h"

void APlayerPrototypeController::BeginItem(FName ID,int32 Quick)
{
    if(bEffectWaiting||bDead)return;auto* Inventory=GetInventory();const auto* Definition=Inventory->Find(ID);
    if(!Definition||Inventory->CountItem(ID)<=0){Show(TEXT("사용할 아이템이 없습니다."));return;}if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}if(Definition->Kind!=EPlayerItemKind::Consumable){Show(TEXT("사용 가능한 소비품이 아닙니다."));return;}
    PendingItem=ID;PendingQuick=Quick;PendingPartyKey=INDEX_NONE;Show(TEXT("대상 선택: Z X C V = 동료 / 같은 키 재입력 = 플레이어 / ESC = 취소"));
}

bool APlayerPrototypeController::UseStatBook(FName ItemID,FName TargetID,const FPlayerItemDefinition& Definition)
{
    if(Definition.Category!=EPlayerItemCategory::Book||Definition.EffectID!=TEXT("GrowMainStat")||Definition.GrowthStat==EPlayerMainStat::None||Definition.GrowthAmount<=0)
    {Show(TEXT("경험의 서 데이터가 올바르지 않습니다."));return false;}
    const bool bPlayer=TargetID==TEXT("Player")||TargetID==TEXT("CHR_PLAYER");int32 Applied=0;
    if(bPlayer)
    {
        if(MainStats.Get(Definition.GrowthStat)>=10000){Show(TEXT("해당 능력치가 이미 최대치입니다. 아이템을 소모하지 않았습니다."));return false;}
        Applied=MainStats.Add(Definition.GrowthStat,Definition.GrowthAmount);
    }
    else
    {
        auto* WorldState=GetGameInstance()->GetSubsystem<UPlayerWorldStateSubsystem>();
        if(!WorldState){Show(TEXT("동료 상태 저장소에 접근할 수 없습니다."));return false;}
        Applied=WorldState->AddMainStat(TargetID,Definition.GrowthStat,Definition.GrowthAmount);
    }
    if(Applied<=0||!GetInventory()->Consume(ItemID,1))return false;
    const TCHAR* StatNames[]={TEXT("근력"),TEXT("민첩"),TEXT("지력"),TEXT("체력"),TEXT("매력"),TEXT("운")};
    const int32 StatIndex=static_cast<int32>(Definition.GrowthStat);
    if(bPlayer)FlushWorldState();
    OnStatBookApplied.Broadcast(bPlayer?FName(TEXT("CHR_PLAYER")):TargetID,Definition.GrowthStat,Applied);
    Show(FString::Printf(TEXT("%s 사용: %s +%d"),*Definition.DisplayName.ToString(),StatNames[FMath::Clamp(StatIndex,0,5)],Applied));
    return true;
}

void APlayerPrototypeController::ConfirmTarget(FName Target)
{
    if(PendingItem.IsNone()||bEffectWaiting||GetInventory()->CountItem(PendingItem)<=0)return;
    const auto* Definition=GetInventory()->Find(PendingItem);if(!Definition)return;
    if(Definition->Category==EPlayerItemCategory::Book){const FName ID=PendingItem;if(UseStatBook(ID,Target,*Definition))CancelTarget();return;}
    if(bUseTestEffects)
    {
        const bool bPlayer=Target==TEXT("Player")||Target==TEXT("CHR_PLAYER");
        if(!bPlayer)
        {
            auto* WorldState=GetGameInstance()->GetSubsystem<UPlayerWorldStateSubsystem>();FPlayerPersistentSnapshot Snapshot;
            if(!WorldState||!WorldState->GetSnapshot(Target,Snapshot)){Show(TEXT("선택한 동료의 상태를 찾을 수 없습니다."));return;}
            if((Definition->RestoreHP>0.f&&Snapshot.Survival.HP>=Snapshot.Survival.MaxHP)||(Definition->RestoreSP>0.f&&Snapshot.Survival.SP>=Snapshot.Survival.MaxSP)){Show(TEXT("이미 최대치입니다. 소모하지 않았습니다."));return;}
            if(!GetInventory()->Consume(PendingItem,1))return;
            Snapshot.Survival.RestoreHP(Definition->RestoreHP);Snapshot.Survival.RestoreSP(Definition->RestoreSP);WorldState->SetSnapshot(Snapshot);CancelTarget();Show(TEXT("동료에게 아이템 효과를 적용했습니다."));return;
        }
        if((Definition->RestoreHP>0.f&&Survival.HP>=Survival.MaxHP)||(Definition->RestoreSP>0.f&&Survival.SP>=Survival.MaxSP)||(Definition->GrowthStat!=EPlayerMainStat::None&&MainStats.Get(Definition->GrowthStat)>=10000)){Show(TEXT("이미 최대치입니다. 소모하지 않았습니다."));return;}
        const FName ID=PendingItem;if(!GetInventory()->Consume(ID,1))return;
        if(Definition->RestoreHP>0.f)Survival.RestoreHP(Definition->RestoreHP);
        if(Definition->RestoreSP>0.f){const bool WasExhausted=Survival.IsExhausted();Survival.RestoreSP(Definition->RestoreSP);if(WasExhausted!=Survival.IsExhausted()){bLastExhausted=Survival.IsExhausted();OnExhaustionChanged.Broadcast(bLastExhausted);}}
        FlushWorldState();CancelTarget();Show(TEXT("아이템 효과를 적용했습니다."));return;
    }
    PendingRequest=FGuid::NewGuid();bEffectWaiting=true;OnEffectRequest.Broadcast(PendingRequest,PendingItem,Target);Show(TEXT("전투 파트의 효과 승인 대기 중"));
}

float APlayerPrototypeController::ConsumeProductionSP(float Amount)
{
    const bool WasExhausted=Survival.IsExhausted();const float Consumed=Survival.ConsumeSP(Amount);
    if(WasExhausted!=Survival.IsExhausted()){bLastExhausted=Survival.IsExhausted();OnExhaustionChanged.Broadcast(bLastExhausted);}bDirty=true;return Consumed;
}

void APlayerPrototypeController::ApplyGameMinutes(int32 Minutes)
{
    if(Minutes<=0)return;ConsumeProductionSP(SPDrainPerGameMinute*Minutes);
}

bool APlayerPrototypeController::Sleep()
{
    if(!bCreated||bDead||bInCombat||bInDialogue||bInEvent||bEffectWaiting){Show(TEXT("현재 상태에서는 잘 수 없습니다."));return false;}
    const int32 Minutes=Survival.CalculateSleepMinutes(SleepSPRecoveryPerGameMinute);
    const bool WasExhausted=Survival.IsExhausted();Survival.CompleteSleep();bLastExhausted=false;
    if(WasExhausted)OnExhaustionChanged.Broadcast(false);
    if(auto* Time=GetWorld()?GetWorld()->GetSubsystem<UPlayerTimeSubsystem>():nullptr)Time->AdvanceGameMinutes(Minutes);
    OnSleepRequested.Broadcast(Minutes);FlushWorldState();bDirty=true;
    Show(FString::Printf(TEXT("수면 완료: 게임 시간 %d분 경과, HP·MP·SP 완전 회복"),Minutes));
    AutoSavePlayer();return true;
}

bool APlayerPrototypeController::CraftRecipe(FName RecipeID)
{
    const auto* Recipe=GetCrafting()?GetCrafting()->Find(RecipeID):nullptr;
    if(!Recipe){Show(TEXT("레시피를 찾을 수 없습니다."));return false;}
    if((Recipe->Type==EPlayerRecipeType::Cook&&ActiveStation!=TEXT("Kitchen"))||(Recipe->Type==EPlayerRecipeType::Craft&&ActiveStation!=TEXT("Forge"))){Show(TEXT("올바른 작업대에서만 제작할 수 있습니다."));return false;}
    if(Survival.SP<Recipe->SPCost){Show(TEXT("SP가 부족합니다."));return false;}
    if(!GetCrafting()->Craft(RecipeID,GetInventory()))return false;
    ConsumeProductionSP(Recipe->SPCost);FlushWorldState();Show(FString::Printf(TEXT("%s 제작 완료"),*Recipe->DisplayName.ToString()));return true;
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
