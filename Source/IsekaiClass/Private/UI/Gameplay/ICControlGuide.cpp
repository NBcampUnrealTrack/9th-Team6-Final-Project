#include "UI/Gameplay/ICControlGuide.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "InputMappingContext.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
FString Key(const AICPlayerController* PC,EICInputCommand Command,int32 Slot=0)
{
    if(PC&&PC->DefaultMappingContext)
        for(const auto& Binding:PC->CommandActions)if(Binding.Command==Command&&Binding.Slot==Slot)
            for(const auto& Mapping:PC->DefaultMappingContext->GetMappings())
                if(Mapping.Action==Binding.Action&&!Mapping.Key.IsGamepadKey())return Mapping.Key.GetDisplayName().ToString();
    return TEXT("설정의 조작키 참고");
}
}

TArray<FICControlGuidePage> ICControlGuide::Pages(const AICPlayerController* PC)
{
    return {
        {NSLOCTEXT("ICGuide","VitalsTitle","모험을 시작하기 전에"),NSLOCTEXT("ICGuide","Vitals","왼쪽 아래에서 HP·MP·SP를 확인하세요. HP는 생명력, MP는 스킬에 필요한 마력, SP는 생활 활동에 필요한 기력입니다."),TEXT("HUD_PlayerStatus")},
        {NSLOCTEXT("ICGuide","MoveTitle","이동과 카메라"),NSLOCTEXT("ICGuide","Move","W·A·S·D로 이동하고 마우스로 주변을 둘러보세요. Space로 점프, Shift로 대시, Ctrl + 이동키로 구르기를 사용할 수 있습니다. 마우스 휠로 카메라 거리를 조절하세요."),NAME_None},
        {NSLOCTEXT("ICGuide","CombatTitle","무기와 전투"),FText::FromString(FString::Printf(TEXT("%s: 일반 공격  ·  %s: 현재 무기 스킬\n%s: 무기 전환\n검·활·마법 스태프·치유 완드는 서로 다른 공격과 스킬을 사용합니다."),*Key(PC,EICInputCommand::Attack),*Key(PC,EICInputCommand::Skill),*Key(PC,EICInputCommand::SwitchWeapon))),TEXT("HUD_PlayerStatus")},
        {NSLOCTEXT("ICGuide","QuickTitle","필요한 아이템을 빠르게"),NSLOCTEXT("ICGuide","Quick","화면 아래 퀵슬롯에 소비 아이템을 등록해 두세요. 1·2·3·4키로 사용할 수 있습니다. 인벤토리에서는 아이템을 선택해 사용하거나 퀵슬롯에 등록할 수 있어요."),TEXT("HUD_QuickSlots")},
        {NSLOCTEXT("ICGuide","PartyTitle","동료와 함께하는 모험"),FText::FromString(FString::Printf(TEXT("%s: 파티 메뉴\n영입한 동료의 상태가 왼쪽에 표시됩니다. 파티에 편성한 동료는 함께 이동하고 전투하며, 동료 스킬도 직접 사용할 수 있습니다."),*Key(PC,EICInputCommand::Party))),TEXT("HUD_Companions")},
        {NSLOCTEXT("ICGuide","QuestTitle","다음 목적지를 확인하세요"),FText::FromString(FString::Printf(TEXT("%s: 퀘스트 메뉴  ·  %s: 퀘스트 경로 안내\n오른쪽 퀘스트 추적창에서 목표를 확인하고 NPC와 대화하며 이야기를 진행하세요."),*Key(PC,EICInputCommand::Quest),*Key(PC,EICInputCommand::QuestFollow))),TEXT("HUD_QuestTracker")},
        {NSLOCTEXT("ICGuide","MapTitle","지도와 워프돌"),FText::FromString(FString::Printf(TEXT("%s: 전체 지도\n집은 처음부터 워프할 수 있습니다. 마을여관·추억의 다리·폐허 마을은 현장의 워프돌과 상호작용한 뒤 열립니다. 지도에서 열린 목적지를 선택해 동료와 함께 이동하세요."),*Key(PC,EICInputCommand::Map))),TEXT("HUD_Minimap")},
        {NSLOCTEXT("ICGuide","LifeTitle","이제 직접 탐험해 보세요"),FText::FromString(FString::Printf(TEXT("%s: 상호작용  ·  %s: 낚시\nNPC·채집물·워프돌 가까이에서 안내 문구를 확인하세요. %s로 인벤토리, Esc로 메뉴를 열 수 있습니다. 준비됐다면 F를 눌러 안내를 마치세요."),*Key(PC,EICInputCommand::Interact),*Key(PC,EICInputCommand::Fishing),*Key(PC,EICInputCommand::Inventory))),NAME_None}
    };
}

