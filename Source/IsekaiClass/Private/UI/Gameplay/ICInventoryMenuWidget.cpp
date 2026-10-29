#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "Settings/ICGameUserSettings.h"
#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "UI/Codex/ICCodexMenuWidget.h"
#include "Codex/ICCodexSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Internationalization/StringTable.h"

namespace
{
const FLinearColor Gold(1,.74f,.3f), InventoryButtonColor(.16f,.21f,.3f);
FName Indexed(const TCHAR* Prefix,int32 I){return FName(*FString::Printf(TEXT("%s%d"),Prefix,I));}
UTextBlock* InventoryLabel(UWidgetTree* T,FName Name,FText Text,int32 Size=16)
{
    auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);W->SetText(Text);
    W->SetFont(ICFrontendLayout::Font(Size,Size>=24));W->SetColorAndOpacity(FLinearColor(.92f,.94f,.98f));
    W->SetVisibility(ESlateVisibility::HitTestInvisible);return W;
}
USizeBox* Size(UWidgetTree* T,UWidget* Child,float W=0,float H=0)
{
    auto* Box=T->ConstructWidget<USizeBox>();if(W>0)Box->SetWidthOverride(W);if(H>0)Box->SetHeightOverride(H);Box->SetContent(Child);return Box;
}
void InventoryRow(UVerticalBox* Box,UWidget* Child,float Gap=6)
{Box->AddChildToVerticalBox(Child)->SetPadding(FMargin(0,0,0,Gap));}
UButton* InventoryButton(UWidgetTree* T,FName Name,FText Text,bool WithCaption=true)
{
    auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),Name);auto Style=B->GetStyle();
    for(auto* Brush:{&Style.Normal,&Style.Hovered,&Style.Pressed})
    {Brush->DrawAs=ESlateBrushDrawType::Box;Brush->Margin=FMargin(0);Brush->TintColor=FLinearColor::White;}
    B->SetStyle(Style);B->SetBackgroundColor(InventoryButtonColor);
    if(!WithCaption)return B;
    auto* Caption=InventoryLabel(T,FName(*(Name.ToString()+TEXT("_Text"))),Text);Caption->SetJustification(ETextJustify::Center);
    auto* S=Cast<UButtonSlot>(B->SetContent(Caption));S->SetPadding(FMargin(10,7));return B;
}
UClass* WidgetClass(const TCHAR* Asset,UClass* Fallback)
{
    const FString Path=FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"),Asset,Asset);
    auto* C=LoadClass<UUserWidget>(nullptr,*Path);return C;
}
void Caption(UUserWidget* W,const TCHAR* Name,FText Text)
{if(auto* T=Cast<UTextBlock>(W->GetWidgetFromName(Name)))T->SetText(Text);}
}

bool UICInventorySlotWidget::BuildLayout(UWidgetTree* T)
{
    auto* B=InventoryButton(T,TEXT("BTN_Select"),FText::GetEmpty(),false);T->RootWidget=Size(T,B,72,92);
    auto* Column=T->ConstructWidget<UVerticalBox>();B->SetContent(Column);CastChecked<UButtonSlot>(Column->Slot)->SetPadding(FMargin(3));
    auto* Overlay=T->ConstructWidget<UOverlay>();InventoryRow(Column,Size(T,Overlay,64,64),3);
    auto* Image=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Icon"));Image->SetVisibility(ESlateVisibility::Hidden);
    auto* IS=Overlay->AddChildToOverlay(Image);IS->SetHorizontalAlignment(HAlign_Fill);IS->SetVerticalAlignment(VAlign_Fill);
    auto* Count=InventoryLabel(T,TEXT("TXT_Count"),FText::GetEmpty(),14);Count->SetShadowOffset(FVector2D(1,1));Count->SetShadowColorAndOpacity(FLinearColor::Black);
    auto* CS=Overlay->AddChildToOverlay(Count);CS->SetHorizontalAlignment(HAlign_Right);CS->SetVerticalAlignment(VAlign_Bottom);
    auto* Name=InventoryLabel(T,TEXT("TXT_Name"),FText::GetEmpty(),12);Name->SetJustification(ETextJustify::Center);Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    InventoryRow(Column,Size(T,Name,64,18),0);return true;
}
void UICInventorySlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Name=CastChecked<UTextBlock>(GetWidgetFromName(TEXT("TXT_Name")));
    Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);Name->SetClipping(EWidgetClipping::ClipToBounds);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Select")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Pick);
}
void UICInventorySlotWidget::Present(int32 Index,FText Name,UTexture2D* Icon,int32 Count,bool Selected)
{
    CellIndex=Index;SetToolTipText(Name);Caption(this,TEXT("TXT_Name"),Name);
    Caption(this,TEXT("TXT_Count"),Count>0?FText::AsNumber(Count):FText::GetEmpty());
    auto* Image=CastChecked<UImage>(GetWidgetFromName(TEXT("IMG_Icon")));Image->SetBrushFromTexture(Icon);
    Image->SetVisibility(Icon?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Select")))->SetBackgroundColor(Selected?Gold:InventoryButtonColor);
}
void UICInventorySlotWidget::Pick(){OnPicked.Broadcast(CellIndex);}

bool UICItemConfirmWidget::BuildLayout(UWidgetTree* T)
{
    auto* Shade=T->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.88f));Shade->SetPadding(FMargin(18));T->RootWidget=Shade;
    Shade->SetHorizontalAlignment(HAlign_Center);Shade->SetVerticalAlignment(VAlign_Center);
    auto* Panel=T->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.07f,.09f,.13f));Panel->SetPadding(FMargin(24));Shade->SetContent(Size(T,Panel,550));
    auto* Column=T->ConstructWidget<UVerticalBox>();Panel->SetContent(Column);
    auto* Message=InventoryLabel(T,TEXT("TXT_Message"),FText::GetEmpty(),20);Message->SetAutoWrapText(true);Message->SetJustification(ETextJustify::Center);InventoryRow(Column,Message,24);
    auto* Actions=T->ConstructWidget<UHorizontalBox>();InventoryRow(Column,Actions,0);
    for(const auto* Key:{TEXT("Accept"),TEXT("Cancel")})
    {
        auto* B=InventoryButton(T,FName(*(FString(TEXT("BTN_"))+Key)),Key==FString(TEXT("Accept"))?NSLOCTEXT("ICInventory","Accept","수락"):NSLOCTEXT("ICInventory","Cancel","취소"));
        auto* S=Actions->AddChildToHorizontalBox(B);S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(4,0));
    }
    return true;
}
void UICItemConfirmWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Accept")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Accept);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Cancel")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Cancel);
}
void UICItemConfirmWidget::Present(FText Message){Choice=1;Caption(this,TEXT("TXT_Message"),Message);SetVisibility(ESlateVisibility::Visible);RefreshChoice();}
void UICItemConfirmWidget::RefreshChoice()
{
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Accept")))->SetBackgroundColor(Choice==0?Gold:InventoryButtonColor);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Cancel")))->SetBackgroundColor(Choice==1?Gold:InventoryButtonColor);
}
bool UICItemConfirmWidget::HandleKey(FKey Key,bool Repeat)
{
    if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Left||Key==EKeys::Gamepad_DPad_Up)Choice=0;
    else if(Key==EKeys::D||Key==EKeys::Right||Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Right||Key==EKeys::Gamepad_DPad_Down)Choice=1;
    else if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key)){if(!Repeat)OnResolved.Broadcast(Choice==0);}
    else if(Key==EKeys::Escape||Key==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(Key)){if(!Repeat)Cancel();}
    else return false;
    RefreshChoice();return true;
}
void UICItemConfirmWidget::Accept(){OnResolved.Broadcast(true);}
void UICItemConfirmWidget::Cancel(){OnResolved.Broadcast(false);}

