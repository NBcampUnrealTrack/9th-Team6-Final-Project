#include "UI/Codex/ICCodexMenuWidget.h"
#include "Settings/ICGameUserSettings.h"
#include "Codex/ICCodexSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"

namespace
{
const FLinearColor CodexGold(1,.76f,.3f),CodexButton(.12f,.18f,.27f);
FText Txt(const TCHAR* Value){return FText::FromString(Value);}
FText CellCaption(FText Value,float Width)
{
    if(Value.IsEmpty()||!FSlateApplication::IsInitialized())return Value;
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();const auto Font=ICFrontendLayout::Font(12);
    FString Caption=Value.ToString();if(Measure->Measure(Caption,Font).X<=Width)return Value;
    while(!Caption.IsEmpty()&&Measure->Measure(Caption+TEXT("…"),Font).X>Width)Caption.LeftChopInline(1);
    return FText::FromString(Caption+TEXT("…"));
}
UTextBlock* Label(UWidgetTree* T,FName Name,FText Text,int32 Font=17)
{
    auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);W->SetText(Text);
    W->SetFont(ICFrontendLayout::Font(Font,Font>=24));W->SetColorAndOpacity(FLinearColor(.93f,.95f,.98f));
    W->SetVisibility(ESlateVisibility::HitTestInvisible);return W;
}
void Text(UUserWidget* W,const TCHAR* Name,FText Value)
{if(auto* L=Cast<UTextBlock>(W->GetWidgetFromName(Name)))L->SetText(Value);}
void Row(UVerticalBox* P,UWidget* W,float Gap=8)
{auto* Placement=P->AddChildToVerticalBox(W);Placement->SetPadding(FMargin(0,0,0,Gap));Placement->SetHorizontalAlignment(HAlign_Fill);}
UButton* Button(UWidgetTree* T,FName Name,FText Caption,bool WithCaption=true)
{
    auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),Name);auto Style=B->GetStyle();
    for(auto* Brush:{&Style.Normal,&Style.Hovered,&Style.Pressed})
    {Brush->DrawAs=ESlateBrushDrawType::Box;Brush->Margin=FMargin(0);Brush->TintColor=FLinearColor::White;}
    B->SetStyle(Style);B->SetBackgroundColor(CodexButton);
    if(WithCaption){auto* L=Label(T,FName(*(Name.ToString()+TEXT("_Text"))),Caption);L->SetJustification(ETextJustify::Center);CastChecked<UButtonSlot>(B->SetContent(L))->SetPadding(FMargin(10));}
    return B;
}
void Fill(UOverlay* P,UWidget* W)
{auto* Placement=P->AddChildToOverlay(W);Placement->SetHorizontalAlignment(HAlign_Fill);Placement->SetVerticalAlignment(VAlign_Fill);}
UVerticalBox* Modal(UWidgetTree* T,float Width)
{
    auto* Root=T->ConstructWidget<UOverlay>();T->RootWidget=Root;
    auto* Shade=T->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(0,0,0,.88f));Fill(Root,Shade);
    auto* Frame=T->ConstructWidget<UBorder>();Frame->SetBrushColor(FLinearColor(.055f,.08f,.12f));Frame->SetPadding(FMargin(22));
    auto* S=T->ConstructWidget<USizeBox>();S->SetWidthOverride(Width);S->SetContent(Frame);
    auto* Placement=Root->AddChildToOverlay(S);Placement->SetHorizontalAlignment(HAlign_Center);Placement->SetVerticalAlignment(VAlign_Center);
    auto* Column=T->ConstructWidget<UVerticalBox>();Frame->SetContent(Column);return Column;
}
UClass* WClass(const TCHAR* Name,UClass* Fallback)
{auto* C=LoadClass<UUserWidget>(nullptr,*FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"),Name,Name));return C?C:Fallback;}
bool Up(FKey K){return K==EKeys::W||K==EKeys::Up||K==EKeys::Gamepad_DPad_Up;}
bool Down(FKey K){return K==EKeys::S||K==EKeys::Down||K==EKeys::Gamepad_DPad_Down;}
bool Left(FKey K){return K==EKeys::A||K==EKeys::Left||K==EKeys::Gamepad_DPad_Left;}
bool Right(FKey K){return K==EKeys::D||K==EKeys::Right||K==EKeys::Gamepad_DPad_Right;}
bool Accept(FKey K){return (K==EKeys::G||K==EKeys::F||K==EKeys::Enter)||K==EKeys::SpaceBar||UICGameUserSettings::IsGamepadAcceptKey(K);}
bool BackKey(FKey K){return K==EKeys::Escape||K==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(K);}
const TCHAR* Categories[]={TEXT("스킬"),TEXT("펫"),TEXT("생산물"),TEXT("제작물"),TEXT("NPC")};
const TCHAR* Products[]={TEXT("채집"),TEXT("농사"),TEXT("낚시"),TEXT("채광"),TEXT("목축")};
const TCHAR* Recipes[]={TEXT("무기"),TEXT("요리")};
}

bool UICCodexCellWidget::BuildLayout(UWidgetTree* T)
{
    auto* S=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("CellSize"));T->RootWidget=S;S->SetClipping(EWidgetClipping::ClipToBounds);
    auto* B=Button(T,TEXT("BTN_Cell"),FText::GetEmpty(),false);S->SetContent(B);
    auto* Column=T->ConstructWidget<UVerticalBox>();auto* Placement=CastChecked<UButtonSlot>(B->SetContent(Column));Placement->SetPadding(FMargin(4));Placement->SetHorizontalAlignment(HAlign_Fill);Placement->SetVerticalAlignment(VAlign_Fill);
    auto* IconBox=T->ConstructWidget<UOverlay>();Column->AddChildToVerticalBox(IconBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Icon=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Icon"));Fill(IconBox,Icon);Icon->SetVisibility(ESlateVisibility::Hidden);
    auto* Unknown=Label(T,TEXT("TXT_Unknown"),Txt(TEXT("?")),30);auto* Center=IconBox->AddChildToOverlay(Unknown);Center->SetHorizontalAlignment(HAlign_Center);Center->SetVerticalAlignment(VAlign_Center);
    auto* Name=Label(T,TEXT("TXT_Name"),FText::GetEmpty(),12);Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    auto* NameBounds=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NameBounds"));NameBounds->SetContent(Name);NameBounds->SetClipping(EWidgetClipping::ClipToBounds);Row(Column,NameBounds,1);
    auto* Progress=Label(T,TEXT("TXT_Progress"),FText::GetEmpty(),12);Progress->SetJustification(ETextJustify::Center);Progress->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    auto* ProgressBounds=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("ProgressBounds"));ProgressBounds->SetContent(Progress);ProgressBounds->SetClipping(EWidgetClipping::ClipToBounds);Row(Column,ProgressBounds,0);return true;
}
void UICCodexCellWidget::NativeOnInitialized()
{Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Cell")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Pick);}
void UICCodexCellWidget::Present(int32 Index,FText Name,FText Progress,UTexture2D* Icon,bool Unknown,bool Selected,float Width,float Height)
{
    CellIndex=Index;bUnknown=Unknown;auto* S=CastChecked<USizeBox>(GetWidgetFromName(TEXT("CellSize")));S->SetWidthOverride(Width);S->SetHeightOverride(Height);
    Text(this,TEXT("TXT_Name"),Unknown?FText::GetEmpty():CellCaption(Name,Width-20));Text(this,TEXT("TXT_Progress"),CellCaption(Progress,Width-20));
    auto* NameBounds=CastChecked<USizeBox>(GetWidgetFromName(TEXT("NameBounds")));NameBounds->SetWidthOverride(Width-20);NameBounds->SetVisibility(Unknown||Name.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    auto* ProgressBounds=CastChecked<USizeBox>(GetWidgetFromName(TEXT("ProgressBounds")));ProgressBounds->SetWidthOverride(Width-20);ProgressBounds->SetVisibility(Progress.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    GetWidgetFromName(TEXT("TXT_Unknown"))->SetVisibility(Unknown?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    auto* Image=CastChecked<UImage>(GetWidgetFromName(TEXT("IMG_Icon")));Image->SetBrushFromTexture(Unknown?nullptr:Icon);Image->SetVisibility(!Unknown&&Icon?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Cell")))->SetBackgroundColor(Selected?FLinearColor(.51f,.39f,.17f):CodexButton);
}
void UICCodexCellWidget::Pick(){OnPicked.Broadcast(CellIndex);}

bool UICCodexPopupWidget::BuildLayout(UWidgetTree* T)
{
    auto* Column=Modal(T,590);auto* Header=T->ConstructWidget<UHorizontalBox>();Row(Column,Header,14);
    auto* Image=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Icon"));auto* S=T->ConstructWidget<USizeBox>();S->SetWidthOverride(74);S->SetHeightOverride(74);S->SetContent(Image);Header->AddChildToHorizontalBox(S)->SetPadding(FMargin(0,0,16,0));
    auto* Name=Label(T,TEXT("TXT_Title"),FText::GetEmpty(),25);Name->SetAutoWrapText(true);auto* TitlePlacement=Header->AddChildToHorizontalBox(Name);TitlePlacement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TitlePlacement->SetVerticalAlignment(VAlign_Center);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("DescriptionScroll"));auto* Limit=T->ConstructWidget<USizeBox>();Limit->SetMaxDesiredHeight(205);Limit->SetContent(Scroll);Row(Column,Limit,16);
    auto* Description=Label(T,TEXT("TXT_Description"),FText::GetEmpty());Description->SetAutoWrapText(true);Scroll->AddChild(Description);
    auto* Actions=T->ConstructWidget<UHorizontalBox>();Row(Column,Actions,0);
    for(const TCHAR* NameKey:{TEXT("BTN_Primary"),TEXT("BTN_Back")}){auto* Placement=Actions->AddChildToHorizontalBox(Button(T,NameKey,FText::GetEmpty()));Placement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Placement->SetPadding(FMargin(4,0));}return true;
}
void UICCodexPopupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Primary")))->OnClicked.AddUniqueDynamic(this,&ThisClass::PrimaryAction);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Back")))->OnClicked.AddUniqueDynamic(this,&ThisClass::BackAction);
}
void UICCodexPopupWidget::Present(FText Title,FText Description,UTexture2D* Icon,FText Primary,FText Back,bool HasPrimary,bool HasBack,int32 DefaultChoice)
{
    Text(this,TEXT("TXT_Title"),Title);Text(this,TEXT("TXT_Description"),Description);Text(this,TEXT("BTN_Primary_Text"),Primary);Text(this,TEXT("BTN_Back_Text"),Back);
    auto* Image=CastChecked<UImage>(GetWidgetFromName(TEXT("IMG_Icon")));Image->SetBrushFromTexture(Icon);Image->GetParent()->SetVisibility(Icon?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("BTN_Primary"))->SetVisibility(HasPrimary?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("BTN_Back"))->SetVisibility(HasBack?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Choice=HasPrimary?(HasBack?DefaultChoice:0):1;RefreshChoice();CastChecked<UScrollBox>(GetWidgetFromName(TEXT("DescriptionScroll")))->SetScrollOffset(0);
}
void UICCodexPopupWidget::RefreshChoice()
{for(int32 I=0;I<2;++I)CastChecked<UButton>(GetWidgetFromName(I==0?TEXT("BTN_Primary"):TEXT("BTN_Back")))->SetBackgroundColor(I==Choice?FLinearColor(.51f,.39f,.17f):CodexButton);}
bool UICCodexPopupWidget::HandleKey(FKey K,bool Repeat)
{
    if(BackKey(K)){if(!Repeat){if(GetWidgetFromName(TEXT("BTN_Back"))->IsVisible())BackAction();else PrimaryAction();}return true;}
    if(Left(K)||Right(K)){if(GetWidgetFromName(TEXT("BTN_Primary"))->IsVisible()&&GetWidgetFromName(TEXT("BTN_Back"))->IsVisible())Choice=1-Choice;RefreshChoice();return true;}
    if(Up(K)||Down(K)){auto* Scroll=CastChecked<UScrollBox>(GetWidgetFromName(TEXT("DescriptionScroll")));Scroll->SetScrollOffset(FMath::Clamp(Scroll->GetScrollOffset()+(Down(K)?60:-60),0.f,Scroll->GetScrollOffsetOfEnd()));return true;}
    if(Accept(K)){if(!Repeat){if(Choice==0)PrimaryAction();else BackAction();}return true;}return false;
}
void UICCodexPopupWidget::PrimaryAction(){OnPrimary.Broadcast();}
void UICCodexPopupWidget::BackAction(){OnBack.Broadcast();}

bool UICCodexTargetsWidget::BuildLayout(UWidgetTree* T)
{
    auto* Column=Modal(T,540);Row(Column,Label(T,TEXT("TargetsTitle"),Txt(TEXT("펫을 설정할 캐릭터")),24),16);
    Row(Column,T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("MemberCards")),16);
    Row(Column,Button(T,TEXT("BTN_Back"),Txt(TEXT("뒤로가기"))),0);return true;
}
void UICCodexTargetsWidget::NativeOnInitialized()
{Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Back")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Back);}
void UICCodexTargetsWidget::Present(UICCodexSubsystem* C)
{
    Members=C?C->GetPartyMembers():TArray<FName>();SelectedIndex=FMath::Clamp(SelectedIndex,0,Members.Num());
    auto* List=CastChecked<UHorizontalBox>(GetWidgetFromName(TEXT("MemberCards")));
    if(List->GetChildrenCount()!=Members.Num()){List->ClearChildren();for(int32 I=0;I<Members.Num();++I){auto* W=WidgetTree->ConstructWidget<UICCodexCellWidget>(WClass(TEXT("WBP_IC_CodexCell"),UICCodexCellWidget::StaticClass()));W->OnPicked.AddUniqueDynamic(this,&ThisClass::Pick);List->AddChildToHorizontalBox(W)->SetPadding(FMargin(4));}}
    for(int32 I=0;I<Members.Num();++I)CastChecked<UICCodexCellWidget>(List->GetChildAt(I))->Present(I,C->CharacterName(Members[I]),FText::GetEmpty(),C->CharacterPortrait(Members[I]),false,I==SelectedIndex,105,130);
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Back")))->SetBackgroundColor(SelectedIndex==Members.Num()?FLinearColor(.51f,.39f,.17f):CodexButton);
}
bool UICCodexTargetsWidget::HandleKey(FKey K,bool Repeat)
{
    if(BackKey(K)){if(!Repeat)Back();return true;}
    if(Up(K)||Down(K)||Left(K)||Right(K)){SelectedIndex=FMath::Clamp(SelectedIndex+((Down(K)||Right(K))?1:-1),0,Members.Num());Present(GetGameInstance()->GetSubsystem<UICCodexSubsystem>());return true;}
    if(Accept(K)){if(!Repeat){if(Members.IsValidIndex(SelectedIndex))Pick(SelectedIndex);else Back();}return true;}return false;
}
void UICCodexTargetsWidget::Pick(int32 Index){if(Members.IsValidIndex(Index)){SelectedIndex=Index;OnPicked.Broadcast(Index);}}
void UICCodexTargetsWidget::Back(){OnBack.Broadcast();}

bool UICCodexMenuWidget::BuildLayout(UWidgetTree* T)
{
    auto* Root=T->ConstructWidget<UOverlay>();T->RootWidget=Root;
    auto* Body=T->ConstructWidget<UHorizontalBox>();Fill(Root,Body);
    auto* Column=T->ConstructWidget<UVerticalBox>();auto* ContentPlacement=Body->AddChildToHorizontalBox(Column);ContentPlacement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ContentPlacement->SetPadding(FMargin(0,0,18,0));
    Row(Column,Label(T,TEXT("TXT_Heading"),Txt(TEXT("도감")),26),10);
    Row(Column,Label(T,TEXT("TXT_Home"),Txt(TEXT("오른쪽에서 분류를 선택하세요."))),0);
    auto* Equipment=T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("EquippedSkills"));Row(Column,Equipment,8);
    for(int32 I=0;I<4;++I){auto* W=T->ConstructWidget<UICCodexCellWidget>(WClass(TEXT("WBP_IC_CodexCell"),UICCodexCellWidget::StaticClass()),FName(*FString::Printf(TEXT("Equipped%d"),I)));Equipment->AddChildToHorizontalBox(W)->SetPadding(FMargin(2));}
    auto* Tabs=T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("SubTabs"));Row(Column,Tabs,10);
    for(int32 I=0;I<5;++I){auto* Placement=Tabs->AddChildToHorizontalBox(Button(T,FName(*FString::Printf(TEXT("BTN_Tab%d"),I)),FText::GetEmpty()));Placement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Placement->SetPadding(FMargin(2,0));}
    auto* Families=T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("SkillFamilies"));Row(Column,Families,4);
    for(const TCHAR* Name:{TEXT("검"),TEXT("활"),TEXT("지팡이"),TEXT("완드")}){auto* L=Label(T,NAME_None,Txt(Name),14);L->SetJustification(ETextJustify::Center);Families->AddChildToHorizontalBox(L)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));}
    Row(Column,Label(T,TEXT("TXT_Empty"),Txt(TEXT("등록된 항목이 없습니다."))),0);
    auto* Scroll=T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("GridScroll"));Column->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Grid=T->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("CellsGrid"));Grid->SetSlotPadding(FMargin(3));Scroll->AddChild(Grid);
    auto* CategoriesColumn=T->ConstructWidget<UVerticalBox>();auto* RightSize=T->ConstructWidget<USizeBox>();RightSize->SetWidthOverride(116);RightSize->SetContent(CategoriesColumn);Body->AddChild(RightSize);
    for(int32 I=0;I<UE_ARRAY_COUNT(Categories);++I){auto* S=T->ConstructWidget<USizeBox>();S->SetHeightOverride(76);S->SetContent(Button(T,FName(*FString::Printf(TEXT("BTN_Category%d"),I)),Txt(Categories[I])));Row(CategoriesColumn,S,8);}
    auto* Detail=T->ConstructWidget<UICCodexPopupWidget>(WClass(TEXT("WBP_IC_CodexPopup"),UICCodexPopupWidget::StaticClass()),TEXT("DetailPopup"));Fill(Root,Detail);Detail->SetVisibility(ESlateVisibility::Collapsed);
    auto* Targets=T->ConstructWidget<UICCodexTargetsWidget>(WClass(TEXT("WBP_IC_CodexTargets"),UICCodexTargetsWidget::StaticClass()),TEXT("TargetPopup"));Fill(Root,Targets);Targets->SetVisibility(ESlateVisibility::Collapsed);
    for(const TCHAR* Name:{TEXT("ConfirmPopup"),TEXT("NoticePopup")}){auto* W=T->ConstructWidget<UICCodexPopupWidget>(WClass(TEXT("WBP_IC_CodexPopup"),UICCodexPopupWidget::StaticClass()),Name);Fill(Root,W);W->SetVisibility(ESlateVisibility::Collapsed);}return true;
}
UICCodexSubsystem* UICCodexMenuWidget::Codex() const{return GetGameInstance()?GetGameInstance()->GetSubsystem<UICCodexSubsystem>():nullptr;}
void UICCodexMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildLayout(WidgetTree);
    const FName Actions[]={TEXT("Category0"),TEXT("Category1"),TEXT("Category2"),TEXT("Category3")};
    for(int32 I=0;I<4;++I){FScriptDelegate D;D.BindUFunction(this,Actions[I]);CastChecked<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Category%d"),I))))->OnClicked.AddUnique(D);CastChecked<UICCodexCellWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("Equipped%d"),I))))->OnPicked.AddUniqueDynamic(this,&ThisClass::CellPicked);}
    CastChecked<UButton>(GetWidgetFromName(TEXT("BTN_Category4")))->OnClicked.AddUniqueDynamic(this,&ThisClass::Category4);
    const FName TabActions[]={TEXT("Tab0"),TEXT("Tab1"),TEXT("Tab2"),TEXT("Tab3"),TEXT("Tab4")};
    for(int32 I=0;I<5;++I){FScriptDelegate D;D.BindUFunction(this,TabActions[I]);CastChecked<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Tab%d"),I))))->OnClicked.AddUnique(D);}
    auto* Detail=CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("DetailPopup")));Detail->OnPrimary.AddUniqueDynamic(this,&ThisClass::DetailAction);Detail->OnBack.AddUniqueDynamic(this,&ThisClass::ClosePopup);
    auto* Targets=CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("TargetPopup")));Targets->OnPicked.AddUniqueDynamic(this,&ThisClass::MemberPicked);Targets->OnBack.AddUniqueDynamic(this,&ThisClass::ClosePopup);
    auto* Confirm=CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("ConfirmPopup")));Confirm->OnPrimary.AddUniqueDynamic(this,&ThisClass::ConfirmPet);Confirm->OnBack.AddUniqueDynamic(this,&ThisClass::CancelPet);
    CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("NoticePopup")))->OnPrimary.AddUniqueDynamic(this,&ThisClass::DismissNotice);RefreshCodex();
}
void UICCodexMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState){BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RefreshCodex);BoundState->OnInventoryChanged.AddUniqueDynamic(this,&ThisClass::RefreshCodex);BoundState->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);}RefreshCodex();
}
void UICCodexMenuWidget::NativeDestruct()
{
    if(BoundState){BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::RefreshCodex);BoundState->OnInventoryChanged.RemoveDynamic(this,&ThisClass::RefreshCodex);BoundState->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);BoundState=nullptr;}Super::NativeDestruct();
}
void UICCodexMenuWidget::CharacterChanged(FName){RefreshCodex();if(Category==4&&Popup==1)RefreshDetail();}
void UICCodexMenuWidget::Activate(){Popup=0;bNoticeOpen=false;NavigationArea=0;RefreshCodex();}
void UICCodexMenuWidget::SelectCategory(int32 Index)
{if(IsModalOpen()||Index<0||Index>=UE_ARRAY_COUNT(Categories))return;Category=CategoryFocus=Index;SubTab=0;SelectedCell=0;NavigationArea=Category==0||Category==2||Category==3?1:2;RefreshCodex();CastChecked<UScrollBox>(GetWidgetFromName(TEXT("GridScroll")))->SetScrollOffset(0);NotifyNavigation();}
void UICCodexMenuWidget::SelectSubTab(int32 Index)
{if(IsModalOpen()||(Category!=2&&Category!=3)||Index<0||Index>=(Category==2?5:2))return;SubTab=Index;SelectedCell=0;NavigationArea=1;RefreshCodex();CastChecked<UScrollBox>(GetWidgetFromName(TEXT("GridScroll")))->SetScrollOffset(0);NotifyNavigation();}
int32 UICCodexMenuWidget::Columns() const{return Category==0?6:Category==1||Category==4?5:7;}
void UICCodexMenuWidget::RefreshCodex()
{
    if(bRefreshing)return;TGuardValue<bool> Guard(bRefreshing,true);auto* C=Codex();if(!C)return;
    for(int32 I=0;I<UE_ARRAY_COUNT(Categories);++I)CastChecked<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Category%d"),I))))->SetBackgroundColor((NavigationArea==0?CategoryFocus:Category)==I?FLinearColor(.51f,.39f,.17f):CodexButton);
    Text(this,TEXT("TXT_Heading"),Txt(Category>=0?Categories[Category]:TEXT("도감")));
    GetWidgetFromName(TEXT("TXT_Home"))->SetVisibility(Category<0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("EquippedSkills"))->SetVisibility(Category==0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("SkillFamilies"))->SetVisibility(ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("SubTabs"))->SetVisibility(Category==2||Category==3?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(Category==0)
    {
        const auto Equipped=C->EquippedSkills();auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
        for(int32 I=0;I<4;++I){const auto* S=D->Find<FICSkillRow>(Equipped[I],R);CastChecked<UICCodexCellWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("Equipped%d"),I))))->Present(1000+I,FText::Format(Txt(TEXT("스킬 {0}")),FText::AsNumber(I+1)),S?C->DefinitionText(S->NameKey,S->ID):Txt(TEXT("미설정")),S?S->Presentation.Icon.LoadSynchronous():nullptr,false,NavigationArea==1&&EquippedIndex==I,165,64);}
    }
    for(int32 I=0;I<5;++I){auto* B=CastChecked<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Tab%d"),I))));B->SetVisibility(Category==2||I<2?ESlateVisibility::Visible:ESlateVisibility::Collapsed);Text(this,*FString::Printf(TEXT("BTN_Tab%d_Text"),I),Txt(Category==2?Products[I]:Recipes[FMath::Min(I,1)]));B->SetBackgroundColor(I==SubTab?FLinearColor(.51f,.39f,.17f):CodexButton);}
    Cells=Category==0?C->SkillCells():Category==1?C->GetPetIDs():Category==2?C->ProductItems(SubTab):Category==3?C->Recipes(SubTab):Category==4?C->GetNPCIDs():TArray<FName>();
    SelectedCell=FMath::Clamp(SelectedCell,0,FMath::Max(0,Cells.Num()-1));RefreshGrid();
    GetWidgetFromName(TEXT("TXT_Empty"))->SetVisibility(Category>=0&&Cells.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("DetailPopup"))->SetVisibility(Popup>0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("TargetPopup"))->SetVisibility(Popup>=2?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("ConfirmPopup"))->SetVisibility(Popup==3?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    GetWidgetFromName(TEXT("NoticePopup"))->SetVisibility(bNoticeOpen?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(Popup==2)CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("TargetPopup")))->Present(C);
}
void UICCodexMenuWidget::RefreshGrid()
{
    auto* C=Codex();auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();auto* Grid=CastChecked<UUniformGridPanel>(GetWidgetFromName(TEXT("CellsGrid")));
    // Category changes can keep the same item count but require different column positions.
    if(Grid->GetChildrenCount()!=Cells.Num()||GridColumns!=Columns())
    {Grid->ClearChildren();GridColumns=Columns();for(int32 I=0;I<Cells.Num();++I){auto* W=WidgetTree->ConstructWidget<UICCodexCellWidget>(WClass(TEXT("WBP_IC_CodexCell"),UICCodexCellWidget::StaticClass()));W->OnPicked.AddUniqueDynamic(this,&ThisClass::CellPicked);Grid->AddChildToUniformGrid(W,I/Columns(),I%Columns());}}
    FICDefinitionResult R;const auto World=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
    const auto Pets=Category==1?C->PetDefinitions():TArray<const FICMonsterRow*>();
    for(int32 I=0;I<Cells.Num();++I)
    {
        FText Name,Progress;UTexture2D* Icon=nullptr;bool Unknown=false;
        if(Category==0){const auto* Player=World.Characters.Find(UICWorldStateSubsystem::PlayerId());Unknown=Cells[I].IsNone()||!Player||!Player->UnlockedSkillIDs.Contains(Cells[I]);if(!Unknown)if(const auto* S=D->Find<FICSkillRow>(Cells[I],R)){Name=C->DefinitionText(S->NameKey,S->ID);Icon=S->Presentation.Icon.LoadSynchronous();}}
        else if(Category==1){const auto* M=Pets[I];const int32 Count=C->GetPetEssence(M->PetCodexIndex);Unknown=Count<100;Progress=FText::Format(Txt(TEXT("{0}/100")),FText::AsNumber(Count));const auto Used=C->PetUsageText(M->PetCodexIndex);if(!Used.IsEmpty())Progress=Used;Name=C->PetName(M->PetCodexIndex);Icon=M->Portrait.LoadSynchronous();}
        else if(Category==2){Unknown=!World.Codex.AcquiredItemIDs.Contains(Cells[I]);if(!Unknown){Name=C->ItemName(Cells[I]);if(const auto* Item=D->Find<FICItemRow>(Cells[I],R))Icon=Item->Icon.LoadSynchronous();}}
        else if(Category==4){Name=C->CharacterName(Cells[I]);Icon=C->CharacterPortrait(Cells[I]);Progress=FText::Format(NSLOCTEXT("ICCodex","NPCGridAffinity","호감도 {0}"),FText::AsNumber(C->NPCAffinity(Cells[I])));}
        else{if(const auto* Recipe=D->Find<FICRecipeRow>(Cells[I],R)){Unknown=!Recipe->bUnlockedByDefault&&!World.UnlockedRecipeIDs.Contains(Cells[I]);if(!Unknown){Name=C->DefinitionText(Recipe->NameKey,Recipe->ID);auto Results=Recipe->Results;if(Results.IsEmpty())if(const auto* P=D->Find<FICProductionRow>(Recipe->ProductionID,R))Results=P->Results;if(!Results.IsEmpty())if(const auto* Item=D->Find<FICItemRow>(Results[0].ItemID,R))Icon=Item->Icon.LoadSynchronous();}}}
        CastChecked<UICCodexCellWidget>(Grid->GetChildAt(I))->Present(I,Name,Progress,Icon,Unknown,NavigationArea==2&&I==SelectedCell,Category==0?105:Category==1||Category==4?125:91,Category==0?78:Category==1||Category==4?114:98);
    }
}
bool UICCodexMenuWidget::OpenCell(int32 Index)
{
    if(IsModalOpen()||!Codex())return false;auto* C=Codex();DetailID=NAME_None;DetailPet=INDEX_NONE;
    if(Index>=1000&&Index<1004&&Category==0){const auto Equipped=C->EquippedSkills();DetailID=Equipped[Index-1000];if(DetailID.IsNone())return false;EquippedIndex=Index-1000;}
    else{if(!Cells.IsValidIndex(Index))return false;SelectedCell=Index;NavigationArea=2;
        if(Category==1){FICDefinitionResult R;const auto* Monster=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>()->Find<FICMonsterRow>(Cells[Index],R);if(!Monster||!C->IsPetUnlocked(Monster->PetCodexIndex))return false;DetailPet=Monster->PetCodexIndex;}
        else{DetailID=Cells[Index];if(DetailID.IsNone()||(Category==0&&!C->IsSkillUnlocked(DetailID))||(Category==2&&!C->HasAcquiredItem(DetailID))||(Category==3&&!C->IsRecipeKnown(DetailID)))return false;}}
    Popup=1;RefreshDetail();RefreshCodex();NotifyNavigation();return true;
}
void UICCodexMenuWidget::RefreshDetail()
{
    auto* C=Codex();auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;FText Name,Description,Action;UTexture2D* Icon=nullptr;
    if(Category==0){if(const auto* S=D->Find<FICSkillRow>(DetailID,R)){Name=C->DefinitionText(S->NameKey,S->ID);Description=C->SkillDescription(DetailID);Icon=S->Presentation.Icon.LoadSynchronous();Action=Txt(C->IsPlayerSkillEquipped(DetailID)?TEXT("스킬 해제하기"):TEXT("스킬 설정하기"));}}
    else if(Category==1){Name=C->PetName(DetailPet);Description=FText::Format(NSLOCTEXT("ICCodex","PetDetailProgress","{0}\n\n정수 {1}/100\n{2}"),C->PetBuffText(DetailPet),FText::AsNumber(C->GetPetEssence(DetailPet)),C->PetUsageText(DetailPet));Action=Txt(TEXT("펫 설정"));if(const auto* M=C->FindPet(DetailPet))Icon=M->Portrait.LoadSynchronous();}
    else if(Category==2){Name=C->ItemName(DetailID);if(const auto* Item=D->Find<FICItemRow>(DetailID,R)){Description=C->DefinitionText(Item->DescriptionKey);Icon=Item->Icon.LoadSynchronous();}}
    else if(Category==4){Name=C->CharacterName(DetailID);Description=C->NPCDescription(DetailID);Icon=C->CharacterPortrait(DetailID);}
    else{if(const auto* Recipe=D->Find<FICRecipeRow>(DetailID,R))Name=C->DefinitionText(Recipe->NameKey,Recipe->ID);Description=C->RecipeDescription(DetailID);}
    CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("DetailPopup")))->Present(Name,Description,Icon,Action,Txt(TEXT("뒤로가기")),Category<2,true);
}
void UICCodexMenuWidget::ClosePopup()
{if(bNoticeOpen){DismissNotice();return;}if(Popup==3){CancelPet();return;}Popup=Popup==2?1:0;RefreshCodex();NotifyNavigation();}
void UICCodexMenuWidget::ShowNotice(FText Value)
{bNoticeOpen=true;CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("NoticePopup")))->Present(Txt(TEXT("알림")),Value,nullptr,Txt(TEXT("확인")),FText::GetEmpty(),true,false);RefreshCodex();NotifyNavigation();}
void UICCodexMenuWidget::DismissNotice(){bNoticeOpen=false;RefreshCodex();NotifyNavigation();}
void UICCodexMenuWidget::DetailAction()
{
    if(Popup!=1||bNoticeOpen)return;
    if(Category==0){const bool Removing=Codex()->IsPlayerSkillEquipped(DetailID);FText Error;const bool Done=Removing?Codex()->UnsetPlayerSkill(DetailID):Codex()->SetPlayerSkill(DetailID,Error);if(!Done)ShowNotice(Error.IsEmpty()?Txt(TEXT("스킬을 설정할 수 없습니다.")):Error);else{RefreshDetail();RefreshCodex();ShowNotice(Txt(Removing?TEXT("스킬을 해제했습니다."):TEXT("스킬을 설정했습니다.")));}}
    else if(Category==1){Popup=2;CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("TargetPopup")))->Present(Codex());RefreshCodex();NotifyNavigation();}
}
void UICCodexMenuWidget::MemberPicked(int32 Index)
{
    if(Popup!=2||bNoticeOpen)return;auto* Targets=CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("TargetPopup")));auto* C=Codex();
    if(!Targets->Members.IsValidIndex(Index)||!C->GetPartyMembers().Contains(Targets->Members[Index]))return;
    PendingCharacter=Targets->Members[Index];PendingPet=DetailPet;PendingPreviousPet=C->GetEquippedPet(PendingCharacter);
    const FText Name=PendingCharacter==UICWorldStateSubsystem::PlayerId()?Txt(TEXT("플레이어")):C->CharacterName(PendingCharacter);
    const auto Question=PendingPreviousPet==INDEX_NONE?FText::Format(Txt(TEXT("{0} 펫으로 설정하겠습니까?")),Name):FText::Format(Txt(TEXT("{0}에게는 {1} 펫이 설정되어있습니다. 해제하고 {2} 펫으로 설정하겠습니까?")),Name,C->PetName(PendingPreviousPet),C->PetName(PendingPet));
    Popup=3;CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("ConfirmPopup")))->Present(Txt(TEXT("펫 설정")),Question,nullptr,Txt(TEXT("확인")),Txt(TEXT("취소")),true,true,1);RefreshCodex();NotifyNavigation();
}
void UICCodexMenuWidget::ResolvePet(bool AcceptChange)
{
    if(Popup!=3||bNoticeOpen)return;
    if(!AcceptChange){Popup=2;RefreshCodex();NotifyNavigation();return;}
    auto* C=Codex();const bool Valid=C->GetPartyMembers().Contains(PendingCharacter)&&C->IsPetUnlocked(PendingPet)&&C->GetEquippedPet(PendingCharacter)==PendingPreviousPet;
    if(Valid&&C->EquipPet(PendingPet,PendingCharacter)){Popup=1;RefreshDetail();RefreshCodex();NotifyNavigation();}
    else{Popup=2;ShowNotice(Txt(TEXT("파티 또는 펫 설정이 변경되었습니다. 다시 선택해주세요.")));}
}
void UICCodexMenuWidget::ConfirmPet(){ResolvePet(true);}
void UICCodexMenuWidget::CancelPet(){ResolvePet(false);}
void UICCodexMenuWidget::CellPicked(int32 Index){OpenCell(Index);RefreshCodex();NotifyNavigation();}
void UICCodexMenuWidget::NotifyNavigation(){OnNavigationChanged.Broadcast();}
void UICCodexMenuWidget::Category0(){SelectCategory(0);}void UICCodexMenuWidget::Category1(){SelectCategory(1);}void UICCodexMenuWidget::Category2(){SelectCategory(2);}void UICCodexMenuWidget::Category3(){SelectCategory(3);}
void UICCodexMenuWidget::Category4(){SelectCategory(4);}
void UICCodexMenuWidget::Tab0(){SelectSubTab(0);}void UICCodexMenuWidget::Tab1(){SelectSubTab(1);}void UICCodexMenuWidget::Tab2(){SelectSubTab(2);}void UICCodexMenuWidget::Tab3(){SelectSubTab(3);}void UICCodexMenuWidget::Tab4(){SelectSubTab(4);}
FText UICCodexMenuWidget::NavigationHint() const
{
    if(IsModalOpen())return Txt(Popup==2&&!bNoticeOpen?TEXT("W A S D  캐릭터 선택     G/F  선택     ESC  뒤로가기"):TEXT("A / D  버튼 선택     W / S  설명 스크롤     G/F  선택     ESC  뒤로가기"));
    return Txt(NavigationArea==0?TEXT("W / S  분류 선택     A / Enter  열기     D  상단 탭     ESC  닫기"):TEXT("W A S D  이동     G / F  상세 보기     A / W  상단 탭     ESC  닫기"));
}
bool UICCodexMenuWidget::HandleMenuKey(FKey K,bool Repeat)
{
    if(bNoticeOpen)return CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("NoticePopup")))->HandleKey(K,Repeat);
    if(Popup==3)return CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("ConfirmPopup")))->HandleKey(K,Repeat);
    if(Popup==2)return CastChecked<UICCodexTargetsWidget>(GetWidgetFromName(TEXT("TargetPopup")))->HandleKey(K,Repeat);
    if(Popup==1)return CastChecked<UICCodexPopupWidget>(GetWidgetFromName(TEXT("DetailPopup")))->HandleKey(K,Repeat);
    if(!Up(K)&&!Down(K)&&!Left(K)&&!Right(K)&&!Accept(K))return false;
    if(NavigationArea==0)
    {
        if(Right(K)||(Up(K)&&CategoryFocus==0))return false;
        if(Up(K)||Down(K))CategoryFocus=FMath::Clamp(CategoryFocus+(Down(K)?1:-1),0,UE_ARRAY_COUNT(Categories)-1);
        else if((Left(K)||Accept(K))&&!Repeat)SelectCategory(CategoryFocus);
    }
    else if(NavigationArea==1)
    {
        if(Up(K))return false;
        if(Category==0){if(Left(K)||Right(K))EquippedIndex=FMath::Clamp(EquippedIndex+(Right(K)?1:-1),0,3);else if(Accept(K)&&!Repeat)OpenCell(1000+EquippedIndex);}
        else if((Category==2||Category==3)&&(Left(K)||Right(K)))SelectSubTab((SubTab+(Right(K)?1:(Category==2?4:1)))%(Category==2?5:2));
        if(Down(K))NavigationArea=Cells.IsEmpty()?0:2;
    }
    else
    {
        const int32 Col=SelectedCell%Columns();
        if(Left(K)&&Col==0)return false;
        if(Right(K)&&Col==Columns()-1){NavigationArea=0;CategoryFocus=FMath::Max(0,Category);}
        else if(Up(K)&&SelectedCell<Columns()){NavigationArea=Category==1||Category==4?0:1;CategoryFocus=FMath::Max(0,Category);}
        else if(Up(K)||Down(K)||Left(K)||Right(K)){SelectedCell=FMath::Clamp(SelectedCell+(Down(K)?Columns():Up(K)?-Columns():Right(K)?1:-1),0,FMath::Max(0,Cells.Num()-1));}
        else if(Accept(K)&&!Repeat)OpenCell(SelectedCell);
    }
    RefreshCodex();if(NavigationArea==2){auto* Grid=CastChecked<UUniformGridPanel>(GetWidgetFromName(TEXT("CellsGrid")));if(SelectedCell<Grid->GetChildrenCount())CastChecked<UScrollBox>(GetWidgetFromName(TEXT("GridScroll")))->ScrollWidgetIntoView(Grid->GetChildAt(SelectedCell),true,EDescendantScrollDestination::IntoView);}NotifyNavigation();return true;
}
