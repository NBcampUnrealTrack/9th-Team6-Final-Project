#include "UI/Life/ICCraftingStationWidget.h"
#include "Life/Crafting/ICCraftingStation.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Data/Rows/Life/ICRecipeRow.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

namespace ICCraftingLayout
{
UTextBlock* Caption(UWidgetTree* T,FName Name,const FText& Text,int32 Size=22)
{
    auto* C=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);C->SetText(Text);C->SetFont(ICFrontendLayout::Font(Size));
    C->SetColorAndOpacity(FLinearColor(.94f,.94f,.9f));C->SetAutoWrapText(true);return C;
}
void Add(UVerticalBox* Box,UWidget* Child,float Bottom=16)
{auto* S=Box->AddChildToVerticalBox(Child);S->SetPadding(FMargin(0,0,0,Bottom));}
UButton* ActionButton(UWidgetTree* T,FName Name,const FText& Text)
{
    auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),Name);auto* C=Caption(T,NAME_None,Text);C->SetAutoWrapText(false);
    auto* Slot=Cast<UButtonSlot>(B->SetContent(C));Slot->SetPadding(FMargin(14,8));
    B->SetBackgroundColor(FLinearColor(.12f,.22f,.28f));return B;
}
void Text(UUserWidget* UI,FName Name,const FText& Value)
{if(auto* W=Cast<UTextBlock>(UI->GetWidgetFromName(Name)))W->SetText(Value);}
FText ItemName(AICPlayerController* PC,FName ID)
{const auto* D=PC&&PC->GetInventory()?PC->GetInventory()->Find(ID):nullptr;return D&&!D->DisplayName.IsEmpty()?D->DisplayName:FText::FromName(ID);}
}

