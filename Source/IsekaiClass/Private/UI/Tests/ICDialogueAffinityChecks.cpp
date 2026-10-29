#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Dialogue/ICAffinityHeartsWidget.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "UI/Gameplay/ICCompanionGroupWidget.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "AI/ICAIController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"
#include "RenderingThread.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "InputMappingContext.h"
#include "InputAction.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICDialogueAffinityCheck, "IC.UI.DialogueAffinity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICDialogueAffinityCheck::RunTest(const FString& Parameters)
{
    auto* Class = LoadClass<UICNPCDialogueWidget>(nullptr, TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCDialogue.WBP_IC_NPCDialogue_C"));
    if (!TestNotNull(TEXT("Dialogue WBP compiles"), Class)) return false;
    auto* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    auto* World = GI->GetWorld();
    auto& Context = *GEngine->GetWorldContextFromWorld(World);
    auto* Viewport = NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport = Viewport;
    Viewport->Init(Context, GI, false);
    World->InitializeActorsForPlay(FURL(), false);
    auto* PC = World->SpawnActor<AICPlayerController>();
    FString Error;
    PC->SetPlayer(GI->CreateLocalPlayer(0, Error, false));
    auto* Dialogue = CreateWidget<UICNPCDialogueWidget>(PC, Class);
    auto* Hearts = Dialogue ? Cast<UICAffinityHeartsWidget>(Dialogue->GetWidgetFromName(TEXT("AffinityHearts"))) : nullptr;
    auto* State = GI->GetSubsystem<UICWorldStateSubsystem>();
    auto* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
    auto* OriginalCatalog = GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous();
    AICNPCBase* Actors[3] = {nullptr, nullptr, nullptr};
    if (TestNotNull(TEXT("Dialogue embeds the reusable hearts WBP"), Hearts))
    {
        auto* Header = Cast<UHorizontalBox>(Dialogue->GetWidgetFromName(TEXT("SpeakerHeader")));
        TestTrue(TEXT("Hearts sit immediately beside the NPC name"), Header && Header->GetChildrenCount() == 2
            && Header->GetChildAt(0) == Dialogue->GetWidgetFromName(TEXT("SpeakerText")) && Header->GetChildAt(1) == Hearts);
        auto* Outline = LoadObject<UTexture2D>(nullptr, TEXT("/Game/IsekaiClass/UI/SampleUI/MorpheusGUI/textures/Icons/512/t_HeartOutline_512.t_HeartOutline_512"));
        auto* Filled = LoadObject<UTexture2D>(nullptr, TEXT("/Game/IsekaiClass/UI/SampleUI/MorpheusGUI/textures/Icons/512/t_Heart_512.t_Heart_512"));
        TestNotNull(TEXT("Requested outline texture exists"), Outline);
        TestNotNull(TEXT("Requested filled texture exists"), Filled);
        UProgressBar* Bars[5];
        for (int32 Index = 0; Index < 5; ++Index)
        {
            Bars[Index] = CastChecked<UProgressBar>(Hearts->GetWidgetFromName(FName(*FString::Printf(TEXT("AffinityHeart%d"), Index + 1))));
            TestEqual(TEXT("Every heart has the requested outline"), Bars[Index]->GetWidgetStyle().BackgroundImage.GetResourceObject(), static_cast<UObject*>(Outline));
            TestEqual(TEXT("Every heart fills with the requested solid texture"), Bars[Index]->GetWidgetStyle().FillImage.GetResourceObject(), static_cast<UObject*>(Filled));
            TestEqual(TEXT("Partial filling clips the icon without stretching it"), Bars[Index]->GetBarFillStyle(), EProgressBarFillStyle::Mask);
        }
        auto Check = [&](int32 Value)
        {
            TestEqual(TEXT("Hearts reflect the selected NPC affinity"), Hearts->Affinity, Value);
            for (int32 Index = 0; Index < 5; ++Index)
                TestEqual(TEXT("Each heart fills for its own 100-point range"), Bars[Index]->GetPercent(), FMath::Clamp(Value / 100.f - Index, 0.f, 1.f));
        };
        Hearts->SetAffinity(-10); Check(0);
        Hearts->SetAffinity(900); Check(500);
        // Minimal transient NPC definitions avoid loading character meshes or changing the project's DT.
        FString NameKeys[2];
        FICNPCRow RecruitDefinition;
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FICDefinitionResult Result;
            const auto* Row = Definitions->Find<FICNPCRow>(Index ? TEXT("NPC_R01_008") : TEXT("NPC_R01_002"), Result);
            NameKeys[Index] = Row ? Row->NameKey : FString();
            if (!Index && Row) RecruitDefinition = *Row;
        }
        auto* Catalog = DuplicateObject<UICDefinitionCatalog>(OriginalCatalog, GI);
        auto* Table = NewObject<UDataTable>(Catalog);
        Table->RowStruct = FICNPCRow::StaticStruct();
        Catalog->Tables.RemoveAll([](const FICDefinitionBinding& Binding) { return Binding.Type == EICDefinitionType::NPC; });
        FICDefinitionBinding Binding;
        Binding.Type = EICDefinitionType::NPC; Binding.RowStruct = FICNPCRow::StaticStruct(); Binding.Table = Table;
        Catalog->Tables.Add(Binding);
        const FName IDs[] = {TEXT("NPC_R01_197"), TEXT("NPC_R01_198")};
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FICNPCRow Row;
            Row.ID = IDs[Index]; Row.NameKey = NameKeys[Index]; Row.Type = EICNPCType::Resident;
            Table->AddRow(Row.ID, Row);
        }
        RecruitDefinition.Mesh.Reset(); RecruitDefinition.AnimationClass.Reset(); RecruitDefinition.AnimationProfile.Reset();
        RecruitDefinition.InitialAnchorID = NAME_None; RecruitDefinition.ScheduleID = NAME_None; RecruitDefinition.ScheduleStateTree.Reset();
        RecruitDefinition.DialogueIDs.Reset(); RecruitDefinition.ServiceIDs.Reset();
        Table->AddRow(RecruitDefinition.ID, RecruitDefinition);
        Definitions->ConfigureCatalog(Catalog);
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* InteractionPawn=World->SpawnActor<ACharacter>(FVector(0,0,96),FRotator::ZeroRotator,Spawn);
        PC->SetPawn(InteractionPawn); // F interaction requires an actual nearby player pawn.
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FICCharacterState Resident;
            Resident.CharacterId = IDs[Index]; Resident.Kind = EICCharacterKind::Resident;
            Resident.HP = Resident.MP = Resident.SP = 0; Resident.MaxHP = Resident.MaxSP = 1; Resident.MaxMP = 0;
            TestTrue(TEXT("Seed resident state without world spawn anchors"), State->SetCharacterState(Resident));
            Actors[Index] = World->SpawnActor<AICNPCBase>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
            TestTrue(TEXT("NPC initializes from its own DT row"), Actors[Index] && Actors[Index]->InitializeNPC(IDs[Index]));
        }
        const auto Slate = Dialogue->TakeWidget();
        auto Key=[&](FKey Pressed,bool Repeat=false)
        {return Slate->OnPreviewKeyDown(FGeometry(),FKeyEvent(Pressed,FModifierKeysState(),0,Repeat,0,0));};
        auto Capture = [&](int32 Value)
        {
            if (!FParse::Param(FCommandLine::Get(), TEXT("ICCaptureDialogueAffinity"))) return;
            const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("NPCDialogueHearts"));
            IFileManager::Get().MakeDirectory(*Folder, true);
            const auto Surface = SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(.055f, .075f, .1f))[Slate];
            FWidgetRenderer Renderer(true, true);
            const FIntPoint Size(1920, 1080);
            auto* Target = Renderer.DrawWidget(Surface, FVector2D(Size));
            if (!TestNotNull(TEXT("Dialogue preview render target exists"), Target)) return;
            Renderer.DrawWidget(Target, Surface, FVector2D(Size), .1f);
            FlushRenderingCommands();
            TArray<FColor> Pixels;
            FReadSurfaceDataFlags Flags;
            Flags.SetLinearToGamma(false);
            if (!TestTrue(TEXT("Read dialogue preview pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))) return;
            for (auto& Pixel : Pixels) Pixel.A = 255;
            TArray64<uint8> PNG;
            FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
            TestTrue(TEXT("Save actual dialogue affinity preview"), FFileHelper::SaveArrayToFile(PNG,
                *(Folder / FString::Printf(TEXT("DialogueAffinity_%d.png"), Value))));
        };
        TestTrue(TEXT("Set actual NPC affinity to 300"), State->SetAffinity(IDs[0], 300));
        TestTrue(TEXT("NPC supports the F interaction path"), Actors[0]->TryInteract(PC));
        Dialogue->OpenForNPC(Actors[0]);
        Check(300);
        Capture(300);
        TestEqual(TEXT("300 fills exactly three hearts"), Bars[2]->GetPercent(), 1.f);
        TestEqual(TEXT("Fourth heart stays empty at 300"), Bars[3]->GetPercent(), 0.f);
        const auto FirstName = CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("SpeakerText")))->GetText().ToString();
        TestFalse(TEXT("NPC name remains visible"), FirstName.IsEmpty());
        TestTrue(TEXT("Changing affinity during conversation succeeds"), State->SetAffinity(IDs[0], 350));
        Check(350);
        Capture(350);
        TestEqual(TEXT("350 fills half of the fourth heart"), Bars[3]->GetPercent(), .5f);
        TestTrue(TEXT("A different NPC changes affinity"), State->SetAffinity(IDs[1], 500));
        Check(350);
        Dialogue->OpenForNPC(Actors[1]);
        Check(500);
        TestNotEqual(TEXT("Switching NPC updates the displayed name"), CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("SpeakerText")))->GetText().ToString(), FirstName);
        TestTrue(TEXT("Old NPC changes after switching"), State->SetAffinity(IDs[0], 0));
        Check(500);
        TestTrue(TEXT("Current NPC affinity can decrease"), State->SetAffinity(IDs[1], 100));
        Check(100);
        const auto Saved = State->ExportState();
        State->SetAffinity(IDs[1], 200);
        Check(200);
        TestTrue(TEXT("Loading restores actual NPC affinity"), State->RestoreState(Saved));
        Check(100);

        auto* Entries = CastChecked<UVerticalBox>(Dialogue->GetWidgetFromName(TEXT("Entries")));
        auto FinishGreeting = [&]()
        {
            for(int32 Step=0;Step<20;++Step)
            {
                auto* First=Cast<UICDialogueEntryWidget>(Entries->GetChildAt(0));
                auto* Text=First?Cast<UTextBlock>(First->GetWidgetFromName(TEXT("EntryLabel"))):nullptr;
                if(!Text||Text->GetText().ToString()==TEXT("동료 영입하기")||Text->GetText().ToString()==TEXT("동료 해제하기")||Text->GetText().ToString()==TEXT("퀘스트"))return;
                TestTrue(TEXT("Enter advances NPC dialogue without a mouse"),Key(EKeys::Enter).IsEventHandled());
            }
        };
        auto IsRecruitFirst = [&]()
        {
            auto* Entry = Cast<UICDialogueEntryWidget>(Entries->GetChildAt(0));
            auto* Label = Entry ? Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("EntryLabel"))) : nullptr;
            return Label && Label->GetText().ToString() == TEXT("동료 영입하기");
        };
        State->SetAffinity(IDs[1], 300);
        TestFalse(TEXT("A resident NPC cannot be recruited just for high affinity"), IsRecruitFirst());
        FICCharacterState Recruit;
        Recruit.CharacterId = RecruitDefinition.ID; Recruit.Affinity = 299;
        TestTrue(TEXT("Seed a recruitable NPC with actual party state"), State->SetCharacterState(Recruit));
        Actors[2] = World->SpawnActor<AICNPCBase>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        TestTrue(TEXT("Recruitable NPC initializes"), Actors[2] && Actors[2]->InitializeNPC(Recruit.CharacterId));
        TestTrue(TEXT("Begin recruitable NPC interaction"), Actors[2]->TryInteract(PC));
        PC->Menu = TEXT("Dialogue");
        Dialogue->OpenForNPC(Actors[2]);
        TestFalse(TEXT("Conversation displays dialogue before interaction choices"),CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("DialogueText")))->GetText().IsEmpty());
        FinishGreeting();
        TestFalse(TEXT("Recruit button stays hidden at 299"), IsRecruitFirst());
        TestFalse(TEXT("Low affinity is rejected by the recruitment API"), Actors[2]->TryRecruit(PC));
        TestTrue(TEXT("Affinity reaches the recruitment threshold"), State->SetAffinity(Recruit.CharacterId, 300));
        TestTrue(TEXT("Recruit appears first immediately at 300"), IsRecruitFirst());
        TestEqual(TEXT("Quest follows recruit"),CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(1))->GetEntryText().ToString(),FString(TEXT("퀘스트")));
        TestEqual(TEXT("Gift follows quest"),CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(2))->GetEntryText().ToString(),FString(TEXT("선물하기")));
        TestEqual(TEXT("End dialogue is last"),CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(3))->GetEntryText().ToString(),FString(TEXT("대화 끝내기")));
        auto EntryAt=[&](int32 Index){return CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(Index));};
        TestTrue(TEXT("New NPC choices automatically select the first entry"),EntryAt(0)->bSelected);
        const auto BeforeRepeat=CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("DialogueText")))->GetText();
        Key(EKeys::Enter,true);
        TestEqual(TEXT("Held Enter cannot repeat an interaction"),CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("DialogueText")))->GetText().ToString(),BeforeRepeat.ToString());
        const auto OriginalContext=PC->DefaultMappingContext;const auto OriginalMove=PC->MoveAction;const auto OriginalRules=PC->DialoguePresentationTable;
        auto* NavigationContext=NewObject<UInputMappingContext>(PC);auto* Move=NewObject<UInputAction>(PC);
        NavigationContext->MapKey(Move,EKeys::W);NavigationContext->MapKey(Move,EKeys::S);PC->MoveAction=Move;PC->DefaultMappingContext=NavigationContext;
        auto* LegacyRules=NewObject<UDataTable>(PC);LegacyRules->RowStruct=FICDialoguePresentationRow::StaticStruct();FICDialoguePresentationRow Legacy;Legacy.bAllowPlayerMovement=true;LegacyRules->AddRow(TEXT("Default"),Legacy);PC->DialoguePresentationTable=LegacyRules;
        Key(EKeys::S);TestTrue(TEXT("S selects the next NPC choice even with W/S gameplay mappings"),EntryAt(1)->bSelected&&!EntryAt(0)->bSelected);
        Key(EKeys::W);TestTrue(TEXT("W selects the previous NPC choice"),EntryAt(0)->bSelected);
        Key(EKeys::W);TestTrue(TEXT("W wraps to the last available NPC choice"),EntryAt(3)->bSelected);
        Key(EKeys::S);TestTrue(TEXT("S wraps to the first available NPC choice"),EntryAt(0)->bSelected);
        CastChecked<UButton>(EntryAt(2)->GetWidgetFromName(TEXT("EntryButton")))->SetIsEnabled(false);
        Key(EKeys::S);Key(EKeys::S);TestTrue(TEXT("Keyboard navigation skips disabled NPC choices"),EntryAt(3)->bSelected);
        Key(EKeys::W);TestTrue(TEXT("Reverse navigation also skips disabled choices"),EntryAt(1)->bSelected);
        PC->DefaultMappingContext=OriginalContext;PC->MoveAction=OriginalMove;PC->DialoguePresentationTable=OriginalRules;
        Key(EKeys::Enter);
        TestFalse(TEXT("Enter opens the selected quest screen"),CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("DialogueText")))->GetText().IsEmpty());
        TestTrue(TEXT("The next NPC screen also has an automatic keyboard selection"),EntryAt(0)->bSelected);
        Dialogue->OpenForNPC(Actors[2]);FinishGreeting();
        State->SetAffinity(Recruit.CharacterId, 299);
        TestFalse(TEXT("Recruit button disappears if affinity falls below 300"), IsRecruitFirst());
        TestFalse(TEXT("A stale recruitment request cannot bypass the threshold"), Actors[2]->TryRecruit(PC));
        State->SetAffinity(Recruit.CharacterId, 300);
        const FName PartyIDs[] = {TEXT("NPC_R01_003"), TEXT("NPC_R01_004"), TEXT("NPC_R01_005")};
        for (int32 Index = 0; Index < 3; ++Index)
        {
            FICCharacterState Member;
            Member.CharacterId = PartyIDs[Index]; Member.Affinity = 300;
            Member.Mode = EICCharacterMode::Companion; Member.PartySlot = Index;
            TestTrue(TEXT("Fill an existing party slot"), State->SetCharacterState(Member));
        }
        auto ClickRecruit = [&]()
        {
            if (!TestTrue(TEXT("Recruit entry is the first choice"), IsRecruitFirst())) return;
            auto* Entry = CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(0));
            CastChecked<UButton>(Entry->GetWidgetFromName(TEXT("EntryButton")))->OnClicked.Broadcast();
        };
        ClickRecruit();
        TestEqual(TEXT("Full party does not recruit a fourth member"), State->GetStoredCharacter(Recruit.CharacterId)->PartySlot, INDEX_NONE);
        TestEqual(TEXT("Full party keeps the conversation open"), PC->Menu, FString(TEXT("Dialogue")));
        TestTrue(TEXT("Full party is explained in the dialogue"), CastChecked<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("DialogueText")))->GetText().ToString().Contains(TEXT("3명")));
        TestTrue(TEXT("Free the second party slot"), State->SetPartySlot(PartyIDs[1], INDEX_NONE));
        auto* GroupClass = LoadClass<UICCompanionGroupWidget>(nullptr, TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CompanionGroup.WBP_IC_CompanionGroup_C"));
        auto* Group = CreateWidget<UICCompanionGroupWidget>(PC, GroupClass);
        const auto GroupSlate = Group->TakeWidget();
        ClickRecruit();
        TestEqual(TEXT("Click recruits the NPC into the available slot"), State->GetStoredCharacter(Recruit.CharacterId)->PartySlot, 1);
        TestEqual(TEXT("Click changes actual companion mode"), State->GetStoredCharacter(Recruit.CharacterId)->Mode, EICCharacterMode::Companion);
        TestTrue(TEXT("Successful recruitment closes the conversation"), PC->Menu.IsEmpty());
        TestFalse(TEXT("Successful recruitment releases the NPC interaction hold"), Actors[2]->IsInteracting());
        TestFalse(TEXT("An existing companion has no recruit option"), Actors[2]->CanRecruit());
        TestFalse(TEXT("Duplicate recruitment is rejected"), Actors[2]->TryRecruit(PC));
        auto* Card = CastChecked<UICCharacterStatusWidget>(Group->GetWidgetFromName(TEXT("Companion2")));
        TestEqual(TEXT("Recruited NPC appears immediately on HUD"), Card->CharacterID, Recruit.CharacterId);
        TestEqual(TEXT("HUD uses that NPC's actual portrait"), Card->Portrait.Get(), RecruitDefinition.Portrait.LoadSynchronous());
        const auto SavedParty = State->ExportState();
        State->SetPartySlot(Recruit.CharacterId, INDEX_NONE);
        TestTrue(TEXT("Restore recruited party from saved state"), State->RestoreState(SavedParty));
        TestEqual(TEXT("Loaded companion retains its party slot"), State->GetStoredCharacter(Recruit.CharacterId)->PartySlot, 1);
        TestEqual(TEXT("Loaded companion returns to HUD"), Card->CharacterID, Recruit.CharacterId);

        TestTrue(TEXT("A companion can interact through F again"), Actors[2]->TryInteract(PC));
        PC->Menu = TEXT("Dialogue");
        Dialogue->OpenForNPC(Actors[2]);
        FinishGreeting();
        auto IsDismissFirst = [&]()
        {
            auto* Entry = Cast<UICDialogueEntryWidget>(Entries->GetChildAt(0));
            auto* Label = Entry ? Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("EntryLabel"))) : nullptr;
            return Label && Label->GetText().ToString() == TEXT("동료 해제하기");
        };
        TestTrue(TEXT("Dismiss is the companion's first choice"), IsDismissFirst());
        TestFalse(TEXT("Recruit is absent while already a companion"), IsRecruitFirst());
        State->SetAffinity(Recruit.CharacterId, 100);
        TestTrue(TEXT("Dismissing does not require high affinity"), IsDismissFirst());
        auto* DismissEntry = CastChecked<UICDialogueEntryWidget>(Entries->GetChildAt(0));
        CastChecked<UButton>(DismissEntry->GetWidgetFromName(TEXT("EntryButton")))->OnClicked.Broadcast();
        TestEqual(TEXT("Dismiss removes the NPC from the party"), State->GetStoredCharacter(Recruit.CharacterId)->PartySlot, INDEX_NONE);
        TestEqual(TEXT("Dismiss resumes scheduled NPC behavior"), State->GetStoredCharacter(Recruit.CharacterId)->Mode, EICCharacterMode::Scheduled);
        TestEqual(TEXT("Dismiss preserves affinity"), State->GetStoredCharacter(Recruit.CharacterId)->Affinity, 100);
        TestEqual(TEXT("Dismiss hides the HUD card immediately"), Card->GetVisibility(), ESlateVisibility::Collapsed);
        TestNull(TEXT("Dismiss clears the previous portrait"), Card->Portrait);
        TestTrue(TEXT("Dismiss closes the conversation"), PC->Menu.IsEmpty());
        TestFalse(TEXT("Duplicate dismissal is rejected"), Actors[2]->TryDismiss(PC));
        State->SetAffinity(Recruit.CharacterId, 300);
        const auto SavedDismissed = State->ExportState();
        TestTrue(TEXT("Dismissed NPC can interact again"), Actors[2]->TryInteract(PC));
        PC->Menu = TEXT("Dialogue");
        Dialogue->OpenForNPC(Actors[2]);
        FinishGreeting();
        TestTrue(TEXT("Dismissed eligible NPC offers recruitment again"), IsRecruitFirst());
        ClickRecruit();

        auto* AI = CastChecked<AICAIController>(Actors[2]->GetController());
        auto Box = [&](FVector Location, FVector Extent)
        {
            auto* Actor = World->SpawnActor<AActor>();
            auto* Shape = NewObject<UBoxComponent>(Actor);
            Actor->SetRootComponent(Shape);
            Shape->SetBoxExtent(Extent);
            Shape->SetCollisionProfileName(TEXT("BlockAll"));
            Shape->RegisterComponent();
            Actor->SetActorLocation(Location);
            return Shape;
        };
        Box(FVector(0, 0, -50), FVector(2500, 2500, 50));
        auto* PlayerPawn = World->SpawnActor<ACharacter>(FVector(1000, 1000, 96), FRotator::ZeroRotator, Spawn);
        PC->SetPawn(PlayerPawn);
        TestEqual(TEXT("Recovery finds the actual player pawn"), UGameplayStatics::GetPlayerPawn(World, 0), static_cast<APawn*>(PlayerPawn));
        Actors[2]->SetActorLocation(PlayerPawn->GetActorLocation() + FVector(200, 0, 0));
        TestFalse(TEXT("Nearby companion is not teleported"), AI->TryRecoverCompanion());
        TestTrue(TEXT("Nearby companion can enter interaction"), Actors[2]->TryInteract(PC));
        Actors[2]->SetActorLocation(PlayerPawn->GetActorLocation() + FVector(6000, 0, 0));
        TestFalse(TEXT("Conversation hold prevents teleporting"), AI->TryRecoverCompanion());
        Actors[2]->EndInteraction();
        TestFalse(TEXT("Distant companion must recover before F interaction"),Actors[2]->TryInteract(PC));
        const auto BeforeRecovery = State->GetSnapshot(Recruit.CharacterId);
        TestTrue(TEXT("Distant companion returns to safe ground near the player"), AI->TryRecoverCompanion());
        TestTrue(TEXT("Recovered companion is within 5 metres"), Actors[2]->GetDistanceTo(PlayerPawn) < 500.f);
        TestTrue(TEXT("Recovery position is recorded for saves"), State->GetStoredCharacter(Recruit.CharacterId)->Position.Equals(Actors[2]->GetActorLocation(), .1f));
        TestEqual(TEXT("Recovery preserves HP"), State->GetSnapshot(Recruit.CharacterId).HP, BeforeRecovery.HP);
        TestEqual(TEXT("Recovery preserves party membership"), State->GetStoredCharacter(Recruit.CharacterId)->PartySlot, 1);
        TestFalse(TEXT("Recovery does not keep teleporting a nearby companion"), AI->TryRecoverCompanion());
        Actors[2]->SetActorLocation(PlayerPawn->GetActorLocation() + FVector(6000, 0, 0));
        auto* Blocker = Box(PlayerPawn->GetActorLocation(), FVector(600, 600, 300));
        const auto FarPosition = Actors[2]->GetActorLocation();
        TestFalse(TEXT("Blocked ground does not allow teleporting into a wall"), AI->TryRecoverCompanion());
        TestTrue(TEXT("Failed placement preserves the NPC position"), Actors[2]->GetActorLocation().Equals(FarPosition));
        Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TestTrue(TEXT("Loading preserves a dismissed NPC"), State->RestoreState(SavedDismissed));
        Actors[2]->SetActorLocation(FarPosition);
        TestFalse(TEXT("Dismissed NPC no longer follows or teleports to the player"), AI->TryRecoverCompanion());
        TestEqual(TEXT("Dismissed NPC stays hidden after load"), Card->GetVisibility(), ESlateVisibility::Collapsed);
        PC->SetPawn(nullptr);
        Group->ReleaseSlateResources(true);
    }
    if (Dialogue) Dialogue->ReleaseSlateResources(true);
    for (auto* Actor : Actors) if (Actor) { Actor->EndInteraction(); Actor->ReleaseForLoad(); }
    Definitions->ConfigureCatalog(OriginalCatalog);
    GI->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
