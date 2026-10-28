#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Settings/ICSettingsLayout.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Framework/ICUITheme.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Settings/ICGameUserSettings.h"
#include "Data/Text/ICLocalization.h"
#include "Core/Controller/ICPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/InputKeySelector.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#define LOCTEXT_NAMESPACE "ICSettings"

void UICSettingBinding::SliderChanged(float Value){if(Owner)Owner->ChangeOption(ID,Value);}
void UICSettingBinding::CheckChanged(bool Value){if(Owner)Owner->ChangeOption(ID,Value?1:0);}
void UICSettingBinding::ChoiceChanged(FString Value,ESelectInfo::Type Type){if(Owner)Owner->ChangeChoice(ID,Value);}
void UICSettingBinding::KeyChanged(FInputChord Value){if(Owner)Owner->ChangeKey(this,Value);}
void UICSettingBinding::Clicked()
{
    if(!Owner)return;
    if(ID==TEXT("Tab"))Owner->SelectTab(Tab);
    else if(ID==TEXT("Apply"))Owner->ApplyOptions();
    else if(ID==TEXT("Reset"))Owner->ResetOptions();
    else if(ID==TEXT("KeepDisplay"))Owner->KeepDisplay();
    else if(ID==TEXT("RevertDisplay"))Owner->RevertDisplay();
    else if(ID==TEXT("Close"))Owner->Close();
}
UICSettingBinding* UICSettingsWidget::Bind(UWidget* Control,FName ID)
{
    auto* Binding=NewObject<UICSettingBinding>(this);Binding->Owner=this;Binding->Control=Control;Binding->ID=ID;Bindings.Add(Binding);return Binding;
}
void UICSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    if(!WidgetTree->RootWidget)ICSettingsLayout::Build(WidgetTree);
    for(const auto& Spec:ICSettingsLayout::Specs())
    {
        auto* Control=GetWidgetFromName(FName(*(TEXT("OPTION_")+Spec.ID.ToString())));if(!Control)continue;
        auto* Binding=Bind(Control,Spec.ID);
        if(auto* Slider=Cast<USlider>(Control))Slider->OnValueChanged.AddDynamic(Binding,&UICSettingBinding::SliderChanged);
        if(auto* Check=Cast<UCheckBox>(Control))Check->OnCheckStateChanged.AddDynamic(Binding,&UICSettingBinding::CheckChanged);
        if(auto* Choice=Cast<UComboBoxString>(Control))Choice->OnSelectionChanged.AddDynamic(Binding,&UICSettingBinding::ChoiceChanged);
    }
    for(int32 I=0;I<ICSettingsLayout::Tabs().Num();++I)
        if(auto* Button=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_SettingsTab_%d"),I)))))
        {auto* Binding=Bind(Button,TEXT("Tab"));Binding->Tab=I;Button->OnClicked.AddDynamic(Binding,&UICSettingBinding::Clicked);}
    for(const TCHAR* ID:{TEXT("Apply"),TEXT("Reset"),TEXT("KeepDisplay"),TEXT("RevertDisplay"),TEXT("Close")})
        if(auto* Button=Cast<UButton>(GetWidgetFromName(FName(*(FString(TEXT("BTN_"))+ID)))))
        {auto* Binding=Bind(Button,ID);Button->OnClicked.AddDynamic(Binding,&UICSettingBinding::Clicked);}
}
void UICSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();ReadCurrent();BuildKeyBindings();RefreshControls();SelectTab(ActiveTab);
    Status(LOCTEXT("Ready","변경 후 적용 · 저장을 눌러주세요. 닫으면 미적용 변경 사항은 취소됩니다."));
}
void UICSettingsWidget::ReadCurrent()
{
    auto* Settings=UICGameUserSettings::Get();Draft.Reset();
    for(const auto& Spec:ICSettingsLayout::Specs())Draft.Add(Spec.ID,Settings->ReadOption(Spec.ID));
    DraftKeys=Settings->KeyOverrides;BeforeKeys=DraftKeys;Before=Draft;
    WindowMode=int32(Settings->GetFullscreenMode());BeforeWindowMode=WindowMode;
    Resolution=Settings->GetScreenResolution();Displays=ICDisplaySettings::Monitors();
    MonitorIndex=Displays.IndexOfByPredicate([&](const FICMonitor& M){return M.Device==Settings->MonitorDevice;});
    if(MonitorIndex==INDEX_NONE){MonitorIndex=Displays.IndexOfByPredicate([](const FICMonitor& M){return M.bPrimary;});if(MonitorIndex==INDEX_NONE)MonitorIndex=0;}
    if(Resolution.X<=0||Resolution.Y<=0)Resolution=Displays.IsValidIndex(MonitorIndex)?FIntPoint(Displays[MonitorIndex].Current.Width,Displays[MonitorIndex].Current.Height):FIntPoint(1920,1080);
    BeforeResolution=Resolution;
    BeforeMonitor=Displays.IsValidIndex(MonitorIndex)?Displays[MonitorIndex].Device:Settings->MonitorDevice;
    RefreshHz=Settings->MonitorRefreshRate>0?Settings->MonitorRefreshRate:Displays.IsValidIndex(MonitorIndex)?Displays[MonitorIndex].Current.Hz:60;
    BeforeHz=RefreshHz;
}
void UICSettingsWidget::SelectTab(int32 Index)
{
    ActiveTab=FMath::Clamp(Index,0,ICSettingsLayout::Tabs().Num()-1);
    if(auto* Tabs=Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("SettingsTabSwitcher"))))Tabs->SetActiveWidgetIndex(ActiveTab);
    for(int32 I=0;I<ICSettingsLayout::Tabs().Num();++I)
        if(auto* Button=Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_SettingsTab_%d"),I)))))
            ICUITheme::SetSelected(Button,I==ActiveTab);
    OptionIndex=0;RefreshNavigation();
}
void UICSettingsWidget::Status(const FText& Value)
{if(auto* Label=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SettingsStatus"))))Label->SetText(Value);}
void UICSettingsWidget::RefreshDisplays()
{
    auto* Monitor=Cast<UComboBoxString>(GetWidgetFromName(TEXT("OPTION_Monitor")));
    auto* ResolutionChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("OPTION_Resolution")));
    auto* HzChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("OPTION_RefreshRate")));
    if(Monitor){Monitor->ClearOptions();for(const auto& Display:Displays)Monitor->AddOption(Display.Label);Monitor->SetSelectedIndex(MonitorIndex);Monitor->SetIsEnabled(Displays.Num()>1);}
    Resolutions.Reset();RefreshRates.Reset();
    if(Displays.IsValidIndex(MonitorIndex))
    {
        const auto& Display=Displays[MonitorIndex];
        const auto Modes=ICDisplaySettings::Modes(Display.Device);
        for(const auto& Mode:Modes)Resolutions.AddUnique(FIntPoint(Mode.Width,Mode.Height));
        if(Resolutions.IsEmpty())Resolutions.Add(FIntPoint(Display.Current.Width,Display.Current.Height));
        if(!Resolutions.Contains(Resolution))Resolution=FIntPoint(Display.Current.Width,Display.Current.Height);
        const FIntPoint Actual=WindowMode==0?Resolution:FIntPoint(Display.Current.Width,Display.Current.Height);
        for(const auto& Mode:Modes)if(Mode.Width==Actual.X&&Mode.Height==Actual.Y)RefreshRates.AddUnique(Mode.Hz);
        if(RefreshRates.IsEmpty())RefreshRates.Add(Display.Current.Hz);
        if(!RefreshRates.Contains(RefreshHz))RefreshHz=RefreshRates.Contains(Display.Current.Hz)?Display.Current.Hz:RefreshRates.Last();
    }
    if(ResolutionChoice)
    {
        ResolutionChoice->ClearOptions();for(const auto& Value:Resolutions)ResolutionChoice->AddOption(FString::Printf(TEXT("%d × %d"),Value.X,Value.Y));
        const FIntPoint Displayed=WindowMode==1&&Displays.IsValidIndex(MonitorIndex)?FIntPoint(Displays[MonitorIndex].Current.Width,Displays[MonitorIndex].Current.Height):Resolution;
        ResolutionChoice->SetSelectedIndex(Resolutions.IndexOfByKey(Displayed));ResolutionChoice->SetIsEnabled(WindowMode!=1);
    }
    if(HzChoice){HzChoice->ClearOptions();for(int32 Hz:RefreshRates)HzChoice->AddOption(FString::Printf(TEXT("%d Hz"),Hz));HzChoice->SetSelectedIndex(RefreshRates.IndexOfByKey(RefreshHz));HzChoice->SetIsEnabled(RefreshRates.Num()>1);}
}
void UICSettingsWidget::RefreshControls()
{
    TGuardValue<bool> Guard(bRefreshing,true);
    for(const auto& Spec:ICSettingsLayout::Specs())
    {
        const double Value=Spec.ID==TEXT("WindowMode")?WindowMode:Draft.FindRef(Spec.ID);
        auto* Control=GetWidgetFromName(FName(*(TEXT("OPTION_")+Spec.ID.ToString())));if(!Control)continue;
        if(auto* Slider=Cast<USlider>(Control))
        {
            Slider->SetValue(Value);
            if(auto* Label=Cast<UTextBlock>(GetWidgetFromName(FName(*(TEXT("VALUE_")+Spec.ID.ToString())))))
            {
                if(Spec.ID.ToString().EndsWith(TEXT("Volume"))||Spec.ID==TEXT("UIScale"))Label->SetText(FText::AsPercent(Value));
                else Label->SetText(FText::AsNumber(Value,&FNumberFormattingOptions().SetMaximumFractionalDigits(2)));
            }
        }
        if(auto* Check=Cast<UCheckBox>(Control))Check->SetIsChecked(Value!=0);
        if(auto* Choice=Cast<UComboBoxString>(Control);Choice&&!Spec.Choices.IsEmpty())
        {
            bool Rebuild=Choice->GetOptionCount()!=Spec.Choices.Num();
            for(int32 I=0;!Rebuild&&I<Spec.Choices.Num();++I)Rebuild=Choice->GetOptionAtIndex(I)!=ICLocalization::Localize(Spec.Choices[I]).ToString();
            if(Rebuild){Choice->ClearOptions();for(const auto& Text:Spec.Choices)Choice->AddOption(ICLocalization::Localize(Text).ToString());}
            Choice->SetSelectedIndex(Spec.Values.IndexOfByPredicate([&](double Candidate){return FMath::IsNearlyEqual(Candidate,Value,.01);}));
        }
    }
    if(auto* Help=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_ControllerHelp"))))Help->SetText(ICSettingsLayout::ControllerHelp(FMath::RoundToInt(Draft.FindRef(TEXT("GamepadLayout")))));
    RefreshDisplays();
    if(auto* AA=GetWidgetFromName(TEXT("OPTION_AntiAliasing")))AA->SetIsEnabled(Draft.FindRef(TEXT("AntiAliasingMethod"))!=0);
    for(const auto& Binding:KeyBindings)if(auto* Key=Cast<UInputKeySelector>(Binding->Control))
    {const FString* Override=DraftKeys.Find(Binding->KeyID);Key->SetSelectedKey(FInputChord(Override?FKey(**Override):Binding->DefaultKey));}
    for(const TCHAR* Name:{TEXT("BTN_KeepDisplay"),TEXT("BTN_RevertDisplay")})if(auto* Button=GetWidgetFromName(Name))if(auto* Parent=Button->GetParent())Parent->SetVisibility(bPreview?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    for(const TCHAR* Name:{TEXT("BTN_Apply"),TEXT("BTN_Reset")})if(auto* Button=GetWidgetFromName(Name))Button->SetIsEnabled(!bPreview);
    if(auto* Body=GetWidgetFromName(TEXT("SettingsBody")))Body->SetIsEnabled(!bPreview);
    RefreshNavigation();
}
void UICSettingsWidget::ChangeOption(FName ID,double Value)
{
    if(bRefreshing||bPreview)return;
    const auto* Spec=ICSettingsLayout::Specs().FindByPredicate([&](const FICSettingSpec& S){return S.ID==ID;});
    if(Spec&&Spec->Control==EICSettingControl::Slider)Value=FMath::Clamp(FMath::GridSnap(Value,Spec->Step),Spec->Min,Spec->Max);
    Draft.Add(ID,Value);
    if(ID==TEXT("Quality")&&Value>=0)
    {for(const TCHAR* Quality:{TEXT("ViewDistance"),TEXT("Texture"),TEXT("Shadow"),TEXT("GlobalIllumination"),TEXT("Reflection"),TEXT("AntiAliasing"),TEXT("VisualEffect"),TEXT("PostProcessing"),TEXT("Foliage"),TEXT("Shading"),TEXT("Landscape")})Draft.Add(Quality,Value);}
    else if(ID==TEXT("ViewDistance")||ID==TEXT("Texture")||ID==TEXT("Shadow")||ID==TEXT("GlobalIllumination")||ID==TEXT("Reflection")||ID==TEXT("AntiAliasing")||ID==TEXT("VisualEffect")||ID==TEXT("PostProcessing")||ID==TEXT("Foliage")||ID==TEXT("Shading")||ID==TEXT("Landscape"))Draft.Add(TEXT("Quality"),-1);
    RefreshControls();Status(LOCTEXT("Changed","변경 사항이 있습니다. 적용 · 저장을 눌러 반영하세요."));
}
void UICSettingsWidget::ChangeChoice(FName ID,const FString& Value)
{
    if(bRefreshing||bPreview)return;
    auto* Choice=Cast<UComboBoxString>(GetWidgetFromName(FName(*(TEXT("OPTION_")+ID.ToString()))));if(!Choice)return;
    const int32 Index=Choice->FindOptionIndex(Value);
    if(ID==TEXT("Monitor")){MonitorIndex=Index;if(Displays.IsValidIndex(Index)){Resolution=FIntPoint(Displays[Index].Current.Width,Displays[Index].Current.Height);RefreshHz=Displays[Index].Current.Hz;}RefreshControls();}
    else if(ID==TEXT("Resolution")){if(Resolutions.IsValidIndex(Index))Resolution=Resolutions[Index];RefreshControls();}
    else if(ID==TEXT("RefreshRate")){if(RefreshRates.IsValidIndex(Index))RefreshHz=RefreshRates[Index];}
    else if(ID==TEXT("WindowMode")){WindowMode=Index;RefreshControls();}
    else if(const auto* Spec=ICSettingsLayout::Specs().FindByPredicate([&](const FICSettingSpec& S){return S.ID==ID;});Spec&&Spec->Values.IsValidIndex(Index))ChangeOption(ID,Spec->Values[Index]);
}
namespace
{
FText BindingName(AICPlayerController* PC,const UInputAction* Action,FKey Key)
{
    if(Action==PC->MoveAction)
    {
        if(Key==EKeys::W||Key==EKeys::Up)return LOCTEXT("Forward","이동 · 앞으로");
        if(Key==EKeys::S||Key==EKeys::Down)return LOCTEXT("Backward","이동 · 뒤로");
        if(Key==EKeys::A||Key==EKeys::Left)return LOCTEXT("Left","이동 · 왼쪽");
        return LOCTEXT("Right","이동 · 오른쪽");
    }
    for(const auto& Binding:PC->CommandActions)if(Binding.Action==Action)
    {
        switch(Binding.Command)
        {
#define IC_BINDING(Command,Label) case EICInputCommand::Command:return LOCTEXT(#Command,Label);
            IC_BINDING(Jump,"점프") IC_BINDING(Attack,"일반 공격") IC_BINDING(Dodge,"회피 / 질주")
            IC_BINDING(NextTarget,"대상 전환") IC_BINDING(ClearTarget,"대상 해제") IC_BINDING(Interact,"상호작용")
            IC_BINDING(Skill,"무기 스킬") IC_BINDING(Inventory,"인벤토리") IC_BINDING(Equipment,"장비")
            IC_BINDING(Quest,"퀘스트") IC_BINDING(Map,"지도") IC_BINDING(Party,"파티")
            IC_BINDING(Menu,"메뉴") IC_BINDING(Save,"저장") IC_BINDING(Load,"불러오기")
            IC_BINDING(Confirm,"확인") IC_BINDING(Stats,"능력치") IC_BINDING(Skills,"스킬")
            IC_BINDING(Codex,"도감") IC_BINDING(SwitchWeapon,"무기 전환") IC_BINDING(Settings,"설정")
            IC_BINDING(Fishing,"낚시") IC_BINDING(QuestFollow,"퀘스트 추적")
#undef IC_BINDING
            case EICInputCommand::CompanionSkill:return FText::Format(LOCTEXT("CompanionBinding","동료 스킬 {0}"),FText::AsNumber(Binding.Slot+1));
            case EICInputCommand::QuickSlot:return FText::Format(LOCTEXT("QuickBinding","퀵 슬롯 {0}"),FText::AsNumber(Binding.Slot+1));
            default:break;
        }
    }
    return LOCTEXT("GameBinding","게임 조작");
}
}
void UICSettingsWidget::BuildKeyBindings()
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* List=Cast<UVerticalBox>(GetWidgetFromName(TEXT("LIST_KeyBindings")));
    if(!PC)if(auto* Type=LoadClass<AICPlayerController>(nullptr,TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController.BP_ICPlayerController_C")))PC=Cast<AICPlayerController>(Type->GetDefaultObject());
    if(!PC||!PC->DefaultMappingContext||!List)return;
    List->ClearChildren();KeyBindings.Reset();
    for(const auto& Mapping:PC->DefaultMappingContext->GetMappings())
    {
        if(!Mapping.Action||Mapping.Key.IsGamepadKey()||Mapping.Key.IsAxis1D()||Mapping.Key.IsAxis2D()||Mapping.Key.IsAxis3D()||Mapping.Action==PC->LookAction)continue;
        bool Roll=false;for(const auto& Binding:PC->CommandActions)if(Binding.Action==Mapping.Action&&Binding.Command==EICInputCommand::Roll)Roll=true;
        if(Roll)continue;
        auto* Frame=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),FName(*FString::Printf(TEXT("KeyRow_%d"),KeyBindings.Num())));
        Frame->SetBrush(ICUITheme::PanelBrush());Frame->SetPadding(FMargin(22,16));List->AddChildToVerticalBox(Frame)->SetPadding(FMargin(0,0,22,10));
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Frame->SetContent(Row);
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetFont(ICFrontendLayout::Font(18));Label->SetText(BindingName(PC,Mapping.Action,Mapping.Key));Row->AddChildToHorizontalBox(Label)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Width=WidgetTree->ConstructWidget<USizeBox>();Width->SetWidthOverride(280);Width->SetHeightOverride(38);Row->AddChildToHorizontalBox(Width);
        auto* Selector=WidgetTree->ConstructWidget<UInputKeySelector>();Selector->SetAllowModifierKeys(false);Selector->SetAllowGamepadKeys(false);Selector->SetEscapeKeys({EKeys::Escape});
        auto Style=Selector->GetTextStyle();Style.SetFont(ICFrontendLayout::Font(17));Selector->SetTextStyle(Style);
        Selector->SetKeySelectionText(LOCTEXT("PressKey","새 키를 누르세요…"));Width->SetContent(Selector);
        auto* Binding=Bind(Selector,TEXT("Key"));Binding->KeyID=Mapping.Action->GetName()+TEXT(":")+Mapping.Key.GetFName().ToString();Binding->DefaultKey=Mapping.Key;
        Binding->Label=Label->GetText();
        Selector->SetIsEnabled(Mapping.Key!=EKeys::Escape);Selector->OnKeySelected.AddDynamic(Binding,&UICSettingBinding::KeyChanged);KeyBindings.Add(Binding);
    }
}
void UICSettingsWidget::ChangeKey(UICSettingBinding* Binding,FInputChord Value)
{
    if(bRefreshing||bPreview||!Value.Key.IsValid())return;
    if(Value.Key==EKeys::Escape||Value.Key.IsGamepadKey()||Value.Key.IsAxis1D()||Value.Key.IsAxis2D())
    {Status(LOCTEXT("ReservedKey","이 키는 사용할 수 없습니다. ESC는 메뉴 복귀용으로 유지됩니다."));RefreshControls();return;}
    const FString* Existing=DraftKeys.Find(Binding->KeyID);
    const FKey Previous=Existing?FKey(**Existing):Binding->DefaultKey;
    // Swapping conflicting bindings preserves access to both commands.
    for(const auto& Other:KeyBindings)if(Other.Get()!=Binding)
    {
        const FString* Saved=DraftKeys.Find(Other->KeyID);const FKey Current=Saved?FKey(**Saved):Other->DefaultKey;
        if(Current==Value.Key&&Other->DefaultKey!=Binding->DefaultKey)DraftKeys.Add(Other->KeyID,Previous.GetFName().ToString());
    }
    DraftKeys.Add(Binding->KeyID,Value.Key.GetFName().ToString());RefreshControls();Status(LOCTEXT("KeyChanged","키가 변경되었습니다. 중복 키는 기존 키와 교환됩니다. 적용 · 저장으로 반영하세요."));
}
void UICSettingsWidget::WriteDraft()
{
    auto* Settings=UICGameUserSettings::Get();Settings->WriteOption(TEXT("Quality"),Draft.FindRef(TEXT("Quality")));
    for(const auto& Pair:Draft)if(Pair.Key!=TEXT("Quality"))Settings->WriteOption(Pair.Key,Pair.Value);
    Settings->KeyOverrides=DraftKeys;Settings->SetFullscreenMode(EWindowMode::Type(WindowMode));Settings->SetScreenResolution(Resolution);
    Settings->MonitorDevice=Displays.IsValidIndex(MonitorIndex)?Displays[MonitorIndex].Device:FString();Settings->MonitorRefreshRate=RefreshHz;
    Settings->ValidateSettings();
}
void UICSettingsWidget::ApplyOptions()
{
    if(bPreview)return;
    auto* Settings=UICGameUserSettings::Get();
    const FString Device=Displays.IsValidIndex(MonitorIndex)?Displays[MonitorIndex].Device:BeforeMonitor;
    const bool DisplayChanged=WindowMode!=BeforeWindowMode||Resolution!=BeforeResolution||Device!=BeforeMonitor||RefreshHz!=BeforeHz;
    if(DisplayChanged&&GetWorld()&&GetWorld()->WorldType==EWorldType::PIE)
    {Status(LOCTEXT("PIEDisplay","모니터·화면 모드·주사율 변경은 독립 실행(Standalone)에서 적용하세요. 다른 옵션은 화면 설정을 유지하면 적용할 수 있습니다."));return;}
    OldDisplays=ICDisplaySettings::Monitors();
    if(DisplayChanged&&Displays.IsValidIndex(MonitorIndex))
    {
        const auto& Monitor=Displays[MonitorIndex];const auto Current=Monitor.Current;
        const FICDisplayMode Mode{WindowMode==0?Resolution.X:Current.Width,WindowMode==0?Resolution.Y:Current.Height,RefreshHz};
        FString Error;if(!ICDisplaySettings::Apply(Device,Mode,Error)){Status(FText::FromString(Error));return;}
        ICDisplaySettings::MoveGameWindow(Monitor);
    }
    WriteDraft();Settings->ApplyNonResolutionSettings();
    if(DisplayChanged)Settings->ApplyResolutionSettings(false);
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer())){Settings->ApplyRuntime(PC);PC->RefreshUserInputMapping();}
    if(DisplayChanged){bPreview=true;ConfirmDeadline=FPlatformTime::Seconds()+15;RefreshControls();}
    else SaveApplied();
}
void UICSettingsWidget::SaveApplied()
{
    auto* Settings=UICGameUserSettings::Get();Settings->ConfirmVideoMode();Settings->SaveSettings();bPreview=false;ReadCurrent();RefreshControls();Status(LOCTEXT("Saved","설정을 적용하고 저장했습니다."));
}
void UICSettingsWidget::KeepDisplay(){if(bPreview)SaveApplied();}
void UICSettingsWidget::RevertDisplay()
{
    if(!bPreview)return;bPreview=false;
    for(const auto& Display:OldDisplays){FString Error;ICDisplaySettings::Apply(Display.Device,Display.Current,Error);}
    auto* Settings=UICGameUserSettings::Get();
    for(const auto& Pair:Before)Settings->WriteOption(Pair.Key,Pair.Value);
    Settings->KeyOverrides=BeforeKeys;Settings->MonitorDevice=BeforeMonitor;Settings->MonitorRefreshRate=BeforeHz;
    Settings->SetFullscreenMode(EWindowMode::Type(BeforeWindowMode));Settings->SetScreenResolution(BeforeResolution);
    Settings->ApplyResolutionSettings(false);Settings->ApplyNonResolutionSettings();
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer())){Settings->ApplyRuntime(PC);PC->RefreshUserInputMapping();}
    const auto Monitors=ICDisplaySettings::Monitors();if(const auto* Previous=Monitors.FindByPredicate([&](const FICMonitor& M){return M.Device==BeforeMonitor;}))ICDisplaySettings::MoveGameWindow(*Previous);
    ReadCurrent();RefreshControls();Status(LOCTEXT("Reverted","화면 변경을 취소하고 이전 설정으로 돌아왔습니다."));
}
void UICSettingsWidget::ResetOptions()
{
    if(bPreview)return;
    auto* Defaults=NewObject<UICGameUserSettings>();Defaults->SetToDefaults();Defaults->SetOverallScalabilityLevel(2);Defaults->SetResolutionScaleValueEx(100);
    for(const auto& Spec:ICSettingsLayout::Specs())Draft.Add(Spec.ID,Defaults->ReadOption(Spec.ID));
    DraftKeys.Reset();RefreshControls();Status(LOCTEXT("Defaults","화면 모드와 해상도를 유지하고 옵션·키 설정을 기본값으로 되돌렸습니다. 적용 · 저장으로 반영하세요."));
}
void UICSettingsWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    if(bPreview)
    {
        const int32 Remaining=FMath::CeilToInt(ConfirmDeadline-FPlatformTime::Seconds());
        if(Remaining<=0)RevertDisplay();
        else Status(FText::Format(LOCTEXT("KeepCountdown","현재 화면 설정을 유지할까요? {0}초 후 이전 설정으로 돌아갑니다."),FText::AsNumber(Remaining)));
    }
}
void UICSettingsWidget::NativeDestruct(){if(bPreview)RevertDisplay();Super::NativeDestruct();}
void UICSettingsWidget::Close(){if(bPreview)RevertDisplay();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->CloseSessionMenu();}
FReply UICSettingsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    for(const auto& Binding:KeyBindings)if(const auto* Key=Cast<UInputKeySelector>(Binding->Control);Key&&Key->GetIsSelectingKey())return Super::NativeOnPreviewKeyDown(Geometry,Event);
    if(Event.GetKey()==EKeys::Escape||UICGameUserSettings::IsGamepadBackKey(Event.GetKey())){if(!Event.IsRepeat())Close();return FReply::Handled();}
    if(const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->DefaultMappingContext)
        for(const auto& Mapping:PC->DefaultMappingContext->GetMappings())for(const auto& Binding:PC->CommandActions)
            if(Binding.Action==Mapping.Action&&Binding.Command==EICInputCommand::Settings)
            {
                const FString ID=Mapping.Action->GetName()+TEXT(":")+Mapping.Key.GetFName().ToString();
                const FString* Override=UICGameUserSettings::Get()->KeyOverrides.Find(ID);
                if(Event.GetKey()==(Override?FKey(**Override):Mapping.Key)){if(!Event.IsRepeat())Close();return FReply::Handled();}
            }
    if(HandleNavigationKey(Event.GetKey(),Event.IsRepeat()))return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

