"""Remove the obsolete eight-anchor restriction from the five default crops."""
import json
from pathlib import Path
import shutil
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
output = root / 'Saved/PlantingFix'
output.mkdir(parents=True, exist_ok=True)
asset_path = '/Game/IsekaiClass/Data/DataTable/Life/DT_IC_Crop'
asset_file = root / 'Content/IsekaiClass/Data/DataTable/Life/DT_IC_Crop.uasset'
backup = output / 'DT_IC_Crop.before_plot_fix.uasset'
table = u.load_asset(asset_path)
assert table, asset_path
rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
original = json.loads(json.dumps(rows))
old_anchors = [f'ANC_PLAYER_PLOT_{i:02d}' for i in range(1, 9)]
crop_ids = {f'CRP_R01_{i:02d}' for i in range(1, 6)}
assert crop_ids.issubset({row['ID'] for row in rows})
changed = []
for row in rows:
    if row['ID'] in crop_ids:
        assert row['AllowedPlotAnchorIDs'] in ([], old_anchors), row['ID']
        if row['AllowedPlotAnchorIDs']:
            row['AllowedPlotAnchorIDs'] = []
            changed.append(row['ID'])
if changed:
    if not backup.exists():
        shutil.copy2(asset_file, backup)
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
    assert u.EditorAssetLibrary.save_loaded_asset(table, False)
verified = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
assert verified == rows
for before, after in zip(original, verified):
    before['AllowedPlotAnchorIDs'] = after['AllowedPlotAnchorIDs']
    assert before == after, after['ID']
(output / 'Applied.json').write_text(json.dumps({'changed': changed, 'rows': verified},
    ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_PLANTING_PLOT_RESTRICTIONS_FIXED')
