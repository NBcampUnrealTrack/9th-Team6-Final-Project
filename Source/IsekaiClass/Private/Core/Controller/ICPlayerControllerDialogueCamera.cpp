#include "Core/Controller/ICPlayerController.h"
#include "NPC/Character/ICNPCBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"

FICDialoguePresentationRow AICPlayerController::GetDialoguePresentation() const
{
    auto* Table=DialoguePresentationTable.LoadSynchronous();
    if(Table)
    {
        if(DialogueNPC.IsValid())if(const auto* Row=Table->FindRow<FICDialoguePresentationRow>(DialogueNPC->GetNPCDefinition().ID,TEXT("Dialogue"),false))return *Row;
        if(const auto* Row=Table->FindRow<FICDialoguePresentationRow>(TEXT("Default"),TEXT("Dialogue"),false))return *Row;
    }
    return {};
}
bool AICPlayerController::IsLiveNPCDialogue() const
{return Menu==TEXT("Dialogue")&&bInDialogue&&DialogueScreen!=nullptr;}
bool AICPlayerController::ForwardDialogueMovementKey(FKey,bool)
{
    return false;
}
namespace
{
void DialoguePose(ACharacter* Speaker, AActor* Listener, const FICDialoguePresentationRow& Rules,
    FVector& Position, FRotator& Rotation)
{
    FVector Head=Speaker->GetActorLocation()+FVector(0,0,65);
    if(auto* Mesh=Speaker->GetMesh())for(const FName Bone:{Rules.HeadBone,FName(TEXT("head"))})
        if(Mesh->DoesSocketExist(Bone)){Head=Mesh->GetSocketLocation(Bone);break;}
    const FVector Focus=Head+Speaker->GetActorQuat().RotateVector(Rules.FocusOffset);
    Position=Head+Speaker->GetActorForwardVector()*FMath::Max(50.f,Rules.CameraDistanceCm)+Speaker->GetActorRightVector()*Rules.CameraSideOffsetCm+FVector(0,0,Rules.CameraHeightCm);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NPCDialogueCamera), false);
    for (AActor* Actor : {static_cast<AActor*>(Speaker), Listener})
    {
        if (!Actor) continue;
        Query.AddIgnoredActor(Actor);
        TArray<AActor*> Attached;
        Actor->GetAttachedActors(Attached, true, true);
        Query.AddIgnoredActors(Attached);
    }
    FHitResult Hit;
    if(Speaker->GetWorld()->SweepSingleByChannel(Hit,Head,Position,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(FMath::Max(.1f,Rules.CameraCollisionRadiusCm)),Query))Position=Hit.Location;
    Rotation=(Focus-Position).Rotation();
}
}

void AICPlayerController::SetDialogueHiddenActor(AActor* ActorToHide)
{
    for (AActor* Actor : DialogueHiddenActors) HiddenActors.Remove(Actor);
    DialogueHiddenActors.Reset();
    if (!IsValid(ActorToHide)) return;
    TArray<AActor*> Attached;
    ActorToHide->GetAttachedActors(Attached, true, true);
    Attached.AddUnique(ActorToHide);
    for (AActor* Actor : Attached)
        if (IsValid(Actor) && !HiddenActors.Contains(Actor))
        {
            HiddenActors.Add(Actor);
            DialogueHiddenActors.Add(Actor);
        }
}

void AICPlayerController::BeginNPCDialogueCamera(AICNPCBase* NPC)
{
    if(!IsValid(NPC)||!GetWorld())return;
    EndNPCDialogueCamera();
    DialogueNPC=NPC;
    const auto Rules=GetDialoguePresentation();if(!Rules.bUseCloseUpCamera)return;
    DialoguePreviousViewTarget=GetViewTarget();
    FVector Position;FRotator Rotation;DialoguePose(NPC,GetPawn(),Rules,Position,Rotation);
    FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    NPCDialogueCamera=GetWorld()->SpawnActor<ACameraActor>(Position,Rotation,Params);
    if(!NPCDialogueCamera)return;
    // The close-up may sit behind the player's shoulder. Hide only this viewer's player/tool primitives.
    SetDialogueHiddenActor(Rules.bHidePlayerInCamera ? GetPawn() : nullptr);
    auto* Camera=NPCDialogueCamera->GetCameraComponent();Camera->SetFieldOfView(FMath::Clamp(Rules.FieldOfView,15.f,100.f));
    Camera->bConstrainAspectRatio=false;
    Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;
    Camera->PostProcessSettings.MotionBlurAmount=0.f;
    SetViewTarget(NPCDialogueCamera);
    if(PlayerCameraManager)
    {
        PlayerCameraManager->SetGameCameraCutThisFrame();
        PlayerCameraManager->UpdateCamera(0.f);
    }
}

