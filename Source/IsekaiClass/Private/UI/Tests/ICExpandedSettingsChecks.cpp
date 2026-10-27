#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Settings/ICGameUserSettings.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Settings/ICSettingsLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/InputKeySelector.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSettingsPersistenceCheck,"IC.Settings.ConfigAndInputMapping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSettingsPersistenceCheck::RunTest(const FString&)
{
    auto* Settings=NewObject<UICGameUserSettings>();Settings->SetToDefaults();Settings->ValidateSettings();
    Settings->WriteOption(TEXT("CameraSensitivity"),1.65);Settings->WriteOption(TEXT("MasterVolume"),.35);
    Settings->WriteOption(TEXT("Texture"),1);Settings->WriteOption(TEXT("VSync"),1);Settings->WriteOption(TEXT("ResolutionScale"),78);
    Settings->KeyOverrides.Add(TEXT("IA_Move:W"),TEXT("H"));
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SettingsTest.ini"));
    Settings->SaveConfig(CPF_Config,*File);
    auto* Restored=NewObject<UICGameUserSettings>();Restored->LoadConfig(UICGameUserSettings::StaticClass(),*File);
    TestEqual(TEXT("Mouse sensitivity persists"),Restored->CameraSensitivity,1.65f);
    TestEqual(TEXT("Volume persists"),Restored->MasterVolume,.35f);
    TestEqual(TEXT("Individual graphics quality setter"),Settings->GetTextureQuality(),1);
    TestTrue(TEXT("VSync persists"),Restored->IsVSyncEnabled());
    TestTrue(TEXT("Absolute render scale setter"),FMath::IsNearlyEqual(Settings->ReadOption(TEXT("ResolutionScale")),78.,.01));
    TestEqual(TEXT("Key overrides persist"),Restored->KeyOverrides.FindRef(TEXT("IA_Move:W")),FString(TEXT("H")));
    auto* Source=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/IsekaiClass/Input/Contexts/IMC_Gameplay.IMC_Gameplay"));
    auto* Move=LoadObject<UInputAction>(nullptr,TEXT("/Game/IsekaiClass/Input/Actions/IA_Move.IA_Move"));
    auto* Roll=LoadObject<UInputAction>(nullptr,TEXT("/Game/IsekaiClass/Input/Actions/IA_Roll.IA_Roll"));
    if(!TestNotNull(TEXT("Gameplay input context"),Source)||!TestNotNull(TEXT("Movement action"),Move))return false;
    auto* Copy=Restored->MakeInputMapping(Source,GetTransientPackage(),Move,Roll);
    TestEqual(TEXT("All original mappings are retained"),Copy->GetMappings().Num(),Source->GetMappings().Num());
    int32 Remapped=0;
    for(int32 I=0;I<Source->GetMappings().Num();++I)
    {
        const auto& Original=Source->GetMappings()[I];const auto& Mapping=Copy->GetMappings()[I];
        TestEqual(TEXT("Axis modifiers are preserved"),Mapping.Modifiers.Num(),Original.Modifiers.Num());
        TestEqual(TEXT("Conditional input triggers are preserved"),Mapping.Triggers.Num(),Original.Triggers.Num());
        if((Original.Action==Move||Original.Action==Roll)&&Original.Key==EKeys::W)
        {TestEqual(TEXT("Forward move and Control-roll follow the same user key"),Mapping.Key,EKeys::H);++Remapped;}
        TestEqual(TEXT("Authored default input asset stays unchanged"),Source->GetMappings()[I].Key,Original.Key);
    }
    TestTrue(TEXT("Movement and dodge chord both remapped"),Remapped>=2);
    Restored->CameraSensitivity=500;Restored->MasterVolume=-10;Restored->ValidateSettings();
    TestEqual(TEXT("Invalid sensitivity is clamped"),Restored->CameraSensitivity,3.f);TestEqual(TEXT("Invalid audio volume is clamped"),Restored->MasterVolume,0.f);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSettingsWidgetCheck,"IC.Settings.WBPControlsAndDraft",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSettingsWidgetCheck::RunTest(const FString&)
{
    auto* Type=LoadClass<UICSettingsWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Settings.WBP_IC_Settings_C"));
    auto* Widget=NewObject<UICSettingsWidget>(GetTransientPackage(),Type);Widget->Initialize();Widget->TakeWidget();
    auto* Switcher=Cast<UWidgetSwitcher>(Widget->GetWidgetFromName(TEXT("SettingsTabSwitcher")));
    bool OK=TestNotNull(TEXT("WBP contains the tab switcher"),Switcher);
    if(OK)
    {
        TestEqual(TEXT("Five authored tabs"),Switcher->GetNumWidgets(),5);
        for(int32 I=0;I<5;++I){Widget->SelectTab(I);TestEqual(TEXT("Each tab can be selected"),Switcher->GetActiveWidgetIndex(),I);}
        for(const auto& Spec:ICSettingsLayout::Specs())TestNotNull(*FString::Printf(TEXT("Authored setting control %s"),*Spec.ID.ToString()),Widget->GetWidgetFromName(FName(*(TEXT("OPTION_")+Spec.ID.ToString()))));
        TestTrue(TEXT("At least forty real options"),ICSettingsLayout::Specs().Num()>=40);
        auto* Keys=Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("LIST_KeyBindings")));
        TestTrue(TEXT("Keyboard and mouse bindings are populated from actual mappings"),Keys&&Keys->GetChildrenCount()>15);
        const float Original=UICGameUserSettings::Get()->CameraSensitivity;
        auto* Slider=Cast<USlider>(Widget->GetWidgetFromName(TEXT("OPTION_CameraSensitivity")));
        Slider->OnValueChanged.Broadcast(1.35f);
        TestEqual(TEXT("Editing a draft leaves the live setting unchanged"),UICGameUserSettings::Get()->CameraSensitivity,Original);
        TestTrue(TEXT("Control retains edited value"),FMath::IsNearlyEqual(Slider->GetValue(),1.35f));
        UICSettingBinding* Forward=nullptr;UICSettingBinding* Right=nullptr;
        Widget->ResetOptions();
        for(TObjectIterator<UICSettingBinding> It;It;++It)if(It->GetOuter()==Widget&&It->ID==TEXT("Key"))
        {if(It->KeyID==TEXT("IA_Move:W"))Forward=*It;if(It->KeyID==TEXT("IA_Move:D"))Right=*It;}
        if(TestNotNull(TEXT("Forward input selector"),Forward)&&TestNotNull(TEXT("Right input selector"),Right))
        {
            Forward->KeyChanged(FInputChord(EKeys::D));
            TestEqual(TEXT("Conflicting key is swapped instead of losing movement"),CastChecked<UInputKeySelector>(Right->Control)->GetSelectedKey().Key,EKeys::W);
        }
        Widget->ResetOptions();TestEqual(TEXT("Reset remains a draft until Apply"),UICGameUserSettings::Get()->CameraSensitivity,Original);
    }
    Widget->ReleaseSlateResources(true);
    return !HasAnyErrors();
}
#endif
