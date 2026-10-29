"""Apply the existing Firefight art to live HUD assets after building the editor.

The sample assets are read only. Backups of changed project assets go to Saved.
Run with -ExecutePythonScript and -RenderOffscreen; the editor exits on completion.
"""
import csv
import io
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/FirefightHUD'
OUT.mkdir(parents=True, exist_ok=True)
FOLDER = '/Game/IsekaiClass/UI/Widget/'
SPECS = [
    ('WBP_IC_PlayerStatus', u.ICCharacterStatusWidget, 'PlayerStatus'),
    ('WBP_IC_CompanionStatus', u.ICCharacterStatusWidget, 'CompanionStatus'),
    ('WBP_IC_CompanionStatusAlternate', u.ICCharacterStatusWidget, 'CompanionStatusAlternate'),
    ('WBP_IC_CompanionGroup', u.ICCompanionGroupWidget, 'CompanionGroup'),
    ('WBP_IC_QuickSlot', u.ICQuickSlotWidget, 'QuickSlot'),
    ('WBP_IC_QuickSlots', u.ICQuickSlotsWidget, 'QuickSlots'),
    ('WBP_IC_QuestTracker', u.ICQuestTrackerWidget, 'QuestTracker'),
    ('WBP_IC_Minimap', u.ICMapWidget, 'Minimap'),
    ('WBP_IC_GameMenu', u.ICGameMenuWidget, 'GameMenu'),
    ('WBP_GameHUD', u.ICGameplayWidget, 'HUD'),
]

def backup(asset):
    path = ROOT / ('Content/' + asset.removeprefix('/Game/') + '.uasset')
    dest = OUT / 'Backup' / path.relative_to(ROOT)
    if path.exists() and not dest.exists():
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)

report = {'widgets': {}, 'art_source': 'existing project assets only', 'item_cooldowns': {}}
for name, parent, kind in SPECS:
    path = FOLDER + name
    backup(path)
    bp = u.load_asset(path)
    if not bp:
        factory = u.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        bp = u.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER.rstrip('/'), u.WidgetBlueprint, factory)
    assert bp
    if not isinstance(u.get_default_object(bp.generated_class()), parent):
        u.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp, kind), kind
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    default = u.get_default_object(bp.generated_class())
    if kind in ('PlayerStatus', 'CompanionStatus', 'CompanionStatusAlternate'):
        default.set_editor_property('display_name', '')
        default.set_editor_property('portrait', None)
        for resource in ('hp_percent', 'mp_percent', 'sp_percent'):
            default.set_editor_property(resource, 0.0)
    if kind == 'Minimap':
        default.set_editor_property('bMinimap', True)
        default.set_editor_property('minimap_zoom', 4.0)
    assert u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), path
    names = set(str(x) for x in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp))
    report['widgets'][name] = sorted(names)
    if kind in ('PlayerStatus', 'CompanionStatus', 'CompanionStatusAlternate'):
        assert {'IMG_Portrait', 'BAR_HP', 'BAR_MP1', 'BAR_MP2', 'BAR_MP3', 'TXT_Name', 'StatusEffectsPanel', 'StatusEffects', 'StatusOverflow', 'TXT_StatusOverflow'} <= names
        assert not {'Image_73', 'Image_76', 'Image_80'} & names
        assert ('BAR_SP' in names) == (kind == 'PlayerStatus')
    if kind == 'HUD':
        assert {'HUDNavigationPanel', 'TXT_GameClock', 'HUD_Minimap', 'HUD_QuestTracker', 'HUD_TargetInfo', 'Image_65', 'Image_66', 'TextBlock_49', 'ProgressBar_13'} <= names
    if kind == 'Minimap':
        assert {'MapSurface', 'MapContent', 'TXT_MiniNorth', 'TXT_MiniHelp', 'TXT_MiniLegend'} <= names
    if kind == 'QuickSlots':
        assert {'CanvasPanel_94', 'QuickSlot1', 'QuickSlot2', 'QuickSlot3', 'QuickSlot4'} <= names

path = '/Game/IsekaiClass/Data/DataTable/Items/DT_IC_Item'
backup(path)
table = u.load_asset(path)
rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
for row in rows:
    row['CooldownPlaySeconds'] = 5.0 if row.get('Kind') == 'Consumable' else 0.0
    if row['CooldownPlaySeconds']:
        report['item_cooldowns'][row['Name']] = row['CooldownPlaySeconds']
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
csv_path = ROOT / 'Data/Items/DT_IC_Item.csv'
reader = csv.DictReader(io.StringIO(csv_path.read_text(encoding='utf-8-sig')))
fields = list(reader.fieldnames)
csv_rows = list(reader)
if 'CooldownPlaySeconds' not in fields:
    fields.insert(fields.index('UseEffect') + 1, 'CooldownPlaySeconds')
for row in csv_rows:
    row['CooldownPlaySeconds'] = '5.000000' if row['Kind'] == 'Consumable' else '0.000000'
with csv_path.open('w', encoding='utf-8', newline='') as file:
    writer = csv.DictWriter(file, fieldnames=fields, lineterminator='\n')
    writer.writeheader()
    writer.writerows(csv_rows)

for name, size in [('WBP_IC_PlayerStatus', (280, 194)), ('WBP_IC_CompanionStatus', (220, 126)),
                   ('WBP_IC_QuickSlots', (580, 140)), ('WBP_IC_GameMenu', (1920, 1080)), ('WBP_GameHUD', (1920, 1080))]:
    if '-nullrhi' not in u.SystemLibrary.get_command_line().lower():
        assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(u.load_asset(FOLDER + name), str(OUT / (name + '.png')), *size)
(OUT / 'Applied.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FIREFIGHT_HUD_APPLIED')
if '-run=pythonscript' not in u.SystemLibrary.get_command_line().lower():
    u.SystemLibrary.quit_editor()
