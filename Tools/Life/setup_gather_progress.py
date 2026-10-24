"""Author the gathering overlay and five-second resource production durations."""
import csv
import io
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
folder = '/Game/IsekaiClass/UI/Widget'
name = 'WBP_IC_GatherProgress'
bp = u.load_asset(folder + '/' + name)
if not bp:
    factory = u.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', u.ICGatherProgressWidget)
    bp = u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.WidgetBlueprint, factory)
assert bp
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp, 'GatherProgress')
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)

relative = 'Life/DT_IC_Production'
table = u.load_asset('/Game/IsekaiClass/Data/DataTable/' + relative)
reader = csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
fields, rows = reader.fieldnames, list(reader)
changed = 0
for row in rows:
    if row['ResultKind'] == 'GatherResource':
        row['DurationPlaySeconds'] = '5'
        changed += 1
assert changed >= 16, changed
output = io.StringIO(newline='')
writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
writer.writeheader()
writer.writerows(rows)
text = output.getvalue()
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, text)
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
destination = root / 'Saved/GatherProgress/Data' / (relative + '.csv')
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(text, encoding='utf-8', newline='')
u.log('Gather progress WBP saved; five-second resource productions: %d' % changed)
