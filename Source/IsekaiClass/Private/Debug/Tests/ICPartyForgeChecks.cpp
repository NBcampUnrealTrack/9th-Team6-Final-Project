#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "NPC/Character/ICNPCBase.h"
#include "Combat/Party/ICPartyStatus.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Combat/ICSynergyRow.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ProgressBar.h"
#include "World/State/ICWorldStateSubsystem.h"

namespace ICPartyForgeChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);UWorld* World;UICWorldStateSubsystem* State;UICDefinitionSubsystem* D;AICPlayerController* PC;AICPlayerCharacter* Player;UICInventoryComponent* Bag;
    FSession()
    {
        GI->InitializeStandalone();World=GI->GetWorld();auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        World->InitializeActorsForPlay(FURL(),false);State=GI->GetSubsystem<UICWorldStateSubsystem>();D=GI->GetSubsystem<UICDefinitionSubsystem>();
        FICWorldState W;W.bPlayerCreated=true;W.PlayerName=TEXT("ForgeTest");W.FirstSkill=TEXT("SKL_NPC_R01_002");FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;W.Characters.Add(P.CharacterId,P);State->RestoreState(W);
        PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);PC->bCreated=true;State->RestoreState(W);Player->InitializeState(P.CharacterId);Bag=Player->Inventory;Bag->BindWorldState(State);State->SetPlayerGold(20000);
    }
    void Recruit(FName ID,int32 Slot){State->EnsureCharacter(ID,EICCharacterKind::Recruitable);State->SetAffinity(ID,300);State->SetPartySlot(ID,Slot);}
    int32 Slot(FName ID)const{return Bag->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& I){return I.ItemID==ID&&!I.IsEmpty();});}
    ~FSession(){Player->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPartySynergyCheck,"IC.PartyForge.SynergyRulesAndCombatRefresh",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPartySynergyCheck::RunTest(const FString&)
{
    ICPartyForgeChecks::FSession F;auto* ASC=F.Player->GetICAbilitySystem();TestTrue(TEXT("Actual player state is initialized"),F.State->GetStoredCharacter(UICWorldStateSubsystem::PlayerId())!=nullptr);
    F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(),EICMainStat::INT,40);F.Bag->AddItem(TEXT("EQP_WPN_STAFF_001"),1,false);TestTrue(TEXT("Player selects a real mage weapon"),F.Bag->Equip(F.Slot(TEXT("EQP_WPN_STAFF_001"))));const double Base=ASC->Stat(EICStat::MagicAttack);TestTrue(TEXT("Real attack stats are nonzero"),Base>10);
    F.Recruit(TEXT("NPC_R01_001"),0);F.Recruit(TEXT("NPC_R01_008"),1);F.Recruit(TEXT("NPC_R01_002"),2);
    auto Rows=ICPartyStatus::ActiveSynergies(F.State->ExportState(),F.D);TestEqual(TEXT("Mage trio and hero triple are active"),Rows.Num(),2);double Bonus=0;
    for(auto* R:Rows)for(const auto& M:R->StatModifiers)if(M.Stat==EICStat::MagicAttack)Bonus+=M.Percent;
    TestTrue(TEXT("20% class plus 30% heroes adds to 50%"),FMath::IsNearlyEqual(Bonus,.5));
    TestEqual(TEXT("Live attack stats refresh when companions join"),ASC->Stat(EICStat::MagicAttack),FMath::RoundToDouble(Base*1.5));
    auto Dead=F.State->GetSnapshot(TEXT("NPC_R01_001"));Dead.HP=0;Dead.LifeState=EICCharacterLifeState::Downed;F.State->SetCharacterState(Dead);
    Rows=ICPartyStatus::ActiveSynergies(F.State->ExportState(),F.D);TestEqual(TEXT("Downed mage loses class bonus and downgrades hero triple"),Rows.Num(),1);
    if(!Rows.IsEmpty())TestEqual(TEXT("Hero double replaces triple"),Rows[0]->ID,FName(TEXT("SYN_HERO_DOUBLE")));
    TestEqual(TEXT("Live stats lose inactive synergy without manual refresh"),ASC->Stat(EICStat::MagicAttack),FMath::RoundToDouble(Base*1.1));
    auto CheckPool=[&](TArray<FName> IDs,FName Expected){auto S=F.State->ExportState();for(auto& P:S.Characters)if(P.Value.Kind!=EICCharacterKind::Player){P.Value.Mode=EICCharacterMode::Scheduled;P.Value.PartySlot=INDEX_NONE;}int32 N=0;for(FName ID:IDs){F.State->EnsureCharacter(ID,EICCharacterKind::Recruitable);auto P=F.State->GetSnapshot(ID);P.Mode=EICCharacterMode::Companion;P.PartySlot=N++;P.HP=P.MaxHP;P.LifeState=EICCharacterLifeState::Alive;S.Characters.Add(ID,P);}const auto Active=ICPartyStatus::ActiveSynergies(S,F.D);TestTrue(TEXT("Named triple activates its authored pool"),Active.ContainsByPredicate([&](const auto* R){return R->ID==Expected;}));};
    CheckPool({TEXT("NPC_R01_009"),TEXT("NPC_R01_010"),TEXT("NPC_R01_011")},TEXT("SYN_CLASSMATES"));CheckPool({TEXT("NPC_R01_005"),TEXT("NPC_R01_006"),TEXT("NPC_R01_007")},TEXT("SYN_VILLAGERS"));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWeaponForgeCheck,"IC.PartyForge.EnhancementTransactions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICWeaponForgeCheck::RunTest(const FString&)
{
    ICPartyForgeChecks::FSession F;const FName Weapon=TEXT("EQP_WPN_SWORD_001"),Ore=TEXT("ITM_R01_ORE_01");F.Bag->AddItem(Weapon,1,false);F.Bag->AddItem(Ore,5,false);
    const FGuid ID=F.Bag->GetSnapshot().Bag.Slots[F.Slot(Weapon)].InstanceID;bool Success=false;FText Message;double Probability;int32 GoldCost,Count;
    TestTrue(TEXT("Offer is read without spending"),F.Bag->GetEnhancementOffer(ID,Ore,Probability,GoldCost,Count,Message));TestEqual(TEXT("Viewing/cancelling consumes no ore"),F.Bag->CountItem(Ore),5);
    const int32 Before=F.State->ExportState().Gold;TestTrue(TEXT("First stage commits"),F.Bag->TryEnhanceWeapon(ID,Ore,Success,Message));TestTrue(TEXT("Guaranteed first stage succeeds"),Success);TestEqual(TEXT("One exact ore consumed"),F.Bag->CountItem(Ore),4);TestEqual(TEXT("Gold consumes once"),F.State->ExportState().Gold,Before-GoldCost);
    TestEqual(TEXT("Same weapon identity gains one upgrade"),F.Bag->GetSnapshot().Bag.Slots[F.Slot(Weapon)].Upgrade,1);
    TestTrue(TEXT("Upgraded bag weapon equips"),F.Bag->Equip(F.Slot(Weapon)));auto Targets=F.Bag->GetEnhancementTargets();TestEqual(TEXT("Mirrored equipment appears only once"),Targets.FilterByPredicate([&](const auto& T){return T.Item.InstanceID==ID;}).Num(),1);
    FICDefinitionResult R;auto* Rules=const_cast<FICEnhancementRow*>(F.D->Find<FICEnhancementRow>(TEXT("ENH_WPN_SWORD_001"),R));if(!TestNotNull(TEXT("Enhance rules loaded"),Rules))return false;
    const double Saved=Rules->Steps[1].SuccessProbability;Rules->Steps[1].SuccessProbability=0;const int32 Gold=F.State->ExportState().Gold;
    TestTrue(TEXT("Failed roll is still a committed attempt"),F.Bag->TryEnhanceWeapon(ID,Ore,Success,Message));Rules->Steps[1].SuccessProbability=Saved;
    TestFalse(TEXT("Zero probability fails"),Success);TestEqual(TEXT("Failure leaves equipped level intact"),F.Bag->GetWeaponLoadout()[0].Upgrade,1);TestEqual(TEXT("Failed attempt still consumes ore"),F.Bag->CountItem(Ore),3);TestEqual(TEXT("Failed attempt uses stage two cost"),F.State->ExportState().Gold,Gold-Rules->Steps[1].GoldCost);
    F.State->SetPlayerGold(0);TestFalse(TEXT("No gold rejects before mutation"),F.Bag->TryEnhanceWeapon(ID,Ore,Success,Message));TestEqual(TEXT("Rejected attempt keeps ore"),F.Bag->CountItem(Ore),3);
    TestFalse(TEXT("Missing weapon rejects before mutation"),F.Bag->TryEnhanceWeapon(FGuid::NewGuid(),Ore,Success,Message));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICRevivalOrganizationCheck,"IC.PartyForge.RevivalAndInventoryUI",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICRevivalOrganizationCheck::RunTest(const FString&)
{
    ICPartyForgeChecks::FSession F;FICDefinitionResult Lookup;const FName Scroll=TEXT("ITM_CONS_REVIVE_SCROLL"),NPC=TEXT("NPC_R01_002");const auto* Item=F.D->Find<FICItemRow>(Scroll,Lookup);if(!TestNotNull(TEXT("Revival scroll exists"),Item))return false;TestEqual(TEXT("Scroll costs 5000G"),Item->BuyPrice,5000);
    TArray<FName> Shops;F.D->GetDefinitionIDs(EICDefinitionType::Shop,Shops);for(FName Shop:Shops)if(const auto* R=F.D->Find<FICShopRow>(Shop,Lookup))TestTrue(TEXT("Shop offers scroll"),R->Entries.ContainsByPredicate([&](const auto& E){return E.ItemID==Scroll;}));
    F.Recruit(NPC,0);auto Dead=F.State->GetSnapshot(NPC);Dead.HP=0;Dead.LifeState=EICCharacterLifeState::Downed;F.State->SetCharacterState(Dead);
    auto* Actor=F.World->SpawnActor<AICNPCBase>(AICNPCBase::StaticClass(),FVector(500,0,100),FRotator::ZeroRotator);if(!TestNotNull(TEXT("Companion actor spawns away from player"),Actor))return false;TestTrue(TEXT("Companion definition initializes"),Actor->InitializeNPC(NPC));TestFalse(TEXT("Downed companion cannot be dismissed"),Actor->CanDismiss());
    auto* Type=LoadClass<UICCharacterStatusWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CompanionStatus.WBP_IC_CompanionStatus_C"));auto* Status=CreateWidget<UICCharacterStatusWidget>(F.PC,Type);Status->UpdateVitals(Dead);TestTrue(TEXT("HUD identifies downed state"),Status->bDowned);TestEqual(TEXT("Downed MP display is emptied"),CastChecked<UProgressBar>(Status->GetWidgetFromName(TEXT("BAR_MP")))->GetPercent(),0.f);
    F.Bag->AddItem(Scroll,1,false);FText Reason;TestTrue(TEXT("Scroll revives selected downed companion"),F.PC->UseInventoryItem(Scroll,NPC,Reason));const auto Alive=F.State->GetSnapshot(NPC);TestFalse(TEXT("Life state restored"),Alive.IsDowned());TestTrue(TEXT("HP restored"),Alive.HP>0);TestEqual(TEXT("Scroll consumed once"),F.Bag->CountItem(Scroll),0);Status->UpdateVitals(Alive);TestFalse(TEXT("HUD leaves gray state after revival"),Status->bDowned);Actor->ReleaseForLoad();
    F.Bag->AddItem(TEXT("ITM_CONS_001"),10,false);auto* UI=CreateWidget<UICInventoryMenuWidget>(F.PC,UICInventoryMenuWidget::StaticClass());UI->RefreshInventory();auto* Search=CastChecked<UEditableTextBox>(UI->GetWidgetFromName(TEXT("InventorySearch")));Search->SetText(FText::FromString(TEXT("ITM_CONS_001")));TestTrue(TEXT("Search result opens"),UI->OpenItem(0));
    UI->LockSelected();TestTrue(TEXT("Lock action updates actual stack"),F.Bag->GetSnapshot().Bag.Slots[F.Slot(TEXT("ITM_CONS_001"))].bLocked);UI->LockSelected();auto* Popup=CastChecked<UICInventoryItemPopupWidget>(UI->GetWidgetFromName(TEXT("ItemPopup")));CastChecked<USpinBox>(Popup->GetWidgetFromName(TEXT("SplitQuantity")))->SetValue(3);UI->SplitSelected();
    TestEqual(TEXT("Splitting preserves total count"),F.Bag->CountItem(TEXT("ITM_CONS_001")),10);TestEqual(TEXT("Split creates two real stacks"),F.Bag->GetSnapshot().Bag.Slots.FilterByPredicate([](const auto& I){return I.ItemID==TEXT("ITM_CONS_001");}).Num(),2);UI->SortItems();TestEqual(TEXT("Sorting preserves total count"),F.Bag->CountItem(TEXT("ITM_CONS_001")),10);
    return !HasAnyErrors();
}
#endif