void ICControlGuide::Build(UWidgetTree* Tree,UCanvasPanel* Canvas)
{
    auto* Root=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("HUD_ControlGuide"));
    auto* RootSlot=Canvas->AddChildToCanvas(Root);RootSlot->SetAnchors(FAnchors(0,0,1,1));RootSlot->SetOffsets(FMargin(0));RootSlot->SetZOrder(30);
    Root->SetVisibility(ESlateVisibility::Collapsed);
    auto* Focus=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("GuideFocus"));
    Focus->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent,4.f,FLinearColor(1,.73f,.22f,.9f),3.f));Root->AddChildToCanvas(Focus);
    auto* Card=Tree->ConstructWidget<UBorder>();Card->SetBrushColor(FLinearColor(.025f,.04f,.07f,.96f));Card->SetPadding(FMargin(20));
    auto* CardSlot=Root->AddChildToCanvas(Card);CardSlot->SetAnchors(FAnchors(.5f,1));CardSlot->SetAlignment({.5f,1});CardSlot->SetPosition({0,-190});CardSlot->SetSize({680,218});
    auto* Content=Tree->ConstructWidget<UCanvasPanel>();Card->SetContent(Content);
    const TCHAR* Names[]={TEXT("TXT_GuideTitle"),TEXT("TXT_GuideBody"),TEXT("TXT_GuideNext")};
    for(int32 I=0;I<3;++I)
    {
        auto* Text=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Names[I]);Text->SetFont(ICFrontendLayout::Font(I==0?24:17,I==0));
        Text->SetColorAndOpacity(I==1?FLinearColor(.9f,.93f,1):FLinearColor(1,.8f,.4f));Text->SetAutoWrapText(true);
        auto* S=Content->AddChildToCanvas(Text);S->SetPosition({0,I==0?0.f:I==1?40.f:154.f});S->SetSize({640,I==1?110.f:30.f});
    }
}

bool UICGameplayWidget::IsControlGuideVisible() const
{
    const auto* PC=GetICPlayerController();
    return ControlGuidePage!=INDEX_NONE&&PC&&PC->bCreated&&!PC->IsDead()&&PC->Menu.IsEmpty()&&!PC->bInCombat;
}

void UICGameplayWidget::RefreshControlGuide()
{
    auto* PC=GetICPlayerController();auto* State=PC?PC->GetWorldState():nullptr;
    if(ControlGuidePage==INDEX_NONE&&!bControlGuideFinished&&State&&PC->bCreated&&PC->Menu.IsEmpty())
        if(State->ExportState().Flags.FindRef(TEXT("TUT_CONTROLS_PENDING")))ControlGuidePage=0;
    auto* Root=GetWidgetFromName(TEXT("HUD_ControlGuide"));if(!Root)return;
    Root->SetVisibility(IsControlGuideVisible()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(!IsControlGuideVisible())return;
    const auto Pages=ICControlGuide::Pages(PC);if(!Pages.IsValidIndex(ControlGuidePage))return;
    const auto& Page=Pages[ControlGuidePage];
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_GuideTitle"))))T->SetText(Page.Title);
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_GuideBody"))))T->SetText(Page.Body);
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_GuideNext"))))T->SetText(FText::Format(NSLOCTEXT("ICGuide","Next","{0} / {1}    F · 다음 안내"),ControlGuidePage+1,Pages.Num()));
    auto* Focus=GetWidgetFromName(TEXT("GuideFocus"));auto* Target=Page.HUDTarget.IsNone()?nullptr:GetWidgetFromName(Page.HUDTarget);
    if(Focus)
    {
        const bool HasTarget=Target&&Target->GetCachedGeometry().GetLocalSize().SizeSquared()>1;
        Focus->SetVisibility(HasTarget?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        if(HasTarget)if(auto* S=Cast<UCanvasPanelSlot>(Focus->Slot))
        {S->SetPosition(GetCachedGeometry().AbsoluteToLocal(Target->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector))-FVector2D(5));S->SetSize(Target->GetCachedGeometry().GetLocalSize()+FVector2D(10));}
    }
}

bool UICGameplayWidget::AdvanceControlGuide()
{
    if(!IsControlGuideVisible())return false;
    if(++ControlGuidePage>=ICControlGuide::Pages(GetICPlayerController()).Num())
    {
        if(auto* State=GetICPlayerController()->GetWorldState())
        {auto Next=State->ExportState();Next.Flags.Add(TEXT("TUT_CONTROLS_PENDING"),false);Next.Flags.Add(TEXT("TUT_CONTROLS_DONE"),true);State->ApplyWorldProgress(Next);}
        ControlGuidePage=INDEX_NONE;bControlGuideFinished=true;
    }
    RefreshControlGuide();return true;
}
