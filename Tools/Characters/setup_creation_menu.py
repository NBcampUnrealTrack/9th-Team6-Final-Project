"""Refresh only the approved character creation and gameplay menu preview assets."""
from pathlib import Path
import unreal as u

project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
strings = u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings, str(project / 'Data/StringTables/ST_IC_UI.csv'))
assert u.EditorAssetLibrary.save_loaded_asset(strings, only_if_is_dirty=False)

for name, kind in (
    ('WBP_CharacterCreate', 'Questions'), ('WBP_GameHUD', 'HUD'),
    ('WBP_IC_SaveSlot', 'SaveSlot'), ('WBP_IC_LoadConfirm', 'SaveConfirm'),
    ('WBP_IC_SaveLoad', 'SaveLoad'),
):
    path = '/Game/IsekaiClass/UI/Widget/' + name
    blueprint = u.load_asset(path)
    assert blueprint, path
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(blueprint, kind), path
    u.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assert u.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False), path

u.log('IC_CREATION_MENU_READY: creation questions and two-level ESC menu updated')
