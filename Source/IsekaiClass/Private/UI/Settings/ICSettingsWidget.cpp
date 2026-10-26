#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Framework/ICText.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
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
    if(!WidgetTree->RootWidget)ICFrontendLayout::Build(WidgetTree,TEXT("Settings"));
    Sensitivity=Cast<USlider>(GetWidgetFromName(TEXT("SLIDER_Sensitivity")));
    Fullscreen=Cast<UCheckBox>(GetWidgetFromName(TEXT("CHECK_Fullscreen")));
    Quality=Cast<UComboBoxString>(GetWidgetFromName(TEXT("COMBO_Quality")));
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Apply"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::Apply);
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Close"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::Close);
}

void UICSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    const auto* Player=GetOwningPlayer()?Cast<AICPlayerCharacter>(GetOwningPlayer()->GetPawn()):nullptr;
    float Value=Player?Player->CameraSensitivity:1.f;
    GConfig->GetFloat(TEXT("/Script/IsekaiClass.Input"),TEXT("CameraSensitivity"),Value,GGameUserSettingsIni);
    if(Sensitivity)Sensitivity->SetValue(FMath::Clamp(Value,.1f,3.f));
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    if(Fullscreen)Fullscreen->SetIsChecked(Settings&&Settings->GetFullscreenMode()!=EWindowMode::Windowed);
    RefreshLocalizedQualityOptions();
    if(Quality)Quality->SetSelectedIndex(Settings?FMath::Clamp(Settings->GetOverallScalabilityLevel(),0,3):2);
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
    const FKey Key=Event.GetKey();
    if(Key==EKeys::Escape||Key==EKeys::Equals||Key==EKeys::P){if(!Event.IsRepeat())Close();return FReply::Handled();}
    if(!Sensitivity||!Fullscreen||!Quality)return Super::NativeOnPreviewKeyDown(Geometry,Event);
    UWidget* Rows[]={Sensitivity.Get(),Fullscreen.Get(),Quality.Get(),GetWidgetFromName(TEXT("BTN_Apply")),GetWidgetFromName(TEXT("BTN_Close"))};
    int32 Selected=0;
    for(int32 Index=0;Index<UE_ARRAY_COUNT(Rows);++Index)
        if(Rows[Index]&&(Rows[Index]->HasAnyUserFocus()||Rows[Index]->HasFocusedDescendants())){Selected=Index;break;}
    if(Key==EKeys::W||Key==EKeys::S||Key==EKeys::Up||Key==EKeys::Down)
    {
        const int32 Next=FMath::Clamp(Selected+(Key==EKeys::S||Key==EKeys::Down?1:-1),0,UE_ARRAY_COUNT(Rows)-1);
        if(Rows[Next]&&GetOwningPlayer())Rows[Next]->SetUserFocus(GetOwningPlayer());
        return FReply::Handled();
    }
    const bool Left=Key==EKeys::A||Key==EKeys::Left;
    const bool Right=Key==EKeys::D||Key==EKeys::Right;
    const bool Accept=Key==EKeys::Enter||Key==EKeys::SpaceBar;
    if(Left||Right||Accept)
    {
        if(Selected==0&&(Left||Right))Sensitivity->SetValue(FMath::Clamp(Sensitivity->GetValue()+(Right?.1f:-.1f),.1f,3.f));
        else if(Selected==1&&(!Accept||!Event.IsRepeat()))Fullscreen->SetIsChecked(Accept?!Fullscreen->IsChecked():Right);
        else if(Selected==2&&(!Accept||!Event.IsRepeat()))Quality->SetSelectedIndex(FMath::Clamp(Quality->GetSelectedIndex()+(Left?-1:1),0,Quality->GetOptionCount()-1));
        else if(Accept&&!Event.IsRepeat())
        {
            if(Selected==3)Apply();
            else if(Selected==4)Close();
        }
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
