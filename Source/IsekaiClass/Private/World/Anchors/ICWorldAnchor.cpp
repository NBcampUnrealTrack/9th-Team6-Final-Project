#include "World/Anchors/ICWorldAnchor.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Components/SceneComponent.h"
#include "Debug/ICLog.h"
AICWorldAnchor::AICWorldAnchor() { IC_TRACE_FUNCTION();RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("AnchorRoot")); }
void AICWorldAnchor::BeginPlay() { IC_TRACE_FUNCTION();Super::BeginPlay();if(auto* S=GetWorld()->GetSubsystem<UICSpawnSubsystem>())S->RegisterAnchor(this); }
void AICWorldAnchor::EndPlay(const EEndPlayReason::Type Reason) { IC_TRACE_FUNCTION();if(auto* S=GetWorld()->GetSubsystem<UICSpawnSubsystem>())S->UnregisterAnchor(this);Super::EndPlay(Reason); }
