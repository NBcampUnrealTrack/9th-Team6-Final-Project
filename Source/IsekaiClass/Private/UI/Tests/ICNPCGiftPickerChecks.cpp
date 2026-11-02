#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Components/GridPanel.h"
#include "Components/TextBlock.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "NPC/Character/ICNPCBase.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "UI/Dialogue/ICNPCGiftWidget.h"
#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCGiftPickerCheck,"IC.UI.NPCGiftPicker",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCGiftPickerCheck::RunTest(const FString&)
{
    auto* Class=LoadClass<UICNPCGiftWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCGiftPicker.WBP_IC_NPCGiftPicker_C"));
    if(!TestNotNull(TEXT("Gift picker WBP exists and compiles"),Class))return false;
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,0);
    for(const auto ID:{TEXT("ITM_R01_FOOD_15"),TEXT("ITM_R01_PLANT_01"),TEXT("ITM_R01_ORE_01"),TEXT("ITM_R01_FISH_ROD"),TEXT("ITM_QST_001")})
        TestTrue(TEXT("Seed actual inventory definitions"),Staging->AddItem(ID,2,false));
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("선물선택 검증");Initial.FirstSkill=TEXT("SKL_NPC_R01_002");Initial.Inventory=Staging->GetSnapshot();
    for(auto& Item:Initial.Inventory.Bag.Slots)if(Item.ItemID==TEXT("ITM_R01_ORE_01"))Item.bLocked=true;
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Inventory state restores"),State->RestoreState(Initial));
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Pawn=World->SpawnActor<AICPlayerCharacter>();Pawn->EnableCombat(true,true);PC->Possess(Pawn);PC->bCreated=true;PC->Menu=TEXT("Dialogue");
    Pawn->Inventory->BindWorldState(State);TestTrue(TEXT("Live player initializes"),Pawn->InitializeState(Player.CharacterId));
    auto* NPC=World->SpawnActor<AICNPCBase>();TestTrue(TEXT("Actual NPC initializes"),NPC->InitializeNPC(TEXT("NPC_R01_002")));
    NPC->SetActorLocation(Pawn->GetActorLocation()+FVector(100,0,0));State->SetAffinity(TEXT("NPC_R01_002"),0);
    TestTrue(TEXT("NPC conversation begins"),NPC->TryInteract(PC));
    PC->SetMenu(TEXT("Dialogue"));
    TestTrue(TEXT("NPC dialogue keeps the mouse visible"),PC->bShowMouseCursor);
    TestEqual(TEXT("NPC dialogue does not capture the mouse"),Viewport->GetMouseCaptureMode(),EMouseCaptureMode::NoCapture);
    TArray<TSharedPtr<SWidget>> HeldSlate;
    auto Open=[&](){auto* Picker=CreateWidget<UICNPCGiftWidget>(PC,Class);HeldSlate.Add(Picker->TakeWidget());Picker->OpenForNPC(NPC);return Picker;};
    auto Key=[](UICNPCGiftWidget* Picker,FKey Pressed,bool Repeat=false)
    {return Picker->TakeWidget()->OnPreviewKeyDown(FGeometry(),FKeyEvent(Pressed,FModifierKeysState(),0,Repeat,0,0));};
    auto Send=[&](UICNPCGiftWidget* Widget){Key(Widget,EKeys::Enter);Key(Widget,EKeys::W);Key(Widget,EKeys::Enter);};
    auto* Picker=Open();auto* List=CastChecked<UGridPanel>(Picker->GetWidgetFromName(TEXT("GiftGrid")));
    TestEqual(TEXT("Gift inventory has a five-column, twenty-slot grid"),List->GetChildrenCount(),20);
    TestEqual(TEXT("First item selected automatically"),Picker->GetSelectedItemID(),FName(TEXT("ITM_R01_FOOD_15")));
    auto* First=CastChecked<UICInventorySlotWidget>(List->GetChildAt(0));
    TestEqual(TEXT("Inventory cells show the actual owned quantity"),CastChecked<UTextBlock>(First->GetWidgetFromName(TEXT("TXT_Count")))->GetText().ToString(),FString(TEXT("2")));
    Key(Picker,EKeys::S);TestTrue(TEXT("S moves down one grid row"),Picker->GetSelectedItemID().IsNone());
    Key(Picker,EKeys::W);TestEqual(TEXT("W moves up one grid row"),Picker->GetSelectedItemID(),FName(TEXT("ITM_R01_FOOD_15")));
    Key(Picker,EKeys::D);TestEqual(TEXT("D selects the item to the right"),Picker->GetSelectedItemID(),FName(TEXT("ITM_R01_PLANT_01")));
    Key(Picker,EKeys::A);TestEqual(TEXT("A selects the item to the left"),Picker->GetSelectedItemID(),FName(TEXT("ITM_R01_FOOD_15")));
    Key(Picker,EKeys::D);Key(Picker,EKeys::Enter,true);
    TestEqual(TEXT("Holding Enter does not send a gift"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),2);
    Key(Picker,EKeys::Enter);
    TestTrue(TEXT("Selecting an item opens confirmation"),Picker->bConfirming);
    TestEqual(TEXT("Opening confirmation consumes nothing"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),2);
    auto* Confirm=CastChecked<UICItemConfirmWidget>(Picker->GetWidgetFromName(TEXT("GiftConfirm")));
    const auto Message=CastChecked<UTextBlock>(Confirm->GetWidgetFromName(TEXT("TXT_Message")))->GetText().ToString();
    const auto NPCName=UICTextLibrary::ResolveText(LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")),NPC->GetNPCDefinition().NameKey);
    TestTrue(TEXT("Confirmation names the selected item and actual NPC"),NPCName.bSuccess&&Message.Contains(TEXT("들꽃 줄기"))&&Message.Contains(NPCName.Text.ToString())&&Message.Contains(TEXT("선물하시겠습니까?")));
    Key(Picker,EKeys::Enter);TestFalse(TEXT("Default Cancel returns to the inventory"),Picker->bConfirming);
    TestEqual(TEXT("Cancel consumes nothing"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),2);
    Key(Picker,EKeys::Enter);Key(Picker,EKeys::Escape);TestFalse(TEXT("ESC dismisses only confirmation"),Picker->bConfirming);
    Send(Picker);
    TestEqual(TEXT("Enter gives exactly one selected item"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),1);
    TestEqual(TEXT("Choosing another item never sends the preferred item"),Pawn->Inventory->CountItem(TEXT("ITM_R01_FOOD_15")),2);
    TestEqual(TEXT("Neutral item follows the NPC DT affinity value"),State->GetSnapshot(TEXT("NPC_R01_002")).Affinity,0);
    Key(Picker,EKeys::Enter);TestEqual(TEXT("Closed picker cannot send twice"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),1);
    Picker=Open();Key(Picker,EKeys::Escape);
    TestEqual(TEXT("ESC consumes nothing"),Pawn->Inventory->CountItem(TEXT("ITM_R01_FOOD_15")),2);
    Picker=Open();Send(Picker);
    TestEqual(TEXT("Preferred gift consumes one selected item"),Pawn->Inventory->CountItem(TEXT("ITM_R01_FOOD_15")),1);
    TestEqual(TEXT("Preferred gift applies actual DT affinity"),State->GetSnapshot(TEXT("NPC_R01_002")).Affinity,20);
    Picker=Open();Key(Picker,EKeys::Enter);TestTrue(TEXT("Pending gift opens confirmation"),Picker->bConfirming);
    const int32 PreferredIndex=Pawn->Inventory->GetSnapshot().Bag.Slots.IndexOfByPredicate([](const auto& Item){return Item.ItemID==TEXT("ITM_R01_FOOD_15");});
    TestTrue(TEXT("The selected item can become locked externally"),Pawn->Inventory->ToggleLock(PreferredIndex));
    TestFalse(TEXT("Locking the pending item invalidates its confirmation"),Picker->bConfirming);
    TestEqual(TEXT("Inventory changes remove a locked selection"),Picker->GetSelectedItemID(),FName(TEXT("ITM_R01_PLANT_01")));
    Send(Picker);TestEqual(TEXT("The last selected item is consumed"),Pawn->Inventory->CountItem(TEXT("ITM_R01_PLANT_01")),0);
    Picker=Open();List=CastChecked<UGridPanel>(Picker->GetWidgetFromName(TEXT("GiftGrid")));
    TestEqual(TEXT("Empty inventory retains its slot layout"),List->GetChildrenCount(),20);
    TestTrue(TEXT("Empty gift list explains the state"),CastChecked<UTextBlock>(Picker->GetWidgetFromName(TEXT("GiftStatus")))->GetText().ToString().Contains(TEXT("없습니다")));
    Key(Picker,EKeys::Enter);
    TestFalse(TEXT("Locked gifts are rechecked at transaction time"),NPC->TryGiveGift(PC,TEXT("ITM_R01_FOOD_15")));
    TestFalse(TEXT("No implicit gift is chosen without an item ID"),NPC->TryGiveGift(PC,NAME_None));
    TestEqual(TEXT("Failed gifts preserve affinity"),State->GetSnapshot(TEXT("NPC_R01_002")).Affinity,20);
    Pawn->Inventory->ToggleLock(PreferredIndex);State->SetAffinity(TEXT("NPC_R01_002"),300);
    TestTrue(TEXT("NPC joins party for companion gifting"),State->SetPartySlot(TEXT("NPC_R01_002"),0));
    Picker=Open();Send(Picker);
    TestEqual(TEXT("Companion can receive the selected gift"),Pawn->Inventory->CountItem(TEXT("ITM_R01_FOOD_15")),0);
    TestEqual(TEXT("Companion gifting applies DT affinity"),State->GetSnapshot(TEXT("NPC_R01_002")).Affinity,320);
    Picker=Open();State->RestoreState(Initial);Key(Picker,EKeys::Enter);
    TestEqual(TEXT("Loading closes the picker before another gift can be sent"),Pawn->Inventory->CountItem(TEXT("ITM_R01_FOOD_15")),2);
    Pawn->ReleaseForLoad();NPC->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
