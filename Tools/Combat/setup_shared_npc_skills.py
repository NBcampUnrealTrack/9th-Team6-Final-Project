"""Persist the eleven shared NPC/player skills and remove retired starter references."""
import csv
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/SharedNPCSkills'
OUT.mkdir(parents=True, exist_ok=True)
EXPECTED = {f'SKL_NPC_R01_{i:03d}' for i in range(1, 12)}
with (ROOT/'Data/Combat/DT_IC_Skill.csv').open(encoding='utf-8-sig', newline='') as stream:
    rows = list(csv.DictReader(stream))
assert {row['ID'] for row in rows} == EXPECTED and len(rows) == 11
assert all(row['OwnerType'] == 'RecruitUnique' and row['NPCID'] == row['ID'].replace('SKL_NPC_', 'NPC_') for row in rows)

for relative in ['Combat/DT_IC_Skill', 'Settings/DT_IC_NewGame', 'Registry/DT_IC_IDRegistry']:
    path = '/Game/IsekaiClass/Data/DataTable/' + relative
    asset = u.load_asset(path)
    assert asset, path
    source = ROOT/'Content/IsekaiClass/Data/DataTable'/Path(relative + '.uasset')
    backup = OUT/'Backup'/Path(relative + '.uasset')
    backup.parent.mkdir(parents=True, exist_ok=True)
    if not backup.exists():
        shutil.copy2(source, backup)
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_file(asset, str(ROOT/'Data'/Path(relative + '.csv'))), path
    assert u.EditorAssetLibrary.save_loaded_asset(asset, False), path

strings = u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings, str(ROOT/'Data/StringTables/ST_IC_Definitions.csv'))
assert u.EditorAssetLibrary.save_loaded_asset(strings, False)

skills = u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Skill')
persisted = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(skills))
assert {row['ID'] for row in persisted} == EXPECTED and len(persisted) == 11
new_game = u.load_asset('/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_NewGame')
rules = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(new_game))[0]
assert set(rules['StartingSkills']) <= EXPECTED
(OUT/'Setup.json').write_text(json.dumps(dict(complete=True, skills=sorted(EXPECTED), starting_skills=rules['StartingSkills']), indent=2), encoding='utf-8')
u.log('IC_SHARED_NPC_SKILLS_SETUP_COMPLETE')
