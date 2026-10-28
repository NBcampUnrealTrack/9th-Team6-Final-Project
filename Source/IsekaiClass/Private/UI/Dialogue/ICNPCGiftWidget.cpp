#include "UI/Dialogue/ICNPCGiftWidget.h"
#include "Settings/ICGameUserSettings.h"
#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "UI/Gameplay/ICGameplayLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Internationalization/StringTable.h"

namespace
{
constexpr int32 GiftGridColumns=5;
FText ItemName(const FICItemRow* Row,FName ID)
{return Row&&!Row->DisplayName.IsEmpty()?Row->DisplayName:FText::FromName(ID);}
}
void UICNPCGiftWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    if(WidgetTree&&!WidgetTree->RootWidget)ICGameplayLayout::Build(WidgetTree,TEXT("NPCGiftPicker"));
    CellClass=LoadClass<UICInventorySlotWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_InventorySlot.WBP_IC_InventorySlot_C"));
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_GiftBack")))->OnClicked.AddUniqueDynamic(this,&ThisClass::ClosePicker);
    CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("GiftConfirm")))->OnResolved.AddUniqueDynamic(this,&ThisClass::ResolveGift);
}
void UICNPCGiftWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState)
    {
        BoundState->OnInventoryChanged.AddUniqueDynamic(this,&ThisClass::RefreshItems);
        BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::ClosePicker);
    }
}
void UICNPCGiftWidget::NativeDestruct()
{
    if(BoundState)
    {
        BoundState->OnInventoryChanged.RemoveDynamic(this,&ThisClass::RefreshItems);
        BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::ClosePicker);
    }
    BoundState=nullptr;Super::NativeDestruct();
}
void UICNPCGiftWidget::OpenForNPC(AICNPCBase* InNPC)
{NPC=InNPC;bClosed=false;bConfirming=false;SelectedIndex=0;bBackSelected=false;RefreshItems();}
void UICNPCGiftWidget::RefreshItems()
{
    if(bSubmitting||bClosed||!GetGameInstance())return;
    auto* Grid=Cast<UGridPanel>(GetWidgetFromName(TEXT("GiftGrid")));if(!Grid||!CellClass)return;
    const FName Previous=SelectedID;
    const auto Snapshot=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->GetInventorySnapshot();
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();ItemIDs.Reset();Counts.Reset();
    for(const auto& Item:Snapshot.Bag.Slots)
    {
        if(Item.IsEmpty()||Item.bLocked)continue;
        FICDefinitionResult Result;const auto* Row=D->Find<FICItemRow>(Item.ItemID,Result);
        if(!Row||!Row->bCanGift||Row->Kind==EPlayerItemKind::Quest||Row->Tab==EPlayerItemTab::Quest)continue;
        if(!Counts.Contains(Item.ItemID))ItemIDs.Add(Item.ItemID);
        Counts.FindOrAdd(Item.ItemID)=static_cast<int32>(FMath::Min<int64>(MAX_int32,int64(Counts.FindRef(Item.ItemID))+Item.Count));
    }
    Grid->ClearChildren();Cells.Reset();
    const int32 Capacity=FMath::Max(20,FMath::DivideAndRoundUp(ItemIDs.Num(),GiftGridColumns)*GiftGridColumns);
    for(int32 Index=0;Index<Capacity;++Index)
    {
        auto* Cell=CreateWidget<UICInventorySlotWidget>(GetOwningPlayer(),CellClass);
        Cell->SetClipping(EWidgetClipping::ClipToBounds);
        Cell->OnPicked.AddUniqueDynamic(this,&ThisClass::ItemPicked);
        Grid->AddChildToGrid(Cell,Index/GiftGridColumns,Index%GiftGridColumns)->SetPadding(FMargin(4));Cells.Add(Cell);
    }
    if(bConfirming&&!Counts.Contains(PendingID))
    {bConfirming=false;PendingID=NAME_None;GetWidgetFromName(TEXT("GiftConfirm"))->SetVisibility(ESlateVisibility::Collapsed);}
    const int32 PreviousIndex=ItemIDs.IndexOfByKey(Previous);
    Select(PreviousIndex==INDEX_NONE?0:PreviousIndex,ItemIDs.IsEmpty());
    CastChecked<UTextBlock>(GetWidgetFromName(TEXT("GiftStatus")))->SetText(ItemIDs.IsEmpty()
        ?NSLOCTEXT("ICGift","Empty","선물할 수 있는 아이템이 없습니다.")
        :NSLOCTEXT("ICGift","GridPrompt","아이템을 선택하면 선물 확인창이 열립니다."));
}
void UICNPCGiftWidget::Select(int32 Index,bool Back)
{
    SelectedIndex=FMath::Clamp(Index,0,FMath::Max(0,Cells.Num()-1));bBackSelected=Back;
    SelectedID=!Back&&ItemIDs.IsValidIndex(SelectedIndex)?ItemIDs[SelectedIndex]:NAME_None;
    bFocusPending=true;RefreshPresentation();
    if(!Back&&Cells.IsValidIndex(SelectedIndex))
        CastChecked<UScrollBox>(GetWidgetFromName(TEXT("GiftScroll")))->ScrollWidgetIntoView(Cells[SelectedIndex],false,EDescendantScrollDestination::IntoView);
}
void UICNPCGiftWidget::RefreshPresentation()
{
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult Result;
    for(int32 Index=0;Index<Cells.Num();++Index)
    {
        const FName ID=ItemIDs.IsValidIndex(Index)?ItemIDs[Index]:NAME_None;
        const auto* Row=ID.IsNone()?nullptr:D->Find<FICItemRow>(ID,Result);
        Cells[Index]->Present(Index,ID.IsNone()?FText::GetEmpty():ItemName(Row,ID),Row?Row->Icon.LoadSynchronous():nullptr,Counts.FindRef(ID),!bBackSelected&&Index==SelectedIndex);
        const FLinearColor TextColor=!bBackSelected&&Index==SelectedIndex?FLinearColor(.035f,.045f,.065f):FLinearColor::White;
        for(const auto* Name:{TEXT("TXT_Name"),TEXT("TXT_Count")})
            CastChecked<UTextBlock>(Cells[Index]->GetWidgetFromName(Name))->SetColorAndOpacity(TextColor);
    }
    const auto* Row=SelectedID.IsNone()?nullptr:D->Find<FICItemRow>(SelectedID,Result);
    CastChecked<UTextBlock>(GetWidgetFromName(TEXT("GiftSelectedName")))->SetText(Row?ItemName(Row,SelectedID):FText::GetEmpty());
    CastChecked<UTextBlock>(GetWidgetFromName(TEXT("GiftSelectedCount")))->SetText(Row?FText::Format(NSLOCTEXT("ICGift","Owned","보유 {0}개"),FText::AsNumber(Counts.FindRef(SelectedID))):FText::GetEmpty());
    auto* Icon=CastChecked<UImage>(GetWidgetFromName(TEXT("GiftSelectedIcon")));auto* Texture=Row?Row->Icon.LoadSynchronous():nullptr;
    Icon->SetBrushFromTexture(Texture);Icon->SetVisibility(Texture?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    FText Description;
    if(Row&&!Row->DescriptionKey.IsEmpty())Description=UICTextLibrary::ResolveText(LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")),Row->DescriptionKey).Text;
    CastChecked<UTextBlock>(GetWidgetFromName(TEXT("GiftDescription")))->SetText(Description);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_GiftBack")))->SetBackgroundColor(bBackSelected?FLinearColor(1,.74f,.3f):FLinearColor(.16f,.21f,.3f));
    GetWidgetFromName(TEXT("GiftBody"))->SetIsEnabled(!bConfirming);
}
void UICNPCGiftWidget::MoveSelection(int32 Horizontal,int32 Vertical)
{
    if(bBackSelected){if(Vertical<0)Select(SelectedIndex);return;}
    if(Vertical<0&&SelectedIndex<GiftGridColumns)return;
    if(Vertical>0&&SelectedIndex+GiftGridColumns>=Cells.Num()){Select(SelectedIndex,true);return;}
    if(Horizontal&&((Horizontal<0&&SelectedIndex%GiftGridColumns==0)||(Horizontal>0&&SelectedIndex%GiftGridColumns==GiftGridColumns-1)))return;
    Select(SelectedIndex+Horizontal+Vertical*GiftGridColumns);
}
void UICNPCGiftWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);if(bClosed)return;
    if(!IsValid(NPC)||NPC->IsActorBeingDestroyed()){ClosePicker();return;}
    if(!bFocusPending)return;UButton* Target=nullptr;
    if(bConfirming)
    {
        auto* Confirm=CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("GiftConfirm")));
        Target=Cast<UButton>(Confirm->GetWidgetFromName(Confirm->Choice==0?TEXT("BTN_Accept"):TEXT("BTN_Cancel")));
    }
    else if(bBackSelected)Target=Cast<UButton>(GetWidgetFromName(TEXT("BTN_GiftBack")));
    else if(Cells.IsValidIndex(SelectedIndex))Target=Cast<UButton>(Cells[SelectedIndex]->GetWidgetFromName(TEXT("BTN_Select")));
    if(Target){Target->SetKeyboardFocus();bFocusPending=!Target->HasKeyboardFocus();}
}
void UICNPCGiftWidget::ItemPicked(int32 Index)
{
    if(bClosed||bConfirming||!ItemIDs.IsValidIndex(Index)||!IsValid(NPC))return;
    Select(Index);PendingID=SelectedID;PendingNPC=NPC->GetNPCDefinition().ID;
    FICDefinitionResult Result;const auto* Row=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICItemRow>(PendingID,Result);
    const auto Name=UICTextLibrary::ResolveText(LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")),NPC->GetNPCDefinition().NameKey);
    bConfirming=true;RefreshPresentation();
    CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("GiftConfirm")))->Present(FText::Format(NSLOCTEXT("ICGift","Confirm","{0}을 {1}에게 선물하시겠습니까?"),ItemName(Row,PendingID),Name.bSuccess?Name.Text:FText::FromName(PendingNPC)));
}
void UICNPCGiftWidget::ResolveGift(bool Accept)
{
    if(!bConfirming||bClosed||bSubmitting)return;
    const FName ItemID=PendingID;const FName NPCID=PendingNPC;
    bConfirming=false;PendingID=NAME_None;GetWidgetFromName(TEXT("GiftConfirm"))->SetVisibility(ESlateVisibility::Collapsed);
    RefreshPresentation();bFocusPending=true;if(!Accept)return;
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!IsValid(NPC)||!PC||NPC->GetNPCDefinition().ID!=NPCID)return;
    bSubmitting=true;const bool Given=NPC->TryGiveGift(PC,ItemID);bSubmitting=false;
    if(Given){Finish(PC->NoticeDisplayText);return;}
    RefreshItems();CastChecked<UTextBlock>(GetWidgetFromName(TEXT("GiftStatus")))->SetText(PC->NoticeDisplayText);
}
void UICNPCGiftWidget::Finish(const FText& Message)
{
    if(bClosed)return;bClosed=true;bConfirming=false;
    if(auto* Local=GetOwningLocalPlayer())Local->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(this);
    else RemoveFromParent();OnClosed.Broadcast(Message);
}
void UICNPCGiftWidget::ClosePicker(){Finish(FText::GetEmpty());}
FReply UICNPCGiftWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(bClosed)return FReply::Handled();const auto Key=Event.GetKey();
    if(bConfirming)
    {CastChecked<UICItemConfirmWidget>(GetWidgetFromName(TEXT("GiftConfirm")))->HandleKey(Key,Event.IsRepeat());bFocusPending=true;return FReply::Handled();}
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up){MoveSelection(0,-1);return FReply::Handled();}
    if(Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down){MoveSelection(0,1);return FReply::Handled();}
    if(Key==EKeys::A||Key==EKeys::Left||Key==EKeys::Gamepad_DPad_Left){MoveSelection(-1,0);return FReply::Handled();}
    if(Key==EKeys::D||Key==EKeys::Right||Key==EKeys::Gamepad_DPad_Right){MoveSelection(1,0);return FReply::Handled();}
    if(Key==EKeys::Enter||UICGameUserSettings::IsGamepadAcceptKey(Key))
    {if(!Event.IsRepeat()){if(bBackSelected)ClosePicker();else ItemPicked(SelectedIndex);}return FReply::Handled();}
    if(Key==EKeys::Escape||Key==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(Key))
    {if(!Event.IsRepeat())ClosePicker();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