namespace
{
TArray<UButton*> SettingsFooter(UICSettingsWidget* Widget)
{
    TArray<UButton*> Out;
    for(const auto* Name:{TEXT("BTN_Reset"),TEXT("BTN_Apply"),TEXT("BTN_KeepDisplay"),TEXT("BTN_RevertDisplay"),TEXT("BTN_Close")})
        if(auto* Button=Cast<UButton>(Widget->GetWidgetFromName(Name));Button&&Button->IsVisible()&&Button->GetIsEnabled()&&(!Button->GetParent()||Button->GetParent()->IsVisible()))Out.Add(Button);
    return Out;
}
}
TArray<UWidget*> UICSettingsWidget::NavigationOptions() const
{
    TArray<UWidget*> Out;
    for(const auto& Spec:ICSettingsLayout::Specs())if(Spec.Tab==ActiveTab)
        if(auto* Control=GetWidgetFromName(FName(*(TEXT("OPTION_")+Spec.ID.ToString())));Control&&Control->GetIsEnabled())Out.Add(Control);
    if(ActiveTab==1)for(const auto& Binding:KeyBindings)if(Binding->Control&&Binding->Control->GetIsEnabled())Out.Add(Binding->Control);
    return Out;
}
void UICSettingsWidget::RefreshNavigation()
{
    const auto Options=NavigationOptions();OptionIndex=FMath::Clamp(OptionIndex,0,FMath::Max(0,Options.Num()-1));
    UWidget* Selected=Options.IsValidIndex(OptionIndex)?Options[OptionIndex]:nullptr;
    for(const auto& Spec:ICSettingsLayout::Specs())
    {
        auto* Control=GetWidgetFromName(FName(*(TEXT("OPTION_")+Spec.ID.ToString())));
        const bool Active=NavigationArea==1&&Selected==Control;
        if(auto* Row=Cast<UBorder>(GetWidgetFromName(FName(*(TEXT("ROW_")+Spec.ID.ToString())))))
            Row->SetBrushColor(Active?FLinearColor(.258f,.896f,1.f,.28f):FLinearColor(.107f,.175f,.258f,.16f));
        if(Selected==Control)
        {
            if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SelectedSetting"))))Text->SetText(Spec.Label);
            if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SelectedSettingHelp"))))Text->SetText(Spec.Help);
        }
    }
    for(int32 I=0;I<ICSettingsLayout::Tabs().Num();++I)
        ICUITheme::SetSelected(Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_SettingsTab_%d"),I)))),I==ActiveTab);
    for(int32 I=0;I<KeyBindings.Num();++I)
    {
        const bool Active=NavigationArea==1&&Selected==KeyBindings[I]->Control;
        if(auto* Row=Cast<UBorder>(GetWidgetFromName(FName(*FString::Printf(TEXT("KeyRow_%d"),I)))))
            Row->SetBrushColor(Active?FLinearColor(.258f,.896f,1.f,.28f):FLinearColor(.107f,.175f,.258f,.16f));
        if(Selected==KeyBindings[I]->Control)
        {
            if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SelectedSetting"))))Text->SetText(KeyBindings[I]->Label);
            if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SelectedSettingHelp"))))Text->SetText(NSLOCTEXT("ICSettings","KeyHelp","F로 선택한 뒤 새 키를 누르세요. ESC로 취소할 수 있습니다. 다른 조작과 중복되면 두 키를 서로 바꿉니다."));
        }
    }
    const auto Actions=SettingsFooter(this);FooterIndex=FMath::Clamp(FooterIndex,0,FMath::Max(0,Actions.Num()-1));
    for(int32 I=0;I<Actions.Num();++I)ICUITheme::SetSelected(Actions[I],NavigationArea==2&&I==FooterIndex);
}
void UICSettingsWidget::AdjustSelected(int32 Direction)
{
    const auto Options=NavigationOptions();if(!Options.IsValidIndex(OptionIndex))return;
    auto* Control=Options[OptionIndex];const FString Name=Control->GetName();
    const FName ID(*Name.RightChop(7));
    if(auto* Check=Cast<UCheckBox>(Control))ChangeOption(ID,Direction==0?!Check->IsChecked():Direction>0);
    else if(auto* Slider=Cast<USlider>(Control))ChangeOption(ID,Slider->GetValue()+(Direction==0?1:Direction)*Slider->GetStepSize());
    else if(auto* Choice=Cast<UComboBoxString>(Control);Choice&&Choice->GetOptionCount()>0)
    {
        const int32 Index=(Choice->GetSelectedIndex()+(Direction<0?-1:1)+Choice->GetOptionCount())%Choice->GetOptionCount();
        const FString Value=Choice->GetOptionAtIndex(Index);Choice->SetSelectedIndex(Index);ChangeChoice(ID,Value);
    }
    else if(auto* Selector=Cast<UInputKeySelector>(Control);Selector&&Direction==0)
    {PendingKeySelector=Selector;}
    RefreshNavigation();
}
bool UICSettingsWidget::HandleNavigationKey(FKey Raw,bool Repeat)
{
    const FKey Key=UICGameUserSettings::MenuKey(Raw);
    const bool Up=Key==EKeys::W||Key==EKeys::Up,Down=Key==EKeys::S||Key==EKeys::Down;
    const bool Left=Key==EKeys::A||Key==EKeys::Left,Right=Key==EKeys::D||Key==EKeys::Right;
    const bool Accept=Key==EKeys::Enter||Key==EKeys::SpaceBar;
    if(!Up&&!Down&&!Left&&!Right&&!Accept&&Key!=EKeys::Tab)return false;
    if(bPreview)NavigationArea=2;
    if(Key==EKeys::Tab){NavigationArea=(NavigationArea+1)%3;RefreshNavigation();return true;}
    const auto Options=NavigationOptions();
    if(NavigationArea==0)
    {
        if((Left||Right)&&!Repeat)SelectTab((ActiveTab+(Right?1:ICSettingsLayout::Tabs().Num()-1))%ICSettingsLayout::Tabs().Num());
        if(Down||Accept){NavigationArea=1;OptionIndex=0;}
    }
    else if(NavigationArea==1)
    {
        if(Up){if(OptionIndex==0)NavigationArea=0;else --OptionIndex;}
        if(Down){if(OptionIndex+1>=Options.Num())NavigationArea=2;else ++OptionIndex;}
        if(Left||Right)AdjustSelected(Right?1:-1);
        if(Accept&&!Repeat)AdjustSelected(0);
        if(Options.IsValidIndex(OptionIndex))if(auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(FName(*FString::Printf(TEXT("SettingsScroll_%d"),ActiveTab)))))Scroll->ScrollWidgetIntoView(Options[OptionIndex],true);
    }
    else
    {
        const auto Actions=SettingsFooter(this);
        if(Up&&!bPreview)NavigationArea=1;
        if(Left||Right)FooterIndex=FMath::Clamp(FooterIndex+(Right?1:-1),0,FMath::Max(0,Actions.Num()-1));
        if(Accept&&!Repeat&&Actions.IsValidIndex(FooterIndex))Actions[FooterIndex]->OnClicked.Broadcast();
    }
    RefreshNavigation();return true;
}
FReply UICSettingsWidget::NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(PendingKeySelector&&(UICGameUserSettings::MenuKey(Event.GetKey())==EKeys::Enter||Event.GetKey()==EKeys::SpaceBar))
    {
        auto* Selector=PendingKeySelector.Get();PendingKeySelector=nullptr;
        // Begin capture after releasing the confirm key, so it cannot bind F/G by accident.
        if(GetOwningPlayer())Selector->SetUserFocus(GetOwningPlayer());
        if(auto Slate=Selector->GetCachedWidget();Slate.IsValid())
        {
            Slate->OnKeyDown(Selector->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),Event.GetUserIndex(),false,0,0));
            Slate->OnKeyUp(Selector->GetCachedGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),Event.GetUserIndex(),false,0,0));
        }
        return FReply::Handled();
    }
    return Super::NativeOnKeyUp(Geometry,Event);
}

#undef LOCTEXT_NAMESPACE
