"""Author reserved movement input, dash settings and its modular cooldown WBP."""
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
INPUT='/Game/IsekaiClass/Input/Actions'
TOOLS=u.AssetToolsHelpers.get_asset_tools()

def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False),asset.get_path_name()

def action(name,value_type):
    path=INPUT+'/'+name
    result=u.load_asset(path)
    if not result:
        factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.InputAction)
        result=TOOLS.create_asset(name,INPUT,u.InputAction,factory)
    assert result,path
    result.set_editor_property('value_type',value_type)
    return result

dash=action('IA_Dodge',u.InputActionValueType.BOOLEAN)
jump=action('IA_Jump',u.InputActionValueType.BOOLEAN)
roll=action('IA_Roll',u.InputActionValueType.AXIS2D)
follow=action('IA_QuestFollow',u.InputActionValueType.BOOLEAN)
roll.set_editor_property('consume_input',False)
save(roll);save(follow)

context=u.load_asset('/Game/IsekaiClass/Input/Contexts/IMC_Gameplay')
data=context.get_editor_property('default_key_mappings')
before=list(data.get_editor_property('mappings'))
updated=[m for m in before if m.action not in (dash,jump,roll,follow)]
def key(name):
    result=u.Key();result.set_editor_property('key_name',name);return result
for a,keys in ((dash,('LeftShift','RightShift')),(jump,('SpaceBar',)),(follow,('G',))):
    for name in keys:updated.append(u.EnhancedActionKeyMapping(action=a,key=key(name)))

# Copy the camera-relative WASD axis transforms; the Control trigger adds no movement behavior.
move=u.load_asset(INPUT+'/IA_Move')
def copy_axis_modifier(source):
    if isinstance(source,u.InputModifierSwizzleAxis):
        clone=u.new_object(u.InputModifierSwizzleAxis,outer=context)
        clone.set_editor_property('order',source.get_editor_property('order'))
    elif isinstance(source,u.InputModifierNegate):
        clone=u.new_object(u.InputModifierNegate,outer=context)
        for axis in ('x','y','z'):clone.set_editor_property(axis,source.get_editor_property(axis))
    else:raise RuntimeError('Unexpected WASD modifier '+source.get_class().get_name())
    return clone
for source in before:
    if source.action!=move or str(source.key.get_editor_property('key_name')) not in ('W','A','S','D'):continue
    gate=u.new_object(u.ICInputTriggerControl,outer=context)
    updated.append(u.EnhancedActionKeyMapping(action=roll,key=source.key,
        modifiers=[copy_axis_modifier(m) for m in source.modifiers],triggers=[gate]))
data.set_editor_property('mappings',updated)
context.set_editor_property('default_key_mappings',data);save(context)

bp=u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
default=u.get_default_object(bp.generated_class())
bindings=[b for b in default.get_editor_property('command_actions') if b.action not in (roll,follow)]
bindings.extend((u.ICInputCommandBinding(action=roll,command=u.ICInputCommand.ROLL,slot=0),
    u.ICInputCommandBinding(action=follow,command=u.ICInputCommand.QUEST_FOLLOW,slot=0)))
default.set_editor_property('command_actions',bindings)
u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)

settings=u.load_asset('/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CombatSettings')
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_file(settings,str(ROOT/'Data/Settings/DT_IC_CombatSettings.csv'))
save(settings)

folder='/Game/IsekaiClass/UI/Widget'
name='WBP_IC_MovementCooldown';path=folder+'/'+name
widget=u.load_asset(path)
if not widget:
    factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',u.ICMovementCooldownWidget)
    widget=TOOLS.create_asset(name,folder,u.WidgetBlueprint,factory)
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(widget,'MovementCooldown')
u.BlueprintEditorLibrary.compile_blueprint(widget);save(widget)
hud=u.load_asset(folder+'/WBP_GameHUD')
assert u.ICBlueprintMaintenanceLibrary.add_movement_cooldown_hud(hud)
u.BlueprintEditorLibrary.compile_blueprint(hud);save(hud)
assert 'HUD_MovementCooldown' in {str(x) for x in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(hud)}
out=ROOT/'Saved/MovementInput';out.mkdir(parents=True,exist_ok=True)
report=[{'action':m.action.get_name(),'key':str(m.key.get_editor_property('key_name')),
    'triggers':[t.get_class().get_name() for t in m.triggers]} for m in updated]
(out/'Mappings.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_MOVEMENT_INPUT_SETUP_OK '+path)
