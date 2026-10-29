"""Author the modular HUD and tabbed menu after building IsekaiClassEditor.

Run using UnrealEditor-Cmd -run=pythonscript -script=<this file>.
For review PNGs, use the full editor's -ExecutePythonScript with
-ICRenderHUDPrototype -RenderOffscreen (rendering needs the editor Slate UI).
Player status uses live state and the selected preset DT portrait.
Companions use party state and NPC DT portraits.
Quick slots use registered item IDs, item DT icons and bag counts.
Use -ICUpdateCompanionHUDOnly to update only companion cards, their group and HUD.
Use -ICUpdatePlayerHUDOnly to update both status cards and the containing HUDs.
Use -ICUpdateHUDReadabilityOnly to update status cards, quest tracker and HUD.
Use -ICUpdateQuickSlotsOnly to update the quick slot cards, their group and HUD.
Use -ICUpdateGameMenuOnly to update the tabbed menu and its containing HUD.
"""
import json
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
FOLDER = '/Game/IsekaiClass/UI/Widget'
REPORT = ROOT / 'Saved/HUDPrototype'
REPORT.mkdir(parents=True, exist_ok=True)

specs = (
    ('WBP_IC_PlayerStatus', u.ICCharacterStatusWidget, 'PlayerStatus'),
    ('WBP_IC_CompanionStatus', u.ICCharacterStatusWidget, 'CompanionStatus'),
    ('WBP_IC_CompanionStatusAlternate', u.ICCharacterStatusWidget, 'CompanionStatusAlternate'),
    ('WBP_IC_CompanionGroup', u.ICCompanionGroupWidget, 'CompanionGroup'),
    ('WBP_IC_Minimap', u.ICMapWidget, 'Minimap'),
    ('WBP_IC_QuickSlot', u.ICQuickSlotWidget, 'QuickSlot'),
    ('WBP_IC_QuickSlots', u.ICQuickSlotsWidget, 'QuickSlots'),
    ('WBP_IC_GameMenu', u.ICGameMenuWidget, 'GameMenu'),
    ('WBP_IC_QuestTracker', u.ICQuestTrackerWidget, 'QuestTracker'),
    ('WBP_GameHUD', u.ICGameplayWidget, 'HUD'),
)
assets = {}
names = {}
arguments = u.SystemLibrary.get_command_line()
scopes = (
    ('-ICUpdateHUDReadabilityOnly', ('PlayerStatus', 'CompanionStatus', 'CompanionStatusAlternate', 'CompanionGroup', 'Minimap', 'QuestTracker', 'HUD')),
    ('-ICUpdateCompanionHUDOnly', ('CompanionStatus', 'CompanionStatusAlternate', 'CompanionGroup', 'HUD')),
    ('-ICUpdatePlayerHUDOnly', ('PlayerStatus', 'CompanionStatus', 'CompanionStatusAlternate', 'CompanionGroup', 'HUD')),
    ('-ICUpdateQuickSlotsOnly', ('QuickSlot', 'QuickSlots', 'HUD')),
    ('-ICUpdateGameMenuOnly', ('GameMenu', 'HUD')),
)
selected_kinds = next((kinds for flag, kinds in scopes if flag in arguments), None)
for name, parent, kind in specs:
    path = FOLDER + '/' + name
    bp = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
    skip = selected_kinds is not None and kind not in selected_kinds
    if skip:
        assert bp, path
        assets[name] = bp
        names[name] = [str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)]
        continue
    if not bp:
        factory = u.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        bp = u.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, u.WidgetBlueprint, factory)
    assert bp, path
    if bp.generated_class() and not isinstance(u.get_default_object(bp.generated_class()), parent):
        u.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp, kind), path
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    default = u.get_default_object(bp.generated_class())
    if kind == 'Minimap':
        default.set_editor_property('bMinimap', True)
        default.set_editor_property('minimap_zoom', 4.0)
    if kind in ('PlayerStatus', 'CompanionStatus', 'CompanionStatusAlternate'):
        default.set_editor_property('display_name', '')
        default.set_editor_property('portrait', None)
        for resource in ('hp_percent', 'mp_percent', 'sp_percent'):
            default.set_editor_property(resource, 0.0)
    assert u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), path
    assets[name] = bp
    names[name] = [str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)]

assert {'IMG_Portrait', 'TXT_Name', 'BAR_HP', 'BAR_MP', 'BAR_SP'} <= set(names['WBP_IC_PlayerStatus'])
for name in ('WBP_IC_PlayerStatus', 'WBP_IC_CompanionStatus'):
    assert {'StatusEffectsPanel', 'StatusEffects', 'StatusOverflow', 'TXT_StatusOverflow'} <= set(names[name]), name
    assert not any(widget.startswith(('TXT_HP', 'TXT_MP', 'TXT_SP', 'VALUE_')) for widget in names[name]), name
assert {'IMG_Portrait', 'TXT_Name', 'BAR_HP', 'BAR_MP'} <= set(names['WBP_IC_CompanionStatus'])
assert 'BAR_SP' not in names['WBP_IC_CompanionStatus']
assert {'Companion1', 'Companion2', 'Companion3'} <= set(names['WBP_IC_CompanionGroup'])
assert {'QuickSlot1', 'QuickSlot2', 'QuickSlot3', 'QuickSlot4'} <= set(names['WBP_IC_QuickSlots'])
assert {'IMG_ItemIcon', 'TXT_Count'} <= set(names['WBP_IC_QuickSlot'])
assert {'MapSurface', 'MapContent', 'IMG_Map', 'TXT_MiniNorth', 'TXT_MiniHelp', 'TXT_MiniLegend'} <= set(names['WBP_IC_Minimap'])
assert {'HUDNavigationPanel', 'TXT_GameClock', 'HUD_Minimap', 'HUD_PlayerStatus', 'HUD_Companions', 'HUD_QuestTracker', 'HUD_QuickSlots', 'GameMenu'} <= set(names['WBP_GameHUD'])
assert 'BAR_HP' not in names['WBP_GameHUD'], 'Player resources must belong to the separate status WBP'
tabs = ('Party', 'Inventory', 'Quest', 'Codex', 'Settings')
assert {'MenuPages', *('TAB_' + tab for tab in tabs), *('Page_' + tab for tab in tabs)} <= set(names['WBP_IC_GameMenu'])
assert not {'TAB_Equipment', 'TAB_Stats', 'TAB_Skills', 'TAB_Map'} & set(names['WBP_IC_GameMenu'])
assert {'SettingsContent', 'MenuActions', 'BTN_Resume', 'BTN_Save', 'BTN_Load', 'BTN_GameSettings', 'BTN_Quit'} <= set(names['WBP_IC_GameMenu'])

images = []
if '-ICRenderHUDPrototype' in u.SystemLibrary.get_command_line():
    for asset, sizes in (
        ('WBP_GameHUD', ((1920, 1080), (1280, 720))),
        ('WBP_IC_GameMenu', ((1920, 1080),)),
        ('WBP_IC_PlayerStatus', ((350, 200),)),
        ('WBP_IC_CompanionGroup', ((350, 280),)),
    ):
        for width, height in sizes:
            file = REPORT / f'{asset}_{width}x{height}.png'
            assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(assets[asset], str(file), width, height), str(file)
            images.append(str(file))
(REPORT / 'Assets.json').write_text(json.dumps({'widgets': names, 'images': images, 'tabs': tabs}, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_HUD_PROTOTYPE_ASSETS_VERIFIED ' + json.dumps(list(assets)))
