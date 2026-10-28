#include "Life/Crafting/ICCraftingStation.h"
#include "UI/Life/ICCraftingStationWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"

AICCraftingStation::AICCraftingStation()
{
    Action=TEXT("Forge");WorkAnchorID=TEXT("ANC_FORGE_001");
    InteractionText=NSLOCTEXT("ICCraft","Workbench","제작대");
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetRelativeScale3D(FVector(1.6,1.1,.8));
    Label->SetText(FText::FromString(TEXT("CRAFT [F]")));
}
bool AICCraftingStation::TryInteract_Implementation(AICPlayerController* Controller)
{
    if(!Controller||!Controller->bCreated||Controller->IsDead()||Controller->bInCombat||Controller->bInDialogue||Controller->bInEvent||!Controller->Menu.IsEmpty()||WorkAnchorID.IsNone())return false;
    if(!OpenWidget||!OpenWidget->IsOpen())
        OpenWidget=CreateWidget<UICCraftingStationWidget>(Controller,StationWidgetClass?StationWidgetClass.Get():UICCraftingStationWidget::StaticClass());
    if(!OpenWidget)return false;
    OpenWidget->OpenStation(this);return OpenWidget->IsOpen();
}
