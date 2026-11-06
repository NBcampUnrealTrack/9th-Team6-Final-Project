#include "UI/Framework/ICInputPresentation.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Core/Controller/ICPlayerController.h"
#include "Settings/ICGameUserSettings.h"
#include "InputMappingContext.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Data/Text/ICLocalization.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"

namespace
{
struct FICDeviceHint
{
    TWeakObjectPtr<AICPlayerController> Controller;
    FText Keyboard, Gamepad;
};
TMap<TWeakObjectPtr<UTextBlock>, FICDeviceHint> DeviceHints;
TMap<TWeakObjectPtr<ULocalPlayer>, uint32> HintSignatures;

FKey ICKeyboardCommandKey(const AICPlayerController* Controller, EICInputCommand Command, int32 Slot)
{
    const auto* Context = Controller ? Controller->GetKeyboardInputMapping() : nullptr;
    if (Context)
        for (const auto& Binding : Controller->CommandActions)
            if (Binding.Command == Command && Binding.Slot == Slot)
                for (const auto& Mapping : Context->GetMappings())
                    if (Mapping.Action == Binding.Action && !Mapping.Key.IsGamepadKey()) return Mapping.Key;
    return EKeys::Invalid;
}
}

bool ICInputPresentation::UsesGamepad(const ULocalPlayer* Player)
{
    const int32 Mode = UICGameUserSettings::Get()->InputDeviceMode;
    if (Mode != 0) return Mode == 2;
    const auto* Input = Player ? Player->GetSubsystem<UCommonInputSubsystem>() : nullptr;
    return Input && Input->GetCurrentInputType() == ECommonInputType::Gamepad;
}

FText ICInputPresentation::KeyLabel(FKey Key)
{
    // The selected face layout is also used by actual accept/back handlers.
    for (int32 Index = 0; Index < 4; ++Index)
        if (Key == UICGameUserSettings::GamepadFaceKey(Index))
        {
            const TCHAR* Labels[] = {TEXT("X"), TEXT("Y"), TEXT("B"), TEXT("A")};
            return FText::FromString(Labels[Index]);
        }
    const TPair<FKey, const TCHAR*> Labels[] = {
        {EKeys::Gamepad_LeftShoulder, TEXT("LB")}, {EKeys::Gamepad_RightShoulder, TEXT("RB")},
        {EKeys::Gamepad_LeftTrigger, TEXT("LT")}, {EKeys::Gamepad_RightTrigger, TEXT("RT")},
        {EKeys::Gamepad_LeftThumbstick, TEXT("LS")}, {EKeys::Gamepad_RightThumbstick, TEXT("RS")},
        {EKeys::Gamepad_Special_Left, TEXT("View")}, {EKeys::Gamepad_Special_Right, TEXT("Menu")},
        {EKeys::Gamepad_DPad_Up, TEXT("↑")}, {EKeys::Gamepad_DPad_Right, TEXT("→")},
        {EKeys::Gamepad_DPad_Down, TEXT("↓")}, {EKeys::Gamepad_DPad_Left, TEXT("←")},
        {EKeys::LeftMouseButton, TEXT("LMB")}, {EKeys::RightMouseButton, TEXT("RMB")}
    };
    for (const auto& Pair : Labels) if (Key == Pair.Key) return FText::FromString(Pair.Value);
    return Key.IsValid() ? Key.GetDisplayName() : FText::FromString(TEXT("—"));
}

FText ICInputPresentation::CommandLabel(const AICPlayerController* Controller, EICInputCommand Command, int32 Slot)
{
    if (!Controller) return FText::GetEmpty();
    if (!Controller->IsUsingGamepad()) return KeyLabel(ICKeyboardCommandKey(Controller, Command, Slot));
    const auto* Context = Controller->GamepadMappingContext.Get();
    if (!Context) return FText::GetEmpty();
    if (Command == EICInputCommand::Skill || Command == EICInputCommand::CompanionSkill)
    {
        const FText Modifier = CommandLabel(Controller, EICInputCommand::Dodge);
        const FText Face = KeyLabel(UICGameUserSettings::GamepadFaceKey(Command == EICInputCommand::Skill ? 0 : Slot + 1));
        return FText::Format(NSLOCTEXT("ICInputHints", "Chord", "{0} + {1}"), Modifier, Face);
    }
    if (Command == EICInputCommand::Confirm) return KeyLabel(UICGameUserSettings::GamepadFaceKey(3));
    for (const auto& Binding : Controller->CommandActions)
        if (Binding.Command == Command && Binding.Slot == Slot)
            for (const auto& Mapping : Context->GetMappings())
                if (Mapping.Action == Binding.Action)
                {
                    FKey Key = Mapping.Key;
                    const FKey Face[] = {EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
                        EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_FaceButton_Bottom};
                    for (int32 Index = 0; Index < 4; ++Index)
                        if (Key == Face[Index]) { Key = UICGameUserSettings::GamepadFaceKey(Index); break; }
                    return KeyLabel(Key);
                }
    const TPair<EICInputCommand,FText> MenuPages[] = {
        {EICInputCommand::Party,NSLOCTEXT("ICInputHints","PartyPage","동료")},
        {EICInputCommand::Inventory,NSLOCTEXT("ICInputHints","InventoryPage","인벤토리")},
        {EICInputCommand::Quest,NSLOCTEXT("ICInputHints","QuestPage","퀘스트")},
        {EICInputCommand::Codex,NSLOCTEXT("ICInputHints","CodexPage","도감")},
        {EICInputCommand::Settings,NSLOCTEXT("ICInputHints","SettingsPage","설정")}
    };
    for(const auto& Page:MenuPages)if(Command==Page.Key)
        return FText::Format(NSLOCTEXT("ICInputHints","MenuPage","{0} → {1}"),CommandLabel(Controller,EICInputCommand::Menu),Page.Value);
    return FText::FromString(TEXT("—"));
}

