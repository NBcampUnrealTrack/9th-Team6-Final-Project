#include "Core/Controller/ICPlayerController.h"
#include "Core/ICGameState.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

void AICPlayerController::BeginItem(FName ID,int32 Quick)
{
    if(bEffectWaiting||bDead)return;
    auto* Inventory=GetInventory();const auto* Definition=Inventory?Inventory->Find(ID):nullptr;
    if(!Definition||Inventory->CountItem(ID)<=0){Show(TEXT("사용할 아이템이 없습니다."));return;}
    if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}
    if(Definition->Kind!=EPlayerItemKind::Consumable){Show(TEXT("사용 가능한 소비품이 아닙니다."));return;}
    PendingItem=ID;PendingQuick=Quick;PendingPartyKey=INDEX_NONE;Show(FString::Printf(TEXT("대상 선택: 같은 숫자 키 = 자신 / Z X C V = 동료 / %s = 취소"),*MenuCancelLabel()));
}
void AICPlayerController::ConfirmTarget(FName Target)
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
    if(bUseNativeItemEffects)
    {
        FICCharacterState TargetCharacter=*Stored;
        const auto* Definition=Inventory->Find(PendingItem);if(!Definition)return;
        if(Definition->Category==EPlayerItemCategory::Book || Definition->EffectID==TEXT("GrowSTR"))
        {const FName ID=PendingItem;if(UseStatBook(ID,Target,*Definition))CancelTarget();return;}
        bool Changed=false;
        if(Definition->RestoreHP>0.f&&TargetCharacter.HP<TargetCharacter.MaxHP)
        {TargetCharacter.HP=FMath::Min(TargetCharacter.MaxHP,TargetCharacter.HP+Definition->RestoreHP);Changed=true;}
        if(Definition->RestoreSP>0.f&&TargetCharacter.SP<TargetCharacter.MaxSP)
        {TargetCharacter.SP=FMath::Min(TargetCharacter.MaxSP,TargetCharacter.SP+Definition->RestoreSP);Changed=true;}
        if(!Changed){Show(TEXT("이미 최대치이거나 사용할 수 없는 효과입니다. 아이템은 소모하지 않았습니다."));return;}
        FICInventoryState NextInventory;
        if(!Inventory->BuildConsumption(PendingItem,1,NextInventory)||!State->CommitCharacterAndInventory(TargetCharacter,NextInventory))
        {Show(TEXT("상태 검증 실패. 아이템과 효과를 변경하지 않았습니다."));return;}
        CancelTarget();Show(TEXT("시험 효과와 아이템 소모를 1회 적용했습니다."));return;
    }
    PendingRequest=FGuid::NewGuid();bEffectWaiting=true;SyncSaveSafety();
    Show(TEXT("전투 파트의 효과 응답 대기 중"));
    OnEffectRequest.Broadcast(PendingRequest,PendingItem,Target);
}
void AICPlayerController::ResolveEffect(FGuid ID,bool Success)
{
    if(!bEffectWaiting||PendingRequest!=ID)return;
    const bool Consumed=Success&&GetInventory()&&GetInventory()->Consume(PendingItem,1);
    bEffectWaiting=false;CancelTarget();SyncSaveSafety();
    Show(Success?(Consumed?TEXT("아이템 사용 완료"):TEXT("효과 처리 후 아이템 소모 실패. 담당 연동을 확인하세요.")):TEXT("효과가 거절되었습니다. 아이템 미소모"));
}
void AICPlayerController::PlayerDied()
{
    if(bEffectWaiting)ResolveEffect(PendingRequest,false);
    bDead=true;SetMenu(TEXT("Load"));Show(TEXT("사망했습니다. 수동 또는 오토 저장을 선택해 불러오세요."));
}
void AICPlayerController::OpenContainer(FName ID,bool Home,bool Access)
{
    if(!Access){Show(TEXT("이 보관함의 접근 권한이 없습니다."));return;}
    auto* State=GetWorld()?GetWorld()->GetGameState<AICGameState>():nullptr;
    OpenStorage=Home&&State?State->SharedHomeStorageID:ID;
    if(OpenStorage.IsNone())return;
    GetInventory()->EnsureStorage(OpenStorage);SetMenu(TEXT("Storage"));SelectedStorage=INDEX_NONE;
}