bool UICInventoryItemPopupWidget::BuildLayout(UWidgetTree* T)
{
    auto* Shade=T->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.88f));Shade->SetPadding(FMargin(18));T->RootWidget=Shade;
    Shade->SetHorizontalAlignment(HAlign_Center);Shade->SetVerticalAlignment(VAlign_Center);
    auto* Frame=T->ConstructWidget<UBorder>();Frame->SetPadding(FMargin(24));Frame->SetBrushColor(FLinearColor(.055f,.08f,.12f));Shade->SetContent(Size(T,Frame,620));
    auto* Column=T->ConstructWidget<UVerticalBox>();Frame->SetContent(Column);auto* Header=T->ConstructWidget<UHorizontalBox>();InventoryRow(Column,Header,18);
    auto* Image=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Item"));Header->AddChildToHorizontalBox(Size(T,Image,88,88))->SetPadding(FMargin(0,0,18,0));
    auto* Name=InventoryLabel(T,TEXT("TXT_Name"),FText::GetEmpty(),25);Name->SetAutoWrapText(true);auto* NamePlacement=Header->AddChildToHorizontalBox(Name);NamePlacement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NamePlacement->SetVerticalAlignment(VAlign_Center);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("DescriptionScroll"));auto* Limit=T->ConstructWidget<USizeBox>();Limit->SetMaxDesiredHeight(180);Limit->SetContent(Scroll);InventoryRow(Column,Limit,14);
    auto* Description=InventoryLabel(T,TEXT("TXT_Description"),FText::GetEmpty(),18);Description->SetAutoWrapText(true);Scroll->AddChild(Description);
    auto* Message=InventoryLabel(T,TEXT("TXT_Message"),FText::GetEmpty(),15);Message->SetAutoWrapText(true);Message->SetColorAndOpacity(Gold);InventoryRow(Column,Message,12);
    auto* Quantity=T->ConstructWidget<USpinBox>(USpinBox::StaticClass(),TEXT("SplitQuantity"));Quantity->SetMinValue(1);Quantity->SetDelta(1);Quantity->SetMinFractionalDigits(0);Quantity->SetMaxFractionalDigits(0);Quantity->SetVisibility(ESlateVisibility::Collapsed);InventoryRow(Column,Quantity,8);
    Quantity->SetToolTipText(NSLOCTEXT("ICInventory","SplitCount","분리할 개수"));
    auto* Actions=T->ConstructWidget<UWrapBox>();Actions->SetInnerSlotPadding(FVector2D(6,6));InventoryRow(Column,Actions,0);
    const TCHAR* Keys[]={TEXT("BTN_Equip"),TEXT("BTN_Use"),TEXT("BTN_Quick"),TEXT("BTN_Lock"),TEXT("BTN_Split"),TEXT("BTN_Trash"),TEXT("BTN_Back")};
    const FText Names[]={NSLOCTEXT("ICInventory","Equip","장비 착용"),NSLOCTEXT("ICInventory","Use","사용"),NSLOCTEXT("ICInventory","Quick","퀵슬롯 설정"),NSLOCTEXT("ICInventory","Lock","잠금"),NSLOCTEXT("ICInventory","Split","묶음 분할"),NSLOCTEXT("ICInventory","Trash","휴지통"),NSLOCTEXT("ICInventory","Back","뒤로가기")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Keys);++I)Actions->AddChildToWrapBox(InventoryButton(T,Keys[I],Names[I]));return true;
}
void UICInventoryItemPopupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Equip")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Equip);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Lock")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Lock);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Split")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Split);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Use")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Use);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Quick")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Quick);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Trash")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Trash);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Back")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);
}
void UICInventoryItemPopupWidget::Present(FText Name,FText Description,UTexture2D* Icon,EPlayerItemKind Kind,bool CanDiscard,bool CanBind,bool CanUse)
{
    Caption(this,TEXT("TXT_Name"),Name);Caption(this,TEXT("TXT_Description"),Description);
    auto* Image=CastChecked<UImage>(GetWidgetFromName(TEXT("IMG_Item")));Image->SetBrushFromTexture(Icon);Image->SetVisibility(Icon?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    Buttons.Reset();if(Kind==EPlayerItemKind::Equipment)Buttons.Add(TEXT("BTN_Equip"));else if(Kind==EPlayerItemKind::Consumable||Kind==EPlayerItemKind::Expansion){if(CanUse)Buttons.Add(TEXT("BTN_Use"));Buttons.Add(TEXT("BTN_Quick"));}
    Buttons.Append({TEXT("BTN_Trash"),TEXT("BTN_Back")});
    for(const auto* Key:{TEXT("BTN_Equip"),TEXT("BTN_Use"),TEXT("BTN_Quick"),TEXT("BTN_Lock"),TEXT("BTN_Split"),TEXT("BTN_Trash"),TEXT("BTN_Back")})GetWidgetFromName(Key)->SetVisibility(Buttons.Contains(FName(Key))?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("BTN_Trash"))->SetIsEnabled(CanDiscard);GetWidgetFromName(TEXT("BTN_Quick"))->SetIsEnabled(CanBind);
    ActionIndex=FMath::Clamp(ActionIndex,0,Buttons.Num()-1);RefreshSelection();
}
void UICInventoryItemPopupWidget::SetMessage(FText Message)
{Caption(this,TEXT("TXT_Message"),Message);GetWidgetFromName(TEXT("TXT_Message"))->SetVisibility(Message.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}
void UICInventoryItemPopupWidget::ConfigureStack(bool Locked,int32 Count,bool AllowLock,bool AllowSplit)
{
    GetWidgetFromName(TEXT("BTN_Equip"))->SetIsEnabled(!Locked);GetWidgetFromName(TEXT("BTN_Use"))->SetIsEnabled(!Locked);
    if(AllowLock)Buttons.Insert(TEXT("BTN_Lock"),FMath::Max(0,Buttons.Num()-2));
    if(AllowSplit&&!Locked&&Count>1)Buttons.Insert(TEXT("BTN_Split"),FMath::Max(0,Buttons.Num()-2));
    for(const auto* Key:{TEXT("BTN_Lock"),TEXT("BTN_Split")})GetWidgetFromName(Key)->SetVisibility(Buttons.Contains(FName(Key))?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Lock"))))if(auto* T=Cast<UTextBlock>(B->GetContent()))T->SetText(Locked?NSLOCTEXT("ICInventory","Unlock","잠금 해제"):NSLOCTEXT("ICInventory","Lock","잠금"));
    auto* Q=CastChecked<USpinBox>(GetWidgetFromName(TEXT("SplitQuantity")));Q->SetVisibility(AllowSplit&&!Locked&&Count>1?ESlateVisibility::Visible:ESlateVisibility::Collapsed);Q->SetMaxValue(FMath::Max(1,Count-1));Q->SetMaxSliderValue(FMath::Max(1,Count-1));if(Q->GetValue()<1||Q->GetValue()>=Count)Q->SetValue(FMath::Max(1,Count/2));
    RefreshSelection();
}
void UICInventoryItemPopupWidget::Lock(){OnLock.Broadcast();}void UICInventoryItemPopupWidget::Split(){OnSplit.Broadcast();}
void UICInventoryItemPopupWidget::RefreshSelection()
{for(int32 I=0;I<Buttons.Num();++I)CastChecked<UButton>(GetWidgetFromName(Buttons[I]))->SetBackgroundColor(I==ActionIndex?Gold:InventoryButtonColor);}
bool UICInventoryItemPopupWidget::HandleKey(FKey Key,bool Repeat)
{
    if(Key==EKeys::Gamepad_LeftShoulder||Key==EKeys::Gamepad_RightShoulder||Key==EKeys::Add||Key==EKeys::Subtract)
    {if(auto* Q=Cast<USpinBox>(GetWidgetFromName(TEXT("SplitQuantity")));Q&&Q->IsVisible())Q->SetValue(FMath::Clamp(Q->GetValue()+((Key==EKeys::Gamepad_RightShoulder||Key==EKeys::Add)?1:-1),Q->GetMinValue(),Q->GetMaxValue()));return true;}
    if(Key==EKeys::Escape||Key==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(Key)){if(!Repeat)Back();return true;}
    if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left)ActionIndex=FMath::Max(0,ActionIndex-1);
    else if(Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right)ActionIndex=FMath::Min(Buttons.Num()-1,ActionIndex+1);
    else if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::S||Key==EKeys::Down)
    {auto* Scroll=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("DescriptionScroll")));Scroll->SetScrollOffset(FMath::Clamp(Scroll->GetScrollOffset()+(Key==EKeys::S||Key==EKeys::Down?60:-60),0.f,Scroll->GetScrollOffsetOfEnd()));}
    else if((Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key)){if(!Repeat&&Buttons.IsValidIndex(ActionIndex)){auto* B=CastChecked<UButton>(GetWidgetFromName(Buttons[ActionIndex]));if(B->GetIsEnabled())B->OnClicked.Broadcast();}}
    else return false;RefreshSelection();return true;
}
void UICInventoryItemPopupWidget::Equip(){OnEquip.Broadcast();}void UICInventoryItemPopupWidget::Use(){OnUse.Broadcast();}void UICInventoryItemPopupWidget::Quick(){OnQuick.Broadcast();}void UICInventoryItemPopupWidget::Trash(){OnTrash.Broadcast();}void UICInventoryItemPopupWidget::Back(){OnBack.Broadcast();}

