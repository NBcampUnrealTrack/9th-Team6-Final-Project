#include "UI/Map/ICMapWidget.h"
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

namespace { FText Local(const FText& Text){return ICLocalization::Localize(Text);} }

void UICMapWidget::BuildLayout(UWidgetTree* Tree,bool Mini)
{
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    auto Fill=[&](UWidget* W,FAnchors A=FAnchors(0,0,1,1),FMargin M=FMargin(0))
    {auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.026f,.045f,Mini?0.f:.98f));Fill(Background);
    auto* Surface=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapSurface"));Surface->SetBrushColor(FLinearColor(.045f,.06f,.08f,Mini?0.f:1.f));
    Fill(Surface,Mini?FAnchors(0,0,1,1):FAnchors(.035f,.17f,.965f,.87f));
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
    Super::NativeOnInitialized();SetIsFocusable(!bMinimap);
    if(!WidgetTree->RootWidget)BuildLayout(WidgetTree,bMinimap);
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
    {Layer=R->Layers[0];MapTexture=Layer.MapTexture.LoadSynchronous();}
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
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapTitle"))))T->SetText(Local(NSLOCTEXT("ICMap","Title","테스트맵 · 포탈 이동")));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapHelp"))))T->SetText(FText::Format(Local(NSLOCTEXT("ICMap","Help","포탈 클릭 / 방향키 선택 + 확인: {0}G로 이동 · 보유 {1}G · M / B: 닫기")),Rules.TravelCostGold,PC?PC->Gold:0));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapStatus"))))T->SetText(Status);
}
FVector2D UICMapWidget::WorldToMapUV(FVector P) const
{
    const FVector2D D=FVector2D(P.X,P.Y)-Layer.WorldOriginCm;
    return Layer.MapOriginUV+FVector2D(FVector2D::DotProduct(D,Layer.WorldAxisU)/FMath::Max(1.,Layer.WorldCmPerUV.X),FVector2D::DotProduct(D,Layer.WorldAxisV)/FMath::Max(1.,Layer.WorldCmPerUV.Y));
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
{const auto UV=WorldToMapUV(P);return {R.Left+UV.X*(R.Right-R.Left),R.Top+UV.Y*(R.Bottom-R.Top)};}
float UICMapWidget::MinimapOpacity(FVector2D UV)
{
    const double X=FMath::Abs(UV.X*2-1),Y=FMath::Abs(UV.Y*2-1);
    return FMath::Square(FMath::Clamp(1.-X*X,0.,1.))*FMath::Square(FMath::Clamp(1.-Y*Y,0.,1.));
}
int32 UICMapWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool Enabled) const
{
    int32 Top=Super::NativePaint(A,G,Clip,E,L,Style,Enabled);if(bTraveling)return Top;
    const FSlateRect R=ImageRect(G);const FVector2D Size(R.Right-R.Left,R.Bottom-R.Top);if(Size.X<1||Size.Y<1)return Top;
    FSlateBrush Brush;Brush.SetResourceObject(MapTexture);Brush.DrawAs=ESlateBrushDrawType::Image;
    if(MapTexture&&bMinimap)
    {
        // Vertex alpha fades the existing map itself; no generated mask texture is needed.
        constexpr int32 Columns=32,Rows=16;
        TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
        const auto Transform=G.GetAccumulatedRenderTransform();
        const float Opacity=Style.GetColorAndOpacityTint().A;
        for(int32 Y=0;Y<=Rows;++Y)for(int32 X=0;X<=Columns;++X)
        {
            const FVector2D UV(double(X)/Columns,double(Y)/Rows);
            const FColor Color(255,255,255,FMath::RoundToInt(255*MinimapOpacity(UV)*Opacity));
            Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,FVector2f(R.Left+UV.X*Size.X,R.Top+UV.Y*Size.Y),FVector2f(UV),Color));
        }
        for(int32 Y=0;Y<Rows;++Y)for(int32 X=0;X<Columns;++X)
        {
            const SlateIndex I=Y*(Columns+1)+X;
            Indices.Append({I,SlateIndex(I+1),SlateIndex(I+Columns+1),SlateIndex(I+1),SlateIndex(I+Columns+2),SlateIndex(I+Columns+1)});
        }
        FSlateDrawElement::MakeCustomVerts(E,++Top,FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush),Vertices,Indices,nullptr,0,0);
    }
    else if(MapTexture)FSlateDrawElement::MakeBox(E,++Top,G.ToPaintGeometry(Size,FSlateLayoutTransform(FVector2D(R.Left,R.Top))),&Brush);
    if(bMinimap)
    {
        const auto* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        FSlateDrawElement::MakeBox(E,++Top,G.ToPaintGeometry(FVector2D(74,20),FSlateLayoutTransform(FVector2D(4,2))),White,ESlateDrawEffect::None,FLinearColor(0,0,0,.75f));
        FSlateDrawElement::MakeText(E,++Top,G.ToPaintGeometry(FVector2D(80,20),FSlateLayoutTransform(FVector2D(8,3))),Local(NSLOCTEXT("ICMap","MiniHelp","M · 지도")),ICFrontendLayout::Font(11),ESlateDrawEffect::None,FLinearColor::White);
    }
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    const auto* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    for(int32 I=0;I<Portals.Num();++I)if(const auto* P=Portals[I].Get())
    {
        const auto UV=WorldToMapUV(P->GetActorLocation());if(UV.X<0||UV.X>1||UV.Y<0||UV.Y>1)continue;
        const auto Pos=MarkerPosition(R,P->GetActorLocation());const bool Unlocked=Player&&Player->PartyWarp&&Player->PartyWarp->IsWarpUnlocked(P->WarpID);
        FLinearColor Color=Unlocked?FLinearColor(.25f,.85f,1):FLinearColor(.4f,.4f,.45f);if(bMinimap)Color.A=MinimapOpacity(UV);const float Radius=bMinimap?4.f:11.f;
        FSlateDrawElement::MakeBox(E,++Top,G.ToPaintGeometry(FVector2D(Radius*2),FSlateLayoutTransform(Pos-FVector2D(Radius))),White,ESlateDrawEffect::None,I==SelectedPortal&&!bMinimap?FLinearColor(1,.8f,.25f):Color);
        if(!bMinimap)FSlateDrawElement::MakeText(E,++Top,G.ToPaintGeometry(FVector2D(180,30),FSlateLayoutTransform(Pos+FVector2D(17,-14))),Local(P->InteractionText),ICFrontendLayout::Font(20),ESlateDrawEffect::None,FLinearColor::White);
    }
    if(Player)
    {
        const auto UV=WorldToMapUV(Player->GetActorLocation());if(UV.X>=0&&UV.X<=1&&UV.Y>=0&&UV.Y<=1)
        {
            const auto Pos=MarkerPosition(R,Player->GetActorLocation());const auto F=Player->GetActorForwardVector();FVector2D Dir(F.X*Layer.WorldAxisU.X+F.Y*Layer.WorldAxisU.Y,F.X*Layer.WorldAxisV.X+F.Y*Layer.WorldAxisV.Y);Dir.Normalize();const FVector2D Side(-Dir.Y,Dir.X);const float N=bMinimap?6.f:13.f;
            TArray<FVector2D> Points={Pos+Dir*N,Pos-Dir*N*.6+Side*N*.65,Pos-Dir*N*.6-Side*N*.65,Pos+Dir*N};
            FSlateDrawElement::MakeLines(E,++Top,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(.4f,1,.4f,bMinimap?MinimapOpacity(UV):1.f),true,bMinimap?2.f:3.f);
        }
    }
    return Top;
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
    if(RefreshElapsed>.25f){RefreshElapsed=0;RefreshMap();RefreshLabels();}
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
    if(Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::Up||Key==EKeys::Down)
    {if(!Portals.IsEmpty())SelectedPortal=(SelectedPortal+((Key==EKeys::Left||Key==EKeys::Up)?Portals.Num()-1:1))%Portals.Num();return FReply::Handled();}
    if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar){if(!Event.IsRepeat()&&Portals.IsValidIndex(SelectedPortal)&&Portals[SelectedPortal].IsValid())RequestTravel(Portals[SelectedPortal]->WarpID);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(G,Event);
}
