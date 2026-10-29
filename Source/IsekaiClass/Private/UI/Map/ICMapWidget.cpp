#include "UI/Map/ICMapWidget.h"
#include "UI/Map/ICMapTextureImage.h"
#include "UI/Framework/ICWidgetTemplates.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Text/ICLocalization.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Rendering/DrawElementTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Settings/ICGameUserSettings.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Warp/ICPartyWarpComponent.h"
#include "World/Warp/ICWarpStone.h"

namespace
{
FText Local(const FText& Text){return ICLocalization::Localize(Text);}
void WarpButtonStyle(UButton* Button,const FLinearColor& Color)
{
    auto Style=Button->GetStyle();
    Style.Normal=FSlateRoundedBoxBrush(FLinearColor::White,6.f);
    Style.Hovered=FSlateRoundedBoxBrush(FLinearColor(.65f,.9f,1.f),6.f);
    Style.Pressed=FSlateRoundedBoxBrush(FLinearColor(.45f,.75f,1.f),6.f);
    Button->SetStyle(Style);Button->SetBackgroundColor(Color);
}
}

void UICMapWidget::BuildLayout(UWidgetTree* Tree,bool Mini)
{
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    auto Fill=[&](UWidget* W,FAnchors A=FAnchors(0,0,1,1),FMargin M=FMargin(0))
    {auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.026f,.045f,Mini?0.f:.98f));Fill(Background);
    if(Mini)
    {
        auto* Surface=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapSurface"));Surface->SetBrushColor(FLinearColor::Transparent);Fill(Surface);
        auto* Content=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MapContent"));Content->SetVisibility(ESlateVisibility::HitTestInvisible);Fill(Content);
        auto* Image=Tree->ConstructWidget<UICMapTextureImage>(UICMapTextureImage::StaticClass(),TEXT("IMG_Map"));Image->bFadeEdges=true;Content->AddChildToCanvas(Image);
        auto* Help=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_MiniHelp"));Help->SetFont(ICFrontendLayout::Font(11));Help->SetText(NSLOCTEXT("ICMap","MiniHelp","M · 지도"));Fill(Help,FAnchors(0,0),FMargin(8,3,80,20));
    }
    if(Mini)return;
    auto AddText=[&](const TCHAR* Name,FVector2D At,int32 Size)
    {auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);T->SetFont(ICFrontendLayout::Font(Size));auto* S=Fill(T,FAnchors(0,0),FMargin(At.X,At.Y,1600,50));return T;};
    AddText(TEXT("TXT_MapTitle"),{60,30},32);AddText(TEXT("TXT_MapHelp"),{60,90},20);
    auto* StatusText=AddText(TEXT("TXT_MapStatus"),{0,0},18);StatusText->SetColorAndOpacity(FLinearColor(1,.8f,.35f));
    auto* StatusSlot=CastChecked<UCanvasPanelSlot>(StatusText->Slot);StatusSlot->SetAnchors(FAnchors(.035f,1));StatusSlot->SetPosition({0,-132});StatusSlot->SetSize({1600,35});
    auto* Destinations=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("WarpDestinations"));
    auto* DestinationSlot=Fill(Destinations,FAnchors(.2f,.23f,.8f,.78f));DestinationSlot->SetZOrder(5);
    for(int32 I=0;I<4;++I)
    {
        auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),FName(*FString::Printf(TEXT("BTN_Warp%d"),I)));
        WarpButtonStyle(Button,I==0?FLinearColor(.12f,.32f,.48f):FLinearColor(.06f,.09f,.14f));
        auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*FString::Printf(TEXT("TXT_Warp%d"),I)));
        Label->SetFont(ICFrontendLayout::Font(24));Label->SetJustification(ETextJustify::Center);Label->SetAutoWrapText(false);Button->SetContent(Label);
        auto* S=Destinations->AddChildToVerticalBox(Button);S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(0,6));
    }
    auto* Close=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BTN_MapClose"));
    WarpButtonStyle(Close,FLinearColor(.06f,.09f,.14f));
    auto* Caption=Tree->ConstructWidget<UTextBlock>();Caption->SetFont(ICFrontendLayout::Font(22));Caption->SetText(Local(NSLOCTEXT("ICMap","Close","닫기")));Close->SetContent(Caption);
    auto* C=Fill(Close,FAnchors(1,0),FMargin(-170,35,110,46));C->SetAlignment(FVector2D::ZeroVector);
    auto* Confirm=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("WarpConfirm"));Confirm->SetBrushColor(FLinearColor(0,0,0,.78f));
    Confirm->SetHorizontalAlignment(HAlign_Center);Confirm->SetVerticalAlignment(VAlign_Center);Fill(Confirm)->SetZOrder(40);
    auto* Card=Tree->ConstructWidget<UBorder>();Card->SetBrushColor(FLinearColor(.045f,.065f,.1f,1));Card->SetPadding(FMargin(36));
    auto* CardSize=Tree->ConstructWidget<USizeBox>();CardSize->SetWidthOverride(650);CardSize->SetContent(Card);Confirm->SetContent(CardSize);
    auto* ConfirmContent=Tree->ConstructWidget<UVerticalBox>();Card->SetContent(ConfirmContent);
    auto* Question=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_WarpQuestion"));Question->SetFont(ICFrontendLayout::Font(28));Question->SetJustification(ETextJustify::Center);Question->SetAutoWrapText(true);
    ConfirmContent->AddChildToVerticalBox(Question)->SetPadding(FMargin(0,12,0,32));
    auto* ConfirmButtons=Tree->ConstructWidget<UHorizontalBox>();ConfirmContent->AddChildToVerticalBox(ConfirmButtons);
    for(int32 I=0;I<2;++I)
    {
        auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),I==0?TEXT("BTN_WarpConfirm"):TEXT("BTN_WarpCancel"));
        WarpButtonStyle(Button,I==0?FLinearColor(.04f,.24f,.36f):FLinearColor(.06f,.09f,.14f));
        auto* Label=Tree->ConstructWidget<UTextBlock>();Label->SetFont(ICFrontendLayout::Font(24));Label->SetText(I==0?NSLOCTEXT("ICMap","ConfirmTravel","확인"):NSLOCTEXT("ICMap","CancelTravel","취소"));
        Button->SetContent(Label);auto* S=ConfirmButtons->AddChildToHorizontalBox(Button);S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(8));
    }
    Confirm->SetVisibility(ESlateVisibility::Collapsed);
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
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_MapOverview"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::ToggleOverview);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_WarpConfirm"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::ConfirmTravel);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_WarpCancel"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::CancelTravelConfirmation);
    const FName Actions[]={TEXT("Portal0"),TEXT("Portal1"),TEXT("Portal2"),TEXT("Portal3")};
    for(int32 I=0;I<4;++I)if(auto* B=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Warp%d"),I)))))
    {FScriptDelegate Click;Click.BindUFunction(this,Actions[I]);B->OnClicked.AddUnique(Click);}
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
    bTraveling=false;bConfirming=false;PendingWarp=NAME_None;Super::NativeDestruct();
}
void UICMapWidget::RefreshMap()
{
    if(MapTable.IsNull())MapTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/World/DT_IC_Map.DT_IC_Map")));
    if(PresentationTable.IsNull())PresentationTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_MapPresentation.DT_IC_MapPresentation")));
    if(auto* T=PresentationTable.LoadSynchronous())if(auto* R=T->FindRow<FICMapPresentationRow>(TEXT("Default"),TEXT("Map"),false))Rules=*R;
    if(bMinimap)if(auto* T=MapTable.LoadSynchronous())if(auto* R=T->FindRow<FICMapRow>(Rules.MapID,TEXT("Map"),false);R&&!R->Layers.IsEmpty())
    {Layer=R->Layers[0];MapTexture=Layer.MapTexture.LoadSynchronous();MapMinimumZoom=R->MinimumZoom;MapMaximumZoom=R->MaximumZoom;}
    Portals.Reset();if(auto* World=GetWorld())for(TActorIterator<AICWarpStone> It(World);It;++It)if(!It->WarpID.IsNone())Portals.Add(*It);
    Portals.Sort([](const auto& A,const auto& B){return A->WarpID.LexicalLess(B->WarpID);});
    if(!bRegistered&&Rules.bUnlockPlacedPortals)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->bCreated&&PC->GetWorldState())
    {
        auto Next=PC->GetWorldState()->ExportState();bool Changed=false;
        for(const auto& P:Portals)if(P.IsValid()&&!Next.UnlockedWarps.Contains(P->WarpID)){Next.UnlockedWarps.Add(P->WarpID);Changed=true;}
        bRegistered=!Changed||PC->GetWorldState()->ApplyWorldProgress(Next);
    }
}
void UICMapWidget::RefreshLabels()
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapTitle"))))T->SetText(NSLOCTEXT("ICMap","WarpListTitle","워프 · 목적지 선택"));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapHelp"))))T->SetText(FText::Format(NSLOCTEXT("ICMap","WarpListHelp","목적지 클릭 / ↑↓ 선택 · 확인  |  이동 {0}G · 보유 {1}G\n잠긴 목적지는 현장의 워프돌과 상호작용하면 열립니다."),Rules.TravelCostGold,PC?PC->Gold:0));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapStatus"))))T->SetText(Status);
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapOverview"))))T->SetText(OverviewZoom>1.f?NSLOCTEXT("ICMap","WholeWorld","전체 맵 보기"):NSLOCTEXT("ICMap","WarpArea","워프 지역 보기"));
    const auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    for(int32 I=0;I<4;++I)
    {
        auto* Portal=Portals.IsValidIndex(I)?Portals[I].Get():nullptr;
        if(auto* B=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Warp%d"),I)))))B->SetVisibility(Portal?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(!Portal)continue;
        const bool Unlocked=Player&&Player->PartyWarp&&Player->PartyWarp->IsWarpUnlocked(Portal->WarpID);
        if(auto* B=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Warp%d"),I)))))
            B->SetBackgroundColor(I==SelectedPortal?FLinearColor(.12f,.32f,.48f):FLinearColor(.06f,.09f,.14f));
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(FName(*FString::Printf(TEXT("TXT_Warp%d"),I)))))
        {T->SetText(FText::Format(NSLOCTEXT("ICMap","DestinationLabel","{0}\n{1}"),Portal->InteractionText,Unlocked?NSLOCTEXT("ICMap","Available","이동 가능"):NSLOCTEXT("ICMap","LockedVisit","잠김 · 워프돌에서 활성화")));T->SetColorAndOpacity(Unlocked?FLinearColor(.7f,.95f,1):FLinearColor(.6f,.62f,.68f));}
    }
}

