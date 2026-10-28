#include "UI/Life/ICLifeWidgets.h"
#include "Settings/ICGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "NPC/LifeNPC/ICShopSubsystem.h"
#include "Codex/ICCodexSubsystem.h"
#include "Life/World/ICLifeActor.h"
#include "Life/Production/ICGatheringSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"

namespace ICExpandedLifeUI
{
UTextBlock* LifeLabel(UWidgetTree* Tree,FName Name,FText Value,int32 Font=20)
{auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Label->SetText(Value);Label->SetFont(ICFrontendLayout::Font(Font));Label->SetColorAndOpacity(FLinearColor::White);Label->SetAutoWrapText(true);return Label;}
void LifeCaption(UUserWidget* Widget,FName Name,FText Value){if(auto* Label=Cast<UTextBlock>(Widget->GetWidgetFromName(Name)))Label->SetText(Value);}
void LifeRow(UVerticalBox* Column,UWidget* Child,float Gap=10){Column->AddChildToVerticalBox(Child)->SetPadding(FMargin(0,0,0,Gap));}
UButton* LifeButton(UWidgetTree* Tree,FName Name,FText Value)
{auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name);Button->SetContent(LifeLabel(Tree,NAME_None,Value));Button->SetBackgroundColor(FLinearColor(.15f,.25f,.35f));return Button;}
UVerticalBox* LifePanel(UWidgetTree* Tree,float Width,float Height)
{
    auto* Root=Tree->ConstructWidget<UOverlay>();Tree->RootWidget=Root;
    auto* Shade=Tree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.7f));Root->AddChildToOverlay(Shade)->SetHorizontalAlignment(HAlign_Fill);
    auto* Body=Tree->ConstructWidget<UBorder>();Body->SetBrushColor(FLinearColor(.045f,.07f,.1f));Body->SetPadding(FMargin(24));
    auto* Box=Tree->ConstructWidget<USizeBox>();Box->SetWidthOverride(Width);Box->SetHeightOverride(Height);Box->SetContent(Body);
    auto* Fit=Tree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetStretchDirection(EStretchDirection::DownOnly);Fit->SetContent(Box);
    auto* Place=Root->AddChildToOverlay(Fit);Place->SetHorizontalAlignment(HAlign_Fill);Place->SetVerticalAlignment(VAlign_Fill);
    auto* Column=Tree->ConstructWidget<UVerticalBox>();Body->SetContent(Column);return Column;
}
UICUIManagerSubsystem* LifeUI(UUserWidget* Widget){return Widget->GetOwningLocalPlayer()?Widget->GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>():nullptr;}
FText LifeItem(UUserWidget* Widget,FName ID){return Widget->GetGameInstance()->GetSubsystem<UICCodexSubsystem>()->ItemName(ID);}
}
void UICLifeChoiceButton::Configure(int32 Index,FText Caption)
{
    Choice=Index;auto* Label=NewObject<UTextBlock>(this);Label->SetText(Caption);Label->SetFont(ICFrontendLayout::Font(20));Label->SetAutoWrapText(true);SetContent(Label);
    SetBackgroundColor(FLinearColor(.13f,.22f,.31f));OnClicked.AddUniqueDynamic(this,&ThisClass::Choose);
}
bool UICSleepFadeWidget::BuildLayout(UWidgetTree* Tree)
{auto* Black=Tree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Tree->RootWidget=Black;return true;}
void UICSleepFadeWidget::NativeOnInitialized(){Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);}