UICCraftingStationWidget::UICCraftingStationWidget(const FObjectInitializer& Initializer):Super(Initializer){SetIsFocusable(true);}
void UICCraftingStationWidget::BuildLayout(UWidgetTree* T)
{
    using namespace ICCraftingLayout;
    auto* Root=T->ConstructWidget<UOverlay>();T->RootWidget=Root;
    auto* Shade=T->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.65f));auto* ShadeSlot=Root->AddChildToOverlay(Shade);ShadeSlot->SetHorizontalAlignment(HAlign_Fill);ShadeSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Size=T->ConstructWidget<USizeBox>();Size->SetWidthOverride(720);Size->SetHeightOverride(620);
    auto* Slot=Root->AddChildToOverlay(Size);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);
    auto* Card=T->ConstructWidget<UBorder>();Card->SetPadding(FMargin(32));Card->SetBrushColor(FLinearColor(.025f,.045f,.065f,.99f));Size->SetContent(Card);
    auto* Body=T->ConstructWidget<UVerticalBox>();Card->SetContent(Body);
    Add(Body,Caption(T,TEXT("TXT_StationTitle"),NSLOCTEXT("ICCraft","Recipes","만들 수 있는 레시피"),30));
    auto* Pages=T->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("StationPages"));
    auto* PageSlot=Body->AddChildToVerticalBox(Pages);PageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* List=T->ConstructWidget<UVerticalBox>();Pages->AddChild(List);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("RecipeScroll"));Scroll->SetAlwaysShowScrollbar(true);
    auto* ScrollSlot=List->AddChildToVerticalBox(Scroll);ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Add(List,Caption(T,TEXT("TXT_Empty"),FText::GetEmpty(),18),8);
    Add(List,Caption(T,NAME_None,NSLOCTEXT("ICCraft","Keys","W / S · ↑ / ↓ 선택     Enter 상세 보기     Esc 닫기"),16),0);
    auto* Detail=T->ConstructWidget<UVerticalBox>();Pages->AddChild(Detail);
    auto* DetailScroll=T->ConstructWidget<UScrollBox>();auto* DS=Detail->AddChildToVerticalBox(DetailScroll);DS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Content=T->ConstructWidget<UVerticalBox>();DetailScroll->AddChild(Content);
    Add(Content,Caption(T,TEXT("TXT_RecipeName"),FText::GetEmpty(),27));
    Add(Content,Caption(T,TEXT("TXT_RecipeInfo"),FText::GetEmpty(),19));
    Add(Content,Caption(T,TEXT("TXT_Ingredients"),FText::GetEmpty(),21));
    Add(Content,Caption(T,TEXT("TXT_Requirements"),FText::GetEmpty(),18));
    Add(Detail,ActionButton(T,TEXT("BTN_Make"),NSLOCTEXT("ICCraft","Make","만들기")),10);
    Add(Detail,ActionButton(T,TEXT("BTN_Back"),NSLOCTEXT("ICCraft","Cancel","취소 · 목록으로")),0);
    auto* Progress=T->ConstructWidget<UVerticalBox>();Pages->AddChild(Progress);
    Add(Progress,Caption(T,TEXT("TXT_WorkName"),FText::GetEmpty(),27),30);
    auto* Bar=T->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("BAR_Work"));Bar->SetFillColorAndOpacity(FLinearColor(.25f,.78f,.65f));
    auto* BarSize=T->ConstructWidget<USizeBox>();BarSize->SetHeightOverride(30);BarSize->SetContent(Bar);Add(Progress,BarSize,20);
    Add(Progress,Caption(T,TEXT("TXT_WorkTime"),FText::GetEmpty(),23));
    Add(Progress,Caption(T,NAME_None,NSLOCTEXT("ICCraft","Continue","창을 닫아도 제작은 계속됩니다."),17));
    Add(Progress,ActionButton(T,TEXT("BTN_Collect"),NSLOCTEXT("ICCraft","Retry","완료 아이템 다시 받기")));
    Add(Body,Caption(T,TEXT("TXT_Error"),FText::GetEmpty(),18),8);
    Add(Body,ActionButton(T,TEXT("BTN_CloseStation"),NSLOCTEXT("ICCraft","Close","닫기")),0);
}
void UICCraftingStationWidget::NativeConstruct()
{
    Super::NativeConstruct();if(!GetWidgetFromName(TEXT("StationPages")))BuildLayout(WidgetTree);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Make"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::MakeRecipe);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Back"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::ShowRecipeList);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_CloseStation"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::CloseStation);
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Collect"))))B->OnClicked.AddUniqueDynamic(this,&ThisClass::MakeRecipe);
}
void UICCraftingStationWidget::OpenStation(AICCraftingStation* Station)
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!Station||!PC||!PC->GetLocalPlayer())return;
    Workstation=Station;bOpen=true;ActiveJob.Invalidate();PC->ActiveStation=Station->RecipeType==EPlayerRecipeType::Cook?TEXT("Kitchen"):TEXT("Forge");PC->ActiveWorkAnchor=Station->WorkAnchorID;
    PC->SetMenu(TEXT("Station"));
    auto* Manager=PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    if(!Manager||!Manager->OpenScreen(this,false,500)){bOpen=false;PC->SetMenu(TEXT(""));return;}
    // Gameplay commands use PC.Menu, while this screen deliberately leaves the play clock running.
    Manager->SetMenuContext(PC,this,false);ShowRecipeList();
    if(auto* State=PC->GetWorldState())for(const auto& Pair:State->ExportState().ProductionJobs)
        if(Pair.Value.OwnerCharacterID==UICWorldStateSubsystem::PlayerId()&&Pair.Value.WorkAnchorID==Station->WorkAnchorID&&(Pair.Value.Status==EICProductionJobStatus::Running||Pair.Value.Status==EICProductionJobStatus::Ready))
        {ActiveJob=Pair.Key;SelectedRecipe=Pair.Value.RecipeID;SetPage(2);RefreshProgress();break;}
    SetKeyboardFocus();
}
void UICCraftingStationWidget::SetPage(int32 Page)
{bDetails=Page==1;if(auto* W=Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("StationPages"))))W->SetActiveWidgetIndex(Page);}
void UICCraftingStationWidget::RefreshList()
{
    using namespace ICCraftingLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC||!Workstation||!PC->GetCrafting())return;
    Recipes=PC->GetCrafting()->GetRecipes(Workstation->RecipeType);
    const auto State=PC->GetWorldState()->ExportState();
    Recipes.RemoveAll([&](FName ID){const auto* R=PC->GetCrafting()->Find(ID);return !R||R->ResultKind!=EICProductionResultKind::BagItems||(!R->bUnlockedByDefault&&!State.UnlockedRecipeIDs.Contains(ID));});
    auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("RecipeScroll")));if(!Scroll)return;Scroll->ClearChildren();
    for(FName ID:Recipes)
    {
        auto* B=WidgetTree->ConstructWidget<UICRecipeChoiceButton>();B->RecipeID=ID;B->StationUI=this;B->OnClicked.AddDynamic(B,&UICRecipeChoiceButton::Choose);
        auto* Name=Caption(WidgetTree,NAME_None,PC->GetCrafting()->Find(ID)->DisplayName);Name->SetAutoWrapText(false);
        auto* ChoiceSlot=Cast<UButtonSlot>(B->SetContent(Name));ChoiceSlot->SetPadding(FMargin(14,9));Scroll->AddChild(B);
    }
    Text(this,TEXT("TXT_Empty"),Recipes.IsEmpty()?NSLOCTEXT("ICCraft","NoRecipes","아직 사용할 수 있는 레시피가 없습니다."):FText::GetEmpty());
    Highlight(FMath::Max(0,Recipes.IndexOfByKey(SelectedRecipe)));
}
void UICCraftingStationWidget::Highlight(int32 Index)
{
    if(!Recipes.IsValidIndex(Index))return;SelectedRecipe=Recipes[Index];
    if(auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("RecipeScroll"))))
    {
        for(int32 I=0;I<Scroll->GetChildrenCount();++I)if(auto* B=Cast<UButton>(Scroll->GetChildAt(I)))B->SetBackgroundColor(I==Index?FLinearColor(.18f,.48f,.48f):FLinearColor(.08f,.13f,.18f));
        Scroll->ScrollWidgetIntoView(Scroll->GetChildAt(Index),true,EDescendantScrollDestination::IntoView);
    }
}
void UICRecipeChoiceButton::Choose(){if(StationUI.IsValid())StationUI->SelectRecipe(RecipeID);}
void UICCraftingStationWidget::SelectRecipe(FName RecipeID)
{if(ActiveJob.IsValid()||!Recipes.Contains(RecipeID))return;SelectedRecipe=RecipeID;SetPage(1);RefreshDetails();SetKeyboardFocus();}
void UICCraftingStationWidget::ShowRecipeList()
{
    if(ActiveJob.IsValid())return;SetPage(0);RefreshList();
    ICCraftingLayout::Text(this,TEXT("TXT_StationTitle"),Workstation&&Workstation->RecipeType==EPlayerRecipeType::Cook?NSLOCTEXT("ICCraft","CookingRecipes","주방 · 요리 레시피"):NSLOCTEXT("ICCraft","CraftRecipes","제작대 · 만들 수 있는 레시피"));
    ICCraftingLayout::Text(this,TEXT("TXT_Error"),FText::GetEmpty());SetKeyboardFocus();
}
void UICCraftingStationWidget::RefreshDetails()
{
    using namespace ICCraftingLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());const auto* R=PC&&PC->GetCrafting()?PC->GetCrafting()->Find(SelectedRecipe):nullptr;if(!R)return;
    Text(this,TEXT("TXT_RecipeName"),R->DisplayName);
    FText Info;auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
    if(!R->DescriptionKey.IsEmpty()){const auto Found=UICTextLibrary::ResolveText(Strings,R->DescriptionKey);if(Found.bSuccess)Info=Found.Text;}
    if(Info.IsEmpty())Info=R->Type==EPlayerRecipeType::Cook?NSLOCTEXT("ICCraft","DefaultCookInfo","재료를 조리해 음식을 만듭니다."):NSLOCTEXT("ICCraft","DefaultCraftInfo","재료를 조합해 아이템을 만듭니다.");
    FString Output;for(const auto& I:R->Results)Output+=FText::Format(NSLOCTEXT("ICCraft","ResultLine","{0} × {1}  "),ItemName(PC,I.ItemID),FText::AsNumber(I.Count)).ToString();
    Text(this,TEXT("TXT_RecipeInfo"),FText::Format(NSLOCTEXT("ICCraft","Info","{0}\n결과: {1}\n제작 시간 {2}초 · SP {3}"),Info,FText::FromString(Output),FText::AsNumber(R->WorkDurationPlaySeconds),FText::AsNumber(R->SPCost)));
    FString Materials=TEXT("필요 재료\n");bool Ready=true;
    for(const auto& I:R->Ingredients){const int32 Have=PC->GetInventory()->CountItem(I.ItemID);Ready&=Have>=I.Count;Materials+=FText::Format(NSLOCTEXT("ICCraft","MaterialLine","{0}  {1} / {2}\n"),ItemName(PC,I.ItemID),FText::AsNumber(Have),FText::AsNumber(I.Count)).ToString();}
    for(const auto& I:R->Tools){const int32 Have=PC->GetInventory()->CountItem(I.ItemID);Ready&=Have>=I.Count;Materials+=FText::Format(NSLOCTEXT("ICCraft","ToolLine","도구: {0}  {1} / {2}\n"),ItemName(PC,I.ItemID),FText::AsNumber(Have),FText::AsNumber(I.Count)).ToString();}
    Text(this,TEXT("TXT_Ingredients"),FText::FromString(Materials));Ready&=PC->GetPlayerSnapshot().SP>=R->SPCost;
    Text(this,TEXT("TXT_Requirements"),Ready?FText::GetEmpty():NSLOCTEXT("ICCraft","Missing","재료, 도구 또는 SP가 부족합니다."));
    if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Make"))))B->SetIsEnabled(Ready);
}
void UICCraftingStationWidget::MakeRecipe()
{
    using namespace ICCraftingLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC||!Workstation||PC->IsDead()||PC->bInCombat||PC->bInEvent||PC->bInDialogue)return;
    FString Reason;
    if(ActiveJob.IsValid())
    {auto* Production=GetGameInstance()->GetSubsystem<UICProductionSubsystem>();if(Production->CompleteJob(ActiveJob,PC->GetInventory(),Reason))CloseStation();else Text(this,TEXT("TXT_Error"),FText::FromString(Reason));return;}
    if(!Recipes.Contains(SelectedRecipe))return;
    if(!PC->GetCrafting()->StartCraft(SelectedRecipe,Workstation->WorkAnchorID,PC->GetInventory(),ActiveJob,Reason))
    {Text(this,TEXT("TXT_Error"),FText::FromString(Reason));RefreshDetails();return;}
    SetPage(2);Text(this,TEXT("TXT_Error"),FText::GetEmpty());RefreshProgress();SetKeyboardFocus();
}
void UICCraftingStationWidget::RefreshProgress()
{
    using namespace ICCraftingLayout;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC||!PC->GetWorldState()||!PC->GetTime())return;
    const auto State=PC->GetWorldState()->ExportState();const auto* Job=State.ProductionJobs.Find(ActiveJob);if(!Job){CloseStation();return;}
    if(Job->Status==EICProductionJobStatus::Completed||Job->Status==EICProductionJobStatus::Cancelled){CloseStation();return;}
    const auto* R=PC->GetCrafting()->Find(Job->RecipeID);Text(this,TEXT("TXT_WorkName"),R?R->DisplayName:FText::FromName(Job->RecipeID));
    const double Total=FMath::Max(.001,Job->CompletesAtPlaySecond-Job->StartedAtPlaySecond),Left=FMath::Max(0.,Job->CompletesAtPlaySecond-PC->GetTime()->GetClock().PlaySecond);
    if(auto* Bar=Cast<UProgressBar>(GetWidgetFromName(TEXT("BAR_Work"))))Bar->SetPercent(FMath::Clamp(1.-Left/Total,0.,1.));
    const bool Pending=Job->Status==EICProductionJobStatus::Ready;
    Text(this,TEXT("TXT_WorkTime"),Pending?NSLOCTEXT("ICCraft","Ready","완료 대기 · 가방 공간과 재료를 확인해 주세요."):FText::Format(NSLOCTEXT("ICCraft","Remaining","제작 중… {0} / {1}초"),FText::AsNumber(FMath::RoundToInt(Total-Left)),FText::AsNumber(Total)));
    if(auto* B=GetWidgetFromName(TEXT("BTN_Collect")))B->SetVisibility(Pending?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
}
void UICCraftingStationWidget::CloseStation()
{
    if(!bOpen)return;bOpen=false;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(PC&&PC->GetLocalPlayer())if(auto* UI=PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->CloseScreen(this);
    if(PC&&PC->Menu==TEXT("Station"))PC->SetMenu(TEXT(""));Workstation=nullptr;ActiveJob.Invalidate();
}
void UICCraftingStationWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);if(!bOpen)return;auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!IsValid(Workstation)||!PC||PC->Menu!=TEXT("Station")||PC->IsDead()||PC->bInCombat){CloseStation();return;}
    if(ActiveJob.IsValid())RefreshProgress();else if(bDetails&&(RefreshAt-=DeltaSeconds)<=0){RefreshAt=.3f;RefreshDetails();}
}
FReply UICCraftingStationWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.IsRepeat()&&Event.GetKey()==EKeys::Enter)return FReply::Handled();
    return HandleStationKey(Event.GetKey())?FReply::Handled():Super::NativeOnPreviewKeyDown(Geometry,Event);
}
bool UICCraftingStationWidget::HandleStationKey(FKey Key)
{
    if(!bOpen)return false;
    if(Key==EKeys::Escape){if(bDetails)ShowRecipeList();else CloseStation();return true;}
    if(bDetails&&Key==EKeys::Enter)
    {if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Make")));B&&B->GetIsEnabled())MakeRecipe();return true;}
    if(!ActiveJob.IsValid()&&!bDetails)
    {
        if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::S||Key==EKeys::Down){const int32 Step=Key==EKeys::W||Key==EKeys::Up?-1:1;Highlight(FMath::Clamp(Recipes.IndexOfByKey(SelectedRecipe)+Step,0,Recipes.Num()-1));return true;}
        if(Key==EKeys::Enter){SelectRecipe(SelectedRecipe);return true;}
    }
    return false;
}
