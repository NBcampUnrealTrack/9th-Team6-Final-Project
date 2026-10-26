"""Update only player preset portraits from the source CSV, preserving other DT edits."""
import csv
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
table = u.load_asset('/Game/IsekaiClass/Data/DataTable/Character/DT_IC_PlayerPreset')
assert table
with (root / 'Data/Character/DT_IC_PlayerPreset.csv').open(encoding='utf-8-sig', newline='') as stream:
    source = {row['ID']: row for row in csv.DictReader(stream)}
rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
portraits = {}
for row in rows:
    if row['ID'] not in ('PRESET_PLAYER_M', 'PRESET_PLAYER_F'):
        continue
    portrait = source[row['ID']]['Portrait']
    assert u.load_asset(portrait), portrait
    row['Portrait'] = portrait
    portraits[row['ID']] = portrait
assert len(portraits) == 2
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
report = root / 'Saved/HUDPrototype/PlayerPortraits.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(portraits, indent=2), encoding='utf-8')
u.log('IC_PLAYER_PORTRAITS_CONFIGURED ' + json.dumps(portraits))
