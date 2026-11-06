#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/CheckBox.h"
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
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "UI/Framework/ICInputPresentation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICRemappedHintCheck,"IC.Settings.RemappedCommandHints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICRemappedHintCheck::RunTest(const FString&)
{
    auto* Settings = UICGameUserSettings::Get();
    TGuardValue<int32> DeviceGuard(Settings->InputDeviceMode, 1);
    TGuardValue<TMap<FString, FString>> BindingGuard(Settings->KeyOverrides, {});
    auto* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    auto* World = GI->GetWorld();
    auto& WorldContext = *GEngine->GetWorldContextFromWorld(World);
    auto* Viewport = NewObject<UGameViewportClient>(GEngine);
    WorldContext.GameViewport = Viewport;
    Viewport->Init(WorldContext, GI, false);
    World->InitializeActorsForPlay(FURL(), false);
    auto* Controller = World->SpawnActor<AICPlayerController>();
    auto* Context = NewObject<UInputMappingContext>(Controller);
    auto* Action = NewObject<UInputAction>(Controller, TEXT("IA_TestInteract"));
    Context->MapKey(Action, EKeys::F);
    Controller->DefaultMappingContext = Context;
    FICInputCommandBinding Binding;
    Binding.Action = Action;
    Binding.Command = EICInputCommand::Interact;
    Controller->CommandActions.Add(Binding);
    auto Label = [&]() { return ICInputPresentation::CommandLabel(Controller, EICInputCommand::Interact).ToString(); };
    TestEqual(TEXT("Default key is available before mapping initialization"), Label(), ICInputPresentation::KeyLabel(EKeys::F).ToString());
    FString Error;
    Controller->SetPlayer(GI->CreateLocalPlayer(0, Error, false));
    const FString BindingID = Action->GetName() + TEXT(":F");
    Settings->KeyOverrides.Add(BindingID, TEXT("G"));
    Controller->RefreshUserInputMapping();
    TestEqual(TEXT("Active input uses the changed key"), Controller->GetKeyboardInputMapping()->GetMappings()[0].Key, EKeys::G);
    TestEqual(TEXT("Interaction and fishing hint follows changed key"), Label(), ICInputPresentation::KeyLabel(EKeys::G).ToString());
    TestEqual(TEXT("Default asset is not modified"), Context->GetMappings()[0].Key, EKeys::F);
    Settings->KeyOverrides.Add(BindingID, TEXT("Escape"));
    Controller->RefreshUserInputMapping();
    TestEqual(TEXT("Rejected override displays the key actually applied"), Label(), ICInputPresentation::KeyLabel(EKeys::F).ToString());
    Settings->KeyOverrides.Add(BindingID, TEXT("G"));
    Controller->RefreshUserInputMapping();
    Settings->KeyOverrides.Reset();
    Controller->RefreshUserInputMapping();
    TestEqual(TEXT("Restoring default keys restores the hint"), Label(), ICInputPresentation::KeyLabel(EKeys::F).ToString());
    GI->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}

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
        Widget->SelectTab(0);
        auto* AutoTarget=CastChecked<UCheckBox>(Widget->GetWidgetFromName(TEXT("OPTION_bAutoSelectTarget")));
        const bool Was=AutoTarget->IsChecked();
        TestTrue(TEXT("F confirms a setting without mouse focus"),Widget->HandleNavigationKey(EKeys::F));
        TestEqual(TEXT("F toggles the selected first setting"),AutoTarget->IsChecked(),!Was);
        Widget->HandleNavigationKey(EKeys::F,true);
        TestEqual(TEXT("Holding F does not toggle again"),AutoTarget->IsChecked(),!Was);
        Widget->SelectTab(1);Widget->HandleNavigationKey(EKeys::W);Widget->HandleNavigationKey(EKeys::D);
        TestEqual(TEXT("W then D moves to the next settings category"),Switcher->GetActiveWidgetIndex(),2);
        Widget->HandleNavigationKey(EKeys::S);
        TestTrue(TEXT("A/D edit the selected setting"),Widget->HandleNavigationKey(EKeys::D));

    }
    Widget->ReleaseSlateResources(true);
    return !HasAnyErrors();
}
#endif