bool UICInventoryMenuWidget::BuildLayout(UWidgetTree* T)
{
    auto* Root=T->ConstructWidget<UOverlay>();T->RootWidget=Root;
    auto* Column=T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("InventoryBody"));
    auto* BodyPlacement=Root->AddChildToOverlay(Column);BodyPlacement->SetHorizontalAlignment(HAlign_Fill);BodyPlacement->SetVerticalAlignment(VAlign_Fill);
    auto* Top=T->ConstructWidget<UHorizontalBox>();auto* TS=Column->AddChildToVerticalBox(Top);TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TS->SetPadding(FMargin(0,0,0,10));
    auto* Roster=T->ConstructWidget<UVerticalBox>();auto* RS=Top->AddChildToHorizontalBox(Size(T,Roster,112));RS->SetPadding(FMargin(0,0,18,0));
    auto* PortraitClass=WidgetClass(TEXT("WBP_IC_PartyPortrait"),UICPartyPortraitWidget::StaticClass());
    for(int32 I=0;I<4;++I)
    {
        auto* P=T->ConstructWidget<UICPartyPortraitWidget>(PortraitClass,Indexed(TEXT("Member"),I));
        auto* S=Roster->AddChildToVerticalBox(Size(T,P,80,80));S->SetHorizontalAlignment(HAlign_Center);S->SetPadding(FMargin(0,0,0,8));
    }
    auto* Gear=T->ConstructWidget<UVerticalBox>();auto* GS=Top->AddChildToHorizontalBox(Size(T,Gear,300));GS->SetPadding(FMargin(0,0,24,0));
    InventoryRow(Gear,InventoryLabel(T,TEXT("TXT_CharacterName"),FText::GetEmpty(),24),8);
    InventoryRow(Gear,InventoryLabel(T,TEXT("TXT_WeaponHeading"),NSLOCTEXT("ICInventory","Weapons","무기")),4);
    auto* Weapons=T->ConstructWidget<UGridPanel>();InventoryRow(Gear,Weapons,8);
    auto* CellClass=WidgetClass(TEXT("WBP_IC_InventorySlot"),UICInventorySlotWidget::StaticClass());
    const TCHAR* Families[]={TEXT("검"),TEXT("활"),TEXT("지팡이"),TEXT("완드")};
    for(int32 I=0;I<4;++I)
    {
        auto* W=T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),Indexed(TEXT("WeaponFrame"),I));
        InventoryRow(W,InventoryLabel(T,Indexed(TEXT("WeaponLabel"),I),FText::FromString(Families[I]),14),2);
        InventoryRow(W,T->ConstructWidget<UICInventorySlotWidget>(CellClass,Indexed(TEXT("Equipment"),I)),0);
        Weapons->AddChildToGrid(W,0,I)->SetPadding(FMargin(0,0,3,6));
    }
    auto* Hint=InventoryLabel(T,TEXT("TXT_QuickPrompt"),NSLOCTEXT("ICInventory","QuickSlots","퀵슬롯"),16);Hint->SetAutoWrapText(true);InventoryRow(Gear,Hint,8);
    auto* QuickSlots=T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("QuickSlotsRow"));InventoryRow(Gear,QuickSlots,8);
    for(int32 I=0;I<4;++I){auto* Cell=T->ConstructWidget<UICInventorySlotWidget>(CellClass,Indexed(TEXT("QuickSlot"),I));QuickSlots->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(0,0,3,0));}
    InventoryRow(Gear,InventoryButton(T,TEXT("BTN_CancelQuick"),NSLOCTEXT("ICInventory","Cancel","취소")),0);
    auto* Bag=T->ConstructWidget<UVerticalBox>();Top->AddChildToHorizontalBox(Bag)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* GoldLabel=InventoryLabel(T,TEXT("TXT_Gold"),FText::GetEmpty(),22);GoldLabel->SetJustification(ETextJustify::Right);GoldLabel->SetColorAndOpacity(Gold);InventoryRow(Bag,GoldLabel,8);
    auto* Notice=InventoryLabel(T,TEXT("TXT_InventoryNotice"),FText::GetEmpty(),15);Notice->SetAutoWrapText(true);Notice->SetColorAndOpacity(Gold);InventoryRow(Bag,Notice,4);
    auto* Tabs=T->ConstructWidget<UHorizontalBox>();InventoryRow(Bag,Tabs,8);
    auto* Search=T->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("InventorySearch"));Search->SetHintText(NSLOCTEXT("ICInventory","Search","아이템 이름 / ID 검색"));auto SearchStyle=Search->GetWidgetStyle();SearchStyle.SetFont(ICFrontendLayout::Font(16));Search->SetWidgetStyle(SearchStyle);InventoryRow(Bag,Search,6);
    auto* Tools=T->ConstructWidget<UHorizontalBox>();InventoryRow(Bag,Tools,6);
    Tools->AddChildToHorizontalBox(InventoryButton(T,TEXT("BTN_SortItems"),NSLOCTEXT("ICInventory","Sort","가방 정렬")));
    Tools->AddChildToHorizontalBox(InventoryButton(T,TEXT("BTN_FilterItems"),NSLOCTEXT("ICInventory","AllFilter","분류: 전체")));
    Tabs->AddChildToHorizontalBox(InventoryButton(T,TEXT("TAB_Normal"),NSLOCTEXT("ICInventory","Normal","일반")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Tabs->AddChildToHorizontalBox(InventoryButton(T,TEXT("TAB_Quest"),NSLOCTEXT("ICInventory","Quest","퀘스트")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("ItemScroll"));Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Scroll->AddChild(T->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(),TEXT("ItemGrid")));
    Bag->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    InventoryRow(Bag,Size(T,InventoryLabel(T,TEXT("TXT_SelectedItem"),FText::GetEmpty(),15),0,22),5);
    auto* Detail=T->ConstructWidget<UICInventoryItemPopupWidget>(WidgetClass(TEXT("WBP_IC_InventoryItemPopup"),UICInventoryItemPopupWidget::StaticClass()),TEXT("ItemPopup"));
    Detail->SetVisibility(ESlateVisibility::Collapsed);auto* DetailPlacement=Root->AddChildToOverlay(Detail);DetailPlacement->SetHorizontalAlignment(HAlign_Fill);DetailPlacement->SetVerticalAlignment(VAlign_Fill);
    auto* Confirm=T->ConstructWidget<UICItemConfirmWidget>(WidgetClass(TEXT("WBP_IC_ItemConfirm"),UICItemConfirmWidget::StaticClass()),TEXT("ItemConfirm"));
    Confirm->SetVisibility(ESlateVisibility::Collapsed);auto* ConfirmPlacement=Root->AddChildToOverlay(Confirm);
    ConfirmPlacement->SetHorizontalAlignment(HAlign_Fill);ConfirmPlacement->SetVerticalAlignment(VAlign_Fill);
    auto* Targets=T->ConstructWidget<UICCodexTargetsWidget>(WidgetClass(TEXT("WBP_IC_CodexTargets"),UICCodexTargetsWidget::StaticClass()),TEXT("UseTargets"));
    Targets->SetVisibility(ESlateVisibility::Collapsed);auto* TargetsPlacement=Root->AddChildToOverlay(Targets);TargetsPlacement->SetHorizontalAlignment(HAlign_Fill);TargetsPlacement->SetVerticalAlignment(VAlign_Fill);return true;
}

UICInventoryComponent* UICInventoryMenuWidget::Inventory() const
{auto* P=GetOwningPlayer()?Cast<AICPlayerCharacter>(GetOwningPlayer()->GetPawn()):nullptr;return P?P->Inventory:nullptr;}
FText UICInventoryMenuWidget::ItemName(FName ID) const
{
    if(ID.IsNone())return FText::GetEmpty();auto* Inv=Inventory();const auto* D=Inv?Inv->Find(ID):nullptr;
    return D&&!D->DisplayName.IsEmpty()?D->DisplayName:FText::FromName(ID);
}
void UICInventoryMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    for(int32 I=0;I<4;++I)
    {
        CastChecked<UICPartyPortraitWidget>(GetWidgetFromName(Indexed(TEXT("Member"),I)))->OnPicked.AddUniqueDynamic(this,&ThisClass::PortraitPicked);
        CastChecked<UICInventorySlotWidget>(GetWidgetFromName(Indexed(TEXT("QuickSlot"),I)))->OnPicked.AddUniqueDynamic(this,&ThisClass::QuickPicked);
    }
    for(int32 I=0;I<4;++I)CastChecked<UICInventorySlotWidget>(GetWidgetFromName(Indexed(TEXT("Equipment"),I)))->OnPicked.AddUniqueDynamic(this,&ThisClass::EquipmentPicked);
    const FName Names[]={TEXT("TAB_Normal"),TEXT("TAB_Quest"),TEXT("BTN_CancelQuick")};
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_SortItems")))->OnClicked.AddUniqueDynamic(this,&ThisClass::SortItems);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_FilterItems")))->OnClicked.AddUniqueDynamic(this,&ThisClass::CycleFilter);
    CastChecked<UEditableTextBox>(GetWidgetFromName(TEXT("InventorySearch")))->OnTextChanged.AddUniqueDynamic(this,&ThisClass::SearchChanged);
    auto* Actions=CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")));Actions->OnLock.AddUniqueDynamic(this,&ThisClass::LockSelected);Actions->OnSplit.AddUniqueDynamic(this,&ThisClass::SplitSelected);
    const FName Functions[]={TEXT("NormalTab"),TEXT("QuestTab"),TEXT("CancelAction")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Names);++I){FScriptDelegate Click;Click.BindUFunction(this,Functions[I]);CastChecked<UButton>(GetWidgetFromName(Names[I]))->OnClicked.AddUnique(Click);}
    CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("ItemConfirm")))->OnResolved.AddUniqueDynamic(this,&ThisClass::ResolveConfirmation);
    auto* Detail=CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")));Detail->OnEquip.AddUniqueDynamic(this,&ThisClass::EquipAction);Detail->OnQuick.AddUniqueDynamic(this,&ThisClass::QuickAction);Detail->OnTrash.AddUniqueDynamic(this,&ThisClass::TrashAction);Detail->OnBack.AddUniqueDynamic(this,&ThisClass::CloseItem);
    Detail->OnUse.AddUniqueDynamic(this,&ThisClass::UseAction);
    auto* Targets=CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("UseTargets")));Targets->OnPicked.AddUniqueDynamic(this,&ThisClass::UseTargetPicked);Targets->OnBack.AddUniqueDynamic(this,&ThisClass::CancelUseTarget);
    RefreshInventory();
}
void UICInventoryMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState){BoundState->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);BoundState->OnInventoryChanged.AddUniqueDynamic(this,&ThisClass::RefreshInventory);BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RefreshInventory);BoundState->OnPlayerGoldChanged.AddUniqueDynamic(this,&ThisClass::RefreshInventory);}
    RefreshInventory();
}
void UICInventoryMenuWidget::NativeDestruct()
{
    if(BoundState){BoundState->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);BoundState->OnInventoryChanged.RemoveDynamic(this,&ThisClass::RefreshInventory);BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::RefreshInventory);BoundState->OnPlayerGoldChanged.RemoveDynamic(this,&ThisClass::RefreshInventory);BoundState=nullptr;}
    Super::NativeDestruct();
}
void UICInventoryMenuWidget::Activate(){if(!IsBusy()){NavigationArea=0;RefreshInventory();}}
void UICInventoryMenuWidget::CharacterChanged(FName){RefreshInventory();}
void UICInventoryMenuWidget::RefreshInventory()
{
    if(bRefreshing)return;TGuardValue<bool> Guard(bRefreshing,true);
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    const auto Data=State?State->ExportState():FICWorldState();Members.Reset();
    Caption(this,TEXT("TXT_Gold"),FText::Format(NSLOCTEXT("ICInventory","GoldAmount","보유 골드  {0} G"),FText::AsNumber(Data.Gold)));
    if(Data.Characters.Contains(UICWorldStateSubsystem::PlayerId()))Members.Add(UICWorldStateSubsystem::PlayerId());
    for(int32 PartyIndex=0;PartyIndex<3;++PartyIndex)for(const auto& Pair:Data.Characters)
        if(Pair.Value.Kind==EICCharacterKind::Recruitable&&Pair.Value.Mode==EICCharacterMode::Companion&&Pair.Value.PartySlot==PartyIndex){Members.Add(Pair.Key);break;}
    const int32 Previous=Members.IndexOfByKey(SelectedCharacter);SelectedMember=Previous!=INDEX_NONE?Previous:FMath::Clamp(SelectedMember,0,FMath::Max(0,Members.Num()-1));
    SelectedCharacter=Members.IsValidIndex(SelectedMember)?Members[SelectedMember]:NAME_None;
    for(int32 I=0;I<4;++I)
    {
        UTexture2D* Portrait=nullptr;FICDefinitionResult R;
        if(D&&Members.IsValidIndex(I))
        {if(I==0){if(const auto* P=D->Find<FICPlayerPresetRow>(Data.Preset==1?TEXT("PRESET_PLAYER_F"):TEXT("PRESET_PLAYER_M"),R))Portrait=P->Portrait.LoadSynchronous();}
         else if(const auto* N=D->Find<FICNPCRow>(Members[I],R))Portrait=N->Portrait.LoadSynchronous();}
        auto* Card=CastChecked<UICPartyPortraitWidget>(GetWidgetFromName(Indexed(TEXT("Member"),I)));Card->Present(I,Members.IsValidIndex(I)?Members[I]:NAME_None,Portrait,I==SelectedMember);
        auto* Box=CastChecked<USizeBox>(Card->GetParent());Box->SetWidthOverride(I==SelectedMember?88:64);Box->SetHeightOverride(I==SelectedMember?88:64);
        Card->GetParent()->SetVisibility(Members.IsValidIndex(I)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    }
    FText Name=FText::FromString(Data.PlayerName);
    if(SelectedCharacter!=UICWorldStateSubsystem::PlayerId())
    {
        FICDefinitionResult R;const auto* NPC=D?D->Find<FICNPCRow>(SelectedCharacter,R):nullptr;
        auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
        const auto Text=UICTextLibrary::ResolveText(Strings,NPC?NPC->NameKey:FString());Name=Text.bSuccess?Text.Text:FText::FromName(SelectedCharacter);
    }
    Caption(this,TEXT("TXT_CharacterName"),Name);
    if((bPickingQuickSlot||bConfirming)&&!PendingStillValid())CancelPending();
    if(bDetailOpen&&(CurrentItem().IsEmpty()||CurrentItem().InstanceID!=DetailInstance)){bDetailOpen=false;bPickingUseTarget=false;NavigationArea=3;}
    RefreshEquipment();RefreshItems();RefreshPresentation();
}
void UICInventoryMenuWidget::RefreshEquipment()
{
    auto* Inv=Inventory();auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    const bool Player=SelectedCharacter==UICWorldStateSubsystem::PlayerId();const auto Gear=State?State->GetSnapshot(SelectedCharacter).Equipment:TArray<FPlayerItemInstance>();
    const auto Weapons=Inv&&Player?Inv->GetWeaponLoadout():TArray<FPlayerItemInstance>();EquipmentCells.Reset();
    for(int32 I=0;I<4;++I)
    {
        const bool Visible=Player||I==0;
        if(Visible)EquipmentCells.Add(I);
        auto* Card=CastChecked<UICInventorySlotWidget>(GetWidgetFromName(Indexed(TEXT("Equipment"),I)));
        if(I<4)GetWidgetFromName(Indexed(TEXT("WeaponFrame"),I))->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(!Visible)continue;
        const int32 GearIndex=0;FPlayerItemInstance Item;
        if(Player&&I<4&&Weapons.IsValidIndex(I))Item=Weapons[I];else if(Gear.IsValidIndex(GearIndex))Item=Gear[GearIndex];
        const auto* D=Inv?Inv->Find(Item.ItemID):nullptr;
        Card->Present(I,Item.IsEmpty()?NSLOCTEXT("ICInventory","Empty","비어 있음"):ItemName(Item.ItemID),D?D->Icon.LoadSynchronous():nullptr,0,NavigationArea==1&&EquipmentCells.Num()-1==EquipmentIndex);
    }
    if(!Player)
    {
        auto* D=Inv?Inv->GetDefinitions():nullptr;FICDefinitionResult R;
        const auto* NPC=D?D->Find<FICNPCRow>(SelectedCharacter,R):nullptr;
        const auto* Recruit=NPC?D->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,R):nullptr;
        const TCHAR* Names[]={TEXT("전용 무기"),TEXT("검"),TEXT("활"),TEXT("지팡이"),TEXT("완드")};
        const int32 Family=Recruit?FMath::Clamp(static_cast<int32>(Recruit->WeaponFamily),0,4):0;
        Caption(this,TEXT("WeaponLabel0"),FText::FromString(Names[Family]));
    }
    else Caption(this,TEXT("WeaponLabel0"),FText::FromString(TEXT("검")));
    EquipmentIndex=FMath::Clamp(EquipmentIndex,0,FMath::Max(0,EquipmentCells.Num()-1));
}
void UICInventoryMenuWidget::RefreshItems()
{
    auto* Grid=CastChecked<UGridPanel>(GetWidgetFromName(TEXT("ItemGrid")));auto* Inv=Inventory();const auto Data=Inv?Inv->GetSnapshot():FICInventoryState();
    VisibleBagIndices.Reset();
    for(int32 I=0;I<Data.Bag.Slots.Num();++I)
    {const auto& S=Data.Bag.Slots[I];const auto* D=Inv?Inv->Find(S.ItemID):nullptr;
     const bool Tab=ItemTab==EPlayerItemTab::Normal?(!D||D->Tab==EPlayerItemTab::Normal):(D&&D->Tab==EPlayerItemTab::Quest);
     const bool Match=SearchText.IsEmpty()||(D&&(ItemName(S.ItemID).ToString().Contains(SearchText)||S.ItemID.ToString().Contains(SearchText)));
     const bool Filter=FilterKind==0||(D&&(FilterKind==1?D->Kind==EPlayerItemKind::Equipment:FilterKind==2?D->Kind==EPlayerItemKind::Consumable:FilterKind==3?D->Category==EPlayerItemCategory::Food:FilterKind==4?D->Category==EPlayerItemCategory::Book:FilterKind==5?D->Kind==EPlayerItemKind::Material:S.bLocked));
     if(Tab&&Match&&Filter)VisibleBagIndices.Add(I);}
    if(ItemTab==EPlayerItemTab::Quest&&FilterKind==0)for(int32 I=0;I<Data.QuestItems.Num();++I)
        if(SearchText.IsEmpty()||ItemName(Data.QuestItems[I].ItemID).ToString().Contains(SearchText)||Data.QuestItems[I].ItemID.ToString().Contains(SearchText))VisibleBagIndices.Add(-I-1);
    SelectedItem=FMath::Clamp(SelectedItem,0,FMath::Max(0,VisibleBagIndices.Num()-1));
    if(Grid->GetChildrenCount()!=VisibleBagIndices.Num())
    {
        Grid->ClearChildren();auto* C=WidgetClass(TEXT("WBP_IC_InventorySlot"),UICInventorySlotWidget::StaticClass());
        for(int32 I=0;I<VisibleBagIndices.Num();++I)
        {auto* Cell=WidgetTree->ConstructWidget<UICInventorySlotWidget>(C);Cell->OnPicked.AddUniqueDynamic(this,&ThisClass::ItemPicked);Grid->AddChildToGrid(Cell,I/5,I%5)->SetPadding(FMargin(0,0,7,7));}
    }
    for(int32 I=0;I<VisibleBagIndices.Num();++I)
    {
        const int32 Source=VisibleBagIndices[I];const auto& Item=Source>=0?Data.Bag.Slots[Source]:Data.QuestItems[-Source-1];const auto* D=Inv?Inv->Find(Item.ItemID):nullptr;
        const auto Label=Item.bLocked?FText::Format(NSLOCTEXT("ICInventory","LockedName","[잠금] {0}"),ItemName(Item.ItemID)):ItemName(Item.ItemID);
        CastChecked<UICInventorySlotWidget>(Grid->GetChildAt(I))->Present(I,Label,D?D->Icon.LoadSynchronous():nullptr,Item.Count,I==SelectedItem);
    }
    const auto Item=CurrentItem();Caption(this,TEXT("TXT_SelectedItem"),Item.IsEmpty()?(ItemTab==EPlayerItemTab::Quest?NSLOCTEXT("ICInventory","NoQuestItems","퀘스트 아이템이 없습니다."):FText::GetEmpty()):FText::Format(NSLOCTEXT("ICInventory","SelectedStack","{0}  × {1}"),ItemName(Item.ItemID),FText::AsNumber(Item.Count)));
}
FPlayerItemInstance UICInventoryMenuWidget::CurrentItem() const
{
    auto* Inv=Inventory();if(!Inv||!VisibleBagIndices.IsValidIndex(SelectedItem))return {};
    const auto Data=Inv->GetSnapshot();const int32 I=VisibleBagIndices[SelectedItem];
    return I>=0?(Data.Bag.Slots.IsValidIndex(I)?Data.Bag.Slots[I]:FPlayerItemInstance()):(Data.QuestItems.IsValidIndex(-I-1)?Data.QuestItems[-I-1]:FPlayerItemInstance());
}
bool UICInventoryMenuWidget::CanQuickSlot() const
{
    const auto Item=CurrentItem();auto* Inv=Inventory();const auto* D=Inv?Inv->Find(Item.ItemID):nullptr;
    return !Item.IsEmpty()&&!Item.bLocked&&VisibleBagIndices.IsValidIndex(SelectedItem)&&VisibleBagIndices[SelectedItem]>=0&&D&&(D->Kind==EPlayerItemKind::Consumable||D->Kind==EPlayerItemKind::Expansion);
}
bool UICInventoryMenuWidget::CanTrash() const
{
    const auto Item=CurrentItem();auto* Inv=Inventory();const auto* D=Inv?Inv->Find(Item.ItemID):nullptr;
    return !Item.IsEmpty()&&!Item.bLocked&&D&&D->bCanDiscard&&D->Kind!=EPlayerItemKind::Quest;
}
void UICInventoryMenuWidget::RefreshPresentation()
{
    auto* Inv=Inventory();const auto Data=Inv?Inv->GetSnapshot():FICInventoryState();
    GetWidgetFromName(TEXT("InventoryBody"))->SetIsEnabled(!bConfirming&&!bPickingUseTarget&&(!bDetailOpen||bPickingQuickSlot));
    GetWidgetFromName(TEXT("ItemPopup"))->SetVisibility(bDetailOpen&&!bPickingQuickSlot&&!bPickingUseTarget?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("UseTargets"))->SetVisibility(bPickingUseTarget?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("ItemPopup"))->SetIsEnabled(!bConfirming);
    if(bDetailOpen)RefreshDetail();
    CastChecked<UButton>(GetWidgetFromName(TEXT("TAB_Normal")))->SetBackgroundColor(ItemTab==EPlayerItemTab::Normal?Gold:InventoryButtonColor);
    CastChecked<UButton>(GetWidgetFromName(TEXT("TAB_Quest")))->SetBackgroundColor(ItemTab==EPlayerItemTab::Quest?Gold:InventoryButtonColor);
    GetWidgetFromName(TEXT("BTN_CancelQuick"))->SetVisibility(bPickingQuickSlot?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Caption(this,TEXT("TXT_QuickPrompt"),bPickingQuickSlot?NSLOCTEXT("ICInventory","ChooseQuick","등록할 퀵슬롯을 선택하세요."):NSLOCTEXT("ICInventory","QuickSlots","퀵슬롯"));
    for(int32 I=0;I<4;++I)
    {
        const FName ID=Data.QuickSlots.IsValidIndex(I)?Data.QuickSlots[I]:NAME_None;const int32 Count=Inv?Inv->CountItem(ID):0;const auto* Def=Inv&&Count>0?Inv->Find(ID):nullptr;
        CastChecked<UICInventorySlotWidget>(GetWidgetFromName(Indexed(TEXT("QuickSlot"),I)))->Present(I,Count>0?ItemName(ID):FText::Format(NSLOCTEXT("ICInventory","EmptyQuick","슬롯 {0}"),FText::AsNumber(I+1)),Def?Def->Icon.LoadSynchronous():nullptr,Count,NavigationArea==5&&I==QuickIndex);
    }
}
bool UICInventoryMenuWidget::SelectMember(int32 Index)
{if(IsBusy()||!Members.IsValidIndex(Index))return false;SelectedMember=Index;SelectedCharacter=Members[Index];EquipmentIndex=0;RefreshInventory();return true;}
void UICInventoryMenuWidget::SelectItemTab(EPlayerItemTab Tab)
{if(IsBusy())return;ItemTab=Tab;SelectedItem=0;RefreshInventory();}
bool UICInventoryMenuWidget::SelectItem(int32 Index)
{if(IsBusy()||!VisibleBagIndices.IsValidIndex(Index))return false;SelectedItem=Index;RefreshInventory();auto* Grid=CastChecked<UGridPanel>(GetWidgetFromName(TEXT("ItemGrid")));CastChecked<UScrollBox>(GetWidgetFromName(TEXT("ItemScroll")))->ScrollWidgetIntoView(Grid->GetChildAt(Index),true,EDescendantScrollDestination::IntoView);return true;}
bool UICInventoryMenuWidget::OpenItem(int32 Index)
{
    if(!SelectItem(Index)||CurrentItem().IsEmpty())return false;bDetailOpen=true;DetailInstance=CurrentItem().InstanceID;NavigationArea=4;
    auto* Detail=CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")));Detail->ActionIndex=0;Detail->SetMessage(FText::GetEmpty());
    CastChecked<UScrollBox>(Detail->GetWidgetFromName(TEXT("DescriptionScroll")))->SetScrollOffset(0);RefreshPresentation();NotifyNavigation();return true;
}
void UICInventoryMenuWidget::CloseItem()
{if(bPickingQuickSlot||bConfirming)return;bDetailOpen=false;DetailInstance.Invalidate();NavigationArea=3;RefreshPresentation();NotifyNavigation();}
void UICInventoryMenuWidget::RefreshDetail()
{
    auto* Inv=Inventory();const auto Item=CurrentItem();const auto* D=Inv?Inv->Find(Item.ItemID):nullptr;if(!D)return;
    auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));const auto Description=UICTextLibrary::ResolveText(Strings,D->DescriptionKey);
    FText Info=Description.bSuccess?Description.Text:FText::GetEmpty();
    if(D->Kind==EPlayerItemKind::Equipment)
    {
        FICDefinitionResult Result;const auto* Gear=Inv->GetDefinitions()->Find<FICEquipmentRow>(D->EquipmentID,Result);
        if(Gear)
        {
            double Attack=0;for(const auto& Modifier:Gear->BaseModifiers)if(Modifier.Stat==EICStat::PhysicalAttack||Modifier.Stat==EICStat::MagicAttack)Attack+=Modifier.Flat;
            Info=FText::Format(NSLOCTEXT("ICInventory","GearInfo","{0}\n기본 공격력 {1}\n요구 능력치: {2} {3}"),Info,FText::AsNumber(Attack),FText::FromString(StaticEnum<EICStat>()->GetNameStringByValue(static_cast<int64>(Gear->RequiredMainStat))),FText::AsNumber(Gear->RequiredMainStatValue));
            FText Reason;if(!Inv->MeetsEquipmentRequirements(SelectedCharacter,Item.ItemID,Reason))Info=FText::Format(NSLOCTEXT("ICInventory","GearUnavailable","{0}\n{1}"),Info,Reason);
        }
    }
    const bool Usable=D->Kind==EPlayerItemKind::Expansion||(D->Kind==EPlayerItemKind::Consumable&&(D->UseEffect!=EICItemEffect::None||!D->StatusEffectIDs.IsEmpty()));
    CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->Present(ItemName(Item.ItemID),Info,D->Icon.LoadSynchronous(),D->Kind,CanTrash(),CanQuickSlot(),Usable);
    const bool InBag=VisibleBagIndices.IsValidIndex(SelectedItem)&&VisibleBagIndices[SelectedItem]>=0&&D->Kind!=EPlayerItemKind::Quest;
    CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->ConfigureStack(Item.bLocked,Item.Count,InBag,InBag&&D->Kind!=EPlayerItemKind::Equipment);
}

