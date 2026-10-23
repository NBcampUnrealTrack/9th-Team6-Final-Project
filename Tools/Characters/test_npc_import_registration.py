"""Integration-test registration/rollback against copied CSVs and copied tables."""
import hashlib
import json
import shutil
import sys
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(PROJECT/'Tools/Characters'))
import npc_import as tool

SCRATCH = PROJECT/'Saved/NPCImport/RegistrationSandbox'
GAME = '/Game/Developers/NPCImportAutomation/WorldRegistration'
IDENTIFIER = 'NPC_AUTOTEST_001'
checks = []


def check(value, message):
    checks.append({'pass': bool(value), 'message': message})
    if not value: raise RuntimeError(message)


def snapshot(folder):
    return {str(path.relative_to(folder)): hashlib.sha256(path.read_bytes()).hexdigest() for path in folder.rglob('*.csv')}


def run():
    live_before = snapshot(PROJECT/'Data')
    check(not SCRATCH.exists(), 'Registration sandbox is unused')
    shutil.copytree(PROJECT/'Data', SCRATCH/'Data')
    report = json.loads((PROJECT/'Saved/NPCImport'/IDENTIFIER/'report.json').read_text(encoding='utf-8'))
    tool.ROOT = SCRATCH; tool.SAVED = SCRATCH/'Saved/NPCImport'; tool.GAME = GAME
    tool._write_json(tool.SAVED/IDENTIFIER/'report.json', report)
    config = dict(report['config'], register_world=True, anchor='ANC_RECRUIT_HOME', home='HOME_AUTOTEST_001',
                  dialogue='DLG_AUTOTEST_001', schedule='SCH_RECRUIT_001', unique_skill='SKL_AUTOTEST_001',
                  portrait='/Game/IsekaiClass/NPC/SampleNPC/R01/R01_portrait/R01_1.R01_1')
    # These stand in for the world/story references authored before registration.
    for table, original_id, new_id in [('World/DT_IC_Home', 'HOME_RECRUIT_001', config['home']),
                                      ('NPC/DT_IC_Dialogue', 'DLG_RECRUIT_001', config['dialogue']),
                                      ('Combat/DT_IC_Skill', 'SKL_NPC_R01_001', config['unique_skill'])]:
        fields, rows = tool._csv(table)
        row = next(row for row in rows if row['ID'] == original_id).copy()
        row = {key: value.replace('NPC_R01_001', IDENTIFIER) for key, value in row.items()}
        row.update({'---': new_id, 'ID': new_id, 'NameKey': new_id+'.NAME'})
        rows.append(row); (SCRATCH/'Data'/(table+'.csv')).write_text(tool._csv_text(fields, rows), encoding='utf-8')
    for name in list(tool.TABLES.values()) + ['StringTables/ST_IC_Definitions']:
        destination = GAME+'/Data/DataTable/'+name
        check(not u.EditorAssetLibrary.does_asset_exist(destination), 'Unused copied table: '+name)
        table = u.EditorAssetLibrary.duplicate_asset('/Game/IsekaiClass/Data/DataTable/'+name, destination)
        check(table is not None, 'Copied table: '+name)
        tool._save(table)
    completed = tool.build(config)
    check(completed['world_registered'], 'Existing generated appearance can be registered later')
    fields, rows = tool._csv(tool.TABLES['npc'])
    check(sum(row['ID'] == IDENTIFIER for row in rows) == 1, 'Append exactly one NPC source row')
    table = u.load_asset(GAME+'/Data/DataTable/'+tool.TABLES['npc'])
    loaded = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    check(sum(row['ID'] == IDENTIFIER for row in loaded) == 1, 'NPC DataTable has exactly one new row')
    check(tool.build(config) == completed, 'Repeated world registration is read-only')
    check(snapshot(PROJECT/'Data') == live_before, 'Live project CSVs are unchanged after successful sandbox registration')
    before_failure = snapshot(SCRATCH/'Data')
    new_config = dict(config, id='NPC_AUTOTEST_003', name='실패 복구 검사')
    mesh = u.load_asset(report['mesh'])
    draft = tool.registration_draft(new_config, mesh, report)
    original_save = tool._save
    calls = [0]
    def fail_once(asset):
        calls[0] += 1
        if calls[0] == 2: raise RuntimeError('Injected DataTable save failure')
        return original_save(asset)
    tool._save = fail_once
    failed = False
    try: tool._register_world(new_config, draft, {})
    except RuntimeError: failed = True
    finally: tool._save = original_save
    check(failed, 'A mid-registration failure is propagated')
    check(snapshot(SCRATCH/'Data') == before_failure, 'Failed registration restores exact CSV bytes')
    loaded = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    check(not any(row['ID'] == new_config['id'] for row in loaded), 'Failed registration restores the in-memory DataTable')
    check(snapshot(PROJECT/'Data') == live_before, 'Live project definitions remain unchanged')


result = {'checks': checks}
try:
    run(); result['success'] = True
except Exception:
    import traceback
    result['success'] = False; result['error'] = traceback.format_exc(); u.log_error(result['error'])
finally:
    path = PROJECT/'Saved/NPCImport/RegistrationTest.json'
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    u.log('IC_NPC_REGISTRATION_TEST '+json.dumps({'success': result['success'], 'checks': len(checks)}))
