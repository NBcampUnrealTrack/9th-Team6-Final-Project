"""Create the approved save UI assets and bind them to the existing native session code."""
import csv
import io
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
strings_file = root / 'Data/StringTables/ST_IC_UI.csv'
with strings_file.open(encoding='utf-8-sig', newline='') as stream:
    rows = list(csv.DictReader(stream))
strings = {row['Key']: row['SourceString'] for row in rows}
strings.update({
    'Save_EmptySlot': '비어 있는 슬롯',
    'Save_Help': 'W/S · ↑/↓ 선택 · F/G 확인 · Delete 삭제 · ESC 뒤로',
    'Save_Delete': '삭제', 'Save_Confirm': '확인', 'Save_Cancel': '취소',
})
output = io.StringIO(newline='')
writer = csv.DictWriter(output, ['Key', 'SourceString'], lineterminator='\n', quoting=csv.QUOTE_ALL)
writer.writeheader()
writer.writerows({'Key': key, 'SourceString': value} for key, value in strings.items())
temporary = strings_file.with_suffix('.csv.tmp')
temporary.write_text(output.getvalue().replace('\r\n', '\n').replace('\r', ''), encoding='utf-8', newline='')
temporary.replace(strings_file)
table = u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(table, str(strings_file))
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)

assets = {}
for name, parent, kind in (
    ('WBP_IC_SaveSlot', u.ICSaveSlotWidget, 'SaveSlot'),
    ('WBP_IC_LoadConfirm', u.ICLoadConfirmWidget, 'SaveConfirm'),
    ('WBP_IC_SaveLoad', u.ICSaveLoadWidget, 'SaveLoad'),
):
    folder = '/Game/IsekaiClass/UI/Widget'
    path = folder + '/' + name
    blueprint = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
    if not blueprint:
        factory = u.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        blueprint = u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.WidgetBlueprint, factory)
    if not blueprint:
        raise RuntimeError('Could not create ' + path)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(blueprint, kind)
    u.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assets[name] = blueprint

portraits = {}
for relative in ('Character/DT_IC_PlayerPreset.csv', 'NPC/DT_IC_NPC.csv'):
    with (root / 'Data' / relative).open(encoding='utf-8-sig', newline='') as stream:
        for row in csv.DictReader(stream):
            path = row.get('Portrait', 'None')
            if path in ('', 'None'):
                continue
            texture = u.load_asset(path)
            if texture:
                key = {'PRESET_PLAYER_M': 'CHR_PLAYER_MALE', 'PRESET_PLAYER_F': 'CHR_PLAYER_FEMALE'}.get(row['ID'], row['ID'])
                portraits[key] = texture
save_bp = assets['WBP_IC_SaveLoad']
default = u.get_default_object(save_bp.generated_class())
default.set_editor_property('confirm_widget_class', assets['WBP_IC_LoadConfirm'].generated_class())
default.set_editor_property('slot_widget_class', assets['WBP_IC_SaveSlot'].generated_class())
default.set_editor_property('portrait_textures', portraits)
for blueprint in assets.values():
    assert u.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

controller = u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
u.BlueprintEditorLibrary.compile_blueprint(controller)
u.get_default_object(controller.generated_class()).set_editor_property('save_load_widget_class', save_bp.generated_class())
assert u.EditorAssetLibrary.save_loaded_asset(controller, only_if_is_dirty=False)
report = {'widgets': {name: bp.get_path_name() for name, bp in assets.items()}, 'portraits': len(portraits), 'controller': controller.get_path_name()}
(root / 'Saved/SaveUISetup.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
u.log('IC_SAVE_UI_CREATED ' + json.dumps(report))
