"""Persist the lock-on Pressed trigger as an owned subobject and verify it."""
import unreal as u

PATH = '/Game/IsekaiClass/Input/Actions/IA_TargetNext'
action = u.load_asset(PATH)
assert action
triggers = action.get_editor_property('triggers')
if len(triggers) != 1 or not triggers[0] or triggers[0].get_outer() != action:
    action.set_editor_property('triggers', [u.InputTriggerPressed(outer=action)])
    assert u.EditorAssetLibrary.save_loaded_asset(action, False)
trigger = action.get_editor_property('triggers')[0]
assert isinstance(trigger, u.InputTriggerPressed) and trigger.get_outer() == action
u.log('NPC_SKILL_INPUT_OK ' + trigger.get_path_name())
