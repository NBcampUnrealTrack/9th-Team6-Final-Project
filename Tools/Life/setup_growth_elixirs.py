"""Persist elixir names, categories, icons and references across the runtime data."""
import csv
import json
import shutil
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/GrowthElixirs';OUT.mkdir(parents=True,exist_ok=True)
tables=['Items/DT_IC_Item','Life/DT_IC_GachaMachine','Registry/DT_IC_IDRegistry',
        'Rewards/DT_IC_Reward','Rewards/DT_IC_DropTable','Settings/DT_IC_NewGame']
strings=u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(ROOT/'Data/StringTables/ST_IC_Definitions.csv'))
assert u.EditorAssetLibrary.save_loaded_asset(strings,False)
for relative in tables:
    table=u.load_asset('/Game/IsekaiClass/Data/DataTable/'+relative)
    assert table,relative
    source=ROOT/'Content/IsekaiClass/Data/DataTable'/Path(relative+'.uasset')
    backup=OUT/'Backup'/Path(relative+'.uasset');backup.parent.mkdir(parents=True,exist_ok=True)
    if not backup.exists():shutil.copy2(source,backup)
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_file(table,str(ROOT/'Data'/Path(relative+'.csv'))),relative
    assert u.EditorAssetLibrary.save_loaded_asset(table,False),relative
items=u.load_asset('/Game/IsekaiClass/Data/DataTable/Items/DT_IC_Item')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(items))
elixirs=[row for row in rows if row['ID'].startswith('ITM_ELIXIR_')]
assert len(elixirs)==15
assert not any(row['ID'].startswith('ITM_BOOK_') for row in rows)
assert all(row['Category']=='Elixir' and row['UseEffect']=='GrowthElixir' and 'Potion' in row['Icon'] for row in elixirs)
for row in elixirs:assert u.load_asset(row['Icon']),row['Icon']
(OUT/'Setup.json').write_text(json.dumps(dict(complete=True,tables=tables,elixirs=[row['ID'] for row in elixirs]),indent=2),encoding='utf-8')
u.log('IC_GROWTH_ELIXIRS_SETUP_COMPLETE')
