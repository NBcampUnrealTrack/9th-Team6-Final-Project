#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Debug/ICLog.h"
#include "Core/ICGameState.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Inventory/Use/ICItemUseService.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "NPC/Character/ICNPCBase.h"

void AICPlayerController::BeginItem(FName ID,int32 Quick)
{
    IC_TRACE_FUNCTION();
    if(bItemTransactionInProgress||bDead)return;
    auto* Inventory=GetInventory();const auto* Definition=Inventory?Inventory->Find(ID):nullptr;
    if(!Definition||Inventory->CountItem(ID)<=0){Show(TEXT("사용할 아이템이 없습니다."));return;}
    if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}
    if(Definition->Kind!=EPlayerItemKind::Consumable){Show(TEXT("사용 가능한 소비품이 아닙니다."));return;}
    UE_LOG(LogICInventory, Verbose, TEXT("[Item.TargetSelection] Item=%s Quick=%d"),*ID.ToString(),Quick);
    PendingItem=ID;PendingQuick=Quick;PendingPartyKey=INDEX_NONE;Show(FString::Printf(TEXT("대상 선택: 같은 숫자 키 = 자신 / Z X C V = 동료 / %s = 취소"),*MenuCancelLabel()));
}
void AICPlayerController::ConfirmPartyTarget(int32 Slot)
{
    IC_TRACE_FUNCTION();
    if(Slot<0||Slot>3||!GetWorldState())return;
    for(const auto& Pair:GetWorldState()->ExportState().Characters)
        if(Pair.Value.PartySlot==Slot){ConfirmTarget(Pair.Key);return;}
    Show(TEXT("해당 슬롯에 동료가 없습니다."));
}
void AICPlayerController::ConfirmTarget(FName Target)
{
    IC_TRACE_FUNCTION();
    auto* Inventory=GetInventory();auto* State=GetWorldState();
    if(PendingItem.IsNone()||bItemTransactionInProgress||!Inventory||!State)return;
    if(!UICWorldStateSubsystem::IsCharacterIdValid(Target)||!State->GetStoredCharacter(Target))
    {Show(TEXT("사용 가능한 대상이 없습니다."));return;}
    if(!State->FlushParticipants()){Show(TEXT("현재 상태를 반영할 수 없습니다. 변경하지 않았습니다."));return;}
    AICNPCBase* ReviveActor=nullptr;const auto* Item=Inventory->Find(PendingItem);
    if(Item&&Item->UseEffect==EICItemEffect::Revival)
    {
        FString Reason;auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());auto* Spawn=GetWorld()->GetSubsystem<UICSpawnSubsystem>();
        ReviveActor=Spawn?Spawn->ReserveReviveTarget(Target,PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr,Reason):nullptr;
        if(!ReviveActor){Show(Reason.IsEmpty()?TEXT("부활 대상을 안전하게 배치할 수 없습니다."):Reason);return;}
    }
    FICItemUseResult Prepared=UICItemUseService::Prepare(Inventory,Inventory->GetDefinitions(),PendingItem,State->GetSnapshot(Target));
    if(!Prepared.bSuccess){Show(Prepared.Reason);return;}
    if(ReviveActor){Prepared.Character.Position=ReviveActor->GetActorLocation();Prepared.Character.Rotation=ReviveActor->GetActorRotation();}
    FICWorldState Next=State->ExportState();Next.Inventory=Prepared.Inventory;Next.Characters.Add(Target,Prepared.Character);
    bool Applied=false;
    {
        TGuardValue<bool> Guard(bItemTransactionInProgress,true);
        Applied=State->CommitGameplayTransaction(Next,FGuid::NewGuid());
    }
    if(!Applied){Show(TEXT("상태 검증 실패. 아이템과 효과를 변경하지 않았습니다."));return;}
    if(Prepared.GrowthAmount>0)OnStatBookApplied.Broadcast(Target,Prepared.GrowthStat,Prepared.GrowthAmount);
    UE_LOG(LogICInventory, Log, TEXT("[Item.Used] Item=%s Target=%s HP=%.2f MP=%.2f SP=%.2f Growth=%d"),*Prepared.ItemID.ToString(),*Target.ToString(),Prepared.Character.HP,Prepared.Character.MP,Prepared.Character.SP,Prepared.GrowthAmount);
    CancelTarget();SyncSaveSafety();Show(TEXT("아이템 사용 완료"));
}
void AICPlayerController::PlayerDied()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICCore, Log, TEXT("[Player.Died] Controller=%s"),*GetNameSafe(this));
    bDead=true;SetMenu(TEXT("Load"));Show(TEXT("사망했습니다. 수동 또는 오토 저장을 선택해 불러오세요."));
}
void AICPlayerController::OpenContainer(FName ID,bool Home,bool Access)
{
    IC_TRACE_FUNCTION();
    if(!Access){Show(TEXT("이 보관함의 접근 권한이 없습니다."));return;}
    auto* State=GetWorld()?GetWorld()->GetGameState<AICGameState>():nullptr;
    OpenStorage=Home&&State?State->SharedHomeStorageID:ID;
    if(OpenStorage.IsNone())return;
    GetInventory()->EnsureStorage(OpenStorage);SetMenu(TEXT("Storage"));SelectedStorage=INDEX_NONE;
}

