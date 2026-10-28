#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Life/World/ICRanchAnimal.h"
#include "Life/World/ICLifeActor.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "Inventory/Use/ICItemUseService.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "NPC/LifeNPC/ICShopSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveGame.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifeCalendarCheck,"IC.Life.CalendarResourceRegeneration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifeCalendarCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("생활 달력 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);State->RestoreState(Initial);
    auto* PC=World->SpawnActor<AICPlayerController>();FICDefinitionResult R;const auto* Row=D->Find<FICResourceRow>(TEXT("RES_R01_GATHER_01"),R);
    auto* Plant=World->SpawnActor<AICLifeActor>(Row->ResourceActorClass.LoadSynchronous());Plant->PointID=TEXT("TEST_CALENDAR_RESOURCE");FText Failure;
    TestTrue(TEXT("Resource initially available"),Plant->IsResourceAvailable());
    TestTrue(TEXT("Harvest grants resource"),Plant->CompleteResourceGather(PC,Failure));
    TestFalse(TEXT("Just harvested resource is unavailable"),Plant->IsResourceAvailable());
    const double PlayBefore=Time->GetClock().PlaySecond;Time->JumpGameMinutes(119);
    TestFalse(TEXT("119 game minutes is still depleted"),Plant->IsResourceAvailable());
    auto* Save=NewObject<UICSaveGame>();Save->WorldState=State->ExportState();Save->Clock=Time->GetClock();TArray<uint8> Bytes;
    TestTrue(TEXT("Calendar cooldown saves"),UICSaveGame::EncodeForFile(Save,Bytes));auto* Loaded=UICSaveGame::DecodeFile(Bytes);
    if(TestNotNull(TEXT("Calendar cooldown reloads"),Loaded)){Time->RestoreClock(Loaded->Clock);State->RestoreState(Loaded->WorldState);TestFalse(TEXT("Reload cannot reset cooldown"),Plant->IsResourceAvailable());}
    Time->JumpGameMinutes(1);TestEqual(TEXT("Sleeping does not advance play seconds"),Time->GetClock().PlaySecond,PlayBefore);
    TestTrue(TEXT("Exactly two game hours restores availability"),Plant->IsResourceAvailable());
    TestTrue(TEXT("A regenerated resource can be harvested again"),Plant->CompleteResourceGather(PC,Failure));
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICRanchHuntingCheck,"IC.Life.ChickenCombatAndSavedLoot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICRanchHuntingCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("닭 사냥 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);State->RestoreState(Initial);
    FICDefinitionResult R;const auto* Row=D->Find<FICRanchingRow>(TEXT("RAN_R01_01"),R);
    auto* Type=Row?Row->AnimalActorClass.LoadSynchronous():nullptr;
    auto* Chicken=Type?Cast<AICRanchAnimal>(World->SpawnActor<AActor>(Type)):nullptr;
    if(!TestNotNull(TEXT("Authored chicken BP is a real combat character"),Chicken)){GI->Shutdown();World->DestroyWorld(false);return false;}
    if(!Chicken->HasActorBegunPlay())Chicken->DispatchBeginPlay();Chicken->RefreshAnimal();
    auto* ASC=Chicken->GetICAbilitySystem();TestTrue(TEXT("Chicken has living GAS vitals"),ASC&&ASC->IsAlive());
    FICAttackCapture Hit;Hit.Magnitude=5;
    TestTrue(TEXT("Actual combat damage can hit chicken"),ASC->ApplyDamage(Hit,nullptr,false,false,false,false));
    const auto Injured=State->ExportState();const float HurtHP=ASC->Capture().HP;State->RestoreState(Injured);
    TestEqual(TEXT("Loading preserves nonlethal damage"),ASC->Capture().HP,HurtHP);
    Hit.Magnitude=1000;TestTrue(TEXT("Combat killing blow applies"),ASC->ApplyDamage(Hit,nullptr,false,false,false,false));ASC->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Killing blow starts the visible slaughter phase"),Chicken->IsSlaughtering());
    // Timers created before the first world tick enter the active heap on the following frame.
    {TGuardValue<uint64> Frame(GFrameCounter,GFrameCounter+1);World->GetTimerManager().Tick(.01f);}
    {TGuardValue<uint64> Frame(GFrameCounter,GFrameCounter+2);World->GetTimerManager().Tick(30.f);}
    auto* Bag=NewObject<UICInventoryComponent>();Bag->BeginStaging(D,0);Bag->ApplySnapshot(State->GetInventorySnapshot());
    TestEqual(TEXT("Kill grants exactly one chicken meat"),Bag->CountItem(TEXT("ITM_R01_CHICKEN_MEAT")),1);
    const auto Killed=State->ExportState();Chicken->HandleZeroHP();State->RestoreState(Killed);Chicken->RefreshAnimal();
    TestTrue(TEXT("Loading keeps the killed animal absent"),Chicken->IsHidden());
    Bag->ApplySnapshot(State->GetInventorySnapshot());TestEqual(TEXT("Death retry/load cannot duplicate meat"),Bag->CountItem(TEXT("ITM_R01_CHICKEN_MEAT")),1);
    int32 WithEgg=0,WithoutEgg=0;TSet<int32> Quantities;
    for(int32 Index=1;Index<=128;++Index)
    {
        auto Next=Initial;TArray<FICItemAmount> Received;FString Reason;const FGuid Receipt(1,2,3,Index);
        TestTrue(TEXT("DT livestock reward rolls"),FICRewardService::PrepareLifeLoot(Next,Row->DropTableID,Receipt,D,FVector::ZeroVector,0,Received,Reason));
        Bag->ApplySnapshot(Next.Inventory);TestEqual(TEXT("Every roll guarantees meat"),Bag->CountItem(TEXT("ITM_R01_CHICKEN_MEAT")),1);
        const int32 Eggs=Bag->CountItem(TEXT("ITM_R01_RANCH_01"));TestTrue(TEXT("Egg drop is absent or within 1..4"),Eggs==0||(Eggs>=1&&Eggs<=4));
        if(Eggs>0){++WithEgg;Quantities.Add(Eggs);}else ++WithoutEgg;
        const auto Once=Next.Inventory;FICRewardService::PrepareLifeLoot(Next,Row->DropTableID,Receipt,D,FVector::ZeroVector,0,Received,Reason);
        TestTrue(TEXT("Same death receipt never grants twice"),Received.IsEmpty());Bag->ApplySnapshot(Next.Inventory);TestEqual(TEXT("Meat remains one after retry"),Bag->CountItem(TEXT("ITM_R01_CHICKEN_MEAT")),1);
    }
    TestTrue(TEXT("DT probability produces success and failure"),WithEgg>0&&WithoutEgg>0);TestEqual(TEXT("All four allowed egg quantities are possible"),Quantities.Num(),4);
    auto Full=Initial;for(auto& Slot:Full.Inventory.Bag.Slots){Slot.ItemID=TEXT("ITM_R01_SEED_01");Slot.Count=99;Slot.InstanceID=FGuid::NewGuid();}
    TArray<FICItemAmount> Received;FString Reason;
    TestTrue(TEXT("Full bag still commits hunting reward"),FICRewardService::PrepareLifeLoot(Full,Row->DropTableID,FGuid::NewGuid(),D,FVector::ZeroVector,0,Received,Reason));
    TestTrue(TEXT("Overflow stays in a saved world drop"),!Full.Drops.IsEmpty());TestTrue(TEXT("Full bag does not show received items"),Received.IsEmpty());
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifeEconomyCheck,"IC.Life.ItemEconomyRecoveryAndMigration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifeEconomyCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("아이템 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Player.HP=Player.SP=1;Initial.Characters.Add(Player.CharacterId,Player);State->RestoreState(Initial);
    FICDefinitionResult R;auto* Bag=NewObject<UICInventoryComponent>();Bag->BeginStaging(D,0);
    for(int32 I=1;I<=30;++I)
    {
        FName ID(*FString::Printf(TEXT("ITM_R01_FOOD_%02d"),I));const auto* Food=D->Find<FICItemRow>(ID,R);
        if(!TestNotNull(TEXT("Every R01 dish exists"),Food))continue;
        Bag->ApplySnapshot(FICInventoryState());Bag->AddItem(ID,1,false);auto Use=UICItemUseService::Prepare(Bag,D,ID,Player);
        TestTrue(TEXT("Every dish is usable through the actual item service"),Use.bSuccess);
        TestEqual(TEXT("Food DT restores HP 25"),Use.Character.HP,26.f);TestEqual(TEXT("Food DT restores SP 15"),Use.Character.SP,16.f);
        TestTrue(TEXT("Food sale is enabled"),Food->bCanSell&&Food->SellPrice>=0);
    }
    auto* Egg=const_cast<FICItemRow*>(D->Find<FICItemRow>(TEXT("ITM_R01_RANCH_01"),R));
    if(Egg){TGuardValue<float> HP(Egg->RestoreHP,17);TGuardValue<float> SP(Egg->RestoreSP,9);Bag->ApplySnapshot(FICInventoryState());Bag->AddItem(Egg->ID,1,false);auto Use=UICItemUseService::Prepare(Bag,D,Egg->ID,Player);TestTrue(TEXT("Livestock recovery works"),Use.bSuccess);TestEqual(TEXT("Edited DT HP is used directly"),Use.Character.HP,18.f);TestEqual(TEXT("Edited DT SP is used directly"),Use.Character.SP,10.f);}
    State->EnsureCharacter(TEXT("NPC_R01_101"),EICCharacterKind::Resident);
    auto Merchant=State->GetSnapshot(TEXT("NPC_R01_101"));Merchant.CurrentAnchorID=TEXT("ANC_MERCHANT_SHOP");State->SetCharacterState(Merchant);
    auto* Time=GI->GetSubsystem<UICTimeSubsystem>();auto Clock=Time->GetClock();Clock.GameMinute=600;Time->RestoreClock(Clock);
    TArray<FICShopOffer> Offers;FString Reason;TestTrue(TEXT("Existing merchant resolves seed offers"),GI->GetSubsystem<UICShopSubsystem>()->GetOffers(TEXT("SRV_MERCHANT_SHOP"),Offers,Reason));
    for(int32 I=1;I<=5;++I){const FName ID(*FString::Printf(TEXT("ITM_R01_SEED_%02d"),I));const auto* Seed=D->Find<FICItemRow>(ID,R);const auto* Offer=Offers.FindByPredicate([&](const auto& Value){return Value.ItemID==ID;});TestTrue(TEXT("Every seed has buy/sell DT prices and an offer"),Seed&&Seed->BuyPrice>0&&Seed->SellPrice>0&&Offer&&Offer->BasePrice==Seed->BuyPrice&&Offer->SellPrice==Seed->SellPrice);}
    const auto* Ring=D->Find<FICItemRow>(TEXT("ITM_QST_001"),R);TestTrue(TEXT("Quest ring restrictions remain"),Ring&&Ring->Tab==EPlayerItemTab::Quest&&!Ring->bCanDiscard&&!Ring->bCanSell&&!Ring->bCanGift);
    auto Legacy=Initial;auto& Old=Legacy.Inventory.Bag.Slots[0];Old.ItemID=TEXT("ITM_MAT_002");Old.Count=3;Old.InstanceID=FGuid::NewGuid();Legacy.Inventory.QuickSlots[0]=Old.ItemID;Legacy.Codex.AcquiredItemIDs.Add(Old.ItemID);
    TestTrue(TEXT("Old inventory still loads"),State->RestoreState(Legacy));const auto Migrated=State->ExportState();
    TestEqual(TEXT("Deleted flour becomes actual wheat"),Migrated.Inventory.Bag.Slots[0].ItemID,FName(TEXT("ITM_R01_CROP_01")));
    TestEqual(TEXT("Migration retains quantity"),Migrated.Inventory.Bag.Slots[0].Count,3);TestTrue(TEXT("Discovery migrates too"),Migrated.Codex.AcquiredItemIDs.Contains(TEXT("ITM_R01_CROP_01")));
    auto* World=GI->GetWorld();GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