bool UICSeedPickerWidget::BuildLayout(UWidgetTree* Tree)
{
    using namespace ICExpandedLifeUI;auto* Body=LifePanel(Tree,700,570);
    LifeRow(Body,LifeLabel(Tree,NAME_None,NSLOCTEXT("ICFarm","ChooseSeed","심을 씨앗 선택"),28));
    LifeRow(Body,LifeLabel(Tree,TEXT("TXT_SeedInfo"),NSLOCTEXT("ICFarm","SeedInfo","씨앗 1개를 사용해 이 칸의 중앙에 심습니다.")));
    auto* List=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("SeedList"));Body->AddChildToVerticalBox(List)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LifeRow(Body,LifeButton(Tree,TEXT("BTN_SeedCancel"),NSLOCTEXT("ICFarm","SeedCancel","취소")),0);return true;
}
void UICSeedPickerWidget::NativeOnInitialized()
{Super::NativeOnInitialized();SetIsFocusable(true);if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_SeedCancel")))->OnClicked.AddUniqueDynamic(this,&ThisClass::ClosePicker);}
bool UICSeedPickerWidget::Open(AICPlayerController* Player,AICLifeActor* Plot)
{
    if(!Player||!Plot||!Player->GetLocalPlayer())return false;
    auto* Type=LoadClass<UICSeedPickerWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SeedPicker.WBP_IC_SeedPicker_C"));
    auto* Widget=CreateWidget<UICSeedPickerWidget>(Player,Type?Type:StaticClass());if(!Widget)return false;
    Widget->Target=Plot;auto* UI=ICExpandedLifeUI::LifeUI(Widget);if(!UI||!UI->OpenScreen(Widget,true,180))return false;Widget->Refresh();Widget->SetKeyboardFocus();return true;
}
void UICSeedPickerWidget::Refresh()
{
    using namespace ICExpandedLifeUI;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC)return;
    auto* Definitions=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();Definitions->GetDefinitionIDs(EICDefinitionType::Crop,CropIDs);CropIDs.Sort(FNameLexicalLess());
    auto* List=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("SeedList")));List->ClearChildren();FICDefinitionResult R;
    for(int32 Index=0;Index<CropIDs.Num();++Index)if(const auto* Crop=Definitions->Find<FICCropRow>(CropIDs[Index],R))
    {
        const int32 Count=PC->GetInventory()->CountItem(Crop->SeedItemID);auto* Button=WidgetTree->ConstructWidget<UICLifeChoiceButton>();
        Button->Configure(Index,FText::Format(NSLOCTEXT("ICFarm","SeedCount","{0}   보유 {1}개"),LifeItem(this,Crop->SeedItemID),FText::AsNumber(Count)));
        Button->SetIsEnabled(Count>0);Button->OnChoice.AddUniqueDynamic(this,&ThisClass::PickSeed);List->AddChild(Button);
    }
}
void UICSeedPickerWidget::PickSeed(int32 Index)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC||!Target.IsValid()||!CropIDs.IsValidIndex(Index))return;
    FICDefinitionResult Lookup;const auto* Definition=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICCropRow>(CropIDs[Index],Lookup);
    if(!Definition||PC->GetInventory()->CountItem(Definition->SeedItemID)<1)return;
    TWeakObjectPtr<AICLifeActor> Plot=Target;TWeakObjectPtr<AICPlayerController> Player=PC;const FName Crop=CropIDs[Index];ClosePicker();
    PC->GetWorldTimerManager().SetTimerForNextTick([Plot,Player,Crop](){if(Plot.IsValid()&&Player.IsValid())Player->GetGameInstance()->GetSubsystem<UICGatheringSubsystem>()->StartPlant(Plot.Get(),Player.Get(),Crop);});
}
void UICSeedPickerWidget::ClosePicker(){if(auto* UI=ICExpandedLifeUI::LifeUI(this))UI->CloseScreen(this);else RemoveFromParent();}
FReply UICSeedPickerWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());if(Key==EKeys::Escape){ClosePicker();return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::S||Key==EKeys::Down)
    {Selected=FMath::Clamp(Selected+(Key==EKeys::S||Key==EKeys::Down?1:-1),0,FMath::Max(0,CropIDs.Num()-1));auto* List=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("SeedList")));if(List->GetChildAt(Selected)){List->ScrollWidgetIntoView(List->GetChildAt(Selected));List->GetChildAt(Selected)->SetKeyboardFocus();}return FReply::Handled();}
    if(Key==EKeys::Enter&&!Event.IsRepeat()){PickSeed(Selected);return FReply::Handled();}return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

