#include "UI/Map/ICMapWidget.h"
#include "UI/Map/ICMapTextureImage.h"
#include "UI/Map/ICMapPlayerMarkerShape.h"
#include "UI/Framework/ICWidgetTemplates.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Text/ICLocalization.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Rendering/DrawElementTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Settings/ICGameUserSettings.h"
#include "Styling/CoreStyle.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Warp/ICPartyWarpComponent.h"
#include "World/Warp/ICWarpStone.h"
#include "NPC/Character/ICNPCBase.h"
#include "Enemy/Character/ICEnemyBase.h"

namespace { FText Local(const FText& Text){return ICLocalization::Localize(Text);} }

void UICMapWidget::BuildLayout(UWidgetTree* Tree,bool Mini)
{
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    auto Fill=[&](UWidget* W,FAnchors A=FAnchors(0,0,1,1),FMargin M=FMargin(0))
    {auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.026f,.045f,Mini?.82f:.98f));Fill(Background);
    auto* Surface=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapSurface"));Surface->SetBrushColor(FLinearColor(.055f,.075f,.095f,Mini?.88f:1.f));
    Fill(Surface,Mini?FAnchors(0,0,1,1):FAnchors(.035f,.17f,.965f,.87f));
    auto* Content=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MapContent"));Content->SetVisibility(ESlateVisibility::HitTestInvisible);Fill(Content);
    auto* Image=Tree->ConstructWidget<UICMapTextureImage>(UICMapTextureImage::StaticClass(),TEXT("IMG_Map"));Image->bFadeEdges=Mini;Content->AddChildToCanvas(Image);
    if(Mini)
    {
        const TCHAR* EdgeNames[]={TEXT("MinimapFrameTop"),TEXT("MinimapFrameBottom"),TEXT("MinimapFrameLeft"),TEXT("MinimapFrameRight")};
        const FAnchors EdgeAnchors[]={FAnchors(0,0,1,0),FAnchors(0,1,1,1),FAnchors(0,0,0,1),FAnchors(1,0,1,1)};
        const FMargin EdgeOffsets[]={FMargin(0,0,0,2),FMargin(0,-2,0,2),FMargin(0,0,2,0),FMargin(-2,0,2,0)};
        for(int32 Index=0;Index<4;++Index){auto* Edge=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),EdgeNames[Index]);Edge->SetBrushColor(FLinearColor(.48f,.62f,.68f,.72f));Fill(Edge,EdgeAnchors[Index],EdgeOffsets[Index])->SetZOrder(5);}
        auto* North=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_MiniNorth"));North->SetFont(ICFrontendLayout::Font(12,true));North->SetText(FText::FromString(TEXT("N")));North->SetJustification(ETextJustify::Center);North->SetColorAndOpacity(FLinearColor(.95f,.78f,.28f));Fill(North,FAnchors(.5f,0),FMargin(-10,4,20,18))->SetZOrder(6);
        auto* Help=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_MiniHelp"));Help->SetFont(ICFrontendLayout::Font(11));Help->SetText(NSLOCTEXT("ICMap","MiniHelp","M · 전체 지도"));Help->SetColorAndOpacity(FLinearColor(.82f,.87f,.9f));Fill(Help,FAnchors(0,1),FMargin(8,-21,100,18))->SetZOrder(6);
        auto* Legend=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_MiniLegend"));Legend->SetFont(ICFrontendLayout::Font(9,true));Legend->SetText(NSLOCTEXT("ICMap","MiniLegend","■ 건물  ● NPC  ◆ 적"));Legend->SetJustification(ETextJustify::Right);Legend->SetColorAndOpacity(FLinearColor(.8f,.86f,.9f));Fill(Legend,FAnchors(1,1),FMargin(-158,-20,150,18))->SetZOrder(6);
    }
    if(Mini)return;
    auto AddText=[&](const TCHAR* Name,FVector2D At,int32 Size)
    {auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);T->SetFont(ICFrontendLayout::Font(Size));auto* S=Fill(T,FAnchors(0,0),FMargin(At.X,At.Y,1600,50));return T;};
    AddText(TEXT("TXT_MapTitle"),{60,30},32);AddText(TEXT("TXT_MapHelp"),{60,90},20);
    auto* StatusText=AddText(TEXT("TXT_MapStatus"),{60,970},22);StatusText->SetColorAndOpacity(FLinearColor(1,.8f,.35f));
    auto* Close=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BTN_MapClose"));
    auto* Caption=Tree->ConstructWidget<UTextBlock>();Caption->SetFont(ICFrontendLayout::Font(22));Caption->SetText(Local(NSLOCTEXT("ICMap","Close","닫기")));Close->SetContent(Caption);
    auto* C=Fill(Close,FAnchors(1,0),FMargin(-170,35,110,46));C->SetAlignment(FVector2D::ZeroVector);
    auto* Loading=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapLoading"));Loading->SetBrushColor(FLinearColor(.01f,.015f,.025f,1));Fill(Loading)->SetZOrder(50);
    auto* Text=Tree->ConstructWidget<UTextBlock>();Text->SetFont(ICFrontendLayout::Font(32));Text->SetText(Local(NSLOCTEXT("ICMap","Loading","이동 중 · 로딩 중…")));Text->SetJustification(ETextJustify::Center);
    Loading->SetHorizontalAlignment(HAlign_Center);Loading->SetVerticalAlignment(VAlign_Center);Loading->SetContent(Text);Loading->SetVisibility(ESlateVisibility::Collapsed);
}
void UICMapWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    // Blueprint defaults can be reset when the native parent gains new properties.
    // The minimap-only help widget is a stable runtime signal, so do not let the
    // HUD silently fall back to full-map coordinates when that happens.
    bMinimap=bMinimap||GetWidgetFromName(TEXT("TXT_MiniHelp"))!=nullptr||GetClass()->GetName().Contains(TEXT("Minimap"));
    SetIsFocusable(!bMinimap);

    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_MapClose"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::CloseMap);
    RefreshMap();RefreshLabels();
}
void UICMapWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // Nested HUD widgets may initialize before their owning player is assigned.
    RefreshMap();RefreshLabels();UpdateCanTick();
}
void UICMapWidget::NativeDestruct()
{
    if(bTraveling)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->bInEvent=false;
    bTraveling=false;Super::NativeDestruct();
}
void UICMapWidget::RefreshMap()
{
    if(MapTable.IsNull())MapTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/World/DT_IC_Map.DT_IC_Map")));
    if(PresentationTable.IsNull())PresentationTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_MapPresentation.DT_IC_MapPresentation")));
    if(auto* T=PresentationTable.LoadSynchronous())if(auto* R=T->FindRow<FICMapPresentationRow>(TEXT("Default"),TEXT("Map"),false))Rules=*R;
    if(auto* T=MapTable.LoadSynchronous())if(auto* R=T->FindRow<FICMapRow>(Rules.MapID,TEXT("Map"),false);R&&!R->Layers.IsEmpty())
    {Layer=R->Layers[0];MapTexture=Layer.MapTexture.LoadSynchronous();MapMinimumZoom=R->MinimumZoom;MapMaximumZoom=R->MaximumZoom;}
    Portals.Reset();if(auto* World=GetWorld())for(TActorIterator<AICWarpStone> It(World);It;++It)if(!It->WarpID.IsNone())Portals.Add(*It);
    Portals.Sort([](const auto& A,const auto& B){return A->WarpID.LexicalLess(B->WarpID);});
    Buildings.Reset();NPCs.Reset();Monsters.Reset();
    if(bMinimap)if(auto* World=GetWorld())
    {
        for(TActorIterator<AActor> It(World);It;++It)if(It->ActorHasTag(TEXT("IC.Map.Building")))Buildings.Add(*It);
        // L_OpenWorld's houses are modular kits rather than one tagged root actor.
        // Detect only a kit's large/core mesh, not every wall, roof, door or window,
        // so a house produces one useful marker instead of a cloud of markers.
        for(TActorIterator<AStaticMeshActor> It(World);It;++It)
        {
            auto* Actor=*It;auto* Component=Actor?Actor->GetStaticMeshComponent():nullptr;UStaticMesh* Mesh=Component?Component->GetStaticMesh().Get():nullptr;
            if(!Mesh||!IsAutomaticBuildingMeshPath(Mesh->GetPathName())||Buildings.Contains(Actor))continue;
            // Modular houses contain several floor pieces. Keep one representative
            // within a building-sized radius instead of drawing a marker per piece.
            const bool bSameBuilding=Buildings.ContainsByPredicate([Actor](const TWeakObjectPtr<AActor>& Existing)
            {return Existing.IsValid()&&FVector::DistSquaredXY(Existing->GetActorLocation(),Actor->GetActorLocation())<=FMath::Square(1500.f);});
            if(!bSameBuilding)Buildings.Add(Actor);
        }
        for(TActorIterator<AICNPCBase> It(World);It;++It)NPCs.Add(*It);
        for(TActorIterator<AICEnemyBase> It(World);It;++It)Monsters.Add(*It);
    }
    if(!bRegistered&&Rules.bUnlockPlacedPortals)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->bCreated&&PC->GetWorldState())
    {
        auto Next=PC->GetWorldState()->ExportState();bool Changed=false;
        for(const auto& P:Portals)if(P.IsValid()&&!Next.UnlockedWarps.Contains(P->WarpID)){Next.UnlockedWarps.Add(P->WarpID);Changed=true;}
        bRegistered=!Changed||PC->GetWorldState()->ApplyWorldProgress(Next);
    }
}
bool UICMapWidget::IsAutomaticBuildingMeshPath(const FString& AssetPath)
{
    FString Path=AssetPath.ToLower();
    if(!Path.Contains(TEXT("/meshes/building/"))&&!Path.Contains(TEXT("/meshes/buildings/")))return false;
    const bool bCore=Path.Contains(TEXT("building_block"))||Path.Contains(TEXT("building_box"))||Path.Contains(TEXT("/sm_floor"))||
        Path.Contains(TEXT("farmhouse"))||Path.Contains(TEXT("farm_house"))||Path.Contains(TEXT("windmill"))||
        Path.Contains(TEXT("/sm_house_"))||Path.Contains(TEXT("/sm_building_"));
    if(!bCore)return false;
    for(const TCHAR* Detail:{TEXT("fence"),TEXT("roof"),TEXT("wall"),TEXT("door"),TEXT("window"),TEXT("stairs"),TEXT("decoration")})
        if(Path.Contains(Detail))return false;
    return true;
}
void UICMapWidget::RefreshLabels()
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapTitle"))))T->SetText(Local(NSLOCTEXT("ICMap","Title","테스트맵 · 포탈 이동")));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapHelp"))))T->SetText(FText::Format(Local(NSLOCTEXT("ICMap","Help","포탈 클릭 / 방향키 선택 + 확인: {0}G로 이동 · 보유 {1}G · M / B: 닫기")),Rules.TravelCostGold,PC?PC->Gold:0));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapStatus"))))T->SetText(Status);
}
FVector2D UICMapWidget::WorldToMapUV(FVector P) const
{
    const FVector2D D=FVector2D(P.X,P.Y)-Layer.WorldOriginCm;
    return Layer.MapOriginUV+FVector2D(FVector2D::DotProduct(D,Layer.WorldAxisU)/FMath::Max(1.,Layer.WorldCmPerUV.X),FVector2D::DotProduct(D,Layer.WorldAxisV)/FMath::Max(1.,Layer.WorldCmPerUV.Y));
}
namespace
{
int32 CountValidActors(const TArray<TWeakObjectPtr<AActor>>& Actors)
{int32 Count=0;for(const auto& Actor:Actors)if(Actor.IsValid())++Count;return Count;}
}
int32 UICMapWidget::GetTrackedBuildingCount() const{return CountValidActors(Buildings);}
int32 UICMapWidget::GetTrackedNPCCount() const{return CountValidActors(NPCs);}
int32 UICMapWidget::GetTrackedMonsterCount() const{return CountValidActors(Monsters);}
FVector2D UICMapWidget::MinimapDisplayUV(FVector2D MapUV,FVector2D CenterUV,float Zoom)
{return FVector2D(.5,.5)+(MapUV-CenterUV)*FMath::Max(1.f,Zoom);}
FVector2D UICMapWidget::DisplayUV(FVector P) const
{
    const FVector2D MapUV=WorldToMapUV(P);
    return bMinimap?MinimapDisplayUV(MapUV,MinimapCenterUV,FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom)):MapUV;
}
FSlateRect UICMapWidget::ImageRect(const FGeometry& G) const
{
    auto* Surface=GetWidgetFromName(TEXT("MapSurface"));if(!Surface)return FSlateRect();
    // Paint-space geometries share a window origin; cached tick geometry includes the desktop offset.
    const auto& SG=Surface->GetPaintSpaceGeometry();const auto& Root=GetPaintSpaceGeometry();
    const FVector2D A=Root.AbsoluteToLocal(SG.LocalToAbsolute(FVector2D::ZeroVector)),B=Root.AbsoluteToLocal(SG.LocalToAbsolute(SG.GetLocalSize()));
    if(bMinimap)return FSlateRect(A.X,A.Y,B.X,B.Y);
    FVector2D Size=B-A;const double Aspect=MapTexture?double(MapTexture->GetSizeX())/FMath::Max(1,MapTexture->GetSizeY()):2.;
    const double W=FMath::Min(Size.X,Size.Y*Aspect),H=W/Aspect;const FVector2D Min=A+(Size-FVector2D(W,H))*.5;
    return FSlateRect(Min.X,Min.Y,Min.X+W,Min.Y+H);
}
FVector2D UICMapWidget::MarkerPosition(const FSlateRect& R,FVector P) const
{const auto UV=DisplayUV(P);return {R.Left+UV.X*(R.Right-R.Left),R.Top+UV.Y*(R.Bottom-R.Top)};}
float UICMapWidget::MinimapOpacity(FVector2D UV)
{
    const double X=FMath::Abs(UV.X*2-1),Y=FMath::Abs(UV.Y*2-1);
    return FMath::Square(FMath::Clamp(1.-X*X,0.,1.))*FMath::Square(FMath::Clamp(1.-Y*Y,0.,1.));
}
void UICMapWidget::RefreshVisuals(const FGeometry& G)
{
    auto* Canvas=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("MapContent")));
    auto* Image=Cast<UICMapTextureImage>(GetWidgetFromName(TEXT("IMG_Map")));if(!Canvas||!Image)return;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    const double Zoom=bMinimap?FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom):1.;
    if(bMinimap&&Player)
    {
        // Radar-style minimap: the player stays centred and nearby markers move.
        MinimapCenterUV=WorldToMapUV(Player->GetActorLocation());
    }
    const auto Rect=ImageRect(G);const FVector2D Size(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
    Image->SetTexture(MapTexture);
    Image->SetVisibility(bMinimap?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    const FVector2D HalfUV(.5/Zoom,.5/Zoom);
    Image->SetUVRegion(bMinimap?MinimapCenterUV-HalfUV:FVector2D::ZeroVector,bMinimap?MinimapCenterUV+HalfUV:FVector2D(1,1));
    if(auto* S=Cast<UCanvasPanelSlot>(Image->Slot)){S->SetPosition({Rect.Left,Rect.Top});S->SetSize(Size);}
    TSet<FName> Active;
    for(int32 I=0;I<Portals.Num();++I)if(auto* Portal=Portals[I].Get())
    {
        Active.Add(Portal->WarpID);auto* Marker=PortalMarkers.FindRef(Portal->WarpID).Get();
        if(!Marker)
        {
            Marker=ICWidgetTemplates::Create(this,TEXT("WBP_IC_MapPortalMarker"));if(!Marker)continue;
            PortalMarkers.Add(Portal->WarpID,Marker);auto* S=Canvas->AddChildToCanvas(Marker);S->SetAutoSize(true);S->SetAlignment({.5f,.5f});S->SetZOrder(1);
            Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        const auto UV=DisplayUV(Portal->GetActorLocation());
        Marker->SetVisibility(UV.X>=0&&UV.X<=1&&UV.Y>=0&&UV.Y<=1?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        const bool Unlocked=Player&&Player->PartyWarp&&Player->PartyWarp->IsWarpUnlocked(Portal->WarpID);
        const bool Selected=!bMinimap&&I==SelectedPortal;
        for(const auto* Name:{TEXT("PortalUnlocked"),TEXT("PortalLocked"),TEXT("PortalSelected")})
        {
            const bool Show=FString(Name)==TEXT("PortalSelected")?Selected:FString(Name)==TEXT("PortalUnlocked")?Unlocked&&!Selected:!Unlocked&&!Selected;
            if(auto* W=Marker->GetWidgetFromName(Name))W->SetVisibility(Show?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        }
        if(auto* Label=Cast<UTextBlock>(Marker->GetWidgetFromName(TEXT("TXT_Label"))))
        {Label->SetText(Local(Portal->InteractionText));Label->SetVisibility(bMinimap?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}
        Marker->SetRenderScale(bMinimap?FVector2D(.65f):FVector2D(1));Marker->SetRenderOpacity(1.f);
        CastChecked<UCanvasPanelSlot>(Marker->Slot)->SetPosition(MarkerPosition(Rect,Portal->GetActorLocation()));
    }
    for(auto It=PortalMarkers.CreateIterator();It;++It)if(!Active.Contains(It.Key())){It.Value()->RemoveFromParent();It.RemoveCurrent();}
    TSet<TObjectPtr<AActor>> ActiveWorldActors;
    auto AddWorldMarkers=[&](const TArray<TWeakObjectPtr<AActor>>& Actors,const TCHAR* Glyph,FLinearColor Color,int32 ZOrder,const FText& Label)
    {
        for(const auto& WeakActor:Actors)if(auto* Actor=WeakActor.Get();IsValid(Actor))
        {
            ActiveWorldActors.Add(Actor);
            auto* Marker=WorldActorMarkers.FindRef(Actor).Get();
            if(!Marker)
            {
                Marker=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
                Marker->SetFont(ICFrontendLayout::Font(15,true));Marker->SetText(FText::FromString(Glyph));Marker->SetJustification(ETextJustify::Center);
                Marker->SetColorAndOpacity(Color);Marker->SetShadowColorAndOpacity(FLinearColor::Black);Marker->SetShadowOffset({1,1});Marker->SetToolTipText(Label);
                auto* Slot=Canvas->AddChildToCanvas(Marker);Slot->SetAutoSize(true);Slot->SetAlignment({.5f,.5f});Slot->SetZOrder(ZOrder);WorldActorMarkers.Add(Actor,Marker);
            }
            const auto UV=DisplayUV(Actor->GetActorLocation());
            Marker->SetVisibility(UV.X>=0&&UV.X<=1&&UV.Y>=0&&UV.Y<=1?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
            CastChecked<UCanvasPanelSlot>(Marker->Slot)->SetPosition(MarkerPosition(Rect,Actor->GetActorLocation()));
        }
    };
    if(bMinimap)
    {
        AddWorldMarkers(Buildings,TEXT("■"),FLinearColor(1.f,.7f,.18f),1,NSLOCTEXT("ICMap","BuildingMarker","건물"));
        AddWorldMarkers(NPCs,TEXT("●"),FLinearColor(.2f,.85f,1.f),2,NSLOCTEXT("ICMap","NPCMarker","NPC"));
        AddWorldMarkers(Monsters,TEXT("◆"),FLinearColor(1.f,.2f,.18f),3,NSLOCTEXT("ICMap","MonsterMarker","몬스터"));
    }
    for(auto It=WorldActorMarkers.CreateIterator();It;++It)if(!IsValid(It.Key())||!ActiveWorldActors.Contains(It.Key())){It.Value()->RemoveFromParent();It.RemoveCurrent();}
    if(!PlayerMarker)
    {
        PlayerMarker=WidgetTree->ConstructWidget<UICMapPlayerMarkerShape>(UICMapPlayerMarkerShape::StaticClass(),TEXT("PlayerDirectionMarker"));
        if(PlayerMarker)
        {
            auto* S=Canvas->AddChildToCanvas(PlayerMarker);S->SetAutoSize(true);S->SetAlignment({.5f,.5f});S->SetZOrder(5);
            PlayerMarker->SetRenderTransformPivot(FVector2D(.5f,.5f));
        }
    }
    if(PlayerMarker)
    {
        const auto UV=Player?DisplayUV(Player->GetActorLocation()):FVector2D(-1);
        PlayerMarker->SetVisibility(Player&&UV.X>=0&&UV.X<=1&&UV.Y>=0&&UV.Y<=1?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        if(Player)
        {
            const auto Forward=Player->GetActorForwardVector();const FVector2D Direction(Forward.X*Layer.WorldAxisU.X+Forward.Y*Layer.WorldAxisU.Y,Forward.X*Layer.WorldAxisV.X+Forward.Y*Layer.WorldAxisV.Y);
            PlayerMarker->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Direction.X,-Direction.Y)));
            PlayerMarker->SetRenderScale(bMinimap?FVector2D(.85f):FVector2D(1));PlayerMarker->SetRenderOpacity(1.f);
            CastChecked<UCanvasPanelSlot>(PlayerMarker->Slot)->SetPosition(MarkerPosition(Rect,Player->GetActorLocation()));
        }
    }
}
bool UICMapWidget::RequestTravel(FName ID)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    if(bMinimap||bTraveling||(!GetParent()&&!IsInViewport())||!PC||!Player||!Player->PartyWarp||!PC->bCreated||PC->IsDead()||PC->bInCombat||PC->bInDialogue||PC->bInEvent||PC->Menu!=TEXT("Map")||(Player->LifeActions&&(Player->LifeActions->IsWorking()||Player->LifeActions->IsSleeping())))return false;
    if(!Portals.ContainsByPredicate([ID](const auto& P){return P.IsValid()&&P->WarpID==ID;}))return false;
    FString Reason;if(!Player->PartyWarp->CanWarp(ID,Rules.TravelCostGold,Reason)){Status=Local(FText::FromString(Reason));RefreshLabels();return false;}
    PendingWarp=ID;bTraveling=true;bMoved=false;TravelElapsed=0;PC->bInEvent=true;Player->GetCharacterMovement()->StopMovementImmediately();
    if(auto* W=GetWidgetFromName(TEXT("MapLoading")))W->SetVisibility(ESlateVisibility::Visible);return true;
}
void UICMapWidget::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);RefreshElapsed+=Delta;
    if(RefreshElapsed>.5f){RefreshElapsed=0;RefreshMap();RefreshLabels();}
    RefreshVisuals(G);
    if(!bTraveling)return;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    TravelElapsed+=FMath::Max(0.f,Delta);const float Duration=FMath::Max(.3f,Rules.LoadingSeconds);
    if(!bMoved&&TravelElapsed>=Duration*.6f)
    {
        FString Reason;
        if(!PC||!Player||PC->IsDead()||PC->bInCombat||!Player->PartyWarp||!Player->PartyWarp->WarpParty(PendingWarp,Rules.TravelCostGold,Reason))
        {
            bTraveling=false;if(PC)PC->bInEvent=false;Status=Local(FText::FromString(Reason));
            if(auto* W=GetWidgetFromName(TEXT("MapLoading")))W->SetVisibility(ESlateVisibility::Collapsed);RefreshLabels();return;
        }
        bMoved=true;
    }
    if(bMoved&&TravelElapsed>=Duration)
    {bTraveling=false;PC->bInEvent=false;PC->SetMenu(TEXT(""));PC->AutoSavePlayer();}
}
void UICMapWidget::CloseMap(){if(!bTraveling)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT(""));}
FReply UICMapWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event)
{
    if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return Super::NativeOnMouseButtonDown(G,Event);
    if(bTraveling)return FReply::Handled();
    if(bMinimap){if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->bCreated&&!PC->bInEvent)PC->SetMenu(TEXT("Map"));return FReply::Handled();}
    const FVector2D Click=G.AbsoluteToLocal(Event.GetScreenSpacePosition());const auto R=ImageRect(G);
    for(int32 I=0;I<Portals.Num();++I)if(const auto* P=Portals[I].Get();P&&FVector2D::Distance(Click,MarkerPosition(R,P->GetActorLocation()))<30){SelectedPortal=I;RequestTravel(P->WarpID);return FReply::Handled();}
    return Super::NativeOnMouseButtonDown(G,Event);
}
FReply UICMapWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    if(bMinimap)return Super::NativeOnPreviewKeyDown(G,Event);
    if(bTraveling)return FReply::Handled();
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());
    if(Key==EKeys::Escape||Key==EKeys::M||Key==EKeys::Gamepad_Special_Left){if(!Event.IsRepeat())CloseMap();return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::S||Key==EKeys::A||Key==EKeys::D||Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::Up||Key==EKeys::Down)
    {if(!Portals.IsEmpty())SelectedPortal=(SelectedPortal+((Key==EKeys::W||Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Up)?Portals.Num()-1:1))%Portals.Num();return FReply::Handled();}
    if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar){if(!Event.IsRepeat()&&Portals.IsValidIndex(SelectedPortal)&&Portals[SelectedPortal].IsValid())RequestTravel(Portals[SelectedPortal]->WarpID);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(G,Event);
}
