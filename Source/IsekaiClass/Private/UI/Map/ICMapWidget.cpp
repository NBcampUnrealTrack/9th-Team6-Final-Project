#include "UI/Map/ICMapWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
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
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Text/ICLocalization.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/World/ICWarpRow.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Rendering/DrawElementTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Rendering/SlateRenderer.h"
#include "Settings/ICGameUserSettings.h"
#include "Styling/CoreStyle.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Warp/ICPartyWarpComponent.h"
#include "World/Warp/ICWarpStone.h"
#include "NPC/Character/ICNPCBase.h"
#include "Enemy/Character/ICEnemyBase.h"

namespace
{
class FICMinimapWheelInput final : public IInputProcessor
{
public:
    explicit FICMinimapWheelInput(UICMapWidget* Widget):Owner(Widget){}
    virtual void Tick(const float,FSlateApplication&,TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event) override
    {
        auto* Widget=Owner.Get();auto* World=Widget?Widget->GetWorld():nullptr;
        auto* Client=World?World->GetGameViewport():nullptr;
        if(!World||!World->IsGameWorld()||!Client||!Client->Viewport||
            !Client->Viewport->HasFocus()||!Client->Viewport->HasMouseCapture())return false;
        return Widget->HandleVillageMapShortcut(Event);
    }
    virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&,const FPointerEvent& Event,const FPointerEvent*) override
    {
        auto* Widget=Owner.Get();
        auto* World=Widget?Widget->GetWorld():nullptr;
        auto* Client=World?World->GetGameViewport():nullptr;
        // Never intercept the editor, another PIE window, or an uncaptured cursor.
        if(!World||!World->IsGameWorld()||!Client||!Client->Viewport||
            !Client->Viewport->HasFocus()||!Client->Viewport->HasMouseCapture())return false;
        return Widget->HandleMinimapWheel(Event);
    }
private:
    TWeakObjectPtr<UICMapWidget> Owner;
};
FText Local(const FText& Text){return ICLocalization::Localize(Text);}
void WarpButtonStyle(UButton* Button,const FLinearColor& Color)
{
    auto Style=Button->GetStyle();
    Style.Normal=FSlateRoundedBoxBrush(FLinearColor::White,6.f);
    Style.Hovered=FSlateRoundedBoxBrush(FLinearColor(.65f,.9f,1.f),6.f);
    Style.Pressed=FSlateRoundedBoxBrush(FLinearColor(.45f,.75f,1.f),6.f);
    Style.Disabled=FSlateRoundedBoxBrush(FLinearColor(.45f,.45f,.45f),6.f);
    Button->SetStyle(Style);Button->SetBackgroundColor(Color);
}
void BuildWarpListLayout(UWidgetTree* Tree)
{
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    auto Fill=[&](UWidget* W,FAnchors A=FAnchors(0,0,1,1),FMargin M=FMargin(0))
    {auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.026f,.045f,.98f));Fill(Background);
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
    auto* Error=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("TXT_WarpConfirmError"));Error->SetFont(ICFrontendLayout::Font(18));Error->SetColorAndOpacity(FLinearColor(1,.55f,.4f));Error->SetJustification(ETextJustify::Center);Error->SetAutoWrapText(true);
    ConfirmContent->AddChildToVerticalBox(Error)->SetPadding(FMargin(0,0,0,16));
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

}

