"""Run in an unattended editor. Uses isolated duplicates, never live NPC rows.

The test leaves temporary assets for inspection. Remove only the reported
temporary_paths after the editor exits. Results remain under Saved/NPCImport.
"""
import hashlib
import json
import shutil
import sys
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT/'Tools/Characters'))
import npc_import as tool

TEMP = '/Game/Developers/NPCImportAutomation'
IDENTIFIER = 'NPC_AUTOTEST_001'
checks = []
RESUME = '-ICNPCImportResume' in u.SystemLibrary.get_command_line()


def check(value, message):
    checks.append({'pass': bool(value), 'message': message})
    if not value: raise RuntimeError(message)


def expect_error(call, message):
    try: call()
    except RuntimeError:
        check(True, message); return
    check(False, message)


def duplicate(source, destination):
    if RESUME and destination.startswith(TEMP+'/') and u.EditorAssetLibrary.does_asset_exist(destination):
        return u.load_asset(destination)
    check(not u.EditorAssetLibrary.does_asset_exist(destination), 'Isolated destination is unused: '+destination)
    result = u.EditorAssetLibrary.duplicate_asset(source, destination)
    check(result is not None, 'Duplicate: '+destination)
    return result


def sources():
    paths = list((ROOT/'Data').rglob('*.csv'))
    paths += list((ROOT/'Content/IsekaiClass/Animation/Common/Retarget').glob('IK_SRC_*.uasset'))
    for folder in ('NPC/SampleNPC', 'Player/SamplePlayer'):
        paths += list((ROOT/'Content/IsekaiClass'/folder).rglob('ABP_Post_*.uasset'))
        paths += list((ROOT/'Content/IsekaiClass'/folder).rglob('VM_*_VrmMeta.uasset'))
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}


def run():
    before = sources()
    source_folder = '/Game/IsekaiClass/NPC/SampleNPC/R01/NPC_R01_001'
    source_mesh = u.load_asset(source_folder+'/SK_R01_1')
    source_list, _, _ = tool.resolve_vrm(source_mesh)
    original = ROOT/'Saved/KawaiiPhysicsReview/OriginalAssets/IsekaiClass/NPC/SampleNPC/R01/NPC_R01_001/ABP_Post_R01_1.uasset'
    destination = ROOT/'Content/Developers/NPCImportAutomation/Original/ABP_Post_R01_1.uasset'
    check(RESUME or not destination.exists(), 'Original graph fixture destination is isolated')
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(original, destination)
    u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([TEMP], True)
    mesh = duplicate(source_mesh.get_path_name(), TEMP+'/Input/SK_Automation')
    meta = duplicate(source_folder+'/VM_R01_1_VrmMeta', TEMP+'/Input/VM_Automation')
    meta.set_editor_property('skeletal_mesh', mesh)
    asset_list = duplicate(source_list.get_path_name(), TEMP+'/Input/VA_Automation')
    asset_list.set_editor_property('skeletal_mesh', mesh); asset_list.set_editor_property('vrm_meta_object', meta)
    bp = duplicate(TEMP+'/Original/ABP_Post_R01_1', TEMP+'/Input/ABP_Post_Automation')
    mesh.set_editor_property('post_process_anim_blueprint', bp.generated_class())
    rig = duplicate(source_folder+'/IK_R01_1_Mannequin', TEMP+'/Input/IK_Automation_Mannequin')
    check(u.IKRigController.get_controller(rig).set_skeletal_mesh(mesh), 'Target IK rig accepts duplicate mesh')
    for item in (mesh, meta, asset_list, bp, rig): tool._save(item)
    config = {'vrm': asset_list.get_path_name(), 'id': IDENTIFIER, 'name': '자동 생성 검사', 'role': 'Mage',
              'classmate': False, 'register_world': False, 'region': 'R01'}
    check(tool.inspect_vrm(config['vrm'])['mesh'] == mesh.get_path_name(), 'MCP read-only inspection resolves the imported original mesh')
    check(tool.plan(config)['id'] == IDENTIFIER, 'MCP plan validates the proposed NPC without creating assets')
    plan = u.ICCharacterSetupLibrary.configure_imported_npc_physics(mesh, meta, False)
    check(plan.startswith('PLAN:'), 'Fresh VRM graph can be converted: '+plan)
    report = tool.build(config)
    check(report['complete'] and not report['world_registered'], 'Assets complete without world registration')
    check(all(report['checks'].values()), 'Skeleton, grip and Kawaii checks pass')
    check('existing' not in report['physics'], 'Fresh VRM SpringBone was replaced')
    check(tool.build(config) == report, 'Completed repeated generation is read-only')
    check(sources() == before, 'Existing CSVs, source IK rigs and all live character physics packages are unchanged')
    expect_error(lambda: tool.build(dict(config, role='Archer')), 'Reject changing an already-generated identity')
    expect_error(lambda: tool.build(dict(config, id='NPC_R01_001')), 'Reject existing live NPC ID')
    expect_error(lambda: tool.build(dict(config, id='NPC_AUTOTEST_002', register_world=True)), 'Reject missing world references before mutation')
    expect_error(lambda: tool.build(dict(config, id='../escape')), 'Reject paths in NPC IDs')
    check(sources() == before, 'Rejected requests preserve all definitions')
    check(u.ICCharacterSetupLibrary.configure_imported_npc_physics(mesh, meta, True).startswith('OK: 5 existing'),
          'Repeated physics setup preserves existing five Kawaii nodes')
    return report


result = {'checks': checks, 'temporary_paths': [TEMP, '/Game/IsekaiClass/Animation/NPC/'+IDENTIFIER,
          '/Game/IsekaiClass/NPC/Blueprints/Generated/BP_'+IDENTIFIER]}
try:
    result['generation'] = run()
    result['success'] = True
except Exception:
    import traceback
    result['success'] = False; result['error'] = traceback.format_exc()
    u.log_error(result['error'])
finally:
    path = ROOT/'Saved/NPCImport/IntegrationTest.json'; path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    u.log('IC_NPC_IMPORT_TEST '+json.dumps({'success': result['success'], 'checks': len(checks)}))
