#include "Inventory/Actors/ICChestPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "NPC/World/ICNPCContainerActor.h"

UICChestPresentationComponent::UICChestPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=false;
    Lid=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestLid"));Lid->SetupAttachment(this);
    Lid->SetCollisionProfileName(TEXT("BlockAllDynamic"));Lid->SetCanEverAffectNavigation(false);
}
void UICChestPresentationComponent::OnRegister()
{
    Super::OnRegister();
    if(Lid&&!IsTemplate())
    {
        Lid->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        // Nested component templates retain a template attachment when a BP instance is duplicated.
        if(Lid->GetAttachParent()!=this)
        {if(Lid->IsRegistered())Lid->AttachToComponent(this,FAttachmentTransformRules::KeepRelativeTransform);else Lid->SetupAttachment(this);}
        if(!Lid->IsRegistered())Lid->RegisterComponent();
    }
    ApplyPose();
}
void UICChestPresentationComponent::BeginPlay()
{
    Super::BeginPlay();ResetChest();
    if(GetWorld()&&GetWorld()->GetGameInstance())GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::ResetChest);
}
void UICChestPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if(GetWorld()&&GetWorld()->GetGameInstance())GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->OnWorldRestored.RemoveDynamic(this,&ThisClass::ResetChest);
    PendingPlayer.Reset();Super::EndPlay(Reason);
}
void UICChestPresentationComponent::ApplyPose()
{
    SetRelativeLocation(HingeOffset);
    const float Alpha=FMath::InterpEaseInOut(0.f,1.f,OpenAmount,2.f);
    SetRelativeRotation(FRotator(OpenRotation.Pitch*Alpha,OpenRotation.Yaw*Alpha,OpenRotation.Roll*Alpha));
    if(Lid)Lid->SetRelativeLocation(LidClosedOffset-HingeOffset);
}
bool UICChestPresentationComponent::OpenForPlayer(AICPlayerController* Player,FName Container,bool Home)
{
    if(!Player||Player->IsDead()||Container.IsNone())return false;
    if(!Lid||!Lid->GetStaticMesh()){Player->OpenContainer(Container,Home,true);return true;}
    if(bOpening||bWaitingForUI)return false;
    if(Player->ActiveChestPresentation.IsValid()&&Player->ActiveChestPresentation.Get()!=this)Player->ActiveChestPresentation->CloseChest();
    Player->ActiveChestPresentation=this;PendingPlayer=Player;ContainerID=Container;bHome=Home;bOpening=true;bWaitingForUI=true;SetComponentTickEnabled(true);return true;
}
void UICChestPresentationComponent::CloseChest()
{bOpening=false;bWaitingForUI=false;PendingPlayer.Reset();SetComponentTickEnabled(OpenAmount>0);}
void UICChestPresentationComponent::ResetChest()
{bOpening=bWaitingForUI=false;PendingPlayer.Reset();OpenAmount=0;ApplyPose();SetComponentTickEnabled(false);}
void UICChestPresentationComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(bWaitingForUI)
    {
        const auto* Player=PendingPlayer.Get();
        if(!Player||Player->IsDead()||!Player->GetPawn()||!Player->Menu.IsEmpty()||GetOwner()->IsHidden()
            ||FVector::DistSquared(Player->GetPawn()->GetActorLocation(),GetOwner()->GetActorLocation())>FMath::Square(350.f))CloseChest();
    }
    OpenAmount=FMath::Clamp(OpenAmount+(bOpening?1:-1)*Delta/FMath::Max(.1f,AnimationSeconds),0.f,1.f);ApplyPose();
    if(bOpening&&OpenAmount>=1)
    {
        if(bWaitingForUI)
        {
            bWaitingForUI=false;
            auto* Player=PendingPlayer.Get();const auto* NPCChest=Cast<AICNPCContainerActor>(GetOwner());
            if(Player&&(!NPCChest||NPCChest->CanAccessContainer(Player)))Player->OpenContainer(ContainerID,bHome,true);else CloseChest();
        }
        SetComponentTickEnabled(false);
    }
    else if(OpenAmount<=0)SetComponentTickEnabled(false);
}
