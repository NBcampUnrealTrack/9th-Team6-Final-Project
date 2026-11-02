#include "Core/Controller/ICPlayerController.h"
#include "Audio/ICBGMSubsystem.h"
#include "Inventory/Actors/ICChestPresentationComponent.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "UI/Life/ICStorageWidget.h"
#include "UI/Framework/ICText.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Debug/ICLog.h"
#include "Core/ICGameState.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Inventory/Use/ICItemUseService.h"
#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "EngineUtils.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"

void AICPlayerController::BeginItem(FName ID,int32 Quick)
{
    IC_TRACE_FUNCTION();
    if(bItemTransactionInProgress||bDead)return;
    auto* Inventory=GetInventory();const auto* Definition=Inventory?Inventory->Find(ID):nullptr;
    if(!Definition||Inventory->CountItem(ID)<=0){ShowLocalized(ICText::Get(TEXT("Native_5d5993cc77fe")));return;}
    if(Definition->Kind==EPlayerItemKind::Expansion){Inventory->ExpandUsingTicket();return;}
    if(Definition->Kind!=EPlayerItemKind::Consumable){ShowLocalized(ICText::Get(TEXT("Native_76b3f1a3cd82")));return;}
    if(GetItemCooldown(ID)>KINDA_SMALL_NUMBER)
    {ShowLocalized(FText::Format(NSLOCTEXT("ICInventory","ItemCooldown","재사용까지 {0}초 남았습니다."),FText::AsNumber(FMath::CeilToInt(GetItemCooldown(ID)))));return;}
    UE_LOG(LogICInventory, Verbose, TEXT("[Item.TargetSelection] Item=%s Quick=%d"),*ID.ToString(),Quick);
    if(!Menu.IsEmpty())SetMenu(TEXT(""));
    CancelTarget();
    PendingItem=ID;PendingQuick=Quick;
    SelectTarget(UICWorldStateSubsystem::PlayerId());
    ShowLocalized(ICText::FormatText(TEXT("Native_784669b9489d"),*MenuCancelLabel()));
}

void AICPlayerController::BeginSkillTarget(AICCharacterBase* Caster,FName SkillID)
{
    if(!Caster||!Caster->GetICAbilitySystem())return;
    CancelTarget();
    PendingSkill=SkillID;
    PendingSkillCaster=Caster;
    ShowLocalized(ICText::FormatText(TEXT("Native_0f035969176e"),*MenuCancelLabel()));
}

bool AICPlayerController::SelectTarget(FName CharacterID)
{
    const auto* State=GetWorldState();
    const auto* Target=State?State->GetStoredCharacter(CharacterID):nullptr;
    if(!HasTargetSelection()||!Target)return false;
    const bool bPlayerTarget=CharacterID==UICWorldStateSubsystem::PlayerId();
    if(!bPlayerTarget&&(Target->Mode!=EICCharacterMode::Companion||Target->PartySlot<0||Target->PartySlot>=3))return false;
    bool Revive=false;
    if(!PendingSkill.IsNone())
    {
        auto* Caster=PendingSkillCaster.Get();
        auto* ASC=Caster?Caster->GetICAbilitySystem():nullptr;
        FICDefinitionResult Result;
        const auto* Skill=ASC&&ASC->Definitions()?ASC->Definitions()->Find<FICSkillRow>(PendingSkill,Result):nullptr;
        if(!Skill)return false;
        Revive=Skill->Purpose==EICSkillPurpose::Revive||Skill->TargetRules.Target==EICCombatTarget::DownedCompanion;
        if(Skill->TargetRules.Target==EICCombatTarget::Self&&CharacterID!=Caster->GetCharacterID())return false;
        if(CharacterID==Caster->GetCharacterID()&&!Skill->TargetRules.bAllowCaster&&Skill->TargetRules.Target!=EICCombatTarget::Self)return false;
    }
    else
    {
        const auto* Item=GetInventory()?GetInventory()->Find(PendingItem):nullptr;
        if(!Item)return false;
        Revive=Item->UseEffect==EICItemEffect::Revival;
        if(Item->UseTarget==EICItemTarget::PlayerOnly&&!bPlayerTarget)return false;
    }
    if(Target->IsDowned()!=Revive)return false;
    PendingTargetID=CharacterID;
    bDirty=true;
    ShowLocalized(ICText::FormatText(TEXT("Native_5f9a808e2ca6"),bPlayerTarget?ICText::Get(TEXT("Target_Self")):FText::FromName(CharacterID),FText::FromString(MenuCancelLabel())));
    return true;
}

