#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICCompanionGroupWidget.h"
#include "UI/Gameplay/ICQuickSlotWidget.h"
#include "UI/Gameplay/ICQuickSlotsWidget.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Text/ICTextLibrary.h"
#include "UI/Framework/ICText.h"
#include "Save/ICSaveSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Internationalization/StringTable.h"
#include "UI/Quest/ICQuestTrackerWidget.h"
#include "UI/Map/ICMapWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Input/Events.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICHUDPrototypeCheck, "IC.UI.HUDPrototype",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICHUDPrototypeCheck::RunTest(const FString& Parameters)
{
    auto* Class = LoadClass<UICGameplayWidget>(nullptr, TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
    if (!TestNotNull(TEXT("HUD blueprint compiled"), Class)) return false;
    auto* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    auto* World = GI->GetWorld();
    auto* Viewport = NewObject<UGameViewportClient>(GEngine);
    auto& Context = *GEngine->GetWorldContextFromWorld(World);
    Context.GameViewport = Viewport;
    Viewport->Init(Context, GI, false);
    World->InitializeActorsForPlay(FURL(), false);
    auto* PC = World->SpawnActor<AICPlayerController>();
    FString Error;
    auto* Player = GI->CreateLocalPlayer(0, Error, false);
    PC->SetPlayer(Player);
    PC->Menu = TEXT("Pause");
    auto* HUD = CreateWidget<UICGameplayWidget>(PC, Class);
    auto* Menu = HUD ? Cast<UICGameMenuWidget>(HUD->GetWidgetFromName(TEXT("GameMenu"))) : nullptr;
    if (TestNotNull(TEXT("HUD contains a separate menu WBP"), Menu))
    {
        auto* Pages = Cast<UWidgetSwitcher>(Menu->GetWidgetFromName(TEXT("MenuPages")));
        const FName ExpectedTabs[] = {TEXT("Party"), TEXT("Inventory"), TEXT("Quest"), TEXT("Codex"), TEXT("Settings")};
        TestEqual(TEXT("Menu has exactly the requested five tabs"), UICGameMenuWidget::Tabs().Num(), 5);
        TestEqual(TEXT("Each tab has a page"), Pages ? Pages->GetNumWidgets() : 0, UICGameMenuWidget::Tabs().Num());
        for (int32 Index = 0; Pages && Index < UICGameMenuWidget::Tabs().Num(); ++Index)
        {
            const FName Tab = UICGameMenuWidget::Tabs()[Index];
            if (Index < UE_ARRAY_COUNT(ExpectedTabs)) TestEqual(TEXT("Tabs follow the requested order"), Tab, ExpectedTabs[Index]);
            auto* Button = Cast<UButton>(Menu->GetWidgetFromName(FName(*(TEXT("TAB_") + Tab.ToString()))));
            if (!TestNotNull(TEXT("Tab button exists"), Button)) continue;
            Button->OnClicked.Broadcast();
            TestEqual(TEXT("Click changes selected tab"), Menu->GetSelectedTab(), Tab);
            TestEqual(TEXT("Click changes visible content"), Pages->GetActiveWidgetIndex(), Index);
            auto* Page = Cast<UPanelWidget>(Pages->GetActiveWidget());
            if (Tab == TEXT("Codex")) TestEqual(TEXT("Codex embeds its own WBP"), Page ? Page->GetChildrenCount() : -1, 1);
            TestEqual(TEXT("Tab clicks do not change game menus"), PC->Menu, FString(TEXT("Pause")));
        }
        TestFalse(TEXT("Unknown tab is rejected"), Menu->SelectTab(TEXT("Missing")));
        for (const auto* Removed : {TEXT("Equipment"), TEXT("Stats"), TEXT("Skills"), TEXT("Map")})
            TestNull(TEXT("Removed tabs are absent from the WBP"), Menu->GetWidgetFromName(FName(*FString::Printf(TEXT("TAB_%s"), Removed))));
        auto* SettingsPage = Menu->GetWidgetFromName(TEXT("Page_Settings"));
        for (const auto* Action : {TEXT("Resume"), TEXT("Save"), TEXT("Load"), TEXT("Quit")})
        {
            auto* Button = Menu->GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_%s"), Action)));
            bool bInsideSettings = false;
            for (auto* Parent = Button ? Button->GetParent() : nullptr; Parent; Parent = Parent->GetParent())
                if (Parent == SettingsPage) { bInsideSettings = true; break; }
            TestTrue(TEXT("Session buttons belong only to the Settings page"), bInsideSettings);
        }
        Menu->SetSessionPresentation(TEXT("Pause"), FText::GetEmpty());
        Menu->SelectTab(TEXT("Inventory"));
        HUD->RefreshCurrentScreen();
        TestEqual(TEXT("HUD refresh preserves local tab selection"), Menu->GetSelectedTab(), FName(TEXT("Inventory")));

        PC->Screen = HUD;
        PC->bCreated = true;
        const auto HUDSlate = HUD->TakeWidget();
        const auto MenuSlate = Menu->TakeWidget();
        auto Press = [](const TSharedRef<SWidget>& Widget, FKey Key, bool bRepeat = false)
        {
            return Widget->OnKeyDown(FGeometry(), FKeyEvent(Key, FModifierKeysState(), 0, bRepeat, 0, 0)).IsEventHandled();
        };
        PC->SetMenu(TEXT(""));
        TestTrue(TEXT("Held ESC is consumed by the HUD"), Press(HUDSlate, EKeys::Escape, true));
        TestTrue(TEXT("Held ESC does not open the menu"), PC->Menu.IsEmpty());
        for (const FName Tab : ExpectedTabs)
        {
            TestTrue(TEXT("ESC opens the menu"), Press(HUDSlate, EKeys::Escape));
            TestEqual(TEXT("ESC uses the gameplay menu"), PC->Menu, FString(TEXT("Pause")));
            TestEqual(TEXT("Menu is visible after opening"), Menu->GetVisibility(), ESlateVisibility::Visible);
            Menu->SelectTab(Tab);
            TestTrue(TEXT("Held ESC is consumed by the menu"), Press(MenuSlate, EKeys::Escape, true));
            TestEqual(TEXT("Held ESC does not close the menu"), PC->Menu, FString(TEXT("Pause")));
            TestTrue(TEXT("ESC closes from every tab"), Press(MenuSlate, EKeys::Escape));
            TestTrue(TEXT("ESC returns directly to gameplay"), PC->Menu.IsEmpty());
            TestEqual(TEXT("Menu is hidden after closing"), Menu->GetVisibility(), ESlateVisibility::Collapsed);
        }
        TestTrue(TEXT("Equals test shortcut still opens the menu"), Press(HUDSlate, EKeys::Equals));
        TestTrue(TEXT("Equals test shortcut also closes the menu"), Press(HUDSlate, EKeys::Equals));
        TestTrue(TEXT("Test shortcut returns to gameplay"), PC->Menu.IsEmpty());
        PC->SetMenu(TEXT("Pause"));
        Menu->SelectTab(TEXT("Settings"));
        CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Save")))->OnClicked.Broadcast();
        TestEqual(TEXT("Settings Save opens the existing save screen"), PC->Menu, FString(TEXT("Save")));
        PC->CloseSessionMenu();
        TestEqual(TEXT("Closing Save returns to Settings"), Menu->GetSelectedTab(), FName(TEXT("Settings")));
        CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Load")))->OnClicked.Broadcast();
        TestEqual(TEXT("Settings Load opens the existing load screen"), PC->Menu, FString(TEXT("Load")));
        PC->CloseSessionMenu();
        // Hold an existing write open so the actual Quit button can be exercised without ending the test process.
        auto* Saves = GI->GetSubsystem<UICSaveSubsystem>();
        Saves->bSaving = true;
        CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Quit")))->OnClicked.Broadcast();
        TestEqual(TEXT("Settings Quit does not open another choice screen"), PC->Menu, FString(TEXT("Pause")));
        TestEqual(TEXT("Quit requests exit after the existing write"), PC->NoticeDisplayText.ToString(), ICText::Get(TEXT("Native_fc02a78eddf6")).ToString());
        TestFalse(TEXT("Direct quit does not request an additional save"), PC->IsQuitAfterSave());
        TestEqual(TEXT("Quit selection message stays hidden"), Menu->GetWidgetFromName(TEXT("TXT_SessionMessage"))->GetVisibility(), ESlateVisibility::Collapsed);
        Saves->bSaving = false;
        Menu->SetSessionPresentation(TEXT("Confirm"), FText::FromString(TEXT("확인")));
        TestEqual(TEXT("Confirmation selects its Settings actions"), Pages->GetActiveWidget(), SettingsPage);
        TestTrue(TEXT("Confirmation actions remain accessible"), Pages->IsVisible());
        TestEqual(TEXT("Save is hidden during confirmation"), Menu->GetWidgetFromName(TEXT("BTN_Save"))->GetVisibility(), ESlateVisibility::Collapsed);
        Menu->SetSessionPresentation(TEXT("Pause"), FText::GetEmpty());
        CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Resume")))->OnClicked.Broadcast();
        TestTrue(TEXT("Settings Resume returns to gameplay"), PC->Menu.IsEmpty());
        PC->Screen = nullptr;
        PC->bCreated = false;
        PC->Menu = TEXT("Pause");
    }
    auto* Status = HUD ? Cast<UICCharacterStatusWidget>(HUD->GetWidgetFromName(TEXT("HUD_PlayerStatus"))) : nullptr;
    if (TestNotNull(TEXT("Player status uses a separate WBP"), Status))
    {
        TestNotNull(TEXT("Player HP"), Status->GetWidgetFromName(TEXT("BAR_HP")));
        TestNotNull(TEXT("Player MP"), Status->GetWidgetFromName(TEXT("BAR_MP")));
        TestNotNull(TEXT("Player SP"), Status->GetWidgetFromName(TEXT("BAR_SP")));
        TestNull(TEXT("Player has no fixed sample portrait"), Status->Portrait);
        for (const auto* Key : {TEXT("HP"), TEXT("MP"), TEXT("SP")})
        {
            TestNull(TEXT("Resource labels are absent"), Status->GetWidgetFromName(FName(*FString::Printf(TEXT("TXT_%s"), Key))));
            TestNull(TEXT("Resource numbers are absent"), Status->GetWidgetFromName(FName(*FString::Printf(TEXT("VALUE_%s"), Key))));
        }
        auto* Slot = Cast<UCanvasPanelSlot>(Status->Slot);
        TestTrue(TEXT("Player card is anchored at bottom left"), Slot && Slot->GetAnchors().Minimum == FVector2D(0, 1));

        auto* State = GI->GetSubsystem<UICWorldStateSubsystem>();
        auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
        FICWorldState Data;
        Data.bPlayerCreated = true;
        Data.PlayerName = TEXT("실제이름");
        Data.FirstSkill = TEXT("SKL_START_SWORD_001");
        FICCharacterState Character;
        Character.CharacterId = UICWorldStateSubsystem::PlayerId();
        Character.Kind = EICCharacterKind::Player;
        Character.HP = 40; Character.MaxHP = 100;
        Character.MP = 30; Character.MaxMP = 60;
        Character.SP = 25; Character.MaxSP = 100;
        Data.Characters.Add(Character.CharacterId, Character);
        TestTrue(TEXT("Create actual player state"), State->RestoreState(Data));
        TestEqual(TEXT("HUD resolves the owning controller"), HUD->GetICPlayerController(), PC);
        TestEqual(TEXT("Controller and HUD test use the same world state"), PC->GetWorldState(), State);
        auto CheckPlayer = [&](FName Preset, const FString& Name, float HP, float MP, float SP)
        {
            HUD->RefreshCurrentScreen();
            FICDefinitionResult Result;
            const auto* Definition = Definitions->Find<FICPlayerPresetRow>(Preset, Result);
            if (!TestNotNull(TEXT("Selected player preset exists"), Definition)) return;
            auto* Portrait = Definition->Portrait.LoadSynchronous();
            TestTrue(TEXT("Player portrait uses requested folder"), Portrait && Portrait->GetPathName().StartsWith(TEXT("/Game/IsekaiClass/Player/Portrait/")));
            TestEqual(TEXT("Selected preset chooses the portrait"), Status->Portrait.Get(), Portrait);
            TestEqual(TEXT("Image widget uses selected portrait"), CastChecked<UImage>(Status->GetWidgetFromName(TEXT("IMG_Portrait")))->GetBrush().GetResourceObject(), static_cast<UObject*>(Portrait));
            TestEqual(TEXT("Created player name appears in HUD"), CastChecked<UTextBlock>(Status->GetWidgetFromName(TEXT("TXT_Name")))->GetText().ToString(), Name);
            TestEqual(TEXT("HP bar reflects actual HP and maximum"), CastChecked<UProgressBar>(Status->GetWidgetFromName(TEXT("BAR_HP")))->GetPercent(), HP);
            TestEqual(TEXT("MP bar reflects actual MP and maximum"), CastChecked<UProgressBar>(Status->GetWidgetFromName(TEXT("BAR_MP")))->GetPercent(), MP);
            TestEqual(TEXT("SP bar reflects actual SP and maximum"), CastChecked<UProgressBar>(Status->GetWidgetFromName(TEXT("BAR_SP")))->GetPercent(), SP);
        };
        CheckPlayer(TEXT("PRESET_PLAYER_M"), Data.PlayerName, .4f, .5f, .25f);
        TestTrue(TEXT("Actual player resources change"), State->SetVitals(Character.CharacterId, 20, 15, 50));
        CheckPlayer(TEXT("PRESET_PLAYER_M"), Data.PlayerName, .2f, .25f, .5f);
        Character = State->GetSnapshot(Character.CharacterId);
        Character.Equipment.Reset(); // Player equipment belongs to the inventory state, not the stored character row.
        Character.MaxHP = 200; Character.MP = 0; Character.MaxMP = 0; Character.MaxSP = 200;
        TestTrue(TEXT("Player maximum resources change"), State->SetCharacterState(Character));
        CheckPlayer(TEXT("PRESET_PLAYER_M"), Data.PlayerName, .1f, 0.f, .25f);
        Data.Preset = 1;
        Data.PlayerName = TEXT("다른이름");
        TestTrue(TEXT("Load a female character with another name"), State->RestoreState(Data));
        CheckPlayer(TEXT("PRESET_PLAYER_F"), Data.PlayerName, .4f, .5f, .25f);
    }
    TestNull(TEXT("HP is no longer owned directly by HUD"), HUD ? HUD->GetWidgetFromName(TEXT("BAR_HP")) : nullptr);
    auto* Group = HUD ? Cast<UUserWidget>(HUD->GetWidgetFromName(TEXT("HUD_Companions"))) : nullptr;
    for (int32 Index = 1; Group && Index <= 3; ++Index)
    {
        auto* Member = Cast<UICCharacterStatusWidget>(Group->GetWidgetFromName(FName(*FString::Printf(TEXT("Companion%d"), Index))));
        if (TestNotNull(TEXT("Companion slot is a reusable status WBP"), Member))
        {
            TestNotNull(TEXT("Companion HP"), Member->GetWidgetFromName(TEXT("BAR_HP")));
            TestNotNull(TEXT("Companion MP"), Member->GetWidgetFromName(TEXT("BAR_MP")));
            TestNull(TEXT("Companion has no SP bar"), Member->GetWidgetFromName(TEXT("BAR_SP")));
            TestNull(TEXT("Companion has no HP text"), Member->GetWidgetFromName(TEXT("TXT_HP")));
            TestNull(TEXT("Companion has no MP text"), Member->GetWidgetFromName(TEXT("TXT_MP")));
            TestNull(TEXT("Empty companion has no sample portrait"), Member->Portrait);
            TestEqual(TEXT("Empty companion is hidden"), Member->GetVisibility(), ESlateVisibility::Collapsed);
        }
    }
    TestNotNull(TEXT("Companion group exists"), Group);
    auto* Companions = Cast<UICCompanionGroupWidget>(Group);
    if (TestNotNull(TEXT("Companion group has party binding code"), Companions))
    {
        const auto CompanionSlate = Companions->TakeWidget(); // Keep the constructed widget and its state subscriptions alive.
        auto* State = GI->GetSubsystem<UICWorldStateSubsystem>();
        auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
        auto* Names = LoadObject<UStringTable>(nullptr, TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
        auto Card = [Companions](int32 Index) { return CastChecked<UICCharacterStatusWidget>(Companions->GetWidgetFromName(FName(*FString::Printf(TEXT("Companion%d"), Index + 1)))); };
        const FName IDs[] = {TEXT("NPC_R01_002"), TEXT("NPC_R01_008"), TEXT("NPC_R01_011")};
        for (FName ID : IDs)
        {
            FICCharacterState NPC;
            NPC.CharacterId = ID;
            NPC.Affinity = 300;
            TestTrue(TEXT("Create an unrecruited NPC"), State->SetCharacterState(NPC));
        }
        TestEqual(TEXT("High affinity alone does not show a companion"), Card(0)->GetVisibility(), ESlateVisibility::Collapsed);
        TestTrue(TEXT("Recruit NPC 8 into third slot"), State->SetPartySlot(IDs[1], 2));
        TestTrue(TEXT("Recruit NPC 2 into first slot"), State->SetPartySlot(IDs[0], 0));
        const auto SavedParty = State->ExportState();
        auto CheckCard = [&](int32 Index, FName ID)
        {
            FICDefinitionResult Result;
            const auto* NPC = Definitions->Find<FICNPCRow>(ID, Result);
            if (!TestNotNull(TEXT("NPC DT row resolves"), NPC)) return;
            TestFalse(TEXT("NPC DT exposes a portrait"), NPC->Portrait.IsNull());
            TestEqual(TEXT("Slot shows its actual companion"), Card(Index)->CharacterID, ID);
            TestEqual(TEXT("Portrait comes from that NPC DT row"), Card(Index)->Portrait.Get(), NPC->Portrait.LoadSynchronous());
            TestEqual(TEXT("Name comes from the NPC DT NameKey"), Card(Index)->DisplayName.ToString(), UICTextLibrary::ResolveText(Names, NPC->NameKey).Text.ToString());
            TestEqual(TEXT("Recruited companion is visible"), Card(Index)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        };
        CheckCard(0, IDs[0]);
        CheckCard(2, IDs[1]);
        TestEqual(TEXT("Unfilled second slot remains hidden"), Card(1)->GetVisibility(), ESlateVisibility::Collapsed);
        TestTrue(TEXT("Companion vitals update"), State->SetVitals(IDs[0], 25, 40, 70));
        TestEqual(TEXT("HUD HP follows actual companion"), Card(0)->HPPercent, .25f);
        TestEqual(TEXT("HUD MP follows actual companion"), Card(0)->MPPercent, .4f);
        TestTrue(TEXT("Companion leaves party"), State->SetPartySlot(IDs[0], INDEX_NONE));
        TestEqual(TEXT("Leaving clears the HUD slot"), Card(0)->GetVisibility(), ESlateVisibility::Collapsed);
        TestNull(TEXT("Leaving clears the old face"), Card(0)->Portrait);
        TestTrue(TEXT("A different NPC joins the vacated slot"), State->SetPartySlot(IDs[2], 0));
        CheckCard(0, IDs[2]);
        TestTrue(TEXT("Load restores original party"), State->RestoreState(SavedParty));
        CheckCard(0, IDs[0]);
        CheckCard(2, IDs[1]);
        TestEqual(TEXT("Unrecruited NPC stays out after load"), Card(1)->GetVisibility(), ESlateVisibility::Collapsed);
        Companions->ReleaseSlateResources(true);
    }
    auto* QuickSlots = HUD ? Cast<UICQuickSlotsWidget>(HUD->GetWidgetFromName(TEXT("HUD_QuickSlots"))) : nullptr;
    if (TestNotNull(TEXT("Four-slot group has inventory binding code"), QuickSlots))
    {
        const auto QuickSlotSlate = QuickSlots->TakeWidget();
        auto* State = GI->GetSubsystem<UICWorldStateSubsystem>();
        auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
        const auto OriginalState = State->ExportState();
        auto Card = [QuickSlots](int32 Index) { return CastChecked<UICQuickSlotWidget>(QuickSlots->GetWidgetFromName(FName(*FString::Printf(TEXT("QuickSlot%d"), Index + 1)))); };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            TestEqual(TEXT("Unregistered slot is visible as an empty frame"), Card(Index)->GetVisibility(), ESlateVisibility::HitTestInvisible);
            TestNull(TEXT("Empty slot has no icon"), Card(Index)->Icon);
        }

        // Exercise DT-driven icons without adding test content to the project's item DT.
        auto* OriginalCatalog = GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous();
        auto* Catalog = DuplicateObject<UICDefinitionCatalog>(OriginalCatalog, GI);
        auto* Table = NewObject<UDataTable>(Catalog);
        Table->RowStruct = FICItemRow::StaticStruct();
        Catalog->Tables.RemoveAll([](const FICDefinitionBinding& Binding) { return Binding.Type == EICDefinitionType::Item; });
        FICDefinitionBinding Binding;
        Binding.Type = EICDefinitionType::Item;
        Binding.RowStruct = FICItemRow::StaticStruct();
        Binding.Table = Table;
        Catalog->Tables.Add(Binding);
        Definitions->ConfigureCatalog(Catalog);
        const FName IDs[] = {TEXT("ITM_CONS_996"), TEXT("ITM_CONS_997"), TEXT("ITM_CONS_998"), TEXT("ITM_CONS_999")};
        UTexture2D* Icons[4];
        for (int32 Index = 0; Index < 4; ++Index)
        {
            Icons[Index] = NewObject<UTexture2D>(GI);
            FICItemRow Row;
            Row.ID = Row.ItemID = IDs[Index];
            Row.Kind = Index == 3 ? EPlayerItemKind::Expansion : EPlayerItemKind::Consumable;
            Row.Icon = Index == 3 ? nullptr : Icons[Index];
            Table->AddRow(IDs[Index], Row);
        }
        auto Snapshot = State->GetInventorySnapshot();
        auto Stack = [&](int32 Index, FName ID, int32 Count)
        {
            auto& Item = Snapshot.Bag.Slots[Index];
            Item.ItemID = ID; Item.InstanceID = FGuid::NewGuid(); Item.Count = Count;
        };
        Stack(0, IDs[0], 60); Stack(1, IDs[1], 2); Stack(2, IDs[2], 7); Stack(3, IDs[3], 1);
        Stack(4, IDs[0], 40); Stack(5, IDs[0], 3);
        Snapshot.Bag.Slots[5].bLocked = true;
        TestTrue(TEXT("Own items before registering quick slots"), State->CommitInventoryState(Snapshot));
        TestEqual(TEXT("Unregistered owned item stays visible as an empty frame"), Card(0)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        auto* Inventory = NewObject<UICInventoryComponent>(PC);
        Inventory->BindDefinitions(Definitions);
        Inventory->BindWorldState(State);
        for (int32 Index = 0; Index < 4; ++Index)
            TestTrue(TEXT("Register through inventory quick slot API"), Inventory->BindQuickSlot(Index, Index));
        auto Check = [&](int32 Index, int32 Count)
        {
            TestEqual(TEXT("Slot uses its registered ID"), Card(Index)->ItemID, IDs[Index]);
            TestEqual(TEXT("Slot count matches usable inventory quantity"), Card(Index)->Count, Count);
            TestEqual(TEXT("Slot shows its DT icon"), Card(Index)->Icon.Get(), Icons[Index]);
            TestEqual(TEXT("Image brush follows the registered item"), CastChecked<UImage>(Card(Index)->GetWidgetFromName(TEXT("IMG_ItemIcon")))->GetBrush().GetResourceObject(), static_cast<UObject*>(Icons[Index]));
            TestEqual(TEXT("Visible count follows inventory"), CastChecked<UTextBlock>(Card(Index)->GetWidgetFromName(TEXT("TXT_Count")))->GetText().ToString(), FText::AsNumber(Count).ToString());
            TestEqual(TEXT("Occupied slot is visible"), Card(Index)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        };
        Check(0, Inventory->CountItem(IDs[0])); Check(1, 2); Check(2, 7);
        TestEqual(TEXT("Missing DT icon keeps a registered slot visible as an empty frame"), Card(3)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        Table->FindRow<FICItemRow>(IDs[3], TEXT("HUDTest"))->Icon = Icons[3];
        QuickSlots->RefreshQuickSlots();
        Check(3, 1);
        const auto SavedSlots = State->ExportState();
        TestTrue(TEXT("Using an item updates HUD immediately"), Inventory->Consume(IDs[0], 1));
        Check(0, 99);
        TestTrue(TEXT("Consume remaining usable stacks"), Inventory->Consume(IDs[0], 99));
        TestEqual(TEXT("Exhausted slot keeps its frame even with a locked stack remaining"), Card(0)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestNull(TEXT("Exhaustion clears old image"), Card(0)->Icon);
        TestTrue(TEXT("Reacquire the registered item"), Inventory->AddItem(IDs[0], 3));
        Check(0, 3);
        Snapshot = State->GetInventorySnapshot();
        Snapshot.QuickSlots[0] = NAME_None;
        TestTrue(TEXT("Unregister an occupied quick slot"), State->CommitInventoryState(Snapshot));
        TestEqual(TEXT("Unregistered slot clears its item and keeps the frame"), Card(0)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestTrue(TEXT("Loading restores registered slots and quantities"), State->RestoreState(SavedSlots));
        Check(0, 100); Check(1, 2); Check(2, 7); Check(3, 1);
        Table->RemoveRow(IDs[1]);
        QuickSlots->RefreshQuickSlots();
        TestEqual(TEXT("Unresolved DT item stays visible as an empty frame"), Card(1)->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestNull(TEXT("Missing definition clears old icon"), Card(1)->Icon);
        TestTrue(TEXT("Restore the original inventory after transient tests"), State->RestoreState(OriginalState));
        Definitions->ConfigureCatalog(OriginalCatalog);
        Inventory->BindWorldState(nullptr);
        QuickSlots->ReleaseSlateResources(true);
    }
    TestNotNull(TEXT("Existing quest tracker is embedded"), HUD ? Cast<UICQuestTrackerWidget>(HUD->GetWidgetFromName(TEXT("HUD_QuestTracker"))) : nullptr);
    auto* Minimap=HUD?Cast<UICMapWidget>(HUD->GetWidgetFromName(TEXT("HUD_Minimap"))):nullptr;
    TestNotNull(TEXT("Minimap widget exists"),Minimap);
    TestTrue(TEXT("HUD map always initializes in minimap mode"),Minimap&&Minimap->bMinimap);
    TestNull(TEXT("Standalone Q skill panel is removed"), HUD ? HUD->GetWidgetFromName(TEXT("HUD_SkillCooldown")) : nullptr);
    TestNotNull(TEXT("Player portrait contains the Q cooldown overlay"), Status ? Status->GetWidgetFromName(TEXT("PortraitCooldownOverlay")) : nullptr);
    TestNotNull(TEXT("Player portrait contains remaining seconds"), Status ? Status->GetWidgetFromName(TEXT("TXT_SkillCooldown")) : nullptr);
    auto* SkillKey = Status ? Status->GetWidgetFromName(TEXT("TXT_SkillKey")) : nullptr;
    TestNotNull(TEXT("Player portrait contains the Q label"), SkillKey);
    TestEqual(TEXT("Q label is hidden without an equipped skill"), SkillKey ? SkillKey->GetVisibility() : ESlateVisibility::Visible, ESlateVisibility::Collapsed);
    GI->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
