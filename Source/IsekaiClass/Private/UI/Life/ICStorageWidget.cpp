#include "Settings/ICGameUserSettings.h"
#include "UI/Life/ICStorageWidget.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"

namespace ICStorageLayout
{
UTextBlock* Label(UWidgetTree* Tree,FName Name,FText Value,int32 Size=20)
{
    auto* Text=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Text->SetText(Value);
    Text->SetFont(ICFrontendLayout::Font(Size));Text->SetColorAndOpacity(FLinearColor(.94f,.95f,.98f));Text->SetAutoWrapText(true);return Text;
}
void Caption(UUserWidget* UI,FName Name,FText Value){if(auto* Text=Cast<UTextBlock>(UI->GetWidgetFromName(Name)))Text->SetText(Value);}
void Row(UVerticalBox* Column,UWidget* Child,float Gap=12){Column->AddChildToVerticalBox(Child)->SetPadding(FMargin(0,0,0,Gap));}
UButton* Button(UWidgetTree* Tree,FName Name,FText Value)
{
    auto* Result=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name);Result->SetBackgroundColor(FLinearColor(.14f,.23f,.31f));
    auto* Text=Label(Tree,FName(*(Name.ToString()+TEXT("_Text"))),Value);Text->SetAutoWrapText(false);Text->SetJustification(ETextJustify::Center);
    Result->SetContent(Text);CastChecked<UButtonSlot>(Result->GetContent()->Slot)->SetPadding(FMargin(12,10));return Result;
}
void Fill(UOverlay* Root,UWidget* Child){auto* Placement=Root->AddChildToOverlay(Child);Placement->SetHorizontalAlignment(HAlign_Fill);Placement->SetVerticalAlignment(VAlign_Fill);}
UVerticalBox* Panel(UWidgetTree* Tree,UOverlay* Root,float Width,float Height)
{
    auto* Fit=Tree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetStretchDirection(EStretchDirection::DownOnly);Fill(Root,Fit);
    auto* Box=Tree->ConstructWidget<USizeBox>();Box->SetWidthOverride(Width);Box->SetHeightOverride(Height);Fit->SetContent(Box);
    auto* Frame=Tree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.035f,.055f,.08f));Frame->SetPadding(FMargin(26));Box->SetContent(Frame);
    auto* Column=Tree->ConstructWidget<UVerticalBox>();Frame->SetContent(Column);return Column;
}
UICUIManagerSubsystem* Manager(UUserWidget* Widget){auto* Local=Widget->GetOwningLocalPlayer();return Local?Local->GetSubsystem<UICUIManagerSubsystem>():nullptr;}
UClass* WidgetClass(const TCHAR* Name,UClass* Fallback)
{auto* Type=LoadClass<UUserWidget>(nullptr,*FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"),Name,Name));return Type?Type:Fallback;}
}

bool UICStorageTransferPopup::BuildLayout(UWidgetTree* Tree)
{
    using namespace ICStorageLayout;auto* Root=Tree->ConstructWidget<UOverlay>();Tree->RootWidget=Root;
    auto* Shade=Tree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.8f));Fill(Root,Shade);
    auto* Body=Panel(Tree,Root,620,480);
    Row(Body,Label(Tree,TEXT("TXT_ItemName"),FText::GetEmpty(),28));
    auto* Description=Tree->ConstructWidget<UScrollBox>();Description->AddChild(Label(Tree,TEXT("TXT_Description"),FText::GetEmpty()));Body->AddChildToVerticalBox(Description)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* QuantityRow=Tree->ConstructWidget<UHorizontalBox>();Row(Body,QuantityRow);
    QuantityRow->AddChildToHorizontalBox(Button(Tree,TEXT("BTN_Less"),FText::FromString(TEXT("<"))));
    auto* QuantityText=Label(Tree,TEXT("TXT_Quantity"),FText::GetEmpty(),24);QuantityText->SetJustification(ETextJustify::Center);
    auto* NumberPlacement=QuantityRow->AddChildToHorizontalBox(QuantityText);NumberPlacement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NumberPlacement->SetVerticalAlignment(VAlign_Center);
    QuantityRow->AddChildToHorizontalBox(Button(Tree,TEXT("BTN_More"),FText::FromString(TEXT(">"))));
    Row(Body,Label(Tree,TEXT("TXT_TransferNotice"),FText::GetEmpty(),17),6);
    Row(Body,Button(Tree,TEXT("BTN_Transfer"),NSLOCTEXT("ICStorage","Deposit","창고로 이동")),8);
    Row(Body,Button(Tree,TEXT("BTN_TransferBack"),NSLOCTEXT("ICStorage","Back","뒤로")),0);return true;
}
void UICStorageTransferPopup::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Less")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Decrease);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_More")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Increase);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Transfer")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Move);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_TransferBack")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);
    for(const auto* Name:{TEXT("BTN_Less_Text"),TEXT("BTN_More_Text"),TEXT("BTN_Transfer_Text"),TEXT("BTN_TransferBack_Text")})
        if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(Name))){Text->SetAutoWrapText(false);Text->SetJustification(ETextJustify::Center);}
}
void UICStorageTransferPopup::Present(FText Name,FText Description,int32 Count,int32 Maximum,bool ToStorage,FText Notice)
{
    using namespace ICStorageLayout;Caption(this,TEXT("TXT_ItemName"),Name);Caption(this,TEXT("TXT_Description"),Description);
    Caption(this,TEXT("TXT_Quantity"),FText::Format(NSLOCTEXT("ICStorage","Amount","{0} / {1}개"),FText::AsNumber(Count),FText::AsNumber(Maximum)));
    Caption(this,TEXT("TXT_TransferNotice"),Notice);Caption(this,TEXT("BTN_Transfer_Text"),ToStorage?NSLOCTEXT("ICStorage","Deposit","창고로 이동"):NSLOCTEXT("ICStorage","Withdraw","인벤토리로 이동"));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Less")))->SetIsEnabled(Count>1);CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_More")))->SetIsEnabled(Count<Maximum);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Transfer")))->SetIsEnabled(Maximum>0);Highlight();
}
void UICStorageTransferPopup::Highlight()
{
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Transfer")))->SetBackgroundColor(Action==0?FLinearColor(.55f,.4f,.13f):FLinearColor(.14f,.23f,.31f));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_TransferBack")))->SetBackgroundColor(Action==1?FLinearColor(.55f,.4f,.13f):FLinearColor(.14f,.23f,.31f));
}
bool UICStorageTransferPopup::HandleKey(FKey Key,bool Repeat)
{
    if((Key==EKeys::Escape||UICGameUserSettings::IsGamepadBackKey(Key))){if(!Repeat)Back();return true;}
    if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left){Decrease();return true;}if(Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right){Increase();return true;}
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up){Action=0;Highlight();return true;}if(Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down){Action=1;Highlight();return true;}
    if(Key==EKeys::Enter||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key)){if(!Repeat){if(Action==0)Move();else Back();}return true;}return false;
}

