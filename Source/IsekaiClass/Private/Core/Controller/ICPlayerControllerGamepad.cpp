#include "Core/Controller/ICPlayerController.h"
#include "Settings/ICGameUserSettings.h"
#include "InputKeyEventArgs.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "InputMappingContext.h"

namespace
{
bool FindGamepadCommand(AICPlayerController* PC,FKey Key,FICInputCommandBinding& Out)
{
    // The IMC stores Xbox positions; translate the selected Nintendo layout by button position.
    const FKey FaceKeys[]={EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_FaceButton_Top,
        EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_FaceButton_Bottom};
    for(int32 I=0;I<4;++I)
        if(Key==UICGameUserSettings::GamepadFaceKey(I)){Key=FaceKeys[I];break;}
    if(Key==EKeys::Gamepad_LeftTriggerAxis)Key=EKeys::Gamepad_LeftTrigger;
    if(Key==EKeys::Gamepad_RightTriggerAxis)Key=EKeys::Gamepad_RightTrigger;
    const auto* Context=PC->GamepadMappingContext.Get();
    if(!Context)return false;
    for(const auto& Mapping:Context->GetMappings())
        if(Mapping.Key==Key)
            for(const auto& Binding:PC->CommandActions)
                if(Binding.Action&&Binding.Action==Mapping.Action){Out=Binding;return true;}
    return false;
}
}

bool AICPlayerController::IsUsingGamepad() const
{const int32 Mode=UICGameUserSettings::Get()->InputDeviceMode;return Mode==2||(Mode==0&&bLastInputGamepad);}
bool AICPlayerController::RouteGameplayGamepadKey(FKey Key,bool Pressed)
{
    if(!Key.IsGamepadKey()||!Menu.IsEmpty()||!CanHandleGameplayInput())return false;
    bLastInputGamepad=true;return RouteGamepadButton(Key,Pressed);
}
bool AICPlayerController::InputKey(const FInputKeyEventArgs& Args)
{
    if(Args.Key.IsGamepadKey())
    {
        if(Args.Event!=IE_Axis||FMath::Abs(Args.AmountDepressed)>.15f||Args.AmountDepressed2D.SizeSquared()>.0225f)bLastInputGamepad=true;
        if(Args.Event==IE_Pressed||Args.Event==IE_Released||Args.Event==IE_Axis)
        {
            const bool Pressed=Args.Event==IE_Axis?Args.AmountDepressed>.5f:Args.Event==IE_Pressed;
            if(RouteGamepadButton(Args.Key,Pressed))return true;
        }
    }
    else if(Args.Event==IE_Pressed||(Args.Event==IE_Axis&&(FMath::Abs(Args.AmountDepressed)>.001f||!Args.AmountDepressed2D.IsNearlyZero())))bLastInputGamepad=false;
    return Super::InputKey(Args);
}
bool AICPlayerController::RouteGamepadButton(FKey Key,bool Pressed)
{
    auto* PlayerPawn=Cast<AICPlayerCharacter>(GetPawn());
    FICInputCommandBinding Binding;
    const bool Mapped=FindGamepadCommand(this,Key,Binding);
    const bool Modifier=Mapped&&Binding.Command==EICInputCommand::Dodge;
    if(Modifier)
    {
        if(Pressed==bGamepadModifier)return true;
        if(Pressed)
        {
            bGamepadModifier=true;bGamepadChordUsed=false;
            if(PlayerPawn&&CanHandleGameplayInput()&&!HasTargetSelection()&&PlayerPawn->GetICAbilitySystem()&&!PlayerPawn->GetICAbilitySystem()->HasRecentCombat())PlayerPawn->bSprintHeld=true;
        }
        else
        {
            const bool Dodge=bGamepadModifier&&!bGamepadChordUsed&&PlayerPawn&&PlayerPawn->GetICAbilitySystem()&&PlayerPawn->GetICAbilitySystem()->HasRecentCombat();
            bGamepadModifier=false;if(PlayerPawn)PlayerPawn->bSprintHeld=false;
            if(Dodge&&CanHandleGameplayInput())HandleInputCommand(EICInputCommand::Dodge,0);
            bGamepadChordUsed=false;
        }
        return true;
    }
    const bool Attack=Mapped&&Binding.Command==EICInputCommand::Attack;
    if(Attack)
    {
        if(Pressed&&!bGamepadAttackHeld&&CanHandleGameplayInput()&&!HasTargetSelection())HandleInputCommand(EICInputCommand::Attack,0);
        bGamepadAttackHeld=Pressed;return true;
    }
    if(!Pressed){if(Mapped)ReleaseInputCommand(Binding.Command,Binding.Slot);return Mapped;}
    const auto* Settings=UICGameUserSettings::Get();int32 Face=INDEX_NONE;
    for(int32 I=0;I<4;++I)if(Key==Settings->GamepadFaceKey(I)){Face=I;break;}
    if(HasTargetSelection())
    {
        if(Face==3){HandleInputCommand(EICInputCommand::Confirm,0);return true;}
        if(Face==2){HandleInputCommand(EICInputCommand::Menu,0);return true;}
    }
    if(Face!=INDEX_NONE&&bGamepadModifier&&CanHandleGameplayInput())
    {
        bGamepadChordUsed=true;if(PlayerPawn)PlayerPawn->bSprintHeld=false;
        HandleInputCommand(Face==0?EICInputCommand::Skill:EICInputCommand::CompanionSkill,Face==0?0:Face-1);return true;
    }
    if(Mapped&&Binding.Command==EICInputCommand::Menu){HandleInputCommand(Binding.Command,Binding.Slot);return true;}
    if(!CanHandleGameplayInput())return false;
    // dev has no separate ClearTarget Input Action; preserve this contextual shortcut.
    if(Key==EKeys::Gamepad_RightThumbstick){HandleInputCommand(EICInputCommand::ClearTarget,0);return true;}
    if(Mapped){HandleInputCommand(Binding.Command,Binding.Slot);return true;}
    return false;
}