void AICPlayerController::FocusDialogueCameraOnPlayer(bool bFocusPlayer)
{
    auto* NPC = DialogueNPC.Get();
    auto* PlayerCharacter = Cast<ACharacter>(GetPawn());
    bFocusPlayer = bFocusPlayer && IsValid(PlayerCharacter);
    if (!IsValid(NPCDialogueCamera) || !IsValid(NPC) || bDialogueCameraOnPlayer == bFocusPlayer) return;

    bDialogueCameraOnPlayer = bFocusPlayer;
    const auto Rules = GetDialoguePresentation();
    // Keep the listener out of the shot when the two characters stand close together.
    SetDialogueHiddenActor(bFocusPlayer ? static_cast<AActor*>(NPC) : (Rules.bHidePlayerInCamera ? GetPawn() : nullptr));
    FVector Position;
    FRotator Rotation;
    DialoguePose(bFocusPlayer ? PlayerCharacter : NPC, bFocusPlayer ? static_cast<AActor*>(NPC) : GetPawn(), Rules, Position, Rotation);
    NPCDialogueCamera->SetActorLocationAndRotation(Position, Rotation);
    if (PlayerCameraManager)
    {
        PlayerCameraManager->SetGameCameraCutThisFrame();
        PlayerCameraManager->UpdateCamera(0.f);
    }
}

void AICPlayerController::UpdateNPCDialogue(float DeltaSeconds)
{
    if(!IsLiveNPCDialogue())return;
    auto* NPC=DialogueNPC.Get();const auto Rules=GetDialoguePresentation();
    if(!IsValid(NPC)||!GetPawn()||NPC->IsSleepingForInteraction()||bDead
        ||FVector::DistSquared(GetPawn()->GetActorLocation(),NPC->GetActorLocation())>FMath::Square(FMath::Max(250.f,Rules.CloseDistanceCm)))
    {SetMenu(TEXT(""));return;}
    if(NPCDialogueCamera&&Rules.bUseCloseUpCamera)
    {
        auto* PlayerCharacter = bDialogueCameraOnPlayer ? Cast<ACharacter>(GetPawn()) : nullptr;
        FVector Position;FRotator Rotation;
        DialoguePose(PlayerCharacter ? PlayerCharacter : NPC, PlayerCharacter ? static_cast<AActor*>(NPC) : GetPawn(), Rules, Position, Rotation);
        const float Speed=FMath::Max(0.f,Rules.FollowSpeed);
        NPCDialogueCamera->SetActorLocationAndRotation(Speed>0?FMath::VInterpTo(NPCDialogueCamera->GetActorLocation(),Position,DeltaSeconds,Speed):Position,
            Speed>0?FMath::RInterpTo(NPCDialogueCamera->GetActorRotation(),Rotation,DeltaSeconds,Speed):Rotation);
        NPCDialogueCamera->GetCameraComponent()->SetFieldOfView(FMath::Clamp(Rules.FieldOfView,15.f,100.f));
    }
}
void AICPlayerController::EndNPCDialogueCamera()
{
    SetDialogueHiddenActor(nullptr);
    bDialogueCameraOnPlayer = false;
    DialogueNPC.Reset();
    if(!NPCDialogueCamera)return;
    if(GetViewTarget()==NPCDialogueCamera)
    {
        SetViewTarget(DialoguePreviousViewTarget.IsValid()?DialoguePreviousViewTarget.Get():GetPawn());
        if(PlayerCameraManager){PlayerCameraManager->SetGameCameraCutThisFrame();PlayerCameraManager->UpdateCamera(0.f);}
    }
    NPCDialogueCamera->Destroy();NPCDialogueCamera=nullptr;DialoguePreviousViewTarget.Reset();
}