bool AICPlayerController::UseStatBook(FName ItemID,FName TargetID,const FPlayerItemDefinition& Definition)
{
    auto* State=GetWorldState();auto* Inventory=GetInventory();
    const auto* Stored=State?State->GetStoredCharacter(TargetID):nullptr;
    const EPlayerMainStat Stat=Definition.EffectID==TEXT("GrowSTR")?EPlayerMainStat::Strength:Definition.GrowthStat;
    const int32 Amount=Definition.EffectID==TEXT("GrowSTR")?1:Definition.GrowthAmount;
    if(!Stored||!Inventory||Stat==EPlayerMainStat::None||Amount<=0)return false;
    FICCharacterState Next=*Stored; EICMainStat CoreStat=static_cast<EICMainStat>(Stat);
    const int32 Values[]={Next.MainStats.STR,Next.MainStats.DEX,Next.MainStats.INT,Next.MainStats.CON,Next.MainStats.CHA,Next.MainStats.LUK};
    const int32 Index=static_cast<int32>(Stat);if(Index<0||Index>5)return false;
    const int32 Before=Values[Index];const int32 After=FMath::Clamp(Before+Amount,1,10000);
    if(After==Before){Show(TEXT("해당 능력치가 최대치입니다. 아이템은 소모하지 않았습니다."));return false;}
    FICInventoryState Items;
    if(!Next.MainStats.Set(CoreStat,After)||!Inventory->BuildConsumption(ItemID,1,Items)||!State->CommitCharacterAndInventory(Next,Items))return false;
    OnStatBookApplied.Broadcast(TargetID,Stat,After-Before);
    Show(FString::Printf(TEXT("%s 사용: 능력치 +%d"),*Definition.GetLabel().ToString(),After-Before));return true;
}
float AICPlayerController::ConsumeProductionSP(float Amount)
{
    const bool WasExhausted=Survival.IsExhausted();const float Consumed=Survival.ConsumeSP(Amount);
    if(WasExhausted!=Survival.IsExhausted()){bLastExhausted=Survival.IsExhausted();OnExhaustionChanged.Broadcast(bLastExhausted);}FlushWorldState();bDirty=true;return Consumed;
}

void AICPlayerController::ApplyGameMinutes(int32 Minutes)
{
    if(Minutes<=0)return;ConsumeProductionSP(SPDrainPerGameMinute*Minutes);
}

bool AICPlayerController::Sleep()
{
    if(!bCreated||bDead||bInCombat||bInDialogue||bInEvent||bEffectWaiting){Show(TEXT("현재 상태에서는 잘 수 없습니다."));return false;}
    const int32 Minutes=Survival.CalculateSleepMinutes(SleepSPRecoveryPerGameMinute);
    const bool WasExhausted=Survival.IsExhausted();Survival.CompleteSleep();bLastExhausted=false;
    if(WasExhausted)OnExhaustionChanged.Broadcast(false);
    if(auto* Time=GetTime())Time->JumpGameMinutes(Minutes);
    OnSleepRequested.Broadcast(Minutes);FlushWorldState();bDirty=true;
    Show(FString::Printf(TEXT("수면 완료: 게임 시간 %d분 경과, HP·MP·SP 완전 회복"),Minutes));
    AutoSavePlayer();return true;
}

bool AICPlayerController::CraftRecipe(FName RecipeID)
{
    const auto* Recipe=GetCrafting()?GetCrafting()->Find(RecipeID):nullptr;
    if(!Recipe){Show(TEXT("레시피를 찾을 수 없습니다."));return false;}
    if((Recipe->Type==EPlayerRecipeType::Cook&&ActiveStation!=TEXT("Kitchen"))||(Recipe->Type==EPlayerRecipeType::Craft&&ActiveStation!=TEXT("Forge"))){Show(TEXT("올바른 작업대에서만 제작할 수 있습니다."));return false;}
    if(Survival.SP<Recipe->SPCost){Show(TEXT("SP가 부족합니다."));return false;}
    if(!GetCrafting()->Craft(RecipeID,GetInventory()))return false;
    ConsumeProductionSP(Recipe->SPCost);FlushWorldState();Show(FString::Printf(TEXT("%s 제작 완료"),*Recipe->DisplayName.ToString()));return true;
}
