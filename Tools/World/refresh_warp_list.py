"""Save only the fullscreen warp-list widget after native layout edits."""
import unreal as u
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Map')
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Map')
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
u.log('IC_WARP_LIST_WIDGET_REFRESHED')
