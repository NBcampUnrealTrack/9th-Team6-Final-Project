#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "UI/Life/ICStorageWidget.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Actors/ICChestPresentationComponent.h"
#include "Life/World/ICPlayerHomeActors.h"
#include "NPC/World/ICNPCContainerActor.h"

namespace ICStorageChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);UWorld* World=nullptr;UICWorldStateSubsystem* State=nullptr;AICPlayerController* PC=nullptr;AICPlayerCharacter* Player=nullptr;UICInventoryComponent* Bag=nullptr;
    FSession(bool Local=false)
    {
        GI->InitializeStandalone();World=GI->GetWorld();
        if(Local)
        {
            auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
            PRAGMA_DISABLE_DEPRECATION_WARNINGS
            Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
            PRAGMA_ENABLE_DEPRECATION_WARNINGS
        }
        World->InitializeActorsForPlay(FURL(),false);State=GI->GetSubsystem<UICWorldStateSubsystem>();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("독립 창고 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
        FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Initial.Characters.Add(P.CharacterId,P);State->RestoreState(Initial);
        PC=World->SpawnActor<AICPlayerController>();if(Local){FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));}
        Player=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Player);PC->bCreated=true;PC->Menu.Reset();Player->InitializeState(P.CharacterId);Bag=Player->Inventory;Bag->BindWorldState(State);
        GI->GetSubsystem<UICTimeSubsystem>()->SetRunning(true);GI->GetSubsystem<UICTimeSubsystem>()->SetMenuPaused(false);
    }
    int32 Slot(FName ID) const{return Bag->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& I){return I.ItemID==ID&&!I.IsEmpty();});}
    ~FSession(){if(PC->StorageScreen)PC->StorageScreen->CloseStorage();Player->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStorageQuantityCheck,"IC.Storage.QuantityCapacityAndReservations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICStorageQuantityCheck::RunTest(const FString&)
{
    ICStorageChecks::FSession F;FText Reason;const FName Home=TEXT("PlayerHome"),Potion=TEXT("ITM_CONS_001");F.Bag->AddItem(Potion,30,false);const int32 Index=F.Slot(Potion);
    F.Bag->BindQuickSlot(0,Index);const auto Original=F.Bag->GetSnapshot().Bag.Slots[Index];
    TestTrue(TEXT("Selected twelve items deposit"),F.Bag->TransferAmount(Home,Index,12,true,Reason));auto Data=F.Bag->GetSnapshot();
    TestEqual(TEXT("Partial deposit leaves eighteen"),Data.Bag.Slots[Index].Count,18);TestEqual(TEXT("Chest receives exactly twelve"),Data.Storage[Home].Slots[0].Count,12);
    TestTrue(TEXT("Split stacks have independent identities"),Data.Bag.Slots[Index].InstanceID!=Data.Storage[Home].Slots[0].InstanceID);
    TestEqual(TEXT("Source stack identity remains"),Data.Bag.Slots[Index].InstanceID,Original.InstanceID);
    TestTrue(TEXT("Three items withdraw"),F.Bag->TransferAmount(Home,0,3,false,Reason));TestEqual(TEXT("Withdrawal merges into bag"),F.Bag->CountItem(Potion),21);
    TestFalse(TEXT("Cannot transfer more than the source count"),F.Bag->TransferAmount(Home,0,10,false,Reason));TestEqual(TEXT("Rejected amount changes nothing"),F.Bag->GetSnapshot().Storage[Home].Slots[0].Count,9);
    F.Bag->EnsureStorage(TEXT("TEST_STORAGE_SECOND"));TestEqual(TEXT("Second chest is independently forty slots"),F.Bag->GetSnapshot().Storage[TEXT("TEST_STORAGE_SECOND")].Slots.Num(),40);
    TestFalse(TEXT("Old home upgrade never grows an individual chest"),F.Bag->SetHomeStorageStage(3));
    TestTrue(TEXT("Remaining potion stack deposits"),F.Bag->TransferAmount(Home,Index,21,true,Reason));TestTrue(TEXT("Moving the final stack clears exhausted quick binding"),F.Bag->GetSnapshot().QuickSlots[0].IsNone());
    F.Bag->AddItem(Potion,5,false);Data=F.Bag->GetSnapshot();const int32 Source=F.Slot(Potion);
    for(auto& Item:Data.Storage[Home].Slots){Item=Original;Item.InstanceID=FGuid::NewGuid();Item.Count=99;}Data.Storage[Home].Slots[39].Count=98;F.Bag->ApplySnapshot(Data);
    TestFalse(TEXT("Insufficient capacity rejects the whole selected amount"),F.Bag->TransferAmount(Home,Source,5,true,Reason));
    TestEqual(TEXT("Failed transfer leaves source intact"),F.Bag->CountItem(Potion),5);TestEqual(TEXT("Failed transfer leaves destination intact"),F.Bag->GetSnapshot().Storage[Home].Slots[39].Count,98);
    TestTrue(TEXT("Exactly one free stack unit can be used"),F.Bag->TransferAmount(Home,Source,1,true,Reason));
    Data=F.Bag->GetSnapshot();Data.Bag.Slots[Source].bLocked=true;F.Bag->ApplySnapshot(Data);TestFalse(TEXT("Locked item cannot move"),F.Bag->TransferAmount(Home,Source,1,true,Reason));
    F.Bag->AddItem(TEXT("ITM_R01_ORE_01"),1,false);F.Bag->AddItem(TEXT("ITM_R01_ORE_02"),1,false);F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(),EICMainStat::CON,10);
    FGuid Job;FString Failure;auto* Production=F.GI->GetSubsystem<UICProductionSubsystem>();
    TestTrue(TEXT("Crafting reserves the ore"),Production->StartRecipe(TEXT("RCP_PLAYER_STORAGE_01"),TEXT("ANC_FORGE_001"),F.Bag,Job,Failure));
    TestEqual(TEXT("Production material is not transferable"),F.Bag->GetTransferableCount(TEXT("TEST_STORAGE_SECOND"),F.Slot(TEXT("ITM_R01_ORE_01")),true),0);
    TestFalse(TEXT("Reserved ore cannot be stashed"),F.Bag->TransferAmount(TEXT("TEST_STORAGE_SECOND"),F.Slot(TEXT("ITM_R01_ORE_01")),1,true,Reason));Production->CancelJob(Job);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStorageScreenCheck,"IC.Storage.IndependentWBPAndQuantityPopup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICStorageScreenCheck::RunTest(const FString&)
{
    ICStorageChecks::FSession F(true);F.Bag->AddItem(TEXT("ITM_CONS_001"),20,false);F.PC->OpenContainer(TEXT("PlayerHome"),false,true);auto* UI=F.PC->StorageScreen.Get();
    if(!TestNotNull(TEXT("F opens the independent storage WBP"),UI))return false;
    TestTrue(TEXT("Storage is not a gameplay menu page"),F.PC->Menu.IsEmpty());TestTrue(TEXT("Authored storage WBP is used"),UI->GetClass()->GetPathName().Contains(TEXT("WBP_IC_Storage_C")));
    TestEqual(TEXT("Left pane has exactly forty cells"),CastChecked<UUniformGridPanel>(UI->GetWidgetFromName(TEXT("ChestGrid")))->GetChildrenCount(),40);
    TestEqual(TEXT("Right pane is the actual bag"),CastChecked<UUniformGridPanel>(UI->GetWidgetFromName(TEXT("StorageBagGrid")))->GetChildrenCount(),F.Bag->GetSnapshot().Bag.Slots.Num());
    TestNull(TEXT("No gameplay menu tabs"),UI->GetWidgetFromName(TEXT("GameMenu")));TestNull(TEXT("No quick slots"),UI->GetWidgetFromName(TEXT("QuickSlot0")));
    const int32 Index=F.Slot(TEXT("ITM_CONS_001"));TestTrue(TEXT("Bag item opens transfer information"),UI->OpenItem(false,Index));auto* Popup=CastChecked<UICStorageTransferPopup>(UI->GetWidgetFromName(TEXT("TransferPopup")));
    for(const TCHAR* Name:{TEXT("BTN_Use"),TEXT("BTN_Equip"),TEXT("BTN_Quick"),TEXT("BTN_Trash")})TestNull(TEXT("Transfer popup contains no inventory actions"),Popup->GetWidgetFromName(Name));
    TestEqual(TEXT("Popup defaults to one item"),UI->Quantity,1);CastChecked<UButton>(Popup->GetWidgetFromName(TEXT("BTN_More")))->OnClicked.Broadcast();TestEqual(TEXT("Right arrow increases quantity"),UI->Quantity,2);
    UI->AdjustQuantity(100);TestEqual(TEXT("Quantity caps at actual stock"),UI->Quantity,20);UI->CloseItem();TestEqual(TEXT("Back never moves an item"),F.Bag->CountItem(TEXT("ITM_CONS_001")),20);
    TestTrue(TEXT("Reopen the item"),UI->OpenItem(false,Index));UI->AdjustQuantity(4);F.PC->ExecuteUI(TEXT("Use"));TestEqual(TEXT("Legacy use command is blocked on this screen"),F.Bag->CountItem(TEXT("ITM_CONS_001")),20);
    TestTrue(TEXT("Five-item deposit commits"),UI->TransferSelected());TestFalse(TEXT("Success closes only the popup"),UI->bPopupOpen);TestTrue(TEXT("Chest screen stays open"),UI->IsOpen());TestEqual(TEXT("Bag retains fifteen"),F.Bag->CountItem(TEXT("ITM_CONS_001")),15);
    TestTrue(TEXT("Chest item opens withdrawal popup"),UI->OpenItem(true,0));TestTrue(TEXT("Popup names the inventory destination"),CastChecked<UTextBlock>(Popup->GetWidgetFromName(TEXT("BTN_Transfer_Text")))->GetText().ToString().Contains(TEXT("인벤토리")));
    UI->AdjustQuantity(1);TestTrue(TEXT("Two-item withdrawal commits"),UI->TransferSelected());TestEqual(TEXT("Bag receives exactly two"),F.Bag->CountItem(TEXT("ITM_CONS_001")),17);
    UI->CloseStorage();TestNull(TEXT("Closing releases the storage screen"),F.PC->StorageScreen.Get());TestTrue(TEXT("Closing returns directly to the world"),F.PC->Menu.IsEmpty());return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStorageRecoveryCheck,"IC.Storage.LegacyOverflowSurvivesUntilPickup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICStorageRecoveryCheck::RunTest(const FString&)
{
    ICStorageChecks::FSession F;auto Old=F.State->ExportState();auto& Slots=Old.Inventory.Storage[TEXT("PlayerHome")].Slots;Slots.SetNum(160);
    for(int32 I=0;I<40;++I){Slots[I].ItemID=TEXT("ITM_R01_SEED_01");Slots[I].Count=99;Slots[I].InstanceID=FGuid::NewGuid();}
    Slots[130].ItemID=TEXT("ITM_BOOK_001");Slots[130].Count=1;Slots[130].InstanceID=FGuid::NewGuid();const auto ItemID=Slots[130].InstanceID;
    TestTrue(TEXT("Expanded save restores without losing its old overflow"),F.State->RestoreState(Old));auto Saved=F.State->ExportState();TestEqual(TEXT("Loaded storage is forty slots"),Saved.Inventory.Storage[TEXT("PlayerHome")].Slots.Num(),40);
    if(!TestTrue(TEXT("Overflow has a saved recovery drop"),Saved.Drops.Num()==1&&Saved.Drops[0].bStorageRecovery))return false;
    auto* Drop=F.World->SpawnActor<AICDroppedItem>(Saved.Drops[0].Position,FRotator::ZeroRotator);Drop->Initialize(Saved.Drops[0]);auto* Time=F.GI->GetSubsystem<UICTimeSubsystem>();Time->SetRunning(true);Time->SetMenuPaused(false);Time->AdvanceClock(1000,false);Drop->Tick(.25f);
    TestEqual(TEXT("Storage migration items never expire on the ground"),F.State->ExportState().Drops.Num(),1);Drop->TryPickup(F.PC);TestEqual(TEXT("Recovered item returns to bag"),F.Bag->CountItem(TEXT("ITM_BOOK_001")),1);
    TestEqual(TEXT("Recovery preserves item identity"),F.Bag->GetSnapshot().Bag.Slots[F.Slot(TEXT("ITM_BOOK_001"))].InstanceID,ItemID);TestEqual(TEXT("Pickup removes the completed recovery drop"),F.State->ExportState().Drops.Num(),0);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICChestAndFurnitureCheck,"IC.Polish.ChestAndFurniture",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICChestAndFurnitureCheck::RunTest(const FString&)
{
    ICStorageChecks::FSession F(true);
    auto* Type=LoadClass<AICPlayerStorage>(nullptr,TEXT("/Game/IsekaiClass/Life/Home/Blueprints/BP_IC_PlayerStorage.BP_IC_PlayerStorage_C"));
    auto* Chest=Type?F.World->SpawnActor<AICPlayerStorage>(Type,F.Player->GetActorLocation()+FVector(100,0,0),FRotator::ZeroRotator):nullptr;
    if(!TestNotNull(TEXT("Authored chest exists"),Chest))return false;
    TestTrue(TEXT("F begins lid animation"),Chest->TryInteract(F.PC));TestNull(TEXT("Storage does not open before the lid"),F.PC->StorageScreen);
    Chest->ChestPresentation->TickComponent(.3f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Lid opens progressively"),Chest->ChestPresentation->GetOpenAmount()>0&&Chest->ChestPresentation->GetOpenAmount()<1);
    TestNull(TEXT("UI still waits at the intermediate pose"),F.PC->StorageScreen);
    Chest->ChestPresentation->TickComponent(.5f,LEVELTICK_All,nullptr);
    if(TestNotNull(TEXT("Storage opens after the lid finishes"),F.PC->StorageScreen.Get()))F.PC->StorageScreen->CloseStorage();
    Chest->ChestPresentation->TickComponent(1,LEVELTICK_All,nullptr);TestEqual(TEXT("Closing storage shuts the lid"),Chest->ChestPresentation->GetOpenAmount(),0.f);
    F.State->EnsureCharacter(TEXT("NPC_R01_001"),EICCharacterKind::Recruitable);F.State->SetAffinity(TEXT("NPC_R01_001"),300);
    auto* FurnitureType=LoadClass<AICNPCContainerActor>(nullptr,TEXT("/Game/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCBookshelf.BP_IC_NPCBookshelf_C"));
    auto* Shelf=FurnitureType?F.World->SpawnActor<AICNPCContainerActor>(FurnitureType):nullptr;
    if(!TestNotNull(TEXT("Authored bookshelf exists"),Shelf))return false;
    Shelf->OwnerNPCID=TEXT("NPC_R01_001");Shelf->ContainerID=TEXT("CNT_POLISH_BOOKSHELF");
    TestTrue(TEXT("Bookshelf provides direct pickup"),Shelf->TryInteract(F.PC));TestNull(TEXT("Bookshelf never opens storage UI"),F.PC->StorageScreen);
    const auto Saved=F.State->ExportState();TestTrue(TEXT("Pickup is persisted against this specific furniture"),Saved.FurnitureLootReadyAt.Contains(Shelf->ContainerID));
    TestFalse(TEXT("No duplicate pickup while depleted"),Shelf->IsFurnitureLootAvailable());
    TestTrue(TEXT("Restore furniture state"),F.State->RestoreState(Saved));TestFalse(TEXT("Loading does not refill once-only furniture"),Shelf->IsFurnitureLootAvailable());return !HasAnyErrors();
}
#endif
