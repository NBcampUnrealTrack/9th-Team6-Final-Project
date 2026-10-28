"""Update only the party-follow settings asset from its checked-in CSV."""
import json
import shutil
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/CompanionTracking'
out.mkdir(parents=True,exist_ok=True)
asset='/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_PartySettings'
source=root/'Content/IsekaiClass/Data/DataTable/Settings/DT_IC_PartySettings.uasset'
backup=out/'DT_IC_PartySettings.before.uasset'
if not backup.exists():shutil.copy2(source,backup)
table=u.load_asset(asset)
assert table
csv=(root/'Data/Settings/DT_IC_PartySettings.csv').read_text(encoding='utf-8-sig')
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,csv)
assert u.EditorAssetLibrary.save_loaded_asset(table,only_if_is_dirty=False)
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
(out/'Settings.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_COMPANION_TRACKING_SETTINGS_APPLIED')
