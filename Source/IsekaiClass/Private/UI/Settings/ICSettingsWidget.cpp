#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Framework/ICText.h"
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
    Label(ICText::Get(TEXT("Native_c14a567ea996")));
    Label(ICText::Get(TEXT("Native_2201a1d33fe0")));
    Sensitivity=WidgetTree->ConstructWidget<USlider>();
    Sensitivity->SetMinValue(.1f);
    Sensitivity->SetMaxValue(3.f);
    const auto* Player=GetOwningPlayer()?Cast<AICPlayerCharacter>(GetOwningPlayer()->GetPawn()):nullptr;
    float Value=Player?Player->CameraSensitivity:1.f;
    GConfig->GetFloat(TEXT("/Script/IsekaiClass.Input"),TEXT("CameraSensitivity"),Value,GGameUserSettingsIni);
    Sensitivity->SetValue(FMath::Clamp(Value,.1f,3.f));
    Items->AddChildToVerticalBox(Sensitivity);
    Label(ICText::Get(TEXT("Native_7c68cb9a9188")));
    Fullscreen=WidgetTree->ConstructWidget<UCheckBox>();
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    Fullscreen->SetIsChecked(Settings&&Settings->GetFullscreenMode()!=EWindowMode::Windowed);
    Items->AddChildToVerticalBox(Fullscreen);
    Label(ICText::Get(TEXT("Native_1e65f19be778")));
    Quality=WidgetTree->ConstructWidget<UComboBoxString>();
    const FText Labels[]={ICText::Get(TEXT("Native_437ac8ceeb09")),ICText::Get(TEXT("Native_c7e54fe95951")),ICText::Get(TEXT("Native_962636f483b3")),ICText::Get(TEXT("Native_791384293309"))};
    for(const FText& Text:Labels)Quality->AddOption(Text.ToString());
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
    Button(ICText::Get(TEXT("Native_e01dd02f527f")))->OnClicked.AddDynamic(this,&ThisClass::Apply);
    Button(ICText::Get(TEXT("Native_94b7dba15907")))->OnClicked.AddDynamic(this,&ThisClass::Close);
}

void UICSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshLocalizedQualityOptions();
}

void UICSettingsWidget::RefreshLocalizedQualityOptions()
{
    if (!Quality) return;
    const int32 Selected=Quality->GetSelectedIndex();
    Quality->ClearOptions();
    for (const TCHAR* Key : {TEXT("Native_437ac8ceeb09"),TEXT("Native_c7e54fe95951"),
                             TEXT("Native_962636f483b3"),TEXT("Native_791384293309")})
        Quality->AddOption(ICText::Get(Key).ToString());
    Quality->SetSelectedIndex(FMath::Clamp(Selected,0,3));
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
    if(auto* Controller=Cast<AICPlayerController>(GetOwningPlayer()))Controller->CloseSessionMenu();
}

FReply UICSettingsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape||Event.GetKey()==EKeys::P){if(!Event.IsRepeat())Close();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