bool UICInventoryMenuWidget::BeginUseSelected()
{
    auto* Inv=Inventory();const auto Item=CurrentItem();const auto* Definition=Inv?Inv->Find(Item.ItemID):nullptr;
    if(Definition&&Definition->Kind==EPlayerItemKind::Expansion&&bDetailOpen&&!bPickingQuickSlot&&!bConfirming&&!bPickingUseTarget&&!Item.IsEmpty()&&!Item.bLocked&&Item.InstanceID==DetailInstance)
    {
        const bool Done=Inv->ExpandUsingTicket();
        if(Done){bDetailOpen=false;DetailInstance.Invalidate();NavigationArea=3;}RefreshInventory();
        Caption(this,TEXT("TXT_InventoryNotice"),Done?NSLOCTEXT("ICInventory","BagExpanded","가방이 10칸 늘어났습니다."):NSLOCTEXT("ICInventory","BagExpandFailed","가방이 최대 크기이거나 확장권을 사용할 수 없습니다."));NotifyNavigation();return Done;
    }
    if(bPickingQuickSlot||bConfirming||bPickingUseTarget||Item.IsEmpty()||Item.bLocked||Item.InstanceID!=DetailInstance||!Definition||Definition->Kind!=EPlayerItemKind::Consumable)return false;
    bPickingUseTarget=true;auto* Targets=CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("UseTargets")));
    Targets->Present(GetGameInstance()->GetSubsystem<UICCodexSubsystem>());
    auto* Title=CastChecked<UTextBlock>(Targets->GetWidgetFromName(TEXT("TargetsTitle")));Title->SetAutoWrapText(true);
    Title->SetText(FText::Format(NSLOCTEXT("ICInventory","UseOnMember","{0}을(를) 사용할 대상"),ItemName(Item.ItemID)));
    RefreshPresentation();NotifyNavigation();return true;
}

