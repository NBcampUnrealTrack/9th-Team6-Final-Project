#include "UI/Life/ICEnhancementWidget.h"
#include "UI/Framework/ICWidgetTemplates.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Engine/DataTable.h"
namespace ICEnhanceLayout
{
UTextBlock* Label(UWidgetTree* T,FName N,FText Text,int32 Size=18){auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),N);W->SetFont(ICFrontendLayout::Font(Size));W->SetText(Text);W->SetAutoWrapText(true);return W;}
void Set(UUserWidget* W,FName N,FText Text){if(auto* T=Cast<UTextBlock>(W->GetWidgetFromName(N)))T->SetText(Text);}
void Add(UVerticalBox* P,UWidget* W,bool Fill=false){auto* S=P->AddChildToVerticalBox(W);S->SetPadding(FMargin(0,0,0,8));if(Fill)S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));}
FText Name(UICInventoryComponent* I,FName ID){const auto* D=I?I->Find(ID):nullptr;return D?D->DisplayName:FText::FromName(ID);}
UButton* Button(UWidgetTree* T,FName N,FText Text){auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),N);B->SetBackgroundColor(FLinearColor(.12f,.25f,.3f));CastChecked<UButtonSlot>(B->SetContent(Label(T,NAME_None,Text)))->SetPadding(FMargin(12,8));return B;}
}
void UICEnhancementChoiceButton::Choose(){if(Screen.IsValid()){if(WeaponID.IsValid())Screen->SelectWeapon(WeaponID);else Screen->SelectOre(OreID);}}
void UICEnhancementWidget::BuildLayout(UWidgetTree* T)
{
    using namespace ICEnhanceLayout;auto* Pages=T->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("EnhancePages"));T->RootWidget=Pages;
    auto* List=T->ConstructWidget<UVerticalBox>();Pages->AddChild(List);ICEnhanceLayout::Add(List,Label(T,NAME_None,NSLOCTEXT("ICEnhance","PickWeapon","강화할 무기를 선택하세요. 장착 중인 무기도 강화할 수 있습니다.")));
    ICEnhanceLayout::Add(List,T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("EnhanceWeapons")),true);
    auto* Detail=T->ConstructWidget<UVerticalBox>();Pages->AddChild(Detail);
    ICEnhanceLayout::Add(Detail,Label(T,TEXT("EnhanceWeaponName"),FText::GetEmpty(),23));ICEnhanceLayout::Add(Detail,Label(T,TEXT("EnhanceOffer"),FText::GetEmpty()));
    ICEnhanceLayout::Add(Detail,Label(T,NAME_None,NSLOCTEXT("ICEnhance","PickOre","보유한 광석을 선택하세요. 실패해도 표시된 비용은 소모됩니다."),16));
    ICEnhanceLayout::Add(Detail,T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("EnhanceOres")),true);
    ICEnhanceLayout::Add(Detail,Label(T,TEXT("EnhanceResult"),FText::GetEmpty(),17));
    ICEnhanceLayout::Add(Detail,Button(T,TEXT("BTN_AttemptEnhance"),NSLOCTEXT("ICEnhance","Attempt","강화")));
    ICEnhanceLayout::Add(Detail,Button(T,TEXT("BTN_CancelEnhance"),NSLOCTEXT("ICEnhance","Cancel","취소 · 무기 목록")));
}
void UICEnhancementWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_AttemptEnhance"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::Attempt);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_CancelEnhance"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::Cancel);
}
void UICEnhancementWidget::Activate(){bDetails=false;SelectedWeapon.Invalidate();SelectedOre=NAME_None;RefreshWeapons();CastChecked<UWidgetSwitcher>(GetWidgetFromName(TEXT("EnhancePages")))->SetActiveWidgetIndex(0);}
void UICEnhancementWidget::RefreshWeapons()
{
    using namespace ICEnhanceLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* I=PC?PC->GetInventory():nullptr;if(!I)return;
    Weapons=I->GetEnhancementTargets();auto* Scroll=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("EnhanceWeapons")));Scroll->ClearChildren();WeaponIndex=FMath::Clamp(WeaponIndex,0,FMath::Max(0,Weapons.Num()-1));
    for(int32 N=0;N<Weapons.Num();++N)
    {
        const auto& W=Weapons[N];auto* B=ICWidgetTemplates::Create<UICEnhancementChoiceButton>(this,TEXT("WBP_IC_EnhancementChoice"));B->WeaponID=W.Item.InstanceID;B->Screen=this;B->OnClicked.AddDynamic(B,&UICEnhancementChoiceButton::Choose);
        B->SetCaption(FText::Format(NSLOCTEXT("ICEnhance","WeaponRow","{0} +{1} · {2}{3}"),Name(I,W.Item.ItemID),W.Item.Upgrade,W.Location,W.Item.bLocked?NSLOCTEXT("ICEnhance","Locked"," · 잠금"):FText::GetEmpty()));B->SetSelected(N==WeaponIndex);Scroll->AddChild(B);
    }
    if(Weapons.IsEmpty())Scroll->AddChild(ICWidgetTemplates::Text(this,NSLOCTEXT("ICEnhance","NoWeapons","강화할 무기가 없습니다.")));
}
void UICEnhancementWidget::SelectWeapon(FGuid ID)
{
    const int32 N=Weapons.IndexOfByPredicate([ID](const auto& W){return W.Item.InstanceID==ID;});if(N==INDEX_NONE)return;
    WeaponIndex=N;SelectedWeapon=ID;SelectedOre=NAME_None;bDetails=true;ICEnhanceLayout::Set(this,TEXT("EnhanceResult"),FText::GetEmpty());CastChecked<UWidgetSwitcher>(GetWidgetFromName(TEXT("EnhancePages")))->SetActiveWidgetIndex(1);RefreshDetails();
}
void UICEnhancementWidget::SelectOre(FName ID){if(!Ores.Contains(ID))return;SelectedOre=ID;OreIndex=Ores.IndexOfByKey(ID);RefreshDetails();}
void UICEnhancementWidget::RefreshDetails()
{
    using namespace ICEnhanceLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* I=PC?PC->GetInventory():nullptr;if(!I)return;
    Weapons=I->GetEnhancementTargets();const auto* W=Weapons.FindByPredicate([&](const auto& X){return X.Item.InstanceID==SelectedWeapon;});if(!W){Cancel();return;}
    Set(this,TEXT("EnhanceWeaponName"),FText::Format(NSLOCTEXT("ICEnhance","Selected","{0} +{1}"),Name(I,W->Item.ItemID),W->Item.Upgrade));
    Ores.Reset();auto* Scroll=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("EnhanceOres")));Scroll->ClearChildren();
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Equipment/DT_IC_EnhancementOre.DT_IC_EnhancementOre"));
    if(Table)for(const auto& Row:Table->GetRowNames())if(const auto* R=Table->FindRow<FICEnhancementOreRow>(Row,TEXT("Enhance"),false);R&&I->CountItem(R->ItemID)>0)
    {
        Ores.Add(R->ItemID);auto* B=ICWidgetTemplates::Create<UICEnhancementChoiceButton>(this,TEXT("WBP_IC_EnhancementChoice"));B->OreID=R->ItemID;B->Screen=this;B->OnClicked.AddDynamic(B,&UICEnhancementChoiceButton::Choose);
        B->SetCaption(FText::Format(NSLOCTEXT("ICEnhance","OreRow","{0} · 보유 {1}개 · 소모 {2}개 · 확률 +{3}%"),Name(I,R->ItemID),I->CountItem(R->ItemID),R->Count,FText::AsNumber(R->SuccessBonus*100)));B->SetSelected(SelectedOre==R->ItemID);Scroll->AddChild(B);
    }
    double P=0;int32 Cost=0,Count=0;FText Reason;const bool Ready=I->GetEnhancementOffer(SelectedWeapon,SelectedOre,P,Cost,Count,Reason);
    Set(this,TEXT("EnhanceOffer"),Ready?FText::Format(NSLOCTEXT("ICEnhance","Offer","다음 +{0} · 성공 확률 {1}% · {2}G / 보유 {3}G"),W->Item.Upgrade+1,FText::AsNumber(P*100),Cost,PC->Gold):Reason);
    GetWidgetFromName(TEXT("BTN_AttemptEnhance"))->SetIsEnabled(Ready&&!SelectedOre.IsNone()&&PC->Gold>=Cost);
    if(Ores.IsEmpty())Scroll->AddChild(ICWidgetTemplates::Text(this,NSLOCTEXT("ICEnhance","NoOres","가방에 강화용 광석이 없습니다.")));
}
void UICEnhancementWidget::Attempt()
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!bDetails||!PC||PC->Menu!=TEXT("Station")||PC->IsDead()||PC->bInCombat)return;
    bool Success=false;FText Message;PC->GetInventory()->TryEnhanceWeapon(SelectedWeapon,SelectedOre,Success,Message);ICEnhanceLayout::Set(this,TEXT("EnhanceResult"),Message);RefreshDetails();
}
void UICEnhancementWidget::Cancel(){Activate();}
bool UICEnhancementWidget::HandleKey(FKey Key)
{
    if(Key==EKeys::Escape){if(bDetails){Cancel();return true;}return false;}
    if(Key==EKeys::Up||Key==EKeys::W||Key==EKeys::Down||Key==EKeys::S)
    {
        const int32 D=(Key==EKeys::Up||Key==EKeys::W)?-1:1;
        if(!bDetails){WeaponIndex=FMath::Clamp(WeaponIndex+D,0,FMath::Max(0,Weapons.Num()-1));RefreshWeapons();if(auto* S=Cast<UScrollBox>(GetWidgetFromName(TEXT("EnhanceWeapons")));S&&S->GetChildrenCount()>WeaponIndex)S->ScrollWidgetIntoView(S->GetChildAt(WeaponIndex),true);}
        else if(!Ores.IsEmpty()){OreIndex=SelectedOre.IsNone()?0:FMath::Clamp(OreIndex+D,0,Ores.Num()-1);SelectOre(Ores[OreIndex]);}return true;
    }
    if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar){if(bDetails)Attempt();else if(Weapons.IsValidIndex(WeaponIndex))SelectWeapon(Weapons[WeaponIndex].Item.InstanceID);return true;}return false;
}