void AICPlayerController::ConfirmSelectedTarget()
{
    if(!HasTargetSelection())return;
    if(PendingTargetID.IsNone()&&!SelectTarget(UICWorldStateSubsystem::PlayerId()))
    {ShowLocalized(ICText::Get(TEXT("Native_1ede9bfb236e")));return;}
    ConfirmTarget(PendingTargetID);
}

void AICPlayerController::ConfirmPartyTarget(int32 Slot)
{
    IC_TRACE_FUNCTION();
    if(Slot<0||Slot>=3||!GetWorldState())return;
    for(const auto& Pair:GetWorldState()->ExportState().Characters)
        if(Pair.Value.PartySlot==Slot){ConfirmTarget(Pair.Key);return;}
    ShowLocalized(ICText::Get(TEXT("Native_d67c832b2943")));
}
void AICPlayerController::ConfirmTarget(FName Target)
{
    IC_TRACE_FUNCTION();
    if(!PendingSkill.IsNone())
    {
        if(!SelectTarget(Target)){ShowLocalized(ICText::Get(TEXT("Native_9723a820ddd7")));return;}
        auto* Caster=PendingSkillCaster.Get();
        auto* ASC=Caster?Caster->GetICAbilitySystem():nullptr;
        if(!ASC){CancelTarget();return;}
        const auto Result=ASC->RequestSkillByID(PendingSkill,Target);
        if(Result.bSuccess){CancelTarget();ShowLocalized(ICText::Get(TEXT("Native_a74488bc1a14")));}
        else Show(Result.Reason);
        return;
    }
    if(PendingItem.IsNone())return;FText Reason;
    if(UseInventoryItem(PendingItem,Target,Reason))ShowLocalized(ICText::Get(TEXT("Native_2a766b32b014")));else ShowLocalized(Reason);
}

