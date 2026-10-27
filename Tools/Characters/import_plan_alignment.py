"""Run in the Unreal editor after align_plan_data.py and the C++ build."""
from pathlib import Path
import json
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base = '/Game/IsekaiClass/Data/DataTable/'
tables = [
    'Settings/DT_IC_TimeSettings', 'Settings/DT_IC_InventorySettings',
    'Settings/DT_IC_StatSettings', 'Settings/DT_IC_CreationOptions',
    'World/DT_IC_Home', 'Items/DT_IC_Item', 'Recipes/DT_IC_Recipe',
    'Life/DT_IC_Production', 'NPC/DT_IC_GiftPreference', 'NPC/DT_IC_NPC',
    'Presentation/DT_IC_Codex', 'Registry/DT_IC_IDRegistry',
]
for relative in tables:
    asset = u.load_asset(base + relative)
    if not asset:
        raise RuntimeError('Missing table: ' + relative)
    source = (root / 'Data' / (relative + '.csv')).read_text(encoding='utf-8-sig')
    if not u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset, source):
        raise RuntimeError('Import failed: ' + relative)
    if not u.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError('Save failed: ' + relative)
for name in ('ST_IC_Definitions', 'ST_IC_UI'):
    asset = u.load_asset(base + 'StringTables/' + name)
    if not u.ICBlueprintMaintenanceLibrary.import_ui_strings(asset, str(root / 'Data/StringTables' / (name + '.csv'))):
        raise RuntimeError('StringTable import failed: ' + name)
    if not u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('StringTable save failed: ' + name)
controllers = u.EditorAssetLibrary.list_assets('/Game/IsekaiClass', recursive=True, include_folder=False)
changed = []
for path in controllers:
    if 'PlayerController' not in path:
        continue
    blueprint = u.load_asset(path)
    if not isinstance(blueprint, u.Blueprint):
        continue
    generated = blueprint.generated_class()
    default = u.get_default_object(generated) if generated else None
    if isinstance(default, u.ICPlayerController):
        u.BlueprintEditorLibrary.compile_blueprint(blueprint)
        u.get_default_object(blueprint.generated_class()).set_editor_property('player_camera_manager_class', u.PlayerCameraManager)
        u.EditorAssetLibrary.save_loaded_asset(blueprint)
        changed.append(path)
(root / 'Saved/PlanAlignmentImport.json').write_text(json.dumps({'tables': tables, 'controllers': changed}, indent=2), encoding='utf-8')
print('PLAN_ALIGNMENT_IMPORTED', len(tables))
