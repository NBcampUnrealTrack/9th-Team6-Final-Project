"""Preserve NPC DT edits, fill only missing portraits from the matching source row,
then verify that every recruitable NPC has a loadable portrait assigned.
"""
import csv
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
table = u.load_asset('/Game/IsekaiClass/Data/DataTable/NPC/DT_IC_NPC')
assert table
rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
with (root / 'Data/NPC/DT_IC_NPC.csv').open(encoding='utf-8-sig', newline='') as stream:
    source = {row['ID']: row for row in csv.DictReader(stream)}
changed = []
portraits = {}
for row in rows:
    if row['Type'] != 'Recruitable':
        continue
    npc_id = row['ID']
    if row.get('Portrait', 'None') in ('', 'None'):
        portrait = source.get(npc_id, {}).get('Portrait', 'None')
        if portrait not in ('', 'None'):
            row['Portrait'] = portrait
            changed.append(npc_id)
    portrait = row.get('Portrait', 'None')
    assert portrait not in ('', 'None') and u.load_asset(portrait), f'Assign Portrait (초상화) in NPC DT: {npc_id}'
    portraits[npc_id] = {'name_key': row['NameKey'], 'portrait': portrait}
assert portraits, 'No recruitable NPC rows were inspected'
if changed:
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
report = root / 'Saved/HUDPrototype/NPCPortraits.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps({'filled_missing': changed, 'npcs': portraits}, ensure_ascii=False, indent=2), encoding='utf-8')
u.log(f'IC_NPC_PORTRAITS_VERIFIED count={len(portraits)} filled={changed}')
