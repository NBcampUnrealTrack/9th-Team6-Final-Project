#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Codex/ICCodexMenuWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "Codex/ICCodexSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Save/ICSaveGame.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCodexMenuCheck,"IC.UI.CodexMenu",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCodexMenuCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* C=GI->GetSubsystem<UICCodexSubsystem>();
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("도감검증");Data.FirstSkill=TEXT("SKL_START_SWORD_001");FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;P.UnlockedSkillIDs.Add(Data.FirstSkill);P.EquippedWeaponSkills.Add(EICWeaponFamily::Sword,NAME_None);Data.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Initialize world"),State->RestoreState(Data));Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);PC->bCreated=true;PC->Menu=TEXT("Pause");
    auto* Class=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));auto* HUD=Class?CreateWidget<UICGameplayWidget>(PC,Class):nullptr;auto* Menu=HUD?Cast<UICGameMenuWidget>(HUD->GetWidgetFromName(TEXT("GameMenu"))):nullptr;
    auto* UI=Menu?Cast<UICCodexMenuWidget>(Menu->GetWidgetFromName(TEXT("CodexContent"))):nullptr;
    if(TestNotNull(TEXT("Separate codex WBP embedded in menu"),UI))
    {
        const auto Slate=HUD->TakeWidget();Menu->SelectTab(TEXT("Codex"));
        auto* Detail=CastChecked<UICCodexPopupWidget>(UI->GetWidgetFromName(TEXT("DetailPopup")));auto* Notice=CastChecked<UICCodexPopupWidget>(UI->GetWidgetFromName(TEXT("NoticePopup")));auto* Confirm=CastChecked<UICCodexPopupWidget>(UI->GetWidgetFromName(TEXT("ConfirmPopup")));auto* Targets=CastChecked<UICCodexTargetsWidget>(UI->GetWidgetFromName(TEXT("TargetPopup")));
        auto Press=[&](FKey K){Menu->HandleMenuKey(K);};auto Primary=[&](){CastChecked<UButton>(Detail->GetWidgetFromName(TEXT("BTN_Primary")))->OnClicked.Broadcast();};
        Press(EKeys::Enter);TestEqual(TEXT("Keyboard opens skills category"),UI->Category,0);TestEqual(TEXT("Four families of 15 cells"),UI->Cells.Num(),60);
        const auto Skills=C->SkillCells();FICDefinitionResult R;for(int32 I=0;I<Skills.Num();++I)if(const auto* S=D->Find<FICSkillRow>(Skills[I],R))TestEqual(TEXT("Skills are in the correct three-column family block"),int32(C->SkillFamily(*S))-1,(I%12)/3);
        const int32 SwordIndex=Skills.IndexOfByKey(TEXT("SKL_START_SWORD_001"));TestTrue(TEXT("Existing player skill has a cell"),SwordIndex>=0);
        UI->OpenCell(SwordIndex);Primary();TestTrue(TEXT("Missing weapon creates nested notice"),UI->bNoticeOpen);TestTrue(TEXT("Notice uses requested weapon message"),CastChecked<UTextBlock>(Notice->GetWidgetFromName(TEXT("TXT_Description")))->GetText().ToString().Contains(TEXT("해당 무기가 장착 안되어있습니다.")));
        TestFalse(TEXT("Modal blocks changing parent tab"),Menu->SelectTab(TEXT("Party")));Press(EKeys::Enter);TestFalse(TEXT("Confirm dismisses only notice"),UI->bNoticeOpen);TestEqual(TEXT("Skill detail remains open"),UI->Popup,1);Press(EKeys::Escape);
        auto* Inv=Player->Inventory.Get();Inv->AddItem(TEXT("EQP_WPN_SWORD_001"),1,false);const int32 WeaponIndex=Inv->GetSnapshot().Bag.Slots.IndexOfByPredicate([](const auto& Item){return Item.ItemID==TEXT("EQP_WPN_SWORD_001");});TestTrue(TEXT("Equip actual sword"),Inv->Equip(WeaponIndex));
        FText SkillError;TestTrue(TEXT("Actual skill can be assigned with its weapon"),C->SetPlayerSkill(TEXT("SKL_START_SWORD_001"),SkillError));
        TestEqual(TEXT("Selected skill is saved per weapon"),C->EquippedSkills()[0],FName(TEXT("SKL_START_SWORD_001")));
        UI->OpenCell(1000);TestEqual(TEXT("Equipped skill offers removal"),CastChecked<UTextBlock>(Detail->GetWidgetFromName(TEXT("BTN_Primary_Text")))->GetText().ToString(),FString(TEXT("스킬 해제하기")));
        Primary();TestTrue(TEXT("Removal keeps a detail notice"),UI->bNoticeOpen);TestTrue(TEXT("Unequipped skill remains acquired"),C->IsSkillUnlocked(Data.FirstSkill));TestTrue(TEXT("Explicit empty assignment prevents fallback skill re-equipping"),C->EquippedSkills()[0].IsNone());Press(EKeys::Enter);UI->ClosePopup();
        const int32 BowIndex=Skills.IndexOfByKey(TEXT("SKL_START_BOW_001"));TestTrue(TEXT("Unacquired bow skill is a question mark"),CastChecked<UICCodexCellWidget>(CastChecked<UUniformGridPanel>(UI->GetWidgetFromName(TEXT("CellsGrid")))->GetChildAt(BowIndex))->bUnknown);TestFalse(TEXT("Unacquired skill opens no details"),UI->OpenCell(BowIndex));
        UI->SelectCategory(1);auto* Grid=CastChecked<UUniformGridPanel>(UI->GetWidgetFromName(TEXT("CellsGrid")));TestEqual(TEXT("Pet codex contains 55 slots"),Grid->GetChildrenCount(),55);
        TestTrue(TEXT("Locked pet is unknown"),CastChecked<UICCodexCellWidget>(Grid->GetChildAt(0))->bUnknown);TestFalse(TEXT("Locked pet opens no detail"),UI->OpenCell(0));
        TestFalse(TEXT("Invalid essence indices rejected"),C->AddPetEssence(55,100));TestFalse(TEXT("Negative essence rejected"),C->AddPetEssence(0,-1));TestTrue(TEXT("Three essence recorded"),C->AddPetEssence(0,3));UI->RefreshCodex();
        TestEqual(TEXT("Locked progress shows actual 3/100"),CastChecked<UTextBlock>(CastChecked<UICCodexCellWidget>(Grid->GetChildAt(0))->GetWidgetFromName(TEXT("TXT_Progress")))->GetText().ToString(),FString(TEXT("3/100")));
        TestTrue(TEXT("One hundred essence unlocks"),C->AddPetEssence(0,97));C->AddPetEssence(1,101);UI->RefreshCodex();TestTrue(TEXT("Unlocked pet opens detail"),UI->OpenCell(0));Primary();TestEqual(TEXT("Only genuine party members appear"),Targets->Members.Num(),1);
        Press(EKeys::Enter);TestEqual(TEXT("Confirmation starts on cancel"),Confirm->Choice,1);Press(EKeys::Enter);TestEqual(TEXT("Cancel returns to targets"),UI->Popup,2);TestEqual(TEXT("Cancel preserves no assigned pet"),C->GetEquippedPet(P.CharacterId),INDEX_NONE);
        Press(EKeys::Enter);Press(EKeys::A);Press(EKeys::Enter);TestEqual(TEXT("Confirm assigns selected pet"),C->GetEquippedPet(P.CharacterId),0);TestEqual(TEXT("Pet assignment does not consume essence"),C->GetPetEssence(0),100);Press(EKeys::Escape);
        UI->OpenCell(1);Primary();Press(EKeys::Enter);TestTrue(TEXT("Replacement names old and new pets"),CastChecked<UTextBlock>(Confirm->GetWidgetFromName(TEXT("TXT_Description")))->GetText().ToString().Contains(C->PetName(0).ToString()));Press(EKeys::A);Press(EKeys::Enter);TestEqual(TEXT("Confirmed replacement applied"),C->GetEquippedPet(P.CharacterId),1);Press(EKeys::Escape);
        {
            TArray<FName> Monsters;D->GetDefinitionIDs(EICDefinitionType::Monster,Monsters);auto* Monster=const_cast<FICMonsterRow*>(D->Find<FICMonsterRow>(Monsters[0],R));const auto* NPCPortrait=D->Find<FICNPCRow>(TEXT("NPC_R01_002"),R);
            auto* ASC=Player->GetICAbilitySystem();const double Before=ASC->Stat(EICStat::STR);
            TGuardValue<int32> PetIndex(Monster->PetCodexIndex,1);TGuardValue<TSoftObjectPtr<UTexture2D>> Portrait(Monster->Portrait,NPCPortrait->Portrait);
            FICStatModifier Bonus;Bonus.Stat=EICStat::STR;Bonus.Flat=7;TGuardValue<TArray<FICStatModifier>> Buffs(Monster->PetBuffs,{Bonus});
            ASC->RebuildDerivedStats();TestEqual(TEXT("Configured pet buff affects actual GAS stats"),ASC->Stat(EICStat::STR),Before+7);
            TestEqual(TEXT("Equipped pet portrait resolves from mapped monster DT"),C->GetEquippedPetPortrait(P.CharacterId),Monster->Portrait.LoadSynchronous());
            Menu->SelectTab(TEXT("Party"));auto* Party=CastChecked<UICPartyMenuWidget>(Menu->GetWidgetFromName(TEXT("PartyContent")));Party->RefreshParty();TestEqual(TEXT("Party panel displays the assigned monster portrait"),CastChecked<UImage>(Party->GetWidgetFromName(TEXT("IMG_Pet")))->GetBrush().GetResourceObject(),static_cast<UObject*>(Monster->Portrait.LoadSynchronous()));
            Menu->SelectTab(TEXT("Codex"));UI->SelectCategory(1);
        }
        Player->GetICAbilitySystem()->RebuildDerivedStats();
        const FName NPC=TEXT("NPC_R01_002");State->EnsureCharacter(NPC,EICCharacterKind::Recruitable);State->SetAffinity(NPC,300);State->SetPartySlot(NPC,0);TestEqual(TEXT("Actual companion joins selector"),C->GetPartyMembers().Num(),2);TestTrue(TEXT("Actual companion can equip pet"),C->EquipPet(0,NPC));State->SetPartySlot(NPC,INDEX_NONE);TestEqual(TEXT("Dismissed companion leaves selector"),C->GetPartyMembers().Num(),1);TestFalse(TEXT("Dismissed companion cannot be assigned a pet"),C->EquipPet(1,NPC));
        FICDefinitionResult Result;auto* Product=const_cast<FICItemRow*>(D->Find<FICItemRow>(TEXT("ITM_MAT_002"),Result));TGuardValue<EICCodexProductCategory> ProductType(Product->CodexProductCategory,EICCodexProductCategory::Ranching);
        UI->SelectCategory(2);UI->SelectSubTab(4);const int32 ProductIndex=UI->Cells.IndexOfByKey(Product->ID);TestTrue(TEXT("DT supports explicit ranching category"),ProductIndex>=0);TestFalse(TEXT("Unseen product cannot open"),UI->OpenCell(ProductIndex));
        Inv->AddItem(Product->ID,3,false);TestTrue(TEXT("Acquisition records permanent discovery"),C->HasAcquiredItem(Product->ID));const int32 BagIndex=Inv->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& Item){return Item.ItemID==Product->ID;});Inv->TrashItem(BagIndex);TestTrue(TEXT("Discovery survives disposing the last stack"),C->HasAcquiredItem(Product->ID));UI->RefreshCodex();TestTrue(TEXT("Discovered product opens detail"),UI->OpenCell(ProductIndex));UI->ClosePopup();
        TArray<FName> Recipes;D->GetDefinitionIDs(EICDefinitionType::Recipe,Recipes);if(TestTrue(TEXT("Existing recipe data"),!Recipes.IsEmpty()))
        {
            auto* Recipe=const_cast<FICRecipeRow*>(D->Find<FICRecipeRow>(Recipes[0],Result));TGuardValue<bool> DefaultUnlock(Recipe->bUnlockedByDefault,false);int32 Kind=0;for(;Kind<3;++Kind)if(C->Recipes(Kind).Contains(Recipe->ID))break;
            UI->SelectCategory(3);UI->SelectSubTab(Kind);int32 Index=UI->Cells.IndexOfByKey(Recipe->ID);TestFalse(TEXT("Unlearned question-mark recipe produces NO popup"),UI->OpenCell(Index));TestEqual(TEXT("Unknown recipe leaves modal closed"),UI->Popup,0);TestTrue(TEXT("Unknown recipe does not reveal ingredients"),C->RecipeDescription(Recipe->ID).IsEmpty());
            auto Next=State->ExportState();Next.UnlockedRecipeIDs.Add(Recipe->ID);TestTrue(TEXT("Unlock actual recipe"),State->CommitGameplayTransaction(Next,FGuid::NewGuid()));UI->RefreshCodex();TestTrue(TEXT("Learned recipe opens its popup"),UI->OpenCell(Index));TestTrue(TEXT("Recipe popup includes actual ingredient section"),CastChecked<UTextBlock>(Detail->GetWidgetFromName(TEXT("TXT_Description")))->GetText().ToString().Contains(TEXT("재료")));UI->ClosePopup();
        }
        auto* Save=NewObject<UICSaveGame>();Save->WorldState=State->ExportState();TArray<uint8> Bytes;TestTrue(TEXT("Codex serializes in actual save"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UICSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if(TestNotNull(TEXT("Saved codex reloads"),Loaded)){TestTrue(TEXT("Item discovery retained"),Loaded->WorldState.Codex.AcquiredItemIDs.Contains(Product->ID));TestEqual(TEXT("Essence retained"),Loaded->WorldState.Codex.PetEssence.FindRef(0),100);TestEqual(TEXT("Character pet retained"),Loaded->WorldState.Codex.CharacterPets.FindRef(P.CharacterId),1);TestTrue(TEXT("Saved codex validates on restore"),State->RestoreState(Loaded->WorldState));}
        HUD->ReleaseSlateResources(true);
    }
    Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
