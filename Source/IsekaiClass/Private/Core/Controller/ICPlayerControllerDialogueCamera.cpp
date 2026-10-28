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
bool AICPlayerController::ForwardDialogueMovementKey(FKey Key,bool Pressed)
{
    if(!IsLiveNPCDialogue()||!GetDialoguePresentation().bAllowPlayerMovement||IsPaused())return false;
    const auto* Context=AppliedMappingContext?AppliedMappingContext.Get():DefaultMappingContext.Get();if(!Context)return false;
    for(const auto& Mapping:Context->GetMappings())if(Mapping.Action==MoveAction&&Mapping.Key==Key)
    {
        if(!PlayerInput||!Pressed||!PlayerInput->IsPressed(Key))InputKey(FInputKeyEventArgs::CreateSimulated(Key,Pressed?IE_Pressed:IE_Released,Pressed?1.f:0.f));
        return true;
    }
    return false;
}
namespace
{
void DialoguePose(AICNPCBase* NPC,AICPlayerController* PC,const FICDialoguePresentationRow& Rules,FVector& Position,FRotator& Rotation)
{
    FVector Head=NPC->GetActorLocation()+FVector(0,0,65);
    if(auto* Mesh=NPC->GetMesh())for(const FName Bone:{Rules.HeadBone,FName(TEXT("head"))})
        if(Mesh->DoesSocketExist(Bone)){Head=Mesh->GetSocketLocation(Bone);break;}
    const FVector Focus=Head+NPC->GetActorQuat().RotateVector(Rules.FocusOffset);
    Position=Head+NPC->GetActorForwardVector()*FMath::Max(50.f,Rules.CameraDistanceCm)+NPC->GetActorRightVector()*Rules.CameraSideOffsetCm+FVector(0,0,Rules.CameraHeightCm);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(NPCDialogueCamera),false);Query.AddIgnoredActor(NPC);Query.AddIgnoredActor(PC->GetPawn());
    TArray<AActor*> Attached;NPC->GetAttachedActors(Attached,true,true);Query.AddIgnoredActors(Attached);
    if(PC->GetPawn()){PC->GetPawn()->GetAttachedActors(Attached,true,true);Query.AddIgnoredActors(Attached);}
    FHitResult Hit;
    if(PC->GetWorld()->SweepSingleByChannel(Hit,Head,Position,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(FMath::Max(.1f,Rules.CameraCollisionRadiusCm)),Query))Position=Hit.Location;
    Rotation=(Focus-Position).Rotation();
}
}

void AICPlayerController::BeginNPCDialogueCamera(AICNPCBase* NPC)
{
    if(!IsValid(NPC)||!GetWorld())return;
    EndNPCDialogueCamera();
    DialogueNPC=NPC;
    const auto Rules=GetDialoguePresentation();if(!Rules.bUseCloseUpCamera)return;
    DialoguePreviousViewTarget=GetViewTarget();
    FVector Position;FRotator Rotation;DialoguePose(NPC,this,Rules,Position,Rotation);
    FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    NPCDialogueCamera=GetWorld()->SpawnActor<ACameraActor>(Position,Rotation,Params);
    if(!NPCDialogueCamera)return;
    // The close-up may sit behind the player's shoulder. Hide only this viewer's player/tool primitives.
    if(GetPawn()&&Rules.bHidePlayerInCamera)
    {
        TArray<AActor*> Attached;
        GetPawn()->GetAttachedActors(Attached,true,true);Attached.AddUnique(GetPawn());
        for(auto* Actor:Attached)if(IsValid(Actor)&&!HiddenActors.Contains(Actor))
        {HiddenActors.Add(Actor);DialogueHiddenActors.Add(Actor);}
    }
    auto* Camera=NPCDialogueCamera->GetCameraComponent();Camera->SetFieldOfView(FMath::Clamp(Rules.FieldOfView,15.f,100.f));
    Camera->bConstrainAspectRatio=false;
    Camera->PostProcessSettings.bOverride_MotionBlurAmount=true;
    Camera->PostProcessSettings.MotionBlurAmount=0.f;
    // Dialogue pauses the world, so switching directly also works on the first paused frame.
    SetViewTarget(NPCDialogueCamera);
    if(PlayerCameraManager)
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
        FVector Position;FRotator Rotation;DialoguePose(NPC,this,Rules,Position,Rotation);
        const float Speed=FMath::Max(0.f,Rules.FollowSpeed);
        NPCDialogueCamera->SetActorLocationAndRotation(Speed>0?FMath::VInterpTo(NPCDialogueCamera->GetActorLocation(),Position,DeltaSeconds,Speed):Position,
            Speed>0?FMath::RInterpTo(NPCDialogueCamera->GetActorRotation(),Rotation,DeltaSeconds,Speed):Rotation);
        NPCDialogueCamera->GetCameraComponent()->SetFieldOfView(FMath::Clamp(Rules.FieldOfView,15.f,100.f));
    }
}
void AICPlayerController::EndNPCDialogueCamera()
{
    for(AActor* Actor:DialogueHiddenActors)HiddenActors.Remove(Actor);
    DialogueHiddenActors.Reset();
    DialogueNPC.Reset();
    if(!NPCDialogueCamera)return;
    if(GetViewTarget()==NPCDialogueCamera)
    {
        SetViewTarget(DialoguePreviousViewTarget.IsValid()?DialoguePreviousViewTarget.Get():GetPawn());
        if(PlayerCameraManager){PlayerCameraManager->SetGameCameraCutThisFrame();PlayerCameraManager->UpdateCamera(0.f);}
    }
    NPCDialogueCamera->Destroy();NPCDialogueCamera=nullptr;DialoguePreviousViewTarget.Reset();
}

