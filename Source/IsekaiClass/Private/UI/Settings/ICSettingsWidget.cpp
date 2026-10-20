#include "UI/Settings/ICSettingsWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "InputCoreTypes.h"

void UICSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    auto* Background=WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(.025f,.035f,.055f,.96f));
    Background->SetHorizontalAlignment(HAlign_Center);
    Background->SetVerticalAlignment(VAlign_Center);
    Background->SetPadding(FMargin(32));
    WidgetTree->RootWidget=Background;
    auto* Box=WidgetTree->ConstructWidget<USizeBox>();
    Box->SetWidthOverride(520);
    Background->SetContent(Box);
    auto* Items=WidgetTree->ConstructWidget<UVerticalBox>();
    Box->SetContent(Items);
    auto Label=[&](const FText& Text)
    {
        auto* Widget=WidgetTree->ConstructWidget<UTextBlock>();
        Widget->SetText(Text);
        Items->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0,12));
    };
    Label(NSLOCTEXT("ICSettings","Title","설정"));
    Label(NSLOCTEXT("ICSettings","Sensitivity","카메라 감도"));
    Sensitivity=WidgetTree->ConstructWidget<USlider>();
    Sensitivity->SetMinValue(.1f);
    Sensitivity->SetMaxValue(3.f);
    const auto* Player=GetOwningPlayer()?Cast<AICPlayerCharacter>(GetOwningPlayer()->GetPawn()):nullptr;
    float Value=Player?Player->CameraSensitivity:1.f;
    GConfig->GetFloat(TEXT("/Script/IsekaiClass.Input"),TEXT("CameraSensitivity"),Value,GGameUserSettingsIni);
    Sensitivity->SetValue(FMath::Clamp(Value,.1f,3.f));
    Items->AddChildToVerticalBox(Sensitivity);
    Label(NSLOCTEXT("ICSettings","Fullscreen","전체 화면"));
    Fullscreen=WidgetTree->ConstructWidget<UCheckBox>();
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    Fullscreen->SetIsChecked(Settings&&Settings->GetFullscreenMode()!=EWindowMode::Windowed);
    Items->AddChildToVerticalBox(Fullscreen);
    Label(NSLOCTEXT("ICSettings","Quality","그래픽 품질"));
    Quality=WidgetTree->ConstructWidget<UComboBoxString>();
    const TCHAR* Labels[]={TEXT("낮음"),TEXT("중간"),TEXT("높음"),TEXT("매우 높음")};
    for(const TCHAR* Text:Labels)Quality->AddOption(Text);
    Quality->SetSelectedIndex(Settings?FMath::Clamp(Settings->GetOverallScalabilityLevel(),0,3):2);
    Items->AddChildToVerticalBox(Quality);
    auto Button=[&](const FText& Text)
    {
        auto* Widget=WidgetTree->ConstructWidget<UButton>();
        auto* Caption=WidgetTree->ConstructWidget<UTextBlock>();
        Caption->SetText(Text);
        Widget->SetContent(Caption);
        Items->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0,16,0,0));
        return Widget;
    };
    Button(NSLOCTEXT("ICSettings","Apply","적용"))->OnClicked.AddDynamic(this,&ThisClass::Apply);
    Button(NSLOCTEXT("ICSettings","Close","닫기"))->OnClicked.AddDynamic(this,&ThisClass::Close);
}

void UICSettingsWidget::Apply()
{
    if(!Sensitivity||!Fullscreen||!Quality)return;
    if(auto* Player=GetOwningPlayer()?Cast<AICPlayerCharacter>(GetOwningPlayer()->GetPawn()):nullptr)Player->CameraSensitivity=Sensitivity->GetValue();
    GConfig->SetFloat(TEXT("/Script/IsekaiClass.Input"),TEXT("CameraSensitivity"),Sensitivity->GetValue(),GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
    if(auto* Settings=UGameUserSettings::GetGameUserSettings())
    {
        Settings->SetFullscreenMode(Fullscreen->IsChecked()?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
        Settings->SetOverallScalabilityLevel(Quality->GetSelectedIndex());
        Settings->ApplySettings(false);
        Settings->SaveSettings();
    }
}

void UICSettingsWidget::Close()
{
    if(auto* Controller=Cast<AICPlayerController>(GetOwningPlayer()))Controller->SetMenu(Controller->IsMainMenuLevel()?TEXT("MainMenu"):TEXT(""));
}

FReply UICSettingsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape||Event.GetKey()==EKeys::P){if(!Event.IsRepeat())Close();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