float AICPlayerController::ConsumeProductionSP(float Amount)
{
    IC_TRACE_FUNCTION();
    CombatCacheChanged();const float Consumed=FMath::IsFinite(Amount)?FMath::Clamp(Amount,0.f,Survival.SP):0.f;
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem())ASC->ChangeResource(TEXT("SP"),-Consumed);const bool WasExhausted=bLastExhausted;CombatCacheChanged();
    if(WasExhausted!=Survival.IsExhausted()){bLastExhausted=Survival.IsExhausted();OnExhaustionChanged.Broadcast(bLastExhausted);}FlushWorldState();bDirty=true;return Consumed;
}

bool AICPlayerController::Sleep()
{
    IC_TRACE_FUNCTION();
    if(!bCreated||bDead||bInCombat||bInDialogue||bInEvent||bItemTransactionInProgress){Show(TEXT("현재 상태에서는 잘 수 없습니다."));return false;}
    CombatCacheChanged();
    const int32 Minutes=Survival.CalculateSleepMinutes(SleepSPRecoveryPerGameMinute);
    const bool WasExhausted=Survival.IsExhausted();if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem()){ASC->CancelAction();ASC->RemoveSleepEffects();const auto S=ASC->Capture();ASC->SetVitals(S.MaxHP,S.MaxMP,S.MaxSP);}CombatCacheChanged();bLastExhausted=false;
    if(WasExhausted)OnExhaustionChanged.Broadcast(false);
    if(auto* Time=GetTime())Time->JumpGameMinutes(Minutes);
    UE_LOG(LogICCore, Log, TEXT("[Player.Sleep] GameMinutes=%d HP=%.2f MP=%.2f SP=%.2f"),Minutes,Survival.HP,Survival.MP,Survival.SP);
    OnSleepRequested.Broadcast(Minutes);FlushWorldState();bDirty=true;
    Show(FString::Printf(TEXT("수면 완료: 게임 시간 %d분 경과, HP·MP·SP 완전 회복"),Minutes));
    AutoSavePlayer();return true;
}

bool AICPlayerController::CraftRecipe(FName RecipeID)
{
    IC_TRACE_FUNCTION();
    const auto* Recipe=GetCrafting()?GetCrafting()->Find(RecipeID):nullptr;
    if(!Recipe){Show(TEXT("레시피를 조회할 수 없습니다."));return false;}
    if((Recipe->Type==EPlayerRecipeType::Cook&&ActiveStation!=TEXT("Kitchen"))||(Recipe->Type==EPlayerRecipeType::Craft&&ActiveStation!=TEXT("Forge")))
    {Show(TEXT("올바른 작업대에서만 제작할 수 있습니다."));return false;}
    if(bInCombat||bDead||bInDialogue||bInEvent){Show(TEXT("현재 상태에서는 제작할 수 없습니다."));return false;}
    FGuid JobID;FString Reason;
    if(!GetCrafting()->StartCraft(RecipeID,ActiveWorkAnchor,GetInventory(),JobID,Reason)){Show(Reason);return false;}
    SetMenu(TEXT(""));Show(FString::Printf(TEXT("%s 제작 시작"),*Recipe->DisplayName.ToString()));return true;
}
