#include "World/Anchors/ICNPCWorldAnchor.h"

#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"

AICNPCWorldAnchor::AICNPCWorldAnchor()
{
    FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
    FacingArrow->SetupAttachment(RootComponent);
    FacingArrow->ArrowColor = FColor(64, 220, 255);
    FacingArrow->ArrowSize = 1.5f;
    FacingArrow->bIsScreenSizeScaled = true;
    FacingArrow->SetHiddenInGame(true);

    AnchorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AnchorLabel"));
    AnchorLabel->SetupAttachment(RootComponent);
    AnchorLabel->SetRelativeLocation(FVector(0.0, 0.0, 120.0));
    AnchorLabel->SetHorizontalAlignment(EHTA_Center);
    AnchorLabel->SetVerticalAlignment(EVRTA_TextCenter);
    AnchorLabel->SetWorldSize(24.0f);
    AnchorLabel->SetTextRenderColor(FColor(64, 220, 255));
    AnchorLabel->SetHiddenInGame(true);

    RefreshEditorLabel();
}

void AICNPCWorldAnchor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshEditorLabel();
}

void AICNPCWorldAnchor::RefreshEditorLabel()
{
    if (AnchorLabel)
    {
        AnchorLabel->SetText(AnchorID.IsNone() ? FText::FromString(TEXT("NPC Anchor")) : FText::FromName(AnchorID));
    }
}
