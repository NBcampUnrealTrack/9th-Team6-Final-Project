"""Create character profile DT and reusable party widgets, preserving the existing HUD."""
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
TOOLS=u.AssetToolsHelpers.get_asset_tools()
def save(asset):assert u.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)

folder='/Game/IsekaiClass/Data/DataTable/Character'
table=u.load_asset(folder+'/DT_IC_CharacterProfile')
if not table:
    factory=u.DataTableFactory();factory.set_editor_property('struct',u.ICCharacterProfileRow.static_struct())
    table=TOOLS.create_asset('DT_IC_CharacterProfile',folder,u.DataTable,factory)
    assert table
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_file(table,str(ROOT/'Data/Character/DT_IC_CharacterProfile.csv'))
    save(table)

folder='/Game/IsekaiClass/UI/Widget'
for name,parent,kind in (
    ('WBP_IC_PartyPortrait',u.ICPartyPortraitWidget,'PartyPortrait'),
    ('WBP_IC_PartyMenu',u.ICPartyMenuWidget,'PartyMenu'),
    ('WBP_IC_GameMenu',u.ICGameMenuWidget,'GameMenu'),
):
    bp=u.load_asset(folder+'/'+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=TOOLS.create_asset(name,folder,u.WidgetBlueprint,factory)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
    if kind=='PartyMenu':assert {'Member0','Member1','Member2','Member3','TXT_Name','TXT_MainSetting','TXT_Story','IMG_Pet','StatsGrid'}<=names
    if kind=='GameMenu':assert {'PartyContent','TXT_Navigation','MenuPages','BTN_Save','BTN_Load'}<=names
hud=u.load_asset(folder+'/WBP_GameHUD')
u.BlueprintEditorLibrary.compile_blueprint(hud);save(hud)
u.log('IC_PARTY_MENU_SETUP_OK '+table.get_path_name())