FText ICInputPresentation::ForDevice(const AICPlayerController* Controller, const FText& Keyboard, const FText& Gamepad)
{
    if (!Controller || !Controller->IsUsingGamepad()) return ICLocalization::Localize(Keyboard);
    FFormatNamedArguments Arguments;
    Arguments.Add(TEXT("Accept"), KeyLabel(UICGameUserSettings::GamepadFaceKey(3)));
    Arguments.Add(TEXT("Back"), KeyLabel(UICGameUserSettings::GamepadFaceKey(2)));
    Arguments.Add(TEXT("FaceLeft"), KeyLabel(UICGameUserSettings::GamepadFaceKey(0)));
    Arguments.Add(TEXT("FaceTop"), KeyLabel(UICGameUserSettings::GamepadFaceKey(1)));
    return FText::Format(ICLocalization::Localize(Gamepad), Arguments);
}

void ICInputPresentation::SetHint(UTextBlock* Label, AICPlayerController* Controller, const FText& Keyboard, const FText& Gamepad)
{
    if (!Label || !Controller) return;
    DeviceHints.Add(Label, {Controller, Keyboard, Gamepad});
    Label->SetText(ForDevice(Controller, Keyboard, Gamepad));
}

void ICInputPresentation::RefreshHints(ULocalPlayer* Player)
{
    if (!Player) return;
    const uint32 Signature = HashCombine(HashCombine(GetTypeHash(UsesGamepad(Player)), GetTypeHash(UICGameUserSettings::Get()->GamepadLayout)),
        GetTypeHash(FInternationalization::Get().GetCurrentCulture()->GetName()));
    const auto* Previous = HintSignatures.Find(Player);
    const bool Changed = !Previous || *Previous != Signature;
    HintSignatures.Add(Player, Signature);
    for (auto It = DeviceHints.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid() || !It.Value().Controller.IsValid()) { It.RemoveCurrent(); continue; }
        if (Changed && It.Value().Controller->GetLocalPlayer() == Player)
            It.Key()->SetText(ForDevice(It.Value().Controller.Get(), It.Value().Keyboard, It.Value().Gamepad));
    }
    for (auto It = HintSignatures.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
}

void ICInputPresentation::BindTreeHints(UUserWidget* Widget)
{
    if(!Widget)return;
    auto* Controller=Cast<AICPlayerController>(Widget->GetOwningPlayer());
    if(!Controller)return;
    struct FKnownHint {FText Keyboard,Gamepad;};
    const FKnownHint Known[]={
        {NSLOCTEXT("ICCraft","Keys","W / S · ↑ / ↓ 선택     Enter 상세 보기     Esc 닫기"),
            NSLOCTEXT("ICCraft","PadKeys","방향키 / LS 선택   ·   {Accept} 상세 보기   ·   {Back} 닫기")},
        {NSLOCTEXT("ICShop","Keys","W/S 선택 · G/F 담기 · A/D 탭 · R 결제 · ESC 취소"),
            NSLOCTEXT("ICShop","PadKeys","↑ / ↓ 선택   ·   {Accept} 담기   ·   ← / → 탭   ·   {FaceLeft} 결제   ·   {Back} 취소")},
        {NSLOCTEXT("ICStorage","Keys","클릭 · Enter 정보/이동  |  WASD 이동  |  Tab 양쪽 전환  |  ESC 닫기"),
            NSLOCTEXT("ICStorage","PadKeys","마우스 클릭 가능   ·   방향키 / LS 이동   ·   {Accept} 정보 / 이동   ·   LB / RB 양쪽 전환   ·   {Back} 닫기")},
        {NSLOCTEXT("ICMap","WarpListShortcut","M · 워프 목록"),NSLOCTEXT("ICMap","PadWarpShortcut","View · 워프 목록")}
    };
    TFunction<void(UWidget*)> Visit=[&](UWidget* Node)
    {
        if(!Node)return;
        if(auto* Text=Cast<UTextBlock>(Node))
        {
            if(DeviceHints.Contains(Text))return;
            for(const auto& Hint:Known)
            {
                const auto Key=FTextInspector::GetKey(Text->GetText());const auto Namespace=FTextInspector::GetNamespace(Text->GetText());
                const bool SameIdentity=Key.IsSet()&&Namespace.IsSet()&&Key==FTextInspector::GetKey(Hint.Keyboard)&&Namespace==FTextInspector::GetNamespace(Hint.Keyboard);
                if(SameIdentity||Text->GetText().ToString()==Hint.Keyboard.ToString())
                {SetHint(Text,Controller,Hint.Keyboard,Hint.Gamepad);break;}
            }
        }
        else if(auto* User=Cast<UUserWidget>(Node)){if(User->WidgetTree)Visit(User->WidgetTree->RootWidget);}
        else if(auto* Panel=Cast<UPanelWidget>(Node))for(auto* Child:Panel->GetAllChildren())Visit(Child);
    };
    Visit(Widget);
}
