"""Update only the movement cooldown WBP and its placement in the existing HUD."""
import unreal as u

folder='/Game/IsekaiClass/UI/Widget/'
widget=u.load_asset(folder+'WBP_IC_MovementCooldown')
assert widget
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(widget,'MovementCooldown')
u.BlueprintEditorLibrary.compile_blueprint(widget)
assert u.EditorAssetLibrary.save_loaded_asset(widget,only_if_is_dirty=False)
names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(widget)}
assert 'RING_Cooldown' in names
assert not any(n.startswith('TXT_') for n in names),names
hud=u.load_asset(folder+'WBP_GameHUD')
assert u.ICBlueprintMaintenanceLibrary.add_movement_cooldown_hud(hud)
u.BlueprintEditorLibrary.compile_blueprint(hud)
assert u.EditorAssetLibrary.save_loaded_asset(hud,only_if_is_dirty=False)
u.log('IC_CHARACTER_COOLDOWN_UI_UPDATED')
