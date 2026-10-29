#include "UI/Gameplay/ICPartyMenuWidget.h"
#include "Codex/ICCodexSubsystem.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Data/Rows/Character/ICCharacterProfileRow.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/Party/ICPartyStatus.h"
#include "Time/ICTimeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Internationalization/StringTable.h"

namespace
{
UTextBlock* Text(UWidgetTree* Tree,FName Name,FText Value,int32 FontSize)
{
    auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
    T->SetFont(ICFrontendLayout::Font(FontSize,FontSize>=24));T->SetText(Value);
    T->SetColorAndOpacity(FLinearColor(.91f,.93f,.97f));T->SetVisibility(ESlateVisibility::HitTestInvisible);return T;
}
USizeBox* Sized(UWidgetTree* Tree,UWidget* Child,float W,float H)
{
    auto* Box=Tree->ConstructWidget<USizeBox>();if(W>0)Box->SetWidthOverride(W);if(H>0)Box->SetHeightOverride(H);Box->SetContent(Child);return Box;
}
void Add(UVerticalBox* Box,UWidget* Child,float Gap=8)
{
    auto* S=Box->AddChildToVerticalBox(Child);S->SetPadding(FMargin(0,0,0,Gap));S->SetHorizontalAlignment(HAlign_Fill);
}
void SetText(UUserWidget* Widget,const TCHAR* Name,FText Value)
{if(auto* T=Cast<UTextBlock>(Widget->GetWidgetFromName(Name)))T->SetText(Value);}
const TCHAR* StatKeys[]={TEXT("HP"),TEXT("MP"),TEXT("SP"),TEXT("STR"),TEXT("DEX"),TEXT("INT"),TEXT("CON"),TEXT("LUK"),TEXT("Physical"),TEXT("Magic"),TEXT("Defense"),TEXT("Speed")};
}

