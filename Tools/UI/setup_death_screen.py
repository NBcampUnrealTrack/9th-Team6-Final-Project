"""Author the death WBP without rebuilding existing HUD, save-slot or main-menu assets."""
import unreal as u
folder='/Game/IsekaiClass/UI/Widget'
path=folder+'/WBP_IC_Death'
bp=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
if not bp:
    factory=u.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',u.ICDeathWidget)
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset('WBP_IC_Death',folder,u.WidgetBlueprint,factory)
assert bp,path
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Death')
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
names={str(name) for name in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
assert {'DeathBackground','TXT_Death','BTN_Load','BTN_MainMenu','BTN_Quit'}<=names
assert {name for name in names if name.startswith('BTN_') and not name.endswith('_Text')}=={'BTN_Load','BTN_MainMenu','BTN_Quit'}
u.log('IC_DEATH_SCREEN_SETUP_OK '+bp.get_path_name())