UICMapWidget* UICMapWidget::CreateVillageMap(APlayerController* PC)
{
    auto* Map=CreateWidget<UICMapWidget>(PC,StaticClass());if(!Map)return nullptr;
    Map->bVillageMap=true;Map->bMinimap=false;Map->VillageZoom=1.;
    auto* Tree=Map->WidgetTree.Get();
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    const auto Fill=[&](UWidget* W,FAnchors A,FMargin Offsets)
    {auto* Slot=Root->AddChildToCanvas(W);Slot->SetAnchors(A);Slot->SetOffsets(Offsets);return Slot;};
    auto* Back=Tree->ConstructWidget<UBorder>();Back->SetBrushColor(FLinearColor(.018f,.026f,.045f,.98f));
    Fill(Back,FAnchors(0,0,1,1),FMargin(0));
    auto* Surface=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapSurface"));Surface->SetBrushColor(FLinearColor(.055f,.075f,.095f,1));
    Fill(Surface,FAnchors(.035f,.17f,.965f,.91f),FMargin(0));
    auto* Content=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MapContent"));Content->SetVisibility(ESlateVisibility::HitTestInvisible);
    Fill(Content,FAnchors(0,0,1,1),FMargin(0));
    Content->AddChildToCanvas(Tree->ConstructWidget<UICMapTextureImage>(UICMapTextureImage::StaticClass(),TEXT("IMG_Map")));
    const auto Text=[&](const TCHAR* Name,int32 FontSize,float Y)
    {auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);T->SetFont(ICFrontendLayout::Font(FontSize));Fill(T,FAnchors(.035f,0,.85f,0),FMargin(0,Y,0,42));return T;};
    Text(TEXT("TXT_MapTitle"),32,30);Text(TEXT("TXT_MapHelp"),18,86);
    auto* Close=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BTN_MapClose"));WarpButtonStyle(Close,FLinearColor(.08f,.2f,.27f));
    auto* Caption=Tree->ConstructWidget<UTextBlock>();Caption->SetFont(ICFrontendLayout::Font(22));Caption->SetText(NSLOCTEXT("ICMap","Close","닫기"));Close->SetContent(Caption);
    Fill(Close,FAnchors(1,0),FMargin(-170,35,110,46));Close->OnClicked.AddUniqueDynamic(Map,&UICMapWidget::CloseMap);
    Map->RefreshMap();Map->RefreshLabels();return Map;
}