bool UICInventoryMenuWidget::ChooseUseTarget(int32 Index)
{
    auto* Targets=CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("UseTargets")));
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());const auto Item=CurrentItem();
    if(!bPickingUseTarget||!PC||!Targets->Members.IsValidIndex(Index)||Item.IsEmpty()||Item.bLocked||Item.InstanceID!=DetailInstance)return false;
    const FName Target=Targets->Members[Index];FText Reason;
    if(!PC->UseInventoryItem(Item.ItemID,Target,Reason)){Caption(Targets,TEXT("TargetsTitle"),Reason);return false;}
    const FText Message=FText::Format(NSLOCTEXT("ICInventory","UsedOnMember","{0}에게 {1} 사용 완료"),GetGameInstance()->GetSubsystem<UICCodexSubsystem>()->CharacterName(Target),ItemName(Item.ItemID));
    bPickingUseTarget=false;RefreshInventory();Caption(this,TEXT("TXT_InventoryNotice"),Message);
    CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->SetMessage(Message);NotifyNavigation();return true;
}

void UICInventoryMenuWidget::CancelUseTarget(){bPickingUseTarget=false;RefreshPresentation();NotifyNavigation();}
void UICInventoryMenuWidget::UseAction(){BeginUseSelected();}
void UICInventoryMenuWidget::UseTargetPicked(int32 Index){ChooseUseTarget(Index);}
void UICInventoryMenuWidget::NotifyNavigation(){OnNavigationChanged.Broadcast();}
bool UICInventoryMenuWidget::PendingStillValid() const
{
    auto* Inv=Inventory();if(!Inv)return false;const auto Data=Inv->GetSnapshot();if(!Data.Bag.Slots.IsValidIndex(PendingBagIndex))return false;
    const auto& Item=Data.Bag.Slots[PendingBagIndex];return !Item.IsEmpty()&&!Item.bLocked&&Item.InstanceID==PendingItem.InstanceID&&Item.ItemID==PendingItem.ItemID&&Item.Count==PendingItem.Count;
}
bool UICInventoryMenuWidget::BeginQuickSlot()
{
    if(bPickingQuickSlot||bConfirming||!CanQuickSlot())return false;PendingBagIndex=VisibleBagIndices[SelectedItem];PendingItem=CurrentItem();bTrashPending=false;bPickingQuickSlot=true;NavigationArea=5;QuickIndex=0;RefreshPresentation();NotifyNavigation();return true;
}
bool UICInventoryMenuWidget::ChooseQuickSlot(int32 Index)
{
    if(!bPickingQuickSlot||bConfirming||!PendingStillValid()||Index<0||Index>=4)return false;
    QuickIndex=Index;PendingPreviousQuick=Inventory()->GetSnapshot().QuickSlots[Index];bPickingQuickSlot=false;bConfirming=true;
    const FText Message=PendingPreviousQuick.IsNone()?FText::Format(NSLOCTEXT("ICInventory","RegisterConfirm","{0}번 퀵슬롯에 {1}을(를) 넣겠습니까?"),FText::AsNumber(Index+1),ItemName(PendingItem.ItemID)):
        FText::Format(NSLOCTEXT("ICInventory","ReplaceConfirm","{0}을(를) 제외하고 {1}을(를) 넣겠습니까?"),ItemName(PendingPreviousQuick),ItemName(PendingItem.ItemID));
    CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("ItemConfirm")))->Present(Message);RefreshPresentation();NotifyNavigation();return true;
}
bool UICInventoryMenuWidget::BeginTrash()
{
    if(bPickingQuickSlot||bConfirming||!CanTrash())return false;PendingBagIndex=VisibleBagIndices[SelectedItem];PendingItem=CurrentItem();bTrashPending=true;bConfirming=true;
    CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("ItemConfirm")))->Present(FText::Format(NSLOCTEXT("ICInventory","TrashConfirm","{0} {1}개를 버리겠습니까?"),ItemName(PendingItem.ItemID),FText::AsNumber(PendingItem.Count)));RefreshPresentation();NotifyNavigation();return true;
}
void UICInventoryMenuWidget::CancelPending()
{bPickingQuickSlot=false;bConfirming=false;PendingBagIndex=INDEX_NONE;PendingItem={};PendingPreviousQuick=NAME_None;GetWidgetFromName(TEXT("ItemConfirm"))->SetVisibility(ESlateVisibility::Collapsed);NavigationArea=bDetailOpen?4:3;}
void UICInventoryMenuWidget::ResolveConfirmation(bool Accept)
{
    if(!bConfirming)return;auto* Inv=Inventory();
    const bool Valid=PendingStillValid()&&(bTrashPending||Inv->GetSnapshot().QuickSlots[QuickIndex]==PendingPreviousQuick);
    const int32 Index=PendingBagIndex,TargetQuick=QuickIndex;const bool Trash=bTrashPending;CancelPending();
    const bool Done=Accept&&Valid&&Inv&&(Trash?Inv->TrashItem(Index):Inv->BindQuickSlot(TargetQuick,Index));if(Done){bDetailOpen=false;NavigationArea=3;}RefreshInventory();NotifyNavigation();
    if(Accept&&!Done)CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->SetMessage(NSLOCTEXT("ICInventory","ChangedItem","아이템 상태가 변경되어 처리할 수 없습니다. 다시 선택하세요."));
}
bool UICInventoryMenuWidget::EquipSelected()
{
    if(bPickingQuickSlot||bConfirming||!VisibleBagIndices.IsValidIndex(SelectedItem)||VisibleBagIndices[SelectedItem]<0)return false;
    auto* Inv=Inventory();const bool Done=Inv&&Inv->EquipForCharacter(SelectedCharacter,VisibleBagIndices[SelectedItem]);if(Done){bDetailOpen=false;NavigationArea=3;}RefreshInventory();NotifyNavigation();
    if(!Done)CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->SetMessage(NSLOCTEXT("ICInventory","WrongEquipment","해당 캐릭터가 착용할 수 없는 장비입니다."));return Done;
}
void UICInventoryMenuWidget::PortraitPicked(int32 Index){if(SelectMember(Index)){NavigationArea=0;RefreshInventory();}}
void UICInventoryMenuWidget::ItemPicked(int32 Index){OpenItem(Index);}
void UICInventoryMenuWidget::EquipmentPicked(int32 Index)
{
    if(IsBusy())return;EquipmentIndex=EquipmentCells.IndexOfByKey(Index);NavigationArea=1;auto* Inv=Inventory();bool Done=false;
    if(Inv&&EquipmentIndex!=INDEX_NONE)
    {if(SelectedCharacter==UICWorldStateSubsystem::PlayerId())Done=Inv->UnequipWeapon(static_cast<EICWeaponFamily>(Index+1));else Done=Inv->UnequipForCharacter(SelectedCharacter,EPlayerEquipSlot::Weapon);}
    RefreshInventory();if(!Done)Caption(this,TEXT("TXT_SelectedItem"),NSLOCTEXT("ICInventory","NoUnequip","빈 장비 슬롯이거나 가방에 빈 공간이 없습니다."));
}
void UICInventoryMenuWidget::QuickPicked(int32 Index)
{
    if(bPickingQuickSlot){ChooseQuickSlot(Index);return;}if(IsBusy())return;
    NavigationArea=5;QuickIndex=Index;auto* Inv=Inventory();if(!Inv)return;const auto Data=Inv->GetSnapshot();
    if(!Data.QuickSlots.IsValidIndex(Index)||Data.QuickSlots[Index].IsNone()){RefreshPresentation();return;}
    SelectItemTab(EPlayerItemTab::Normal);const FName ItemID=Data.QuickSlots[Index];
    const int32 Cell=VisibleBagIndices.IndexOfByPredicate([&](int32 BagIndex){return Data.Bag.Slots.IsValidIndex(BagIndex)&&Data.Bag.Slots[BagIndex].ItemID==ItemID&&!Data.Bag.Slots[BagIndex].bLocked;});
    if(Cell!=INDEX_NONE)OpenItem(Cell);else RefreshPresentation();
}
void UICInventoryMenuWidget::NormalTab(){if(IsBusy())return;SelectItemTab(EPlayerItemTab::Normal);NavigationArea=2;RefreshPresentation();}
void UICInventoryMenuWidget::QuestTab(){if(IsBusy())return;SelectItemTab(EPlayerItemTab::Quest);NavigationArea=2;RefreshPresentation();}
void UICInventoryMenuWidget::QuickAction(){BeginQuickSlot();}
void UICInventoryMenuWidget::TrashAction(){BeginTrash();}
void UICInventoryMenuWidget::EquipAction(){EquipSelected();}
void UICInventoryMenuWidget::CancelAction(){if(bPickingQuickSlot||bConfirming){CancelPending();RefreshInventory();NotifyNavigation();}}
FText UICInventoryMenuWidget::NavigationHint() const
{
    if(bPickingUseTarget)return NSLOCTEXT("ICInventory","UseTargetHint","A / D  대상 선택     Enter  사용     ESC  취소");
    if(bConfirming)return NSLOCTEXT("ICInventory","ConfirmHint","WASD  수락 / 취소 선택     Enter  결정     ESC  취소");
    if(bPickingQuickSlot)return NSLOCTEXT("ICInventory","PickHint","A / D  슬롯 선택     Enter  등록 확인     ESC  취소");
    if(bDetailOpen)return NSLOCTEXT("ICInventory","DetailHint","A / D  버튼 선택     W / S  설명 스크롤     G/F  선택     ESC  뒤로가기");
    if(NavigationArea==0)return NSLOCTEXT("ICInventory","RosterHint","W / S  캐릭터 선택     D  장비     A  상단 탭     ESC  닫기");
    if(NavigationArea==1)return NSLOCTEXT("ICInventory","EquipmentHint","A / D  무기 선택     S  퀵슬롯     Enter  무기 해제");
    if(NavigationArea==2)return NSLOCTEXT("ICInventory","ItemTabsHint","A / D  일반 / 퀘스트     S  아이템     W  상단 탭");
    if(NavigationArea==3)return NSLOCTEXT("ICInventory","ItemsHint","WASD  아이템 이동     Enter  아이템 작업     W  위쪽 탭     A  장비");
    return NSLOCTEXT("ICInventory","QuickHint","A / D  퀵슬롯 선택     Enter  아이템 보기     Delete  등록 해제     W  무기");
}
bool UICInventoryMenuWidget::HandleMenuKey(FKey Key,bool Repeat)
{
    if(IsSearchFocused())return false;
    if(!IsBusy()&&(Key==EKeys::V||Key==EKeys::Gamepad_FaceButton_Left)){if(!Repeat)SortItems();return true;}
    if(!IsBusy()&&(Key==EKeys::C||Key==EKeys::Gamepad_FaceButton_Top)){if(!Repeat)CycleFilter();return true;}
    if(bPickingUseTarget)return CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("UseTargets")))->HandleKey(Key,Repeat);
    if(bConfirming)return CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("ItemConfirm")))->HandleKey(Key,Repeat);
    const bool Up=Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up,Down=Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down;
    const bool Left=Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left,Right=Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right;
    const bool Accept=(Key==EKeys::G||Key==EKeys::F||Key==EKeys::Enter)||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key);
    const bool Back=Key==EKeys::Escape||Key==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(Key);
    if(bPickingQuickSlot)
    {
        const FKey NumberKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
        for(int32 Index=0;Index<4;++Index)if(Key==NumberKeys[Index]){if(!Repeat)ChooseQuickSlot(Index);return true;}
        if(Left||Right)QuickIndex=FMath::Clamp(QuickIndex+(Right?1:-1),0,3);
        else if(Accept&&!Repeat)ChooseQuickSlot(QuickIndex);else if(Back&&!Repeat)CancelPending();
        RefreshPresentation();NotifyNavigation();return true;
    }
    if(bDetailOpen)return CastChecked<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")))->HandleKey(Key,Repeat);
    if(NavigationArea==5&&(Key==EKeys::Delete||Key==EKeys::BackSpace)){if(!Repeat&&Inventory())Inventory()->ClearQuickSlot(QuickIndex);RefreshPresentation();NotifyNavigation();return true;}
    if(!Up&&!Down&&!Left&&!Right&&!Accept)return false;
    if(NavigationArea==0)
    {if(Left||(Up&&SelectedMember==0))return false;else if(Up||Down)SelectMember(FMath::Clamp(SelectedMember+(Down?1:-1),0,FMath::Max(0,Members.Num()-1)));else if(Right||Accept)NavigationArea=1;}
    else if(NavigationArea==1)
    {if(Left&&EquipmentIndex==0)NavigationArea=0;else if(Right&&EquipmentIndex+1>=EquipmentCells.Num())NavigationArea=2;else if(Left||Right)EquipmentIndex=FMath::Clamp(EquipmentIndex+(Right?1:-1),0,FMath::Max(0,EquipmentCells.Num()-1));else if(Down)NavigationArea=5;else if(Up)return false;else if(Accept&&!Repeat&&EquipmentCells.IsValidIndex(EquipmentIndex))EquipmentPicked(EquipmentCells[EquipmentIndex]);}
    else if(NavigationArea==2)
    {if(Up)return false;else if(Left)SelectItemTab(EPlayerItemTab::Normal);else if(Right)SelectItemTab(EPlayerItemTab::Quest);else if(Down||Accept)NavigationArea=3;}
    else if(NavigationArea==3)
    {
        if(Up&&SelectedItem<5)NavigationArea=2;
        else if(Left&&SelectedItem%5==0)NavigationArea=1;
        else if(Down&&SelectedItem+5>=VisibleBagIndices.Num())NavigationArea=5;
        else if(Up||Down||Left||Right)SelectItem(FMath::Clamp(SelectedItem+(Up?-5:Down?5:Right?1:-1),0,FMath::Max(0,VisibleBagIndices.Num()-1)));
        else if(Accept&&!Repeat&&!CurrentItem().IsEmpty())OpenItem(SelectedItem);
    }
    else if(NavigationArea==5)
    {if(Up)NavigationArea=1;else if(Accept&&!Repeat)QuickPicked(QuickIndex);else if(Left&&QuickIndex==0)NavigationArea=0;else if(Right&&QuickIndex==3)NavigationArea=2;else if(Left||Right)QuickIndex=FMath::Clamp(QuickIndex+(Right?1:-1),0,3);}
    RefreshEquipment();RefreshPresentation();NotifyNavigation();return true;
}