void UICMapWidget::ChoosePortal(int32 Index)
{if(Portals.IsValidIndex(Index)&&Portals[Index].IsValid()){SelectedPortal=Index;RequestTravel(Portals[Index]->WarpID);}}
void UICMapWidget::Portal0(){ChoosePortal(0);}
void UICMapWidget::Portal1(){ChoosePortal(1);}
void UICMapWidget::Portal2(){ChoosePortal(2);}
void UICMapWidget::Portal3(){ChoosePortal(3);}
FVector2D UICMapWidget::WorldToMapUV(FVector P) const
{
    const FVector2D D=FVector2D(P.X,P.Y)-Layer.WorldOriginCm;
    return Layer.MapOriginUV+FVector2D(FVector2D::DotProduct(D,Layer.WorldAxisU)/FMath::Max(1.,Layer.WorldCmPerUV.X),FVector2D::DotProduct(D,Layer.WorldAxisV)/FMath::Max(1.,Layer.WorldCmPerUV.Y));
}
FVector2D UICMapWidget::MinimapDisplayUV(FVector2D MapUV,FVector2D CenterUV,float Zoom)
{return FVector2D(.5,.5)+(MapUV-CenterUV)*FMath::Max(1.f,Zoom);}
FVector2D UICMapWidget::DisplayUV(FVector P) const
{
    const FVector2D MapUV=WorldToMapUV(P);
    return bMinimap?MinimapDisplayUV(MapUV,MinimapCenterUV,FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom)):MinimapDisplayUV(MapUV,OverviewCenterUV,OverviewZoom);
}

