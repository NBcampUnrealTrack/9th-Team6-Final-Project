import unreal as u
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Enhancement');assert bp
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Enhancement');u.BlueprintEditorLibrary.compile_blueprint(bp);assert u.EditorAssetLibrary.save_loaded_asset(bp,False);u.log('ENHANCEMENT_WIDGET_REFRESH_OK')
