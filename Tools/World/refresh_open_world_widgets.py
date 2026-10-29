"""Refresh the four runtime UI assets after compiling native layout changes."""
import unreal as u
for name,kind in [('WBP_IC_InventoryMenu','InventoryMenu'),('WBP_IC_Map','Map'),('WBP_IC_Minimap','Minimap'),('WBP_GameHUD','HUD')]:
    bp=u.load_asset('/Game/IsekaiClass/UI/Widget/'+name)
    assert bp and u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    if kind=='HUD':assert u.ICBlueprintMaintenanceLibrary.prepare_managed_widget(bp,'HUD')
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
    if kind=='Map':assert {'WarpConfirm','BTN_WarpConfirm','BTN_WarpCancel','BTN_Warp0','BTN_Warp3'}<=names and 'IMG_Map' not in names
u.log('IC_OPENWORLD_WIDGETS_REFRESHED')
