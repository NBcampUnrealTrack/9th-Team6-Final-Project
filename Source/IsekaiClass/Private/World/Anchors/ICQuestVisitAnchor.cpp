#include "World/Anchors/ICQuestVisitAnchor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Quest/ICQuestSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

AICQuestVisitAnchor::AICQuestVisitAnchor()
{
    VisitArea = CreateDefaultSubobject<USphereComponent>(TEXT("VisitArea"));
    VisitArea->SetupAttachment(RootComponent);
    VisitArea->InitSphereRadius(200.0f);
    VisitArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    VisitArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    VisitArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    VisitArea->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleVisit);
}

void AICQuestVisitAnchor::HandleVisit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool IsFromSweep,
    const FHitResult& SweepResult)
{
    const AICPlayerCharacter* Player = Cast<AICPlayerCharacter>(OtherActor);
    if (!Player || !Player->IsPlayerControlled() || OtherComponent != Player->GetCapsuleComponent()
        || !GetGameInstance())
    {
        return;
    }

    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    FICAnchorRow Anchor;
    if (!Definitions || !State || !Quests || !Definitions->GetAnchor(AnchorID, Anchor).bSuccess)
    {
        return;
    }

    FICWorldState World = State->ExportState();
    if (!World.bPlayerCreated)
    {
        return;
    }
    World.DiscoveredAnchorIDs.Add(AnchorID);
    if (!Anchor.RegionID.IsNone())
    {
        World.DiscoveredRegionIDs.Add(Anchor.RegionID);
    }
    if (!State->ApplyWorldProgress(World))
    {
        return;
    }
    Quests->ReportObjectiveEvent(EICQuestObjectiveType::Visit, AnchorID, 1, FGuid::NewGuid());
    if (!Anchor.RegionID.IsNone())
    {
        Quests->ReportObjectiveEvent(EICQuestObjectiveType::Visit, Anchor.RegionID, 1, FGuid::NewGuid());
    }
}
