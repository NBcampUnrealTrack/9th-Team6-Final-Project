"""Author the settings layout as an editable Widget Blueprint."""
import unreal as u

folder = '/Game/IsekaiClass/UI/Widget'
name = 'WBP_IC_Settings'
bp = u.load_asset(folder + '/' + name)
if not bp:
    factory = u.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', u.ICSettingsWidget)
    bp = u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.WidgetBlueprint, factory)
assert bp
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp, 'Settings')
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
u.log('IC_SETTINGS_WIDGET_SETUP_OK')