void UICMapWidget::BuildLayout(UWidgetTree* Tree,bool Mini)
{
    if(!Mini){BuildWarpListLayout(Tree);return;}
    auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;
    auto Fill=[&](UWidget* W,FAnchors A=FAnchors(0,0,1,1),FMargin M=FMargin(0))
    {auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.018f,.026f,.045f,Mini?.82f:.98f));Fill(Background);
    auto* Surface=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MapSurface"));Surface->SetBrushColor(FLinearColor(.055f,.075f,.095f,Mini?.88f:1.f));
    Fill(Surface,Mini?FAnchors(0,0,1,1):FAnchors(.035f,.17f,.965f,.87f));
    auto* Content=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MapContent"));Content->SetVisibility(ESlateVisibility::HitTestInvisible);Fill(Content);
    auto* Image=Tree->ConstructWidget<UICMapTextureImage>(UICMapTextureImage::StaticClass(),TEXT("IMG_Map"));Image->bFadeEdges=false;Content->AddChildToCanvas(Image);
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
}
void UICMapWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    // Blueprint defaults can be reset when the native parent gains new properties.
    // The minimap-only help widget is a stable runtime signal, so do not let the
    // HUD silently fall back to full-map coordinates when that happens.
    bMinimap=bMinimap||GetWidgetFromName(TEXT("TXT_MiniHelp"))!=nullptr||GetClass()->GetName().Contains(TEXT("Minimap"));
    SetIsFocusable(!bMinimap);
    if(bMinimap)if(auto* Help=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MiniHelp"))))Help->SetText(NSLOCTEXT("ICMap","VillageShortcut","N 지도 · M 워프"));

    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_MapClose"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::CloseMap);
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
    if(bMinimap&&!MinimapWheelInput&&FSlateApplication::IsInitialized())
    {
        MinimapWheelInput=MakeShared<FICMinimapWheelInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(MinimapWheelInput,0);
    }
}
void UICMapWidget::NativeDestruct()
{
    EndVillageDrag();
    if(MinimapWheelInput&&FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(MinimapWheelInput);
    MinimapWheelInput.Reset();
    if(bTraveling)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->bInEvent=false;
    bTraveling=false;bConfirming=false;Super::NativeDestruct();
}
void UICMapWidget::RefreshMap()
{
    if(MapTable.IsNull())MapTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/World/DT_IC_Map.DT_IC_Map")));
    if(PresentationTable.IsNull())PresentationTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_MapPresentation.DT_IC_MapPresentation")));
    if(auto* T=PresentationTable.LoadSynchronous())if(auto* R=T->FindRow<FICMapPresentationRow>(TEXT("Default"),TEXT("Map"),false))Rules=*R;
    const FICMapRow* MapRow=nullptr;
    ActiveLevelMap=NAME_None;
    const FName LevelName(*UGameplayStatics::GetCurrentLevelName(this,true));
    if(auto* T=LevelMapTable.LoadSynchronous())
        if(const auto* R=T->FindRow<FICMapRow>(LevelName,TEXT("Level map"),false);R&&!R->Layers.IsEmpty())
        {MapRow=R;ActiveLevelMap=LevelName;}
    if(!MapRow)if(auto* T=MapTable.LoadSynchronous())MapRow=T->FindRow<FICMapRow>(Rules.MapID,TEXT("Map"),false);
    Layer=FICMapLayerDefinition();MapTexture=nullptr;MapMinimumZoom=1.;MapMaximumZoom=4.;
    if(MapRow&&!MapRow->Layers.IsEmpty())
    {
        Layer=MapRow->Layers[0];MapTexture=Layer.MapTexture.LoadSynchronous();
        MapMinimumZoom=FMath::Max(1.,MapRow->MinimumZoom);MapMaximumZoom=FMath::Max(MapMinimumZoom,MapRow->MaximumZoom);
    }
    Portals.Reset();if(auto* World=GetWorld())for(TActorIterator<AICWarpStone> It(World);It;++It)if(!It->WarpID.IsNone())Portals.Add(*It);
    Portals.Sort([](const auto& A,const auto& B){return A->WarpID.LexicalLess(B->WarpID);});
    SelectedPortal=FMath::Clamp(SelectedPortal,0,FMath::Max(0,Portals.Num()-1));
    Buildings.Reset();NPCs.Reset();Monsters.Reset();
    if(bMinimap||bVillageMap)if(auto* World=GetWorld())
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
FText UICMapWidget::DestinationName(FName ID) const
{
    if(auto* GI=GetGameInstance())if(auto* D=GI->GetSubsystem<UICDefinitionSubsystem>())
    {
        FICDefinitionResult Result;
        if(const auto* Row=D->Find<FICWarpRow>(ID,Result);Row&&!Row->NameKey.IsEmpty())
            return Local(FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Row->NameKey));
    }
    return FText::FromName(ID);
}
void UICMapWidget::RefreshLabels()
{
    if(bMinimap)return;
    if(bVillageMap)
    {
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapTitle"))))T->SetText(NSLOCTEXT("ICMap","VillageTitle","마을 지도"));
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapHelp"))))T->SetText(FText::Format(NSLOCTEXT("ICMap","VillageDragHelp","휠: 확대·축소 · 좌클릭 드래그: 이동 · N / Esc: 닫기 · M: 워프  |  {0}배"),FText::AsNumber(VillageZoom)));
        return;
    }
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    const auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    const int32 PlayerGold=PC&&PC->GetWorldState()?PC->GetWorldState()->ExportState().Gold:0;
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapTitle"))))T->SetText(NSLOCTEXT("ICMap","WarpListTitle","워프 · 목적지 선택"));
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MapHelp"))))
        T->SetText(FText::Format(NSLOCTEXT("ICMap","WarpListHelp","목적지 클릭 / ↑↓ 선택 · 확인  |  이동 {0}G · 보유 {1}G\n잠긴 목적지는 현장의 워프돌과 상호작용하면 열립니다."),Rules.TravelCostGold,PlayerGold));
    for(const auto* Name:{TEXT("TXT_MapStatus"),TEXT("TXT_WarpConfirmError")})
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(Name)))T->SetText(Status);
    for(int32 I=0;I<4;++I)
    {
        auto* Portal=Portals.IsValidIndex(I)?Portals[I].Get():nullptr;
        auto* Button=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Warp%d"),I))));
        if(Button)Button->SetVisibility(Portal?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(!Portal)continue;
        const bool Unlocked=Player&&Player->PartyWarp&&Player->PartyWarp->IsWarpUnlocked(Portal->WarpID);
        if(Button)
        {
            Button->SetIsEnabled(Unlocked&&!bConfirming&&!bTraveling);
            Button->SetBackgroundColor(I==SelectedPortal?FLinearColor(.12f,.32f,.48f):FLinearColor(.06f,.09f,.14f));
        }
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(FName(*FString::Printf(TEXT("TXT_Warp%d"),I)))))
        {
            T->SetText(FText::Format(NSLOCTEXT("ICMap","DestinationLabel","{0}\n{1}"),DestinationName(Portal->WarpID),Unlocked?NSLOCTEXT("ICMap","Available","이동 가능"):NSLOCTEXT("ICMap","LockedVisit","잠김 · 워프돌에서 활성화")));
            T->SetColorAndOpacity(Unlocked?FLinearColor(.7f,.95f,1):FLinearColor(.6f,.62f,.68f));
        }
    }
    if(bConfirming)
    {
        const FText Name=DestinationName(PendingWarp);const FString String=Name.ToString();
        const TCHAR Last=String.IsEmpty()?0:String[String.Len()-1];
        const int32 Final=Last>=0xac00&&Last<=0xd7a3?(Last-0xac00)%28:0;
        const FText Particle=FText::FromString(Final!=0&&Final!=8?TEXT("으로"):TEXT("로"));
        if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_WarpQuestion"))))
            T->SetText(FText::Format(NSLOCTEXT("ICMap","WarpNamedQuestion","{0}{1} 이동하겠습니까?\n\n이동 비용: {2}G · 보유 {3}G"),Name,Particle,Rules.TravelCostGold,PlayerGold));
        if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_WarpConfirm"))))B->SetBackgroundColor(bConfirmSelected?FLinearColor(.04f,.24f,.36f):FLinearColor(.06f,.09f,.14f));
        if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_WarpCancel"))))B->SetBackgroundColor(!bConfirmSelected?FLinearColor(.04f,.24f,.36f):FLinearColor(.06f,.09f,.14f));
    }
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
    return bMinimap?MinimapDisplayUV(MapUV,MinimapCenterUV,FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom)):bVillageMap?MinimapDisplayUV(MapUV,VillageCenterUV,VillageZoom):MapUV;
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
    if(!bMinimap&&!bVillageMap)return;
    auto* Canvas=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("MapContent")));
    auto* Image=Cast<UICMapTextureImage>(GetWidgetFromName(TEXT("IMG_Map")));if(!Canvas||!Image)return;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    const double Zoom=bMinimap?FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom):bVillageMap?VillageZoom:1.;
    if(bMinimap&&Player)
    {
        // The image and every marker use the same player-centred UV window.
        MinimapCenterUV=WorldToMapUV(Player->GetActorLocation());
    }
    const auto Rect=ImageRect(G);const FVector2D Size(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
    // Saved minimap blueprints may still contain the old edge-fade setting.
    if(Image->bFadeEdges){Image->bFadeEdges=false;Image->SynchronizeProperties();}
    Image->SetTexture(MapTexture);
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    const FVector2D HalfUV(.5/Zoom,.5/Zoom);
    const FVector2D Center=bMinimap?MinimapCenterUV:VillageCenterUV;
    Image->SetUVRegion(Center-HalfUV,Center+HalfUV);
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
        const bool Selected=!bMinimap&&!bVillageMap&&I==SelectedPortal;
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
    if(bMinimap||bVillageMap)
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
void UICMapWidget::ChoosePortal(int32 Index)
{if(Portals.IsValidIndex(Index)&&Portals[Index].IsValid()){SelectedPortal=Index;RequestTravel(Portals[Index]->WarpID);}}
void UICMapWidget::Portal0(){ChoosePortal(0);}
void UICMapWidget::Portal1(){ChoosePortal(1);}
void UICMapWidget::Portal2(){ChoosePortal(2);}
void UICMapWidget::Portal3(){ChoosePortal(3);}
bool UICMapWidget::RequestTravel(FName ID)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    if(bMinimap||bVillageMap||bConfirming||bTraveling||(!GetParent()&&!IsInViewport())||!PC||!Player||!Player->PartyWarp||!PC->bCreated||PC->IsDead()||PC->bInCombat||PC->bInDialogue||PC->bInEvent||PC->Menu!=TEXT("Map")||(Player->LifeActions&&(Player->LifeActions->IsWorking()||Player->LifeActions->IsSleeping())))return false;
    const int32 Index=Portals.IndexOfByPredicate([ID](const auto& P){return P.IsValid()&&P->WarpID==ID;});
    if(Index==INDEX_NONE||!Player->PartyWarp->IsWarpUnlocked(ID))return false;
    PendingWarp=ID;SelectedPortal=Index;bConfirming=true;bConfirmSelected=true;Status=FText::GetEmpty();
    if(auto* W=GetWidgetFromName(TEXT("WarpConfirm")))W->SetVisibility(ESlateVisibility::Visible);
    RefreshLabels();SetUserFocus(PC);return true;
}
void UICMapWidget::CancelTravelConfirmation()
{
    if(!bConfirming)return;
    bConfirming=false;PendingWarp=NAME_None;Status=FText::GetEmpty();
    if(auto* W=GetWidgetFromName(TEXT("WarpConfirm")))W->SetVisibility(ESlateVisibility::Collapsed);
    RefreshLabels();SetUserFocus(GetOwningPlayer());
}
void UICMapWidget::ConfirmTravel()
{
    if(!bConfirming)return;
    if(BeginTravel(PendingWarp))
    {
        bConfirming=false;
        if(auto* W=GetWidgetFromName(TEXT("WarpConfirm")))W->SetVisibility(ESlateVisibility::Collapsed);
        RefreshLabels();
    }
    else if(Status.IsEmpty())
    {Status=NSLOCTEXT("ICMap","TravelUnavailable","현재 상태에서는 이동할 수 없습니다.");RefreshLabels();}
}
bool UICMapWidget::BeginTravel(FName ID)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;
    if(bMinimap||bVillageMap||bTraveling||(!GetParent()&&!IsInViewport())||!PC||!Player||!Player->PartyWarp||!PC->bCreated||PC->IsDead()||PC->bInCombat||PC->bInDialogue||PC->bInEvent||PC->Menu!=TEXT("Map")||(Player->LifeActions&&(Player->LifeActions->IsWorking()||Player->LifeActions->IsSleeping())))return false;
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
void UICMapWidget::CloseMap(){if(bVillageMap){EndVillageDrag();if(auto* PC=GetOwningPlayer())if(auto* LocalPlayer=PC->GetLocalPlayer())LocalPlayer->GetSubsystem<UICUIManagerSubsystem>()->ToggleVillageMap();return;}if(bConfirming){CancelTravelConfirmation();return;}if(!bTraveling)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT(""));}
bool UICMapWidget::HandleVillageMapShortcut(const FKeyEvent& Event)
{
    if(Event.GetKey()!=EKeys::N||Event.IsAltDown()||Event.IsControlDown()||Event.IsCommandDown()||!bMinimap||!IsVisible()||!GetIsEnabled())return false;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!PC||!PC->IsLocalController()||!PC->GetLocalPlayer()||!PC->Menu.IsEmpty()||!PC->bCreated||!PC->GetPawn()||PC->IsDead()||PC->bInEvent||PC->bInDialogue)return false;
    if(Event.IsRepeat())return true;
    return PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->ToggleVillageMap();
}
void UICMapWidget::ClampVillageCenter()
{
    const double Half=.5/FMath::Max(1.,VillageZoom);
    VillageCenterUV.X=FMath::Clamp(VillageCenterUV.X,Half,1.-Half);
    VillageCenterUV.Y=FMath::Clamp(VillageCenterUV.Y,Half,1.-Half);
}
void UICMapWidget::EndVillageDrag()
{
    bDraggingVillageMap=false;
    if(HasMouseCaptureByUser(VillageDragUser)&&FSlateApplication::IsInitialized())
        FSlateApplication::Get().ReleaseAllPointerCapture(VillageDragUser);
}
FReply UICMapWidget::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& Event)
{
    if(!bVillageMap||!bDraggingVillageMap)return Super::NativeOnMouseMove(G,Event);
    if(!Event.IsMouseButtonDown(EKeys::LeftMouseButton))
    {bDraggingVillageMap=false;return FReply::Handled().ReleaseMouseCapture();}
    const auto Rect=ImageRect(G);const FVector2D Size(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
    const FVector2D Current=Event.GetScreenSpacePosition();
    if(Size.X>0&&Size.Y>0)
    {
        // Convert screen motion through geometry for DPI scaling. The paper follows the hand.
        const FVector2D Delta=G.AbsoluteToLocal(Current)-G.AbsoluteToLocal(VillageDragLastScreen);
        VillageCenterUV-=Delta/(Size*VillageZoom);
        ClampVillageCenter();RefreshVisuals(G);
    }
    VillageDragLastScreen=Current;
    return FReply::Handled();
}
FReply UICMapWidget::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& Event)
{
    if(bVillageMap&&bDraggingVillageMap&&Event.GetEffectingButton()==EKeys::LeftMouseButton)
    {bDraggingVillageMap=false;return FReply::Handled().ReleaseMouseCapture();}
    return Super::NativeOnMouseButtonUp(G,Event);
}
void UICMapWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    bDraggingVillageMap=false;
    Super::NativeOnMouseCaptureLost(Event);
}
FReply UICMapWidget::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& Event)
{
    if(!bVillageMap)return Super::NativeOnMouseWheel(G,Event);
    const float Delta=Event.GetWheelDelta();
    const auto Rect=ImageRect(G);const FVector2D Size(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
    const FVector2D Mouse=G.AbsoluteToLocal(Event.GetScreenSpacePosition());
    if(FMath::IsFinite(Delta)&&Size.X>0&&Size.Y>0&&Rect.ContainsPoint(Mouse))
    {
        const double Next=FMath::Clamp(VillageZoom*FMath::Pow(1.25,FMath::Clamp(double(Delta),-32.,32.)),1.,FMath::Max(1.,MapMaximumZoom));
        const FVector2D LocalUV=(Mouse-FVector2D(Rect.Left,Rect.Top))/Size;
        VillageCenterUV+=(LocalUV-FVector2D(.5,.5))*(1./VillageZoom-1./Next);
        VillageZoom=Next;ClampVillageCenter();
        if(VillageZoom<=1.)EndVillageDrag();
        RefreshVisuals(G);RefreshLabels();
    }
    return FReply::Handled();
}

bool UICMapWidget::HandleMinimapWheel(const FPointerEvent& Event)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    const float Delta=Event.GetWheelDelta();
    if(!Event.IsAltDown()||Event.IsControlDown()||Event.IsCommandDown()||!FMath::IsFinite(Delta)||FMath::IsNearlyZero(Delta)||
        !bMinimap||!IsVisible()||!GetIsEnabled()||!PC||!PC->IsLocalController()||!PC->bCreated||!PC->GetPawn()||
        PC->IsDead()||PC->IsPaused()||!PC->Menu.IsEmpty()||PC->bInEvent||PC->bInDialogue||bTraveling||bConfirming)return false;
    const double Current=FMath::Clamp(double(MinimapZoom),MapMinimumZoom,MapMaximumZoom);
    MinimapZoom=static_cast<float>(FMath::Clamp(Current*FMath::Pow(1.25,FMath::Clamp(double(Delta),-32.,32.)),MapMinimumZoom,MapMaximumZoom));
    RefreshVisuals(GetPaintSpaceGeometry());
    // Consume even at the limit, so an extra wheel notch cannot move the camera.
    return true;
}