bool UICShopWidget::BuildLayout(UWidgetTree* Tree)
{
    using namespace ICExpandedLifeUI;auto* Body=LifePanel(Tree,950,730);
    LifeRow(Body,LifeLabel(Tree,NAME_None,NSLOCTEXT("ICShop","Title","씨앗 상점"),30));
    LifeRow(Body,LifeLabel(Tree,TEXT("TXT_ShopGold"),FText::GetEmpty()));
    auto* Tabs=Tree->ConstructWidget<UHorizontalBox>();LifeRow(Body,Tabs);
    Tabs->AddChildToHorizontalBox(LifeButton(Tree,TEXT("BTN_BuyTab"),NSLOCTEXT("ICShop","BuyTab","구매")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Tabs->AddChildToHorizontalBox(LifeButton(Tree,TEXT("BTN_SellTab"),NSLOCTEXT("ICShop","SellTab","판매")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Items=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ShopItems"));Body->AddChildToVerticalBox(Items)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LifeRow(Body,LifeLabel(Tree,NAME_None,NSLOCTEXT("ICShop","Cart","장바구니 · 클릭하면 1개씩 제외"),22),4);
    auto* Cart=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ShopCart"));auto* CartBox=Tree->ConstructWidget<USizeBox>();CartBox->SetHeightOverride(135);CartBox->SetContent(Cart);LifeRow(Body,CartBox);
    LifeRow(Body,LifeLabel(Tree,TEXT("TXT_ShopTotal"),FText::GetEmpty()));LifeRow(Body,LifeLabel(Tree,TEXT("TXT_ShopNotice"),FText::GetEmpty(),17));
    auto* Actions=Tree->ConstructWidget<UHorizontalBox>();LifeRow(Body,Actions);
    Actions->AddChildToHorizontalBox(LifeButton(Tree,TEXT("BTN_Checkout"),NSLOCTEXT("ICShop","CheckoutBuy","구매하기")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Actions->AddChildToHorizontalBox(LifeButton(Tree,TEXT("BTN_ShopCancel"),NSLOCTEXT("ICShop","Cancel","취소하고 나가기")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LifeRow(Body,LifeLabel(Tree,NAME_None,NSLOCTEXT("ICShop","Keys","W/S 선택 · Enter 담기 · A/D 탭 · F 결제 · ESC 취소"),16),0);return true;
}
void UICShopWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_BuyTab")))->OnClicked.AddUniqueDynamic(this,&ThisClass::BuyTab);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_SellTab")))->OnClicked.AddUniqueDynamic(this,&ThisClass::SellTab);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Checkout")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Checkout);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_ShopCancel")))->OnClicked.AddUniqueDynamic(this,&ThisClass::CloseShop);
}
bool UICShopWidget::Open(AICPlayerController* Player,FName ServiceID)
{
    if(!Player||!Player->GetLocalPlayer())return false;FString Failure;
    if(!Player->GetGameInstance()->GetSubsystem<UICShopSubsystem>()->CanUseShop(ServiceID,Failure)){Player->Show(Failure);return false;}
    auto* Type=LoadClass<UICShopWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Shop.WBP_IC_Shop_C"));auto* Widget=CreateWidget<UICShopWidget>(Player,Type?Type:StaticClass());if(!Widget)return false;
    Player->SetMenu(TEXT(""));Widget->Service=ServiceID;auto* UI=ICExpandedLifeUI::LifeUI(Widget);if(!UI||!UI->OpenScreen(Widget,true,180))return false;Widget->Refresh();Widget->SetKeyboardFocus();return true;
}
void UICShopWidget::SelectTab(bool Sell){if(bSubmitting)return;bSelling=Sell;Selected=0;Cart.Reset();ICExpandedLifeUI::LifeCaption(this,TEXT("TXT_ShopNotice"),FText::GetEmpty());Refresh();}
void UICShopWidget::Refresh()
{
    using namespace ICExpandedLifeUI;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC)return;auto* Shop=GetGameInstance()->GetSubsystem<UICShopSubsystem>();
    Items.Reset();Prices.Reset();auto* List=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("ShopItems")));List->ClearChildren();TArray<FICShopOffer> Offers;FString Failure;
    if(!Shop->GetOffers(Service,Offers,Failure)){LifeCaption(this,TEXT("TXT_ShopNotice"),FText::FromString(Failure));return;}
    if(bSelling)
    {
        for(const auto& ItemSlot:PC->GetInventory()->GetSnapshot().Bag.Slots)if(!ItemSlot.IsEmpty()&&!Items.Contains(ItemSlot.ItemID))
            if(const auto* Item=PC->GetInventory()->Find(ItemSlot.ItemID);Item&&Item->bCanSell&&Item->SellPrice>0&&Item->Kind!=EPlayerItemKind::Quest&&Shop->GetSellableCount(PC,ItemSlot.ItemID)>0){Items.Add(ItemSlot.ItemID);Prices.Add(ItemSlot.ItemID,Item->SellPrice);}
    }
    else for(const auto& Offer:Offers){Items.Add(Offer.ItemID);Prices.Add(Offer.ItemID,Offer.FinalPrice);}
    for(int32 Index=0;Index<Items.Num();++Index)
    {
        auto* Button=WidgetTree->ConstructWidget<UICLifeChoiceButton>();Button->Configure(Index,FText::Format(NSLOCTEXT("ICShop","OfferLine","{0}   {1} G   보유 {2}개"),LifeItem(this,Items[Index]),FText::AsNumber(Prices.FindRef(Items[Index])),FText::AsNumber(PC->GetInventory()->CountItem(Items[Index]))));
        Button->OnChoice.AddUniqueDynamic(this,&ThisClass::ItemPicked);List->AddChild(Button);
    }
    auto* CartList=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("ShopCart")));CartList->ClearChildren();int64 Total=0;
    for(int32 Index=0;Index<Cart.Num();++Index)
    {
        const auto& Entry=Cart[Index];Total+=int64(Prices.FindRef(Entry.ItemID))*Entry.Count;auto* Button=WidgetTree->ConstructWidget<UICLifeChoiceButton>();
        Button->Configure(10000+Index,FText::Format(NSLOCTEXT("ICShop","CartLine","{0} × {1}   {2} G"),LifeItem(this,Entry.ItemID),FText::AsNumber(Entry.Count),FText::AsNumber(int64(Prices.FindRef(Entry.ItemID))*Entry.Count)));
        Button->OnChoice.AddUniqueDynamic(this,&ThisClass::ItemPicked);CartList->AddChild(Button);
    }
    LifeCaption(this,TEXT("TXT_ShopGold"),FText::Format(NSLOCTEXT("ICShop","Gold","보유 골드 {0} G"),FText::AsNumber(PC->Gold)));
    LifeCaption(this,TEXT("TXT_ShopTotal"),FText::Format(NSLOCTEXT("ICShop","Total","합계 {0} G"),FText::AsNumber(Total)));
    auto* Submit=CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Checkout")));Submit->SetIsEnabled(!Cart.IsEmpty()&&(bSelling||Total<=PC->Gold));
    if(auto* Caption=Cast<UTextBlock>(Submit->GetContent()))Caption->SetText(bSelling?NSLOCTEXT("ICShop","CheckoutSell","판매하기"):NSLOCTEXT("ICShop","CheckoutBuy","구매하기"));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_BuyTab")))->SetBackgroundColor(!bSelling?FLinearColor(.55f,.4f,.13f):FLinearColor(.13f,.22f,.31f));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_SellTab")))->SetBackgroundColor(bSelling?FLinearColor(.55f,.4f,.13f):FLinearColor(.13f,.22f,.31f));
    SetKeyboardFocus();
}
void UICShopWidget::ItemPicked(int32 Index)
{
    if(bSubmitting)return;
    if(Index>=10000){const int32 Cell=Index-10000;if(Cart.IsValidIndex(Cell)&&--Cart[Cell].Count<=0)Cart.RemoveAt(Cell);Refresh();return;}
    if(!Items.IsValidIndex(Index))return;Selected=Index;const FName ID=Items[Index];auto* Entry=Cart.FindByPredicate([&](const auto& Value){return Value.ItemID==ID;});
    if(Entry&&Entry->Count>=999)return;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(bSelling&&GetGameInstance()->GetSubsystem<UICShopSubsystem>()->GetSellableCount(PC,ID)<(Entry?Entry->Count:0)+1)return;
    if(Entry)++Entry->Count;else{FICItemAmount Line;Line.ItemID=ID;Line.Count=1;Cart.Add(Line);}Refresh();
}
bool UICShopWidget::SubmitCart()
{
    if(bSubmitting||Cart.IsEmpty())return false;TGuardValue<bool> Guard(bSubmitting,true);FText Reason;const bool Done=GetGameInstance()->GetSubsystem<UICShopSubsystem>()->Checkout(Cast<AICPlayerController>(GetOwningPlayer()),Service,Cart,bSelling,Reason);
    if(Done)Cart.Reset();Refresh();ICExpandedLifeUI::LifeCaption(this,TEXT("TXT_ShopNotice"),Done?NSLOCTEXT("ICShop","Done","거래가 완료됐습니다."):Reason);return Done;
}
void UICShopWidget::CloseShop(){if(bSubmitting)return;Cart.Reset();if(auto* UI=ICExpandedLifeUI::LifeUI(this))UI->CloseScreen(this);else RemoveFromParent();}
FReply UICShopWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());if(Key==EKeys::Escape){CloseShop();return FReply::Handled();}
    if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::D||Key==EKeys::Right){SelectTab(Key==EKeys::D||Key==EKeys::Right);return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::S||Key==EKeys::Down){Selected=FMath::Clamp(Selected+(Key==EKeys::S||Key==EKeys::Down?1:-1),0,FMath::Max(0,Items.Num()-1));auto* List=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("ShopItems")));if(List->GetChildrenCount()>Selected){List->ScrollWidgetIntoView(List->GetChildAt(Selected));List->GetChildAt(Selected)->SetKeyboardFocus();}return FReply::Handled();}
    if(Key==EKeys::Enter&&!Event.IsRepeat()){ItemPicked(Selected);return FReply::Handled();}if((Key==EKeys::F||Event.GetKey()==UICGameUserSettings::GamepadFaceKey(0))&&!Event.IsRepeat()){SubmitCart();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