bool UICStorageWidget::BuildLayout(UWidgetTree* Tree)
{
    using namespace ICStorageLayout;auto* Root=Tree->ConstructWidget<UOverlay>();Tree->RootWidget=Root;auto* Body=Panel(Tree,Root,1360,790);
    auto* Heading=Tree->ConstructWidget<UHorizontalBox>();Row(Body,Heading);
    Heading->AddChildToHorizontalBox(Label(Tree,NAME_None,NSLOCTEXT("ICStorage","Title","개인 창고"),30))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Heading->AddChildToHorizontalBox(Button(Tree,TEXT("BTN_StorageClose"),NSLOCTEXT("ICStorage","Close","닫기")));
    auto* Columns=Tree->ConstructWidget<UHorizontalBox>();Body->AddChildToVerticalBox(Columns)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    for(int32 Side=0;Side<2;++Side)
    {
        auto* Frame=Tree->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.06f,.09f,.13f));Frame->SetPadding(FMargin(12));
        auto* Placement=Columns->AddChildToHorizontalBox(Frame);Placement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Placement->SetPadding(Side==0?FMargin(0,0,12,0):FMargin(12,0,0,0));
        auto* Column=Tree->ConstructWidget<UVerticalBox>();Frame->SetContent(Column);
        Row(Column,Label(Tree,Side==0?TEXT("TXT_ChestTitle"):TEXT("TXT_BagTitle"),FText::GetEmpty(),23));
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),Side==0?TEXT("ChestScroll"):TEXT("StorageBagScroll"));Column->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Grid=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),Side==0?TEXT("ChestGrid"):TEXT("StorageBagGrid"));Grid->SetSlotPadding(FMargin(2));Scroll->AddChild(Grid);
    }
    Row(Body,Label(Tree,TEXT("TXT_StorageNotice"),FText::GetEmpty(),18),5);
    Row(Body,Label(Tree,NAME_None,NSLOCTEXT("ICStorage","Keys","클릭 · Enter 정보/이동  |  WASD 이동  |  Tab 양쪽 전환  |  ESC 닫기"),17),0);
    auto* Popup=Tree->ConstructWidget<UICStorageTransferPopup>(WidgetClass(TEXT("WBP_IC_StorageTransferPopup"),UICStorageTransferPopup::StaticClass()),TEXT("TransferPopup"));Fill(Root,Popup);Popup->SetVisibility(ESlateVisibility::Collapsed);return true;
}
void UICStorageWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_StorageClose")))->OnClicked.AddUniqueDynamic(this,&ThisClass::CloseStorage);
    auto* Popup=CastChecked<UICStorageTransferPopup>(GetWidgetFromName(TEXT("TransferPopup")));Popup->OnQuantity.AddUniqueDynamic(this,&ThisClass::AdjustQuantity);Popup->OnMove.AddUniqueDynamic(this,&ThisClass::MovePressed);Popup->OnBack.AddUniqueDynamic(this,&ThisClass::CloseItem);
}
bool UICStorageWidget::OpenStorage(FName ID)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC||ID.IsNone()||bOpen)return false;Inventory=PC->GetInventory();State=PC->GetWorldState();
    if(!Inventory||!State)return false;const auto Snapshot=Inventory->GetSnapshot();const auto* Chest=Snapshot.Storage.Find(ID);if(!Chest||Chest->Slots.Num()!=40)return false;
    ContainerID=ID;bOpen=true;Inventory->OnChanged.AddUniqueDynamic(this,&ThisClass::RefreshStorage);State->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::CloseStorage);
    auto* UI=ICStorageLayout::Manager(this);if(!UI||!UI->OpenScreen(this,true,190)){bOpen=false;return false;}
    RefreshStorage();SetKeyboardFocus();return true;
}
void UICStorageWidget::CloseStorage()
{
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->CloseChestPresentation();
    if(!bOpen)return;bOpen=false;bPopupOpen=false;
    if(Inventory)Inventory->OnChanged.RemoveDynamic(this,&ThisClass::RefreshStorage);if(State)State->OnWorldRestored.RemoveDynamic(this,&ThisClass::CloseStorage);
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->StorageScreen==this){PC->StorageScreen=nullptr;PC->OpenStorage=NAME_None;}
    if(auto* UI=ICStorageLayout::Manager(this))UI->CloseScreen(this);else RemoveFromParent();
}
void UICStorageWidget::NativeDestruct()
{
    if(Inventory)Inventory->OnChanged.RemoveDynamic(this,&ThisClass::RefreshStorage);if(State)State->OnWorldRestored.RemoveDynamic(this,&ThisClass::CloseStorage);
    Super::NativeDestruct();
}
void UICStorageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // Slate can rebuild the screen during mounting or viewport changes.
    if(bOpen){if(Inventory)Inventory->OnChanged.AddUniqueDynamic(this,&ThisClass::RefreshStorage);if(State)State->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::CloseStorage);}
}
FPlayerItemInstance UICStorageWidget::CurrentItem() const
{
    if(!Inventory||!bPopupOpen)return {};const auto Snapshot=Inventory->GetSnapshot();const auto* Chest=Snapshot.Storage.Find(ContainerID);
    const auto* Source=bDetailFromStorage?Chest:&Snapshot.Bag;return Source&&Source->Slots.IsValidIndex(DetailSlot)?Source->Slots[DetailSlot]:FPlayerItemInstance();
}
void UICStorageWidget::RefreshStorage()
{
    if(!bOpen||!Inventory||bRefreshing||bSubmitting)return;TGuardValue<bool> Guard(bRefreshing,true);using namespace ICStorageLayout;
    const auto Snapshot=Inventory->GetSnapshot();const auto* Chest=Snapshot.Storage.Find(ContainerID);if(!Chest){CloseStorage();return;}
    Caption(this,TEXT("TXT_ChestTitle"),NSLOCTEXT("ICStorage","FortySlots","창고 · 40칸 고정"));
    Caption(this,TEXT("TXT_BagTitle"),FText::Format(NSLOCTEXT("ICStorage","BagCapacity","인벤토리 · {0}칸"),FText::AsNumber(Snapshot.Bag.Slots.Num())));
    for(int32 Side=0;Side<2;++Side)
    {
        auto* Grid=CastChecked<UUniformGridPanel>(GetWidgetFromName(Side==0?TEXT("ChestGrid"):TEXT("StorageBagGrid")));const auto& Slots=Side==0?Chest->Slots:Snapshot.Bag.Slots;
        if(Grid->GetChildrenCount()!=Slots.Num())
        {
            Grid->ClearChildren();for(int32 Index=0;Index<Slots.Num();++Index){auto* Cell=WidgetTree->ConstructWidget<UICInventorySlotWidget>(WidgetClass(TEXT("WBP_IC_InventorySlot"),UICInventorySlotWidget::StaticClass()));Cell->OnPicked.AddUniqueDynamic(this,&ThisClass::CellPicked);Grid->AddChildToUniformGrid(Cell,Index/8,Index%8);}
        }
        for(int32 Index=0;Index<Slots.Num();++Index)
        {
            const auto& Item=Slots[Index];const auto* Definition=Inventory->Find(Item.ItemID);
            auto* Cell=CastChecked<UICInventorySlotWidget>(Grid->GetChildAt(Index));Cell->Present(Side*1000+Index,Definition?Definition->GetLabel():FText::GetEmpty(),Definition?Definition->Icon.LoadSynchronous():nullptr,Item.Count,bStorageSide==(Side==0)&&Selected==Index);
        }
    }
    if(bPopupOpen){const auto Item=CurrentItem();if(Item.IsEmpty()||Item.InstanceID!=DetailInstance)CloseItem();else RefreshPopup();}
}
bool UICStorageWidget::OpenItem(bool FromStorage,int32 Index)
{
    if(!bOpen||bSubmitting||bPopupOpen)return false;bDetailFromStorage=FromStorage;DetailSlot=Index;bPopupOpen=true;const auto Item=CurrentItem();
    if(Item.IsEmpty()){bPopupOpen=false;return false;}
    DetailInstance=Item.InstanceID;bStorageSide=FromStorage;Selected=Index;Quantity=1;PopupNotice=FText::GetEmpty();
    CastChecked<UICStorageTransferPopup>(GetWidgetFromName(TEXT("TransferPopup")))->ResetSelection();
    GetWidgetFromName(TEXT("TransferPopup"))->SetVisibility(ESlateVisibility::Visible);RefreshStorage();RefreshPopup();SetKeyboardFocus();return true;
}
void UICStorageWidget::RefreshPopup()
{
    const auto Item=CurrentItem();const auto* Definition=Inventory?Inventory->Find(Item.ItemID):nullptr;if(!Definition)return;
    MaximumQuantity=Inventory->GetTransferableCount(ContainerID,DetailSlot,!bDetailFromStorage);Quantity=MaximumQuantity>0?FMath::Clamp(Quantity,1,MaximumQuantity):0;
    auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));const auto Text=UICTextLibrary::ResolveText(Strings,Definition->DescriptionKey);
    FText Description=Text.bSuccess?Text.Text:NSLOCTEXT("ICStorage","NoDescription","등록된 설명이 없습니다.");
    if(Definition->RestoreHP>0||Definition->RestoreSP>0)
        Description=FText::Format(NSLOCTEXT("ICStorage","RecoveryInfo","{0}\nHP {1} · SP {2} 회복"),Description,FText::AsNumber(Definition->RestoreHP),FText::AsNumber(Definition->RestoreSP));
    if(Definition->Kind==EPlayerItemKind::Equipment)
    {
        FICDefinitionResult R;if(const auto* Gear=Inventory->GetDefinitions()->Find<FICEquipmentRow>(Definition->EquipmentID,R))
        {double Attack=0;for(const auto& M:Gear->BaseModifiers)if(M.Stat==EICStat::PhysicalAttack||M.Stat==EICStat::MagicAttack)Attack+=M.Flat;Description=FText::Format(NSLOCTEXT("ICStorage","EquipmentInfo","{0}\n기본 공격력 {1} · 강화 +{2}"),Description,FText::AsNumber(Attack),FText::AsNumber(Item.Upgrade));}
    }
    if(MaximumQuantity==0&&PopupNotice.IsEmpty())PopupNotice=NSLOCTEXT("ICStorage","ReservedItem","잠금·제작 예약된 아이템은 이동할 수 없습니다.");
    CastChecked<UICStorageTransferPopup>(GetWidgetFromName(TEXT("TransferPopup")))->Present(Definition->GetLabel(),Description,Quantity,MaximumQuantity,!bDetailFromStorage,PopupNotice);
}
void UICStorageWidget::CloseItem(){bPopupOpen=false;DetailInstance.Invalidate();DetailSlot=INDEX_NONE;GetWidgetFromName(TEXT("TransferPopup"))->SetVisibility(ESlateVisibility::Collapsed);SetKeyboardFocus();}
void UICStorageWidget::AdjustQuantity(int32 Delta){if(!bPopupOpen||bSubmitting)return;Quantity=FMath::Clamp<int64>(int64(Quantity)+Delta,1,FMath::Max(1,MaximumQuantity));PopupNotice=FText::GetEmpty();RefreshPopup();}
bool UICStorageWidget::TransferSelected()
{
    if(!bOpen||!bPopupOpen||!Inventory||bSubmitting)return false;const auto Item=CurrentItem();
    if(Item.IsEmpty()||Item.InstanceID!=DetailInstance){CloseItem();RefreshStorage();return false;}
    bool Success=false;{TGuardValue<bool> Guard(bSubmitting,true);Success=Inventory->TransferAmount(ContainerID,DetailSlot,Quantity,!bDetailFromStorage,PopupNotice);}
    if(Success){CloseItem();ICStorageLayout::Caption(this,TEXT("TXT_StorageNotice"),NSLOCTEXT("ICStorage","Moved","선택한 수량을 이동했습니다."));}
    RefreshStorage();if(!Success)RefreshPopup();SetKeyboardFocus();return Success;
}
void UICStorageWidget::CellPicked(int32 Cell){OpenItem(Cell<1000,Cell%1000);}
FReply UICStorageWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const auto Key=Event.GetKey();if(bPopupOpen){if(CastChecked<UICStorageTransferPopup>(GetWidgetFromName(TEXT("TransferPopup")))->HandleKey(Key,Event.IsRepeat()))return FReply::Handled();return FReply::Handled();}
    if((Key==EKeys::Escape||UICGameUserSettings::IsGamepadBackKey(Key))){if(!Event.IsRepeat())CloseStorage();return FReply::Handled();}
    if(Key==EKeys::Tab||Key==EKeys::Gamepad_LeftShoulder||Key==EKeys::Gamepad_RightShoulder){if(!Event.IsRepeat()){bStorageSide=!bStorageSide;Selected=0;RefreshStorage();}return FReply::Handled();}
    int32 Delta=0;if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up)Delta=-8;else if(Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down)Delta=8;else if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left)Delta=-1;else if(Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right)Delta=1;
    if(Delta){const int32 Count=bStorageSide?40:Inventory->GetSnapshot().Bag.Slots.Num();Selected=FMath::Clamp(Selected+Delta,0,Count-1);RefreshStorage();auto* Grid=CastChecked<UUniformGridPanel>(GetWidgetFromName(bStorageSide?TEXT("ChestGrid"):TEXT("StorageBagGrid")));CastChecked<UScrollBox>(GetWidgetFromName(bStorageSide?TEXT("ChestScroll"):TEXT("StorageBagScroll")))->ScrollWidgetIntoView(Grid->GetChildAt(Selected));return FReply::Handled();}
    if(Key==EKeys::Enter||Key==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(Key)){if(!Event.IsRepeat())OpenItem(bStorageSide,Selected);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

