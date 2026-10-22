"""Rebuild the native title menu widget after C++ layout and label changes."""
import unreal as u
from pathlib import Path

project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
strings = u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings, str(project / 'Data/StringTables/ST_IC_UI.csv'))
assert u.EditorAssetLibrary.save_loaded_asset(strings, only_if_is_dirty=False)

path = '/Game/IsekaiClass/UI/Widget/WBP_MainMenu'
blueprint = u.load_asset(path)
assert blueprint, path
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(blueprint, 'Mainmenu'), path
u.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert u.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False), path
u.log('IC_MAIN_MENU_READY: continue, start, save list, settings, quit')
