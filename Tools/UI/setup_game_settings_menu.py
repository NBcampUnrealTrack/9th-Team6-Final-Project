"""Add the game settings action to the existing menu Widget Blueprint."""
import unreal as u

folder = '/Game/IsekaiClass/UI/Widget/'
menu = u.load_asset(folder + 'WBP_IC_GameMenu')
assert menu
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(menu, 'GameMenu')
u.BlueprintEditorLibrary.compile_blueprint(menu)
assert 'BTN_GameSettings' in {str(name) for name in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(menu)}
assert u.EditorAssetLibrary.save_loaded_asset(menu, only_if_is_dirty=False)
# Refresh the embedded class without replacing the HUD's authored layout.
hud = u.load_asset(folder + 'WBP_GameHUD')
assert hud
u.BlueprintEditorLibrary.compile_blueprint(hud)
assert u.EditorAssetLibrary.save_loaded_asset(hud, only_if_is_dirty=False)
u.log('IC_GAME_SETTINGS_MENU_SETUP_OK')
