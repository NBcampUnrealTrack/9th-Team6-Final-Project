#include "Core/Controller/ICPlayerController.h"
#include "Settings/ICGameUserSettings.h"
#include "InputKeyEventArgs.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"

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
    const bool Modifier=Key==EKeys::Gamepad_LeftTrigger||Key==EKeys::Gamepad_LeftTriggerAxis;
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
    const bool Attack=Key==EKeys::Gamepad_RightTrigger||Key==EKeys::Gamepad_RightTriggerAxis;
    if(Attack)
    {
        if(Pressed&&!bGamepadAttackHeld&&CanHandleGameplayInput()&&!HasTargetSelection())HandleInputCommand(EICInputCommand::Attack,0);
        bGamepadAttackHeld=Pressed;return true;
    }
    if(!Pressed){if(Key==UICGameUserSettings::GamepadFaceKey(3)&&PlayerPawn)PlayerPawn->StopJumping();return false;}
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
    if(Face==2){HandleInputCommand(EICInputCommand::Menu,0);return true;}
    if(!CanHandleGameplayInput())return false;
    if(Face==0){HandleInputCommand(EICInputCommand::Interact,0);return true;}
    if(Face==1){HandleInputCommand(EICInputCommand::Fishing,0);return true;}
    if(Face==3){HandleInputCommand(EICInputCommand::Jump,0);return true;}
    if(Key==EKeys::Gamepad_LeftShoulder){HandleInputCommand(EICInputCommand::NextTarget,0);return true;}
    if(Key==EKeys::Gamepad_RightShoulder){HandleInputCommand(EICInputCommand::SwitchWeapon,0);return true;}
    if(Key==EKeys::Gamepad_RightThumbstick){HandleInputCommand(EICInputCommand::ClearTarget,0);return true;}
    if(Key==EKeys::Gamepad_Special_Right){HandleInputCommand(EICInputCommand::Menu,0);return true;}
    if(Key==EKeys::Gamepad_Special_Left){HandleInputCommand(EICInputCommand::Map,0);return true;}
    const FKey Direction[]={EKeys::Gamepad_DPad_Up,EKeys::Gamepad_DPad_Right,EKeys::Gamepad_DPad_Down,EKeys::Gamepad_DPad_Left};
    for(int32 I=0;I<4;++I)if(Key==Direction[I]){HandleInputCommand(EICInputCommand::QuickSlot,I);return true;}
    return false;
}

