#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICInventoryMenuCheck,"IC.UI.InventoryMenu",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICInventoryMenuCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("인벤토리");Data.FirstSkill=TEXT("SKL_NPC_R01_002");
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Data.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Initialize actual world inventory"),State->RestoreState(Data));Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);
    PC->bCreated=true;PC->Menu=TEXT("Pause");auto* Inv=Player->Inventory.Get();
    const auto Class=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
    auto* HUD=Class?CreateWidget<UICGameplayWidget>(PC,Class):nullptr;
    auto* Menu=HUD?Cast<UICGameMenuWidget>(HUD->GetWidgetFromName(TEXT("GameMenu"))):nullptr;
    auto* UI=Menu?Cast<UICInventoryMenuWidget>(Menu->GetWidgetFromName(TEXT("InventoryContent"))):nullptr;
    if(TestNotNull(TEXT("Inventory has its own embedded WBP"),UI))
    {
        const auto Slate=Menu->TakeWidget();Menu->SelectTab(TEXT("Inventory"));UI->RefreshInventory();
        TestNull(TEXT("Retired armor section is removed"),UI->GetWidgetFromName(TEXT("ArmorHeading")));
        TestNull(TEXT("Inventory search is removed"),UI->GetWidgetFromName(TEXT("InventorySearch")));
        TestNull(TEXT("Global equipment action bar is removed"),UI->GetWidgetFromName(TEXT("BTN_Equip")));
        TestEqual(TEXT("Only one held weapon slot is stored"),Inv->GetSnapshot().Equipment.Num(),1);
        TestNull(TEXT("Class DT no longer exposes starting armor"),FICClassRow::StaticStruct()->FindPropertyByName(TEXT("StartingClothesID")));
        TestNull(TEXT("NPC DT no longer exposes a ring slot"),FICInitialEquipment::StaticStruct()->FindPropertyByName(TEXT("RingID")));
        auto Label=[&](const TCHAR* Key){return CastChecked<UTextBlock>(UI->GetWidgetFromName(Key))->GetText().ToString();};
        auto BagIndex=[&](FName ID){return Inv->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& Item){return Item.ItemID==ID&&!Item.IsEmpty();});};
        auto SelectBag=[&](FName ID){UI->SelectItemTab(EPlayerItemTab::Normal);return UI->SelectItem(BagIndex(ID));};
        Inv->AddItem(TEXT("ITM_TKT_001"),1,false);SelectBag(TEXT("ITM_TKT_001"));TestTrue(TEXT("Bag ticket opens its detail"),UI->OpenItem(UI->SelectedItem));
        const int32 BeforeCapacity=Inv->GetSnapshot().Bag.Slots.Num();TestTrue(TEXT("Inventory detail uses expansion ticket directly"),UI->BeginUseSelected());
        TestFalse(TEXT("Expansion ticket does not ask for a party target"),UI->bPickingUseTarget);TestEqual(TEXT("Ticket adds ten bag slots"),Inv->GetSnapshot().Bag.Slots.Num(),BeforeCapacity+10);TestEqual(TEXT("Ticket consumes once"),Inv->CountItem(TEXT("ITM_TKT_001")),0);
        UI->Activate(); // The following roster checks start from the menu's member area.
        TestEqual(TEXT("Only the actual player appears initially"),UI->Members.Num(),1);
        for(int32 I=0;I<4;++I)TestTrue(TEXT("All four player weapon panels visible"),UI->GetWidgetFromName(FName(*FString::Printf(TEXT("WeaponFrame%d"),I)))->IsVisible());
        const FName Weapons[]={TEXT("EQP_WPN_SWORD_001"),TEXT("EQP_WPN_BOW_001"),TEXT("EQP_WPN_STAFF_001"),TEXT("EQP_WPN_WAND_001")};
        for(FName ID:Weapons){TestTrue(TEXT("Add actual weapon"),Inv->AddItem(ID,1,false));TestTrue(TEXT("Equip correct family"),Inv->Equip(BagIndex(ID)));}
        const auto Loadout=Inv->GetWeaponLoadout();for(int32 I=0;I<4;++I)TestEqual(TEXT("Each family has a separate actual weapon"),Loadout[I].ItemID,Weapons[I]);
        const FName NPC=TEXT("NPC_R01_002");TestTrue(TEXT("Initialize genuine companion and gear"),State->EnsureCharacter(NPC,EICCharacterKind::Recruitable));State->SetAffinity(NPC,300);State->SetPartySlot(NPC,0);UI->RefreshInventory();
        Menu->HandleMenuKey(EKeys::S);TestEqual(TEXT("WASD selects real companion"),UI->SelectedCharacter,NPC);
        for(int32 I=1;I<4;++I)TestEqual(TEXT("Companion has only one weapon panel"),UI->GetWidgetFromName(FName(*FString::Printf(TEXT("WeaponFrame%d"),I)))->GetVisibility(),ESlateVisibility::Collapsed);
        Inv->AddItem(Weapons[2],1,false);TestFalse(TEXT("Companion rejects another weapon family"),Inv->EquipForCharacter(NPC,BagIndex(Weapons[2])));
        Inv->AddItem(Weapons[0],1,false);const FGuid Incoming=Inv->GetSnapshot().Bag.Slots[BagIndex(Weapons[0])].InstanceID;
        TestTrue(TEXT("Companion swaps eligible weapon atomically"),Inv->EquipForCharacter(NPC,BagIndex(Weapons[0])));
        TestEqual(TEXT("Companion actual equipment changed"),State->GetSnapshot(NPC).Equipment[0].InstanceID,Incoming);
        TestTrue(TEXT("Companion can unequip into shared inventory"),Inv->UnequipForCharacter(NPC,EPlayerEquipSlot::Weapon));
        Inv->AddItem(TEXT("ITM_CONS_001"),7,false);Inv->AddItem(TEXT("ITM_FOOD_001"),3,false);
        UI->SelectMember(0);SelectBag(TEXT("ITM_CONS_001"));
        auto* Confirm=CastChecked<UICItemConfirmWidget>(UI->GetWidgetFromName(TEXT("ItemConfirm")));
        for(int32 Slot=0;Slot<4;++Slot)
        {
            if(Slot==0)
            {
                TestTrue(TEXT("Selecting an item opens its own popup"),UI->OpenItem(UI->SelectedItem));
                auto* Detail=CastChecked<UICInventoryItemPopupWidget>(UI->GetWidgetFromName(TEXT("ItemPopup")));
                TestFalse(TEXT("Consumable hides equipment action"),Detail->GetWidgetFromName(TEXT("BTN_Equip"))->IsVisible());
                TestTrue(TEXT("Consumable offers quick-slot assignment"),Detail->GetWidgetFromName(TEXT("BTN_Quick"))->IsVisible());
                TestFalse(TEXT("Item popup blocks switching the main tab"),Menu->SelectTab(TEXT("Party")));
                CastChecked<UButton>(Detail->GetWidgetFromName(TEXT("BTN_Quick")))->OnClicked.Broadcast();
                TestTrue(TEXT("Quick-slot button starts picking"),UI->bPickingQuickSlot);
                CastChecked<UButton>(UI->GetWidgetFromName(TEXT("TAB_Quest")))->OnClicked.Broadcast();
                TestEqual(TEXT("A tab click cannot interrupt slot picking"),UI->NavigationArea,5);
                auto* Cell=CastChecked<UICInventorySlotWidget>(UI->GetWidgetFromName(TEXT("QuickSlot0")));
                CastChecked<UButton>(Cell->GetWidgetFromName(TEXT("BTN_Select")))->OnClicked.Broadcast();
            }
            else {TestTrue(TEXT("Begin slot picking"),UI->BeginQuickSlot());TestTrue(TEXT("Choose one of all four slots"),UI->ChooseQuickSlot(Slot));}
            TestTrue(TEXT("Confirmation opens before a change"),UI->bConfirming);
            TestTrue(TEXT("Empty slot confirmation asks to register"),CastChecked<UTextBlock>(Confirm->GetWidgetFromName(TEXT("TXT_Message")))->GetText().ToString().Contains(TEXT("넣겠습니까")));
            TestTrue(TEXT("Confirmation defaults to cancel"),Confirm->Choice==1);
            if(Slot==0)CastChecked<UButton>(Confirm->GetWidgetFromName(TEXT("BTN_Accept")))->OnClicked.Broadcast();else UI->ResolveConfirmation(true);
            TestEqual(TEXT("Confirmed slot uses actual binding"),Inv->GetSnapshot().QuickSlots[Slot],FName(TEXT("ITM_CONS_001")));
        }
        {
            // Future DT icons must appear without rebuilding the inventory WBP.
            FICDefinitionResult R;const auto* NPCRow=Definitions->Find<FICNPCRow>(NPC,R);
            auto* Potion=const_cast<FICItemRow*>(Inv->Find(TEXT("ITM_CONS_001")));
            TGuardValue<TSoftObjectPtr<UTexture2D>> TemporaryIcon(Potion->Icon,NPCRow->Portrait);
            TGuardValue<FString> TemporaryDescription(Potion->DescriptionKey,Potion->NameKey);
            UI->RefreshInventory();auto* Cell=CastChecked<UICInventorySlotWidget>(UI->GetWidgetFromName(TEXT("QuickSlot0")));
            TestEqual(TEXT("Quick slot loads the registered item's DT icon"),CastChecked<UImage>(Cell->GetWidgetFromName(TEXT("IMG_Icon")))->GetBrush().GetResourceObject(),static_cast<UObject*>(NPCRow->Portrait.LoadSynchronous()));
            TestEqual(TEXT("Quick slot shows the actual stack total"),CastChecked<UTextBlock>(Cell->GetWidgetFromName(TEXT("TXT_Count")))->GetText().ToString(),FString(TEXT("7")));
            TestTrue(TEXT("Open actual item popup with configured DT presentation"),UI->OpenItem(UI->SelectedItem));auto* Popup=CastChecked<UICInventoryItemPopupWidget>(UI->GetWidgetFromName(TEXT("ItemPopup")));
            TestEqual(TEXT("Item popup reads its DT icon"),CastChecked<UImage>(Popup->GetWidgetFromName(TEXT("IMG_Item")))->GetBrush().GetResourceObject(),static_cast<UObject*>(NPCRow->Portrait.LoadSynchronous()));
            TestEqual(TEXT("Item popup resolves its description from the DT text key"),CastChecked<UTextBlock>(Popup->GetWidgetFromName(TEXT("TXT_Description")))->GetText().ToString(),Potion->DisplayName.ToString());UI->CloseItem();
        }
        SelectBag(TEXT("ITM_FOOD_001"));UI->BeginQuickSlot();UI->ChooseQuickSlot(2);
        TestTrue(TEXT("Replacement prompt names both existing and new item"),CastChecked<UTextBlock>(Confirm->GetWidgetFromName(TEXT("TXT_Message")))->GetText().ToString().Contains(TEXT("제외하고")));
        TestFalse(TEXT("Cannot switch top tabs under confirmation"),Menu->SelectTab(TEXT("Party")));
        TestTrue(TEXT("ESC cancels instead of closing the menu"),Menu->HandleMenuKey(EKeys::Escape));
        TestFalse(TEXT("Cancellation closes modal"),UI->IsBusy());TestEqual(TEXT("Cancel preserves old binding"),Inv->GetSnapshot().QuickSlots[2],FName(TEXT("ITM_CONS_001")));
        UI->BeginQuickSlot();Menu->HandleMenuKey(EKeys::D);Menu->HandleMenuKey(EKeys::D);Menu->HandleMenuKey(EKeys::Enter);
        Menu->HandleMenuKey(EKeys::A);Menu->HandleMenuKey(EKeys::Enter);
        TestEqual(TEXT("Full keyboard flow confirms selected slot"),Inv->GetSnapshot().QuickSlots[2],FName(TEXT("ITM_FOOD_001")));
        UI->BeginQuickSlot();UI->ChooseQuickSlot(1);Inv->Consume(TEXT("ITM_FOOD_001"),1);UI->ResolveConfirmation(true);
        TestEqual(TEXT("Stale item cannot replace another binding"),Inv->GetSnapshot().QuickSlots[1],FName(TEXT("ITM_CONS_001")));
        SelectBag(TEXT("ITM_CONS_001"));UI->BeginTrash();UI->ResolveConfirmation(false);TestEqual(TEXT("Trash cancel preserves full stack"),Inv->CountItem(TEXT("ITM_CONS_001")),7);
        auto Locked=Inv->GetSnapshot();Locked.Bag.Slots[BagIndex(TEXT("ITM_CONS_001"))].bLocked=true;Inv->ApplySnapshot(Locked);
        TestFalse(TEXT("Locked stack cannot open trash confirmation"),UI->BeginTrash());TestFalse(TEXT("Component also protects locked stacks"),Inv->TrashItem(BagIndex(TEXT("ITM_CONS_001"))));
        Locked.Bag.Slots[BagIndex(TEXT("ITM_CONS_001"))].bLocked=false;Inv->ApplySnapshot(Locked);UI->RefreshInventory();
        TestTrue(TEXT("Trash requires a confirmation"),UI->BeginTrash());Menu->HandleMenuKey(EKeys::A);Menu->HandleMenuKey(EKeys::Enter);
        TestEqual(TEXT("Confirmed trash removes selected full stack"),Inv->CountItem(TEXT("ITM_CONS_001")),0);
        TestTrue(TEXT("Last-stack trash clears exhausted bindings"),Inv->GetSnapshot().QuickSlots[0].IsNone()&&Inv->GetSnapshot().QuickSlots[1].IsNone()&&Inv->GetSnapshot().QuickSlots[3].IsNone());
        TestEqual(TEXT("Trash does not touch other items"),Inv->CountItem(TEXT("ITM_FOOD_001")),2);
        Inv->AddItem(TEXT("ITM_QST_001"),2,false);UI->SelectItemTab(EPlayerItemTab::Quest);UI->SelectItem(0);
        TestTrue(TEXT("Quest tab displays its actual quest stack"),Label(TEXT("TXT_SelectedItem")).Contains(TEXT("2")));
        TestFalse(TEXT("Quest tab cannot bind quest cells"),UI->BeginQuickSlot());TestFalse(TEXT("Quest tab cannot trash quest cells"),UI->BeginTrash());
        TestTrue(TEXT("Quest item detail opens"),UI->OpenItem(0));auto* Detail=CastChecked<UICInventoryItemPopupWidget>(UI->GetWidgetFromName(TEXT("ItemPopup")));
        TestFalse(TEXT("Quest item popup protects trash action"),Detail->GetWidgetFromName(TEXT("BTN_Trash"))->GetIsEnabled());Menu->HandleMenuKey(EKeys::Escape);TestFalse(TEXT("ESC returns to item list"),UI->bDetailOpen);
        UI->SelectItemTab(EPlayerItemTab::Normal);Inv->AddItem(TEXT("ITM_R01_CROP_01"),2,false);UI->SelectItem(BagIndex(TEXT("ITM_R01_CROP_01")));UI->OpenItem(UI->SelectedItem);
        TestFalse(TEXT("Material popup has no equip action"),Detail->GetWidgetFromName(TEXT("BTN_Equip"))->IsVisible());TestFalse(TEXT("Material popup has no quick-slot action"),Detail->GetWidgetFromName(TEXT("BTN_Quick"))->IsVisible());
        TestTrue(TEXT("Material popup retains trash and back"),Detail->GetWidgetFromName(TEXT("BTN_Trash"))->IsVisible()&&Detail->GetWidgetFromName(TEXT("BTN_Back"))->IsVisible());UI->CloseItem();
        Inv->AddItem(Weapons[0],1,false);SelectBag(Weapons[0]);UI->OpenItem(UI->SelectedItem);TestTrue(TEXT("Weapon popup offers equip"),Detail->GetWidgetFromName(TEXT("BTN_Equip"))->IsVisible());TestFalse(TEXT("Weapon popup hides quick slots"),Detail->GetWidgetFromName(TEXT("BTN_Quick"))->IsVisible());
        Menu->HandleMenuKey(EKeys::Enter);TestFalse(TEXT("Equipping from popup returns to inventory"),UI->bDetailOpen);
        const auto Saved=State->ExportState();UI->SelectItemTab(EPlayerItemTab::Normal);UI->SelectMember(1);State->SetPartySlot(NPC,INDEX_NONE);UI->RefreshInventory();
        TestEqual(TEXT("Dismissed NPC disappears from inventory roster"),UI->Members.Num(),1);
        TestTrue(TEXT("Inventory and party survive restore"),State->RestoreState(Saved));UI->RefreshInventory();TestEqual(TEXT("Restored companion appears"),UI->Members.Num(),2);
        TestEqual(TEXT("Quick binding survives restore"),Inv->GetSnapshot().QuickSlots[2],FName(TEXT("ITM_FOOD_001")));
        auto Legacy=State->ExportState();Legacy.Inventory.Equipment.SetNum(4);
        auto OldItem=[](FName ID){FPlayerItemInstance Item;Item.ItemID=ID;Item.Count=1;Item.InstanceID=FGuid::NewGuid();return Item;};
        Legacy.Inventory.Equipment[1]=OldItem(TEXT("EQP_ARM_001"));Legacy.Inventory.Storage[TEXT("PlayerHome")].Slots[0]=OldItem(TEXT("EQP_NCK_001"));
        const int32 Empty=Legacy.Inventory.Bag.Slots.IndexOfByPredicate([](const auto& Item){return Item.IsEmpty();});Legacy.Inventory.Bag.Slots[Empty]=OldItem(TEXT("EQP_RNG_001"));
        Legacy.Characters[NPC].Equipment.SetNum(4);Legacy.Characters[NPC].Equipment[2]=OldItem(TEXT("EQP_RNG_001"));
        TestTrue(TEXT("Old saves with retired slots migrate safely"),State->RestoreState(Legacy));TestEqual(TEXT("Migration retains a single held weapon slot"),Inv->GetSnapshot().Equipment.Num(),1);
        TestEqual(TEXT("Migration removes retired gear from bag"),Inv->CountItem(TEXT("EQP_RNG_001")),0);TestTrue(TEXT("Migration removes retired gear from storage"),Inv->GetSnapshot().Storage[TEXT("PlayerHome")].Slots[0].IsEmpty());TestEqual(TEXT("Migration removes companion accessory slots"),State->GetSnapshot(NPC).Equipment.Num(),1);
        Menu->ReleaseSlateResources(true);
    }
    Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