void UICMapWidget::FocusWarpArea()
{
    if(Portals.IsEmpty()){OverviewZoom=1.f;OverviewCenterUV=FVector2D(.5);return;}
    FVector2D Min(1),Max(0);
    for(const auto& Portal:Portals)if(Portal.IsValid())
    {const auto UV=WorldToMapUV(Portal->GetActorLocation());Min.X=FMath::Min(Min.X,UV.X);Min.Y=FMath::Min(Min.Y,UV.Y);Max.X=FMath::Max(Max.X,UV.X);Max.Y=FMath::Max(Max.Y,UV.Y);}
    OverviewZoom=FMath::Clamp(.7f/FMath::Max(.01f,float(FMath::Max(Max.X-Min.X,Max.Y-Min.Y))),1.f,8.f);
    const double Half=.5/OverviewZoom;OverviewCenterUV=(Min+Max)*.5;
    OverviewCenterUV.X=FMath::Clamp(OverviewCenterUV.X,Half,1.-Half);OverviewCenterUV.Y=FMath::Clamp(OverviewCenterUV.Y,Half,1.-Half);
}
void UICMapWidget::ToggleOverview()
{if(OverviewZoom>1.f){OverviewZoom=1.f;OverviewCenterUV=FVector2D(.5);}else FocusWarpArea();RefreshLabels();}
FReply UICMapWidget::NativeOnMouseWheel(const FGeometry&,const FPointerEvent& Event)
{
    return FReply::Unhandled();
}
FSlateRect UICMapWidget::ImageRect(const FGeometry& G) const
{
    auto* Surface=GetWidgetFromName(TEXT("MapSurface"));if(!Surface)return FSlateRect();
    // Paint-space geometries share a window origin; cached tick geometry includes the desktop offset.
    const auto& SG=Surface->GetPaintSpaceGeometry();const auto& Root=GetPaintSpaceGeometry();
    const FVector2D A=Root.AbsoluteToLocal(SG.LocalToAbsolute(FVector2D::ZeroVector)),B=Root.AbsoluteToLocal(SG.LocalToAbsolute(SG.GetLocalSize()));
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
    const double Zoom=bMinimap?FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom):OverviewZoom;
    if(bMinimap&&Player)
    {
        const FVector2D Desired=WorldToMapUV(Player->GetActorLocation());const double Half=.5/Zoom;
        MinimapCenterUV={FMath::Clamp(Desired.X,Half,1.-Half),FMath::Clamp(Desired.Y,Half,1.-Half)};
    }
    const auto Rect=ImageRect(G);const FVector2D Size(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
    Image->SetTexture(MapTexture);
    const FVector2D HalfUV(.5/Zoom,.5/Zoom);
    const FVector2D Center=bMinimap?MinimapCenterUV:OverviewCenterUV;
    Image->SetUVRegion(Center-HalfUV,Center+HalfUV);
    if(auto* S=Cast<UCanvasPanelSlot>(Image->Slot)){S->SetPosition({Rect.Left,Rect.Top});S->SetSize(Size);}
    TSet<FName> Active;
    for(int32 I=0;I<Portals.Num();++I)if(auto* Portal=Portals[I].Get())
    {
        Active.Add(Portal->WarpID);auto* Marker=PortalMarkers.FindRef(Portal->WarpID).Get();
        if(!Marker)
        {
            Marker=ICWidgetTemplates::Create(this,TEXT("WBP_IC_MapPortalMarker"));if(!Marker)continue;
            PortalMarkers.Add(Portal->WarpID,Marker);auto* S=Canvas->AddChildToCanvas(Marker);S->SetAutoSize(true);S->SetZOrder(1);
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
        Marker->SetRenderScale(bMinimap?FVector2D(.36f):FVector2D(1));Marker->SetRenderOpacity(bMinimap?MinimapOpacity(UV):1.f);
        CastChecked<UCanvasPanelSlot>(Marker->Slot)->SetPosition(MarkerPosition(Rect,Portal->GetActorLocation()));
    }
    for(auto It=PortalMarkers.CreateIterator();It;++It)if(!Active.Contains(It.Key())){It.Value()->RemoveFromParent();It.RemoveCurrent();}
    if(!PlayerMarker)
    {
        PlayerMarker=ICWidgetTemplates::Create(this,TEXT("WBP_IC_MapPlayerMarker"));
        if(PlayerMarker){auto* S=Canvas->AddChildToCanvas(PlayerMarker);S->SetAutoSize(true);S->SetAlignment({.5f,.5f});S->SetZOrder(2);}
    }
    if(PlayerMarker)
    {
        const auto UV=Player?DisplayUV(Player->GetActorLocation()):FVector2D(-1);
        PlayerMarker->SetVisibility(Player&&UV.X>=0&&UV.X<=1&&UV.Y>=0&&UV.Y<=1?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        if(Player)
        {
            const auto Forward=Player->GetActorForwardVector();const FVector2D Direction(Forward.X*Layer.WorldAxisU.X+Forward.Y*Layer.WorldAxisU.Y,Forward.X*Layer.WorldAxisV.X+Forward.Y*Layer.WorldAxisV.Y);
            PlayerMarker->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Direction.X,-Direction.Y)));
            PlayerMarker->SetRenderScale(bMinimap?FVector2D(.5f):FVector2D(1));PlayerMarker->SetRenderOpacity(bMinimap?MinimapOpacity(UV):1.f);
            CastChecked<UCanvasPanelSlot>(PlayerMarker->Slot)->SetPosition(MarkerPosition(Rect,Player->GetActorLocation()));
        }
    }
}
bool UICMapWidget::RequestTravel(FName ID)
{
    if(bConfirming||bTraveling)return false;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    if(bMinimap||(!GetParent()&&!IsInViewport())||!PC||!Player||!Player->PartyWarp||!PC->bCreated||PC->IsDead()||PC->bInCombat||PC->bInDialogue||PC->bInEvent||PC->Menu!=TEXT("Map")||(Player->LifeActions&&(Player->LifeActions->IsWorking()||Player->LifeActions->IsSleeping())))return false;
    const auto* Portal=Portals.FindByPredicate([ID](const auto& P){return P.IsValid()&&P->WarpID==ID;});if(!Portal)return false;
    FString Reason;if(!Player->PartyWarp->CanWarp(ID,Rules.TravelCostGold,Reason)){Status=Local(FText::FromString(Reason));RefreshLabels();return false;}
    PendingWarp=ID;bConfirming=true;bConfirmSelected=true;Status=FText::GetEmpty();
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_WarpQuestion"))))
        T->SetText(FText::Format(NSLOCTEXT("ICMap","WarpQuestion","{0}\n여기로 이동하겠습니까?\n\n이동 비용: {1}G"),(*Portal)->InteractionText,Rules.TravelCostGold));
    if(auto* W=GetWidgetFromName(TEXT("WarpConfirm")))W->SetVisibility(ESlateVisibility::Visible);
    if(auto* B=GetWidgetFromName(TEXT("BTN_WarpConfirm")))B->SetUserFocus(PC);
    RefreshLabels();return true;
}
void UICMapWidget::CancelTravelConfirmation()
{
    bConfirming=false;PendingWarp=NAME_None;
    if(auto* W=GetWidgetFromName(TEXT("WarpConfirm")))W->SetVisibility(ESlateVisibility::Collapsed);
    if(auto* B=GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Warp%d"),SelectedPortal))))B->SetUserFocus(GetOwningPlayer());
}
void UICMapWidget::ConfirmTravel()
{
    if(!bConfirming)return;const FName Destination=PendingWarp;CancelTravelConfirmation();BeginTravel(Destination);
}
bool UICMapWidget::BeginTravel(FName ID)
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
    if(RefreshElapsed>.25f){RefreshElapsed=0;RefreshMap();RefreshLabels();}
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
void UICMapWidget::CloseMap(){if(bConfirming){CancelTravelConfirmation();return;}if(!bTraveling)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT(""));}
FReply UICMapWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event)
{
    if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return Super::NativeOnMouseButtonDown(G,Event);
    if(bTraveling)return FReply::Handled();
    if(bMinimap){if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->bCreated&&!PC->bInEvent)PC->SetMenu(TEXT("Map"));return FReply::Handled();}
    return Super::NativeOnMouseButtonDown(G,Event);
}
FReply UICMapWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    if(bMinimap)return Super::NativeOnPreviewKeyDown(G,Event);
    if(bTraveling)return FReply::Handled();
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());
    if(bConfirming)
    {
        if(Event.IsRepeat())return FReply::Handled();
        if(Key==EKeys::Escape||Key==EKeys::M){CancelTravelConfirmation();return FReply::Handled();}
        if(Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::A||Key==EKeys::D||Key==EKeys::Tab)
        {bConfirmSelected=!bConfirmSelected;if(auto* W=GetWidgetFromName(bConfirmSelected?TEXT("BTN_WarpConfirm"):TEXT("BTN_WarpCancel")))W->SetUserFocus(GetOwningPlayer());return FReply::Handled();}
        if(Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter||Key==EKeys::SpaceBar){if(bConfirmSelected)ConfirmTravel();else CancelTravelConfirmation();return FReply::Handled();}
        return FReply::Handled();
    }
    if(Key==EKeys::Escape||Key==EKeys::M||Key==EKeys::Gamepad_Special_Left){if(!Event.IsRepeat())CloseMap();return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::S||Key==EKeys::A||Key==EKeys::D||Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::Up||Key==EKeys::Down)
    {if(!Portals.IsEmpty())SelectedPortal=(SelectedPortal+((Key==EKeys::W||Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Up)?Portals.Num()-1:1))%Portals.Num();RefreshLabels();return FReply::Handled();}
    if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar){if(!Event.IsRepeat()&&Portals.IsValidIndex(SelectedPortal)&&Portals[SelectedPortal].IsValid())RequestTravel(Portals[SelectedPortal]->WarpID);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(G,Event);
}
