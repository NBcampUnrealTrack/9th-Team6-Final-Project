"""Read current input mappings and controller bindings before making targeted edits."""
import json
from pathlib import Path
import unreal as u

context=u.load_asset('/Game/IsekaiClass/Input/Contexts/IMC_Gameplay')
mappings=context.get_editor_property('default_key_mappings').get_editor_property('mappings')
bp=u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
default=u.get_default_object(bp.generated_class())
result={'mappings':[], 'bindings':[]}
for m in mappings:
    result['mappings'].append({'action':m.action.get_name(),'key':str(m.key.get_editor_property('key_name')),
        'modifiers':[str(x.get_class().get_name()) for x in m.modifiers],
        'triggers':[str(x.get_class().get_name()) for x in m.triggers]})
for b in default.get_editor_property('command_actions'):
    result['bindings'].append({'action':b.action.get_name(),'command':str(b.command),'slot':b.slot})
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'MovementInput'
out.mkdir(parents=True,exist_ok=True)
(out/'Before.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_MOVEMENT_INPUT_INSPECTED '+json.dumps(result))