FReply UICMapWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event)
{
    if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return Super::NativeOnMouseButtonDown(G,Event);
    if(bVillageMap)
    {
        const auto Rect=ImageRect(G);
        if(VillageZoom>1.&&Rect.Right>Rect.Left&&Rect.Bottom>Rect.Top&&Rect.ContainsPoint(G.AbsoluteToLocal(Event.GetScreenSpacePosition())))
        {
            bDraggingVillageMap=true;VillageDragUser=Event.GetUserIndex();VillageDragLastScreen=Event.GetScreenSpacePosition();
            return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget(),EFocusCause::Mouse);
        }
        return FReply::Handled();
    }

    if(bTraveling||bConfirming)return FReply::Handled();
    if(bMinimap){if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->bCreated&&!PC->bInEvent)PC->SetMenu(TEXT("Map"));return FReply::Handled();}
    return Super::NativeOnMouseButtonDown(G,Event);
}
FReply UICMapWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    if(bVillageMap)
    {
        if(Event.GetKey()==EKeys::N||Event.GetKey()==EKeys::Escape||Event.GetKey()==EKeys::M)
        {
            if(!Event.IsRepeat()){CloseMap();if(Event.GetKey()==EKeys::M)if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT("Map"));}
            return FReply::Handled();
        }
        return Super::NativeOnPreviewKeyDown(G,Event);
    }
    if(bMinimap)return Super::NativeOnPreviewKeyDown(G,Event);
    if(bTraveling)return FReply::Handled();
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());
    if(bConfirming)
    {
        if(Event.IsRepeat())return FReply::Handled();
        if(Key==EKeys::Escape||Key==EKeys::M||Key==EKeys::Gamepad_Special_Left){CancelTravelConfirmation();return FReply::Handled();}
        if(Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::A||Key==EKeys::D||Key==EKeys::Up||Key==EKeys::Down||Key==EKeys::Tab)
        {bConfirmSelected=!bConfirmSelected;RefreshLabels();return FReply::Handled();}
        if(Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter||Key==EKeys::SpaceBar){if(bConfirmSelected)ConfirmTravel();else CancelTravelConfirmation();return FReply::Handled();}
        return FReply::Handled();
    }
    if(Key==EKeys::Escape||Key==EKeys::M||Key==EKeys::Gamepad_Special_Left){if(!Event.IsRepeat())CloseMap();return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::S||Key==EKeys::A||Key==EKeys::D||Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::Up||Key==EKeys::Down)
    {if(!Portals.IsEmpty())SelectedPortal=(SelectedPortal+((Key==EKeys::W||Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Up)?Portals.Num()-1:1))%Portals.Num();RefreshLabels();return FReply::Handled();}
    if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar){if(!Event.IsRepeat()&&Portals.IsValidIndex(SelectedPortal)&&Portals[SelectedPortal].IsValid())RequestTravel(Portals[SelectedPortal]->WarpID);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(G,Event);
}