bool UICPartyPortraitWidget::BuildLayout(UWidgetTree* Tree)
{
    auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BTN_Select"));Tree->RootWidget=Button;
    auto Style=Button->GetStyle();
    for(auto* Brush:{&Style.Normal,&Style.Hovered,&Style.Pressed})
    {Brush->DrawAs=ESlateBrushDrawType::Box;Brush->Margin=FMargin(0);Brush->TintColor=FLinearColor::White;}
    Button->SetStyle(Style);
    auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Portrait"));
    auto* Slot=Cast<UButtonSlot>(Button->SetContent(Image));Slot->SetPadding(FMargin(4));Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);
    return true;
}
void UICPartyPortraitWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Select"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::Pick);
}
void UICPartyPortraitWidget::Present(int32 Index,FName ID,UTexture2D* Portrait,bool Selected)
{
    MemberIndex=Index;CharacterID=ID;bSelected=Selected;
    if(auto* Image=Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait"))))
    {Image->SetBrushFromTexture(Portrait);Image->SetVisibility(Portrait?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);}
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Select"))))
        Button->SetBackgroundColor(Selected?FLinearColor(1,.74f,.3f):FLinearColor(.16f,.21f,.3f));
    if(auto* Box=Cast<USizeBox>(GetParent()))
    {Box->SetWidthOverride(Selected?104:80);Box->SetHeightOverride(Selected?104:80);}
}
void UICPartyPortraitWidget::Pick(){OnPicked.Broadcast(MemberIndex);}

bool UICPartyMenuWidget::BuildLayout(UWidgetTree* Tree)
{
    auto* Root=Tree->ConstructWidget<UHorizontalBox>();Tree->RootWidget=Root;
    auto* Roster=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("PortraitList"));
    auto* Left=Root->AddChildToHorizontalBox(Sized(Tree,Roster,144,0));Left->SetPadding(FMargin(0,0,32,0));
    auto* CardClass=LoadClass<UICPartyPortraitWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_PartyPortrait.WBP_IC_PartyPortrait_C"));
    for(int32 Index=0;Index<4;++Index)
    {
        auto* Card=Tree->ConstructWidget<UICPartyPortraitWidget>(CardClass?CardClass:UICPartyPortraitWidget::StaticClass(),FName(*FString::Printf(TEXT("Member%d"),Index)));
        auto* Box=Sized(Tree,Card,Index?80:104,Index?80:104);Box->SetVisibility(ESlateVisibility::Collapsed);
        auto* Placement=Roster->AddChildToVerticalBox(Box);Placement->SetHorizontalAlignment(HAlign_Center);Placement->SetPadding(FMargin(0,0,0,10));
    }
    auto* Details=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("CharacterDetails"));
    auto* Right=Root->AddChildToHorizontalBox(Details);Right->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Add(Details,Text(Tree,TEXT("TXT_Name"),FText::GetEmpty(),30),4);
    auto* Role=Text(Tree,TEXT("TXT_MainSetting"),FText::GetEmpty(),18);Role->SetAutoWrapText(true);Role->SetColorAndOpacity(FLinearColor(1,.76f,.34f));Add(Details,Role,12);
    auto* Story=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("StoryScroll"));
    Story->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    auto* StoryText=Text(Tree,TEXT("TXT_Story"),FText::GetEmpty(),17);StoryText->SetAutoWrapText(true);Story->AddChild(StoryText);
    auto* Reading=Tree->ConstructWidget<UHorizontalBox>();auto* ReadingSlot=Details->AddChildToVerticalBox(Reading);ReadingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ReadingSlot->SetPadding(FMargin(0,0,0,14));
    auto* StorySlot=Reading->AddChildToHorizontalBox(Story);StorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));StorySlot->SetPadding(FMargin(0,0,20,0));
    auto* StatusColumn=Tree->ConstructWidget<UVerticalBox>();Reading->AddChildToHorizontalBox(StatusColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Add(StatusColumn,Text(Tree,TEXT("StatusHeading"),NSLOCTEXT("ICParty","CurrentStatuses","현재 상태"),18),6);
    auto* StatusList=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("CurrentStatusScroll"));
    auto* StatusText=Text(Tree,TEXT("TXT_CurrentStatuses"),FText::GetEmpty(),16);StatusText->SetAutoWrapText(true);StatusList->AddChild(StatusText);StatusColumn->AddChildToVerticalBox(StatusList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Bottom=Tree->ConstructWidget<UHorizontalBox>();Add(Details,Bottom,0);
    auto* PetColumn=Tree->ConstructWidget<UVerticalBox>();
    Add(PetColumn,Text(Tree,TEXT("PetHeading"),NSLOCTEXT("ICParty","Pet","펫"),16),8);
    auto* PetFrame=Tree->ConstructWidget<UBorder>();PetFrame->SetBrushColor(FLinearColor(.012f,.018f,.03f));PetFrame->SetPadding(FMargin(5));
    auto* Pet=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("IMG_Pet"));Pet->SetVisibility(ESlateVisibility::Hidden);PetFrame->SetContent(Pet);
    Add(PetColumn,Sized(Tree,PetFrame,112,112),0);
    Bottom->AddChildToHorizontalBox(PetColumn)->SetPadding(FMargin(0,0,24,0));
    auto* Stats=Tree->ConstructWidget<UVerticalBox>();Bottom->AddChildToHorizontalBox(Stats)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Add(Stats,Text(Tree,TEXT("StatsHeading"),NSLOCTEXT("ICParty","Stats","스탯"),18),8);
    auto* Groups=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("StatsGrid"));Add(Stats,Groups,0);
    UVerticalBox* Panels[3];const TCHAR* GroupNames[]={TEXT("ResourcesGroup"),TEXT("AttributesGroup"),TEXT("CombatGroup")};
    const TCHAR* Headings[]={TEXT("HP / MP / SP"),TEXT("기본 능력"),TEXT("전투 능력")};
    for(int32 Group=0;Group<3;++Group)
    {
        auto* Frame=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),GroupNames[Group]);Frame->SetBrushColor(FLinearColor(.045f,.065f,.095f));Frame->SetPadding(FMargin(12));
        auto* Placement=Groups->AddChildToHorizontalBox(Frame);Placement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Placement->SetPadding(FMargin(0,0,Group<2?10:0,0));Placement->SetVerticalAlignment(VAlign_Fill);
        Panels[Group]=Tree->ConstructWidget<UVerticalBox>();Frame->SetContent(Panels[Group]);
        auto* Heading=Text(Tree,NAME_None,FText::FromString(Headings[Group]),14);Heading->SetColorAndOpacity(FLinearColor(1,.76f,.34f));Add(Panels[Group],Heading,12);
    }
    const FText Labels[]={NSLOCTEXT("ICParty","HP","HP"),NSLOCTEXT("ICParty","MP","MP"),NSLOCTEXT("ICParty","SP","SP"),
        NSLOCTEXT("ICParty","STR","근력"),NSLOCTEXT("ICParty","DEX","민첩"),NSLOCTEXT("ICParty","INT","지능"),NSLOCTEXT("ICParty","CON","체력"),NSLOCTEXT("ICParty","LUK","행운"),
        NSLOCTEXT("ICParty","Physical","물공"),NSLOCTEXT("ICParty","Magic","마공"),NSLOCTEXT("ICParty","Defense","방어력"),NSLOCTEXT("ICParty","Speed","이속")};
    for(int32 Index=0;Index<UE_ARRAY_COUNT(StatKeys);++Index)
    {
        auto* Row=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),FName(*FString::Printf(TEXT("ROW_%s"),StatKeys[Index])));
        Row->AddChildToHorizontalBox(Text(Tree,FName(*FString::Printf(TEXT("LABEL_%s"),StatKeys[Index])),Labels[Index],14))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Value=Text(Tree,FName(*FString::Printf(TEXT("STAT_%s"),StatKeys[Index])),FText::GetEmpty(),14);
        Value->SetJustification(ETextJustify::Right);Row->AddChildToHorizontalBox(Value);
        Add(Panels[Index<3?0:Index<8?1:2],Row,7);
    }
    return true;
}
UICPartyMenuWidget::UICPartyMenuWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
    ProfileTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Character/DT_IC_CharacterProfile.DT_IC_CharacterProfile")));
}
void UICPartyMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    for(int32 Index=0;Index<4;++Index)if(auto* Card=Cast<UICPartyPortraitWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("Member%d"),Index)))))
        Card->OnPicked.AddUniqueDynamic(this,&ThisClass::PortraitPicked);
    RefreshParty();
}
void UICPartyMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState){BoundState->OnCharacterChanged.AddUniqueDynamic(this,&ThisClass::CharacterChanged);BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::RefreshParty);}
    RefreshParty();
}
void UICPartyMenuWidget::NativeDestruct()
{
    if(BoundState){BoundState->OnCharacterChanged.RemoveDynamic(this,&ThisClass::CharacterChanged);BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::RefreshParty);BoundState=nullptr;}
    Super::NativeDestruct();
}
void UICPartyMenuWidget::CharacterChanged(FName){RefreshParty();}
void UICPartyMenuWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{Super::NativeTick(Geometry,DeltaSeconds);StatusRefreshAt-=DeltaSeconds;if(StatusRefreshAt<=0){StatusRefreshAt=.25f;RefreshDetails();}}
void UICPartyMenuWidget::PortraitPicked(int32 Index){SelectMember(Index);}
void UICPartyMenuWidget::RefreshParty()
{
    if(bRefreshing)return;TGuardValue<bool> Guard(bRefreshing,true);
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    const auto Data=State?State->ExportState():FICWorldState();Members.Reset();
    if(Data.Characters.Contains(UICWorldStateSubsystem::PlayerId()))Members.Add(UICWorldStateSubsystem::PlayerId());
    for(int32 PartySlot=0;PartySlot<3;++PartySlot)for(const auto& Pair:Data.Characters)
        if(Pair.Value.Kind==EICCharacterKind::Recruitable&&Pair.Value.Mode==EICCharacterMode::Companion&&Pair.Value.PartySlot==PartySlot)
        {Members.Add(Pair.Key);break;}
    const int32 Previous=Members.IndexOfByKey(SelectedCharacter);SelectedIndex=Members.IsEmpty()?INDEX_NONE:Previous!=INDEX_NONE?Previous:FMath::Clamp(SelectedIndex,0,Members.Num()-1);
    SelectedCharacter=Members.IsValidIndex(SelectedIndex)?Members[SelectedIndex]:NAME_None;
    for(int32 Index=0;Index<4;++Index)if(auto* Card=Cast<UICPartyPortraitWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("Member%d"),Index)))))
    {
        UTexture2D* Portrait=nullptr;
        if(D&&Members.IsValidIndex(Index))
        {
            FICDefinitionResult R;
            if(Members[Index]==UICWorldStateSubsystem::PlayerId())
            {if(const auto* Preset=D->Find<FICPlayerPresetRow>(Data.Preset==1?TEXT("PRESET_PLAYER_F"):TEXT("PRESET_PLAYER_M"),R))Portrait=Preset->Portrait.LoadSynchronous();}
            else if(const auto* NPC=D->Find<FICNPCRow>(Members[Index],R))Portrait=NPC->Portrait.LoadSynchronous();
        }
        Card->Present(Index,Members.IsValidIndex(Index)?Members[Index]:NAME_None,Portrait,Index==SelectedIndex);
        Card->SetVisibility(Members.IsValidIndex(Index)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(Card->GetParent())Card->GetParent()->SetVisibility(Card->GetVisibility());
    }
    RefreshDetails();
}
bool UICPartyMenuWidget::SelectMember(int32 Index)
{
    if(!Members.IsValidIndex(Index))return false;
    if(SelectedCharacter!=Members[Index])ScrollStory(-100000);
    SelectedIndex=Index;SelectedCharacter=Members[Index];RefreshParty();return true;
}
bool UICPartyMenuWidget::MoveSelection(int32 Direction){return !Members.IsEmpty()&&SelectMember(FMath::Clamp(SelectedIndex+Direction,0,Members.Num()-1));}
void UICPartyMenuWidget::ScrollStory(float Amount)
{if(auto* Story=Cast<UScrollBox>(GetWidgetFromName(TEXT("StoryScroll"))))Story->SetScrollOffset(FMath::Clamp(Story->GetScrollOffset()+Amount,0.f,Story->GetScrollOffsetOfEnd()));}
void UICPartyMenuWidget::RefreshDetails()
{
    auto* GI=GetGameInstance();auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    const bool HasCharacter=State&&!SelectedCharacter.IsNone()&&State->GetStoredCharacter(SelectedCharacter);
    if(auto* Details=GetWidgetFromName(TEXT("CharacterDetails")))Details->SetVisibility(HasCharacter?ESlateVisibility::Visible:ESlateVisibility::Hidden);
    if(!HasCharacter)return;
    const bool Player=SelectedCharacter==UICWorldStateSubsystem::PlayerId();const auto Data=State->ExportState();const auto Snapshot=State->GetSnapshot(SelectedCharacter);
    FText Name=FText::FromString(Data.PlayerName);FICDefinitionResult R;
    if(!Player)
    {
        const auto* NPC=D?D->Find<FICNPCRow>(SelectedCharacter,R):nullptr;
        auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
        const auto Text=UICTextLibrary::ResolveText(Strings,NPC?NPC->NameKey:FString());Name=Text.bSuccess?Text.Text:FText::FromName(SelectedCharacter);
    }
    SetText(this,TEXT("TXT_Name"),Name);
    auto* Table=ProfileTable.LoadSynchronous();const auto* Profile=Table?Table->FindRow<FICCharacterProfileRow>(SelectedCharacter,TEXT("PartyProfile"),false):nullptr;
    SetText(this,TEXT("TXT_MainSetting"),Profile?Profile->MainSetting:FText::GetEmpty());SetText(this,TEXT("TXT_Story"),Profile?Profile->Story:FText::GetEmpty());
    if(auto* Image=Cast<UImage>(GetWidgetFromName(TEXT("IMG_Pet"))))
    {auto* Codex=GI?GI->GetSubsystem<UICCodexSubsystem>():nullptr;auto* Texture=Codex?Codex->GetEquippedPetPortrait(SelectedCharacter):nullptr;
     if(!Texture&&(!Codex||Codex->GetEquippedPet(SelectedCharacter)==INDEX_NONE))Texture=Profile?Profile->PetPortrait.LoadSynchronous():nullptr;
     Image->SetBrushFromTexture(Texture);Image->SetVisibility(Texture?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);}
    const UICAbilitySystemComponent* ASC=nullptr;
    for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->GetCharacterID()==SelectedCharacter){ASC=It->GetICAbilitySystem();break;}
    const auto* Clock=GI?GI->GetSubsystem<UICTimeSubsystem>():nullptr;
    SetText(this,TEXT("TXT_CurrentStatuses"),ICPartyStatus::Describe(ASC?ASC->Capture():Snapshot,Data,D,Clock?Clock->GetClock().PlaySecond:0,ASC));
    const double Main[]={double(Snapshot.MainStats.STR),double(Snapshot.MainStats.DEX),double(Snapshot.MainStats.INT),double(Snapshot.MainStats.CON),double(Snapshot.MainStats.LUK)};
    const EICStat Stats[]={EICStat::STR,EICStat::DEX,EICStat::INT,EICStat::CON,EICStat::LUK,EICStat::PhysicalAttack,EICStat::MagicAttack,EICStat::Defense,EICStat::MoveSpeed};
    for(int32 Index=0;Index<UE_ARRAY_COUNT(StatKeys);++Index)
    {
        FString Value;
        if(Index<3)
        {const float Current[]={Snapshot.HP,Snapshot.MP,Snapshot.SP},Max[]={Snapshot.MaxHP,Snapshot.MaxMP,Snapshot.MaxSP};Value=FString::Printf(TEXT("%d / %d"),FMath::FloorToInt(Current[Index]),FMath::FloorToInt(Max[Index]));}
        else if(ASC)Value=FString::FromInt(FMath::RoundToInt(ASC->Stat(Stats[Index-3])));
        else if(Index<8)Value=FString::FromInt(FMath::RoundToInt(Main[Index-3]));
        else Value=TEXT("—");
        SetText(this,*FString::Printf(TEXT("STAT_%s"),StatKeys[Index]),FText::FromString(Value));
    }
    if(auto* SP=GetWidgetFromName(TEXT("ROW_SP")))SP->SetVisibility(Player?ESlateVisibility::Visible:ESlateVisibility::Hidden);
}
