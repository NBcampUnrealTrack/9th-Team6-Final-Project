"""Create the editable gamepad IMC using the controller's existing Input Actions."""
import unreal as u

FOLDER = '/Game/IsekaiClass/Input/Contexts'
PATH = FOLDER + '/IMC_Gamepad'
controller = u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
defaults = u.get_default_object(controller.generated_class())
context = u.load_asset(PATH)
if not context:
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', u.InputMappingContext)
    context = u.AssetToolsHelpers.get_asset_tools().create_asset(
        'IMC_Gamepad', FOLDER, u.InputMappingContext, factory)
if not context:
    raise RuntimeError('Could not create IMC_Gamepad')

mappings = []
def key(name):
    result = u.Key()
    result.set_editor_property('key_name', name)
    return result

def add(action, name, modifiers=None):
    if not action:
        raise RuntimeError('Missing Input Action for ' + name)
    mappings.append(u.EnhancedActionKeyMapping(action=action, key=key(name), modifiers=modifiers or []))

for property_name, key_name, camera in (
    ('move_action', 'Gamepad_Left2D', False),
    ('look_action', 'Gamepad_Right2D', True),
):
    deadzone = u.new_object(u.InputModifierDeadZone, outer=context)
    deadzone.set_editor_property('lower_threshold', 0.2)
    deadzone.set_editor_property('upper_threshold', 1.0)
    deadzone.set_editor_property('type', u.DeadZoneType.RADIAL)
    modifiers = [deadzone]
    # The latest controller applies camera delta time and sensitivity in Look().
    add(defaults.get_editor_property(property_name), key_name, modifiers)

buttons = {
    u.ICInputCommand.JUMP: 'Gamepad_FaceButton_Bottom',
    u.ICInputCommand.DODGE: 'Gamepad_LeftTrigger',
    u.ICInputCommand.INTERACT: 'Gamepad_FaceButton_Left',
    u.ICInputCommand.FISHING: 'Gamepad_FaceButton_Top',
    u.ICInputCommand.SWITCH_WEAPON: 'Gamepad_RightShoulder',
    u.ICInputCommand.ATTACK: 'Gamepad_RightTrigger',
    u.ICInputCommand.NEXT_TARGET: 'Gamepad_LeftShoulder',
    u.ICInputCommand.MENU: 'Gamepad_Special_Right',
    u.ICInputCommand.MAP: 'Gamepad_Special_Left',
}
directions = ('Gamepad_DPad_Up', 'Gamepad_DPad_Right', 'Gamepad_DPad_Down', 'Gamepad_DPad_Left')
for binding in defaults.get_editor_property('command_actions'):
    if binding.command == u.ICInputCommand.SKILL and binding.slot != 0:
        continue
    name = buttons.get(binding.command)
    if binding.command == u.ICInputCommand.QUICK_SLOT and 0 <= binding.slot < 4:
        name = directions[binding.slot]
    if name:
        add(binding.action, name)
        if binding.command == u.ICInputCommand.MENU:
            add(binding.action, 'Gamepad_FaceButton_Right')

data = context.get_editor_property('default_key_mappings')
data.set_editor_property('mappings', mappings)
context.set_editor_property('default_key_mappings', data)
if not u.EditorAssetLibrary.save_loaded_asset(context, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + PATH)
u.log('IC_GAMEPAD_ASSET_SAVED mappings=' + str(len(mappings)))
