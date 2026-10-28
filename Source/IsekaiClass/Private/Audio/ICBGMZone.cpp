#include "Audio/ICBGMZone.h"
#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
AICBGMZone::AICBGMZone()
{
    Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("MusicBounds"));RootComponent=Bounds;
    Bounds->SetBoxExtent(FVector(2000,2000,2000));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetHiddenInGame(true);Bounds->ShapeColor=FColor(64,190,255);
}
bool AICBGMZone::ContainsPosition(FVector Position) const
{
    const FVector P=Bounds->GetComponentTransform().InverseTransformPosition(Position),E=Bounds->GetUnscaledBoxExtent();
    return FMath::Abs(P.X)<=E.X&&FMath::Abs(P.Y)<=E.Y&&FMath::Abs(P.Z)<=E.Z;
}
TArray<FName> AICBGMZone::GetMusicRows() const
{
    const auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Audio/DT_IC_BGM.DT_IC_BGM"));
    return Table?Table->GetRowNames():TArray<FName>{};
}
