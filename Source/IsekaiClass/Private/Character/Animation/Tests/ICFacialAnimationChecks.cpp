#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "AssetCompilingManager.h"
#include "Animation/MorphTarget.h"
#include "Character/Animation/ICFacialAnimationComponent.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/VerticalBox.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NPC/Character/ICNPCBase.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "Widgets/SWidget.h"

namespace
{
const FName NPC11(TEXT("NPC_R01_011"));
const FName MouthNames[] = {TEXT("Fcl_MTH_A"), TEXT("Fcl_MTH_I"), TEXT("Fcl_MTH_U"), TEXT("Fcl_MTH_E"), TEXT("Fcl_MTH_O")};

struct FFaceSession
{
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    UWorld* World;
    AICNPCBase* NPC;
    AICNPCBase* Other;
    AICPlayerCharacter* Player = nullptr;
    TArray<AICNPCBase*> NPCs;
    FFaceSession()
    {
        GI->InitializeStandalone(); World = GI->GetWorld();
        World->InitializeActorsForPlay(FURL(), false);
        FICWorldState Initial; Initial.bPlayerCreated = true; Initial.PlayerName = TEXT("Face check");
        Initial.FirstSkill = TEXT("SKL_NPC_R01_002");
        FICCharacterState P; P.CharacterId = UICWorldStateSubsystem::PlayerId(); P.Kind = EICCharacterKind::Player;
        Initial.Characters.Add(P.CharacterId, P); GI->GetSubsystem<UICWorldStateSubsystem>()->RestoreState(Initial);
        NPC = SpawnNPC(11); Other = SpawnNPC(10);
        FAssetCompilingManager::Get().FinishAllCompilation();
    }
    AICNPCBase* SpawnNPC(int32 Number)
    {
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor = World->SpawnActor<AICNPCBase>(AICNPCBase::StaticClass(), FVector(Number*1000,0,0), FRotator::ZeroRotator, Params);
        Actor->InitializeNPC(FName(*FString::Printf(TEXT("NPC_R01_%03d"), Number)));
        Actor->GetCharacterMovement()->SetMovementMode(MOVE_Walking); NPCs.Add(Actor);
        return Actor;
    }
    ~FFaceSession()
    {
        for(auto* Actor : NPCs)Actor->ReleaseForLoad();
        if(Player)Player->ReleaseForLoad();
        GI->Shutdown(); World->DestroyWorld(false);
    }
};

float RenderedWeight(USkeletalMeshComponent* Mesh, FName Name)
{
    const auto* Target = Mesh->GetSkeletalMeshAsset()->FindMorphTarget(Name);
    const int32* Index = Mesh->ActiveMorphTargets.Find(Target);
    return Index && Mesh->MorphTargetWeights.IsValidIndex(*Index) ? Mesh->MorphTargetWeights[*Index] : 0.f;
}
float BlinkMaximum(UICFacialAnimationComponent* Face, USkeletalMeshComponent* Mesh, int32 Frames = 480)
{
    float Maximum = 0.f;
    for(int32 Frame = 0; Frame < Frames; ++Frame)
    {
        Face->TickComponent(1.f/60.f, LEVELTICK_All, nullptr);
        Maximum = FMath::Max(Maximum, Mesh->GetMorphTarget(TEXT("Fcl_EYE_Close")));
    }
    return Maximum;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCFaceCheck, "IC.Characters.Face.MorphsAndLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICNPCFaceCheck::RunTest(const FString&)
{
    FFaceSession Session;
    auto* Face = Session.NPC->FacialAnimation.Get(); auto* Mesh = Session.NPC->GetMesh();
    if (!TestNotNull(TEXT("NPC11 receives the runtime face component"), Face)) return false;
    TestTrue(TEXT("All required morphs resolve"), Face->IsFaceReady());
    TestTrue(TEXT("NPC10 also receives facial animation"), Session.Other->FacialAnimation && Session.Other->FacialAnimation->IsFaceReady());
    for (FName Name : {FName(TEXT("Fcl_EYE_Close")), MouthNames[0], MouthNames[1], MouthNames[2], MouthNames[3], MouthNames[4]})
    {
        const auto* Morph = Mesh->GetSkeletalMeshAsset()->FindMorphTarget(Name);
        TestTrue(*FString::Printf(TEXT("%s contains real vertex deltas"), *Name.ToString()), Morph && Morph->HasDataForLOD(0));
    }
    for (int32 FPS : {8, 30, 60, 120})
    {
        Face->ResetFace(); float Maximum = 0.f;
        for (int32 Frame = 0; Frame < FPS * 8; ++Frame)
        {
            Face->TickComponent(1.f / FPS, LEVELTICK_All, nullptr);
            Maximum = FMath::Max(Maximum, Mesh->GetMorphTarget(TEXT("Fcl_EYE_Close")));
        }
        TestTrue(*FString::Printf(TEXT("Idle blink closes at %d Hz"), FPS), Maximum > .7f);
    }
    Face->ResetFace(); Session.NPC->GetCharacterMovement()->Velocity = FVector(100,0,0);
    TestTrue(TEXT("Walking NPCs continue blinking"), BlinkMaximum(Face, Mesh) > .7f);
    Session.NPC->GetCharacterMovement()->Velocity = FVector::ZeroVector;

    Face->PlayDialogueLine(FText::FromString(TEXT("아 이 우 에 오.")), NPC11);
    bool Seen[5] = {};
    for (int32 Frame = 0; Frame < 240; ++Frame)
    {
        Face->TickComponent(1.f / 60.f, LEVELTICK_All, nullptr);
        for (int32 Index = 0; Index < 5; ++Index) Seen[Index] |= Mesh->GetMorphTarget(MouthNames[Index]) > .1f;
    }
    for (int32 Index = 0; Index < 5; ++Index)
    {
        TestTrue(*FString::Printf(TEXT("Korean vowels drive %s"), *MouthNames[Index].ToString()), Seen[Index]);
        TestEqual(TEXT("Finished dialogue settles to a closed mouth"), Mesh->GetMorphTarget(MouthNames[Index]), 0.f);
    }
    TestFalse(TEXT("Completed text does not loop while awaiting a choice"), Face->IsSpeaking());

    Face->PlayDialogueLine(FText::FromString(TEXT("아아아")), NPC11);
    Face->TickComponent(.07f, LEVELTICK_All, nullptr);
    Mesh->TickAnimation(1.f / 60.f, false); Mesh->RefreshBoneTransforms();
    Mesh->HandleExistingParallelEvaluationTask(true, true);
    TestTrue(TEXT("Mouth survives the existing body and VRM post-process evaluation"), RenderedWeight(Mesh, MouthNames[0]) > .1f);
    Face->StopDialogue();
    TestEqual(TEXT("Stopping publishes the closed mouth before a menu pauses mesh ticking"), RenderedWeight(Mesh, MouthNames[0]), 0.f);
    Face->PlayDialogueLine(FText::FromString(TEXT("플레이어 대사")), NAME_None);
    TestFalse(TEXT("Unset player/narrator speaker never moves NPC11's mouth"), Face->IsSpeaking());
    Face->PlayDialogueLine(FText::FromString(TEXT("다른 NPC")), TEXT("NPC_R01_010"));
    TestFalse(TEXT("Another speaker never moves NPC11's mouth"), Face->IsSpeaking());
    Face->PlayDialogueLine(FText::FromString(TEXT("아아아")), NPC11);
    Face->TickComponent(.07f, LEVELTICK_All, nullptr); Session.NPC->EndInteraction();
    TestFalse(TEXT("Ending the interaction cancels speech"), Face->IsSpeaking());
    TestEqual(TEXT("Ending the interaction clears mouth overrides"), Mesh->GetMorphTarget(MouthNames[0]), 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCDialogueFaceCheck, "IC.Characters.Face.DialogueIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICNPCDialogueFaceCheck::RunTest(const FString&)
{
    FFaceSession Session;
    auto& Context = *GEngine->GetWorldContextFromWorld(Session.World);
    auto* Viewport = NewObject<UGameViewportClient>(GEngine); Context.GameViewport = Viewport;
    Viewport->Init(Context, Session.GI, false);
    auto* PC = Session.World->SpawnActor<AICPlayerController>(); FString Error;
    PC->SetPlayer(Session.GI->CreateLocalPlayer(0, Error, false));
    auto* Class = LoadClass<UICNPCDialogueWidget>(nullptr, TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCDialogue.WBP_IC_NPCDialogue_C"));
    auto* Dialogue = Class ? CreateWidget<UICNPCDialogueWidget>(PC, Class) : nullptr;
    if (!TestNotNull(TEXT("Existing dialogue widget loads"), Dialogue)) return false;
    // NullRHI has no viewport overlay. Keep the Slate host alive for construct/destruct and button visibility.
    TSharedPtr<SWidget> SlateHost = Dialogue->TakeWidget();
    Dialogue->OpenForNPC(Session.NPC);
    auto* Face = Session.NPC->FacialAnimation.Get();
    TestTrue(TEXT("Opening NPC11's greeting starts its speech"), Face && Face->IsSpeaking());
    auto* Entries = Cast<UVerticalBox>(Dialogue->GetWidgetFromName(TEXT("Entries")));
    auto* Next = Entries && Entries->GetChildrenCount() ? Cast<UICDialogueEntryWidget>(Entries->GetChildAt(0)) : nullptr;
    if (TestNotNull(TEXT("Greeting offers a next entry"), Next))
    {
        TestTrue(TEXT("Live dialogue entries can be activated"), Next->CanSelectEntry());
        Next->ActivateEntry();
    }
    TestFalse(TEXT("Opening the interaction choices cancels speech"), Face->IsSpeaking());
    Dialogue->OpenForNPC(Session.NPC); Dialogue->OpenForNPC(Session.Other);
    TestFalse(TEXT("Switching NPCs stops the previous speaker"), Face->IsSpeaking());
    TestTrue(TEXT("Switching NPCs starts the new speaker"), Session.Other->FacialAnimation->IsSpeaking());
    Dialogue->OpenForNPC(Session.NPC); SlateHost.Reset(); Dialogue->ReleaseSlateResources(true);
    TestFalse(TEXT("Closing the widget stops speech"), Face->IsSpeaking());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICAllCharacterFaceCheck, "IC.Characters.Face.AllCharactersAndPlayerOnlyBlink",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICAllCharacterFaceCheck::RunTest(const FString&)
{
    FFaceSession Session;
    for(int32 Number = 1; Number <= 9; ++Number)Session.SpawnNPC(Number);
    FAssetCompilingManager::Get().FinishAllCompilation();
    TestEqual(TEXT("All eleven imported NPCs are covered"), Session.NPCs.Num(), 11);
    for(auto* Actor : Session.NPCs)
    {
        const FString ID = Actor->GetCharacterID().ToString();
        auto* Face = Actor->FacialAnimation.Get(); auto* Mesh = Actor->GetMesh();
        if(!TestTrue(*(ID + TEXT(" can blink and speak")), Face && Face->IsFaceReady() && Face->CanAnimateSpeech()))continue;
        for(FName Name : {FName(TEXT("Fcl_EYE_Close")), MouthNames[0], MouthNames[1], MouthNames[2], MouthNames[3], MouthNames[4]})
        {
            const auto* Morph = Mesh->GetSkeletalMeshAsset()->FindMorphTarget(Name);
            TestTrue(*(ID + TEXT(" has vertex deltas for ") + Name.ToString()), Morph && Morph->HasDataForLOD(0));
        }
        TestTrue(*(ID + TEXT(" blinks while awake")), BlinkMaximum(Face, Mesh) > .7f);
        Face->PlayDialogueLine(FText::FromString(TEXT("아 이 우 에 오.")), Actor->GetCharacterID());
        bool Seen[5] = {};
        for(int32 Frame = 0; Frame < 180; ++Frame)
        {
            Face->TickComponent(1.f/60.f, LEVELTICK_All, nullptr);
            for(int32 Index = 0; Index < 5; ++Index)Seen[Index] |= Mesh->GetMorphTarget(MouthNames[Index]) > .1f;
        }
        for(int32 Index = 0; Index < 5; ++Index)TestTrue(*(ID + TEXT(" speaks with ") + MouthNames[Index].ToString()), Seen[Index]);
        Actor->EndInteraction();
    }
    auto* State = Session.GI->GetSubsystem<UICWorldStateSubsystem>();
    TestTrue(TEXT("Companion affinity configured"), State->SetAffinity(NPC11, 300));
    TestTrue(TEXT("NPC joins the actual party"), State->SetPartySlot(NPC11, 0));
    Session.NPC->GetCharacterMovement()->Velocity = FVector(180,0,0);
    auto* Face = Session.NPC->FacialAnimation.Get(); auto* Mesh = Session.NPC->GetMesh();
    Face->ResetFace(); Face->PlayDialogueLine(FText::FromString(FString::ChrN(80, TCHAR(0xC544))), NPC11);
    TestTrue(TEXT("Moving party member blinks during speech"), BlinkMaximum(Face, Mesh) > .7f);
    TestTrue(TEXT("Party member mouth remains active for its own dialogue"), Face->IsSpeaking() && Mesh->GetMorphTarget(MouthNames[0]) > .1f);
    auto* Anim = Cast<UICCharacterAnimInstance>(Mesh->GetAnimInstance());
    if(TestNotNull(TEXT("NPC uses the common animation instance"), Anim))
    {
        Anim->bSleeping = true;
        TestEqual(TEXT("Sleeping suppresses blinking"), BlinkMaximum(Face, Mesh), 0.f);
        TestFalse(TEXT("Sleeping cancels speech"), Face->IsSpeaking());
        Anim->bSleeping = false;
    }
    Session.Player = Session.World->SpawnActor<AICPlayerCharacter>();
    Session.Player->EnableCombat(true, true); Session.Player->InitializeState(UICWorldStateSubsystem::PlayerId());
    for(int32 Preset : {0, 1, 0})
    {
        Session.Player->ApplyPreset(Preset); FAssetCompilingManager::Get().FinishAllCompilation();
        auto* PlayerFace = Session.Player->FacialAnimation.Get(); auto* PlayerMesh = Session.Player->GetMesh();
        if(!TestTrue(TEXT("Both player presets receive blinking"), PlayerFace && PlayerFace->IsFaceReady()))continue;
        TestFalse(TEXT("Player mouth animation is disabled"), PlayerFace->CanAnimateSpeech());
        PlayerMesh->SetMorphTarget(MouthNames[0], .23f);
        PlayerFace->PlayDialogueLine(FText::FromString(TEXT("플레이어 대사")), Session.Player->GetCharacterID());
        PlayerFace->ResetFace(); Session.Player->GetCharacterMovement()->Velocity = FVector(180,0,0);
        TestTrue(TEXT("Player blinks while moving after preset changes"), BlinkMaximum(PlayerFace, PlayerMesh) > .7f);
        TestFalse(TEXT("Player dialogue never starts mouth animation"), PlayerFace->IsSpeaking());
        TestEqual(TEXT("Blinking never modifies player mouth overrides"), PlayerMesh->GetMorphTarget(MouthNames[0]), .23f);
        PlayerMesh->SetMorphTarget(MouthNames[0], 0.f);
    }
    return true;
}
#endif