bool AICPlayerController::UseInventoryItem(FName ItemID,FName Target,FText& Reason)
{
    Reason=FText::GetEmpty();auto* Inventory=GetInventory();auto* State=GetWorldState();
    auto Fail=[&](FText Text){Reason=Text;return false;};
    if(ItemID.IsNone()||bItemTransactionInProgress||bDead||!Inventory||!State)return Fail(NSLOCTEXT("ICInventory","UseUnavailable","지금은 아이템을 사용할 수 없습니다."));
    const double Remaining=GetItemCooldown(ItemID);
    if(Remaining>KINDA_SMALL_NUMBER)return Fail(FText::Format(NSLOCTEXT("ICInventory","ItemCooldown","재사용까지 {0}초 남았습니다."),FText::AsNumber(FMath::CeilToInt(Remaining))));
    if(!UICWorldStateSubsystem::IsCharacterIdValid(Target)||!State->GetStoredCharacter(Target))
        return Fail(ICText::Get(TEXT("Native_f1e41db0ad98")));
    if(!State->FlushParticipants())return Fail(ICText::Get(TEXT("Native_1878d244a70a")));
    AICNPCBase* ReviveActor=nullptr;const auto* Item=Inventory->Find(ItemID);
    if(Item&&Item->UseEffect==EICItemEffect::Revival)
    {
        FString ReviveReason;auto* PlayerCharacter=Cast<AICPlayerCharacter>(GetPawn());auto* Spawn=GetWorld()->GetSubsystem<UICSpawnSubsystem>();
        ReviveActor=Spawn?Spawn->ReserveReviveTarget(Target,PlayerCharacter?PlayerCharacter->GetICAbilitySystem():nullptr,ReviveReason):nullptr;
        if(!ReviveActor)return Fail(ReviveReason.IsEmpty()?ICText::Get(TEXT("Target_RevivePlacementFailed")):FText::FromString(ReviveReason));
    }
    FICItemUseResult Prepared=UICItemUseService::Prepare(Inventory,Inventory->GetDefinitions(),ItemID,State->GetSnapshot(Target));
    if(!Prepared.bSuccess)return Fail(FText::FromString(Prepared.Reason));
    if(Item)
    {
        if(!FMath::IsFinite(Item->CooldownPlaySeconds)||Item->CooldownPlaySeconds<0)
            return Fail(NSLOCTEXT("ICInventory","InvalidItemCooldown","아이템의 재사용 대기시간 설정이 올바르지 않습니다."));
        if(Item->CooldownPlaySeconds>0)
        {
            const auto* Time=GetTime();
            Prepared.Inventory.ItemReadyAtPlaySecond.Add(ItemID,(Time?Time->GetClock().PlaySecond:0.)+Item->CooldownPlaySeconds);
        }
    }
    if(ReviveActor){Prepared.Character.Position=ReviveActor->GetActorLocation();Prepared.Character.Rotation=ReviveActor->GetActorRotation();}
    FICWorldState Next=State->ExportState();Next.Inventory=Prepared.Inventory;Next.Characters.Add(Target,Prepared.Character);
    bool Applied=false;
    {
        TGuardValue<bool> Guard(bItemTransactionInProgress,true);
        Applied=State->CommitGameplayTransaction(Next,FGuid::NewGuid());
    }
    if(!Applied)return Fail(ICText::Get(TEXT("Native_13ad4c07f5b9")));
    if(Prepared.GrowthAmount>0)OnStatElixirApplied.Broadcast(Target,Prepared.GrowthStat,Prepared.GrowthAmount);
    UE_LOG(LogICInventory, Log, TEXT("[Item.Used] Item=%s Target=%s HP=%.2f MP=%.2f SP=%.2f Growth=%d"),*Prepared.ItemID.ToString(),*Target.ToString(),Prepared.Character.HP,Prepared.Character.MP,Prepared.Character.SP,Prepared.GrowthAmount);
    CancelTarget();SyncSaveSafety();return true;
}
double AICPlayerController::GetItemCooldown(FName ItemID) const
{
    const auto* State=GetWorldState();const auto* Time=GetTime();
    if(!State||ItemID.IsNone())return 0.;
    const double Ready=State->GetInventorySnapshot().ItemReadyAtPlaySecond.FindRef(ItemID);
    return FMath::Max(0.,Ready-(Time?Time->GetClock().PlaySecond:0.));
}
void AICPlayerController::PlayerDied()
{
    IC_TRACE_FUNCTION();
    if(bDead)return;
    UE_LOG(LogICCore, Log, TEXT("[Player.Died] Controller=%s"),*GetNameSafe(this));
    bDead=true;bExitAfterSave=false;bQuitAfterWrite=false;
    if(auto* Audio=GetWorld()->GetSubsystem<UICBGMSubsystem>())Audio->SilenceForDeath();
    LoadReturnMenu=SaveReturnMenu=TEXT("Death");
    SetMenu(TEXT("Death"));ShowLocalized(ICText::Get(TEXT("Native_2a469f1dc6da")));
}
void AICPlayerController::OpenContainer(FName ID,bool Home,bool Access)
{
    IC_TRACE_FUNCTION();
    if(!Access){ShowLocalized(ICText::Get(TEXT("Native_26f38145a6c6")));return;}
    auto* State=GetWorld()?GetWorld()->GetGameState<AICGameState>():nullptr;
    if(StorageScreen)StorageScreen->CloseStorage();
    OpenStorage=Home&&State?State->SharedHomeStorageID:ID;
    if(OpenStorage.IsNone())return;
    const FName Container=OpenStorage;GetInventory()->EnsureStorage(Container);SetMenu(TEXT(""));SelectedStorage=INDEX_NONE;
    auto* Type=LoadClass<UICStorageWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Storage.WBP_IC_Storage_C"));
    StorageScreen=CreateWidget<UICStorageWidget>(this,Type);
    if(!StorageScreen||!StorageScreen->OpenStorage(Container)){StorageScreen=nullptr;OpenStorage=NAME_None;}
}
void AICPlayerController::CloseChestPresentation()
{if(ActiveChestPresentation.IsValid())ActiveChestPresentation->CloseChest();ActiveChestPresentation.Reset();}

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
    const auto* SleepingPlayer=Cast<AICPlayerCharacter>(GetPawn());
    const bool OwnSleep=SleepingPlayer&&SleepingPlayer->LifeActions&&SleepingPlayer->LifeActions->IsSleeping();
    if(!bCreated||bDead||bInCombat||bInDialogue||(bInEvent&&!OwnSleep)||bItemTransactionInProgress){ShowLocalized(ICText::Get(TEXT("Native_6a365287176f")));return false;}
    CombatCacheChanged();
    auto* State=GetWorldState();auto* Definitions=GetInventory()?GetInventory()->GetDefinitions():nullptr;
    if(!State||!Definitions||!State->FlushParticipants())return false;
    FICDefinitionResult Lookup;const auto* Rules=Definitions->Find<FICTimeSettingsRow>(TEXT("CFG_TIME_001"),Lookup);
    const int32 Minutes=FMath::Max(Rules?Rules->MinimumSleepGameMinutes:120,Survival.CalculateSleepMinutes(Rules?Rules->SleepRecoveryPerGameMinute:SleepSPRecoveryPerGameMinute));
    const bool WasExhausted=Survival.IsExhausted();
    auto Next=State->ExportState();
    for(auto& Pair:Next.Characters)
        if(Pair.Key==UICWorldStateSubsystem::PlayerId() || (Pair.Value.Kind==EICCharacterKind::Recruitable&&Pair.Value.Mode==EICCharacterMode::Companion&&Pair.Value.PartySlot>=0))
            ICCombatSimulation::RecoverFromSleep(Pair.Value,Next,Definitions);
    if(!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))return false;
    // Live GAS also includes equipped gear and current party synergies.
    for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)
        if(auto* ASC=It->GetICAbilitySystem())
        {
            const auto Snapshot=ASC->Capture();
            if(Snapshot.HP>0&&!Snapshot.IsDowned()&&(Snapshot.Kind==EICCharacterKind::Player || (Snapshot.Kind==EICCharacterKind::Recruitable&&Snapshot.Mode==EICCharacterMode::Companion&&Snapshot.PartySlot>=0)))
                ASC->SetVitals(Snapshot.MaxHP,Snapshot.MaxMP,Snapshot.MaxSP);
        }
    if(auto* C=Cast<AICPlayerCharacter>(GetPawn()))if(auto* ASC=C->GetICAbilitySystem())ASC->CancelAction();
    CombatCacheChanged();bLastExhausted=false;
    if(WasExhausted)OnExhaustionChanged.Broadcast(false);
    if(auto* Time=GetTime())Time->JumpGameMinutes(Minutes);
    UE_LOG(LogICCore, Log, TEXT("[Player.Sleep] GameMinutes=%d HP=%.2f MP=%.2f SP=%.2f"),Minutes,Survival.HP,Survival.MP,Survival.SP);
    OnSleepRequested.Broadcast(Minutes);FlushWorldState();bDirty=true;
    ShowLocalized(ICText::FormatText(TEXT("Native_d1dcacd6f1a5"),Minutes));
    if(!OwnSleep)AutoSavePlayer();return true;
}

bool AICPlayerController::CraftRecipe(FName RecipeID)
{
    IC_TRACE_FUNCTION();
    const auto* Recipe=GetCrafting()?GetCrafting()->Find(RecipeID):nullptr;
    if(!Recipe){ShowLocalized(ICText::Get(TEXT("Native_a9456ea3cc16")));return false;}
    if((Recipe->Type==EPlayerRecipeType::Cook&&ActiveStation!=TEXT("Kitchen"))||(Recipe->Type==EPlayerRecipeType::Craft&&ActiveStation!=TEXT("Forge")))
    {ShowLocalized(ICText::Get(TEXT("Native_77a6c968d1bd")));return false;}
    if(bInCombat||bDead||bInDialogue||bInEvent){ShowLocalized(ICText::Get(TEXT("Native_ed52ef7c0a0c")));return false;}
    FGuid JobID;FString Reason;
    if(!GetCrafting()->StartCraft(RecipeID,ActiveWorkAnchor,GetInventory(),JobID,Reason)){Show(Reason);return false;}
    SetMenu(TEXT(""));ShowLocalized(ICText::FormatText(TEXT("Native_10792f0439c9"),Recipe->DisplayName));return true;
}
