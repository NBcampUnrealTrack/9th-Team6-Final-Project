"""Author the separate forty-slot storage screen and quantity popup WBPs."""
import unreal as u
tools=u.AssetToolsHelpers.get_asset_tools()
folder='/Game/IsekaiClass/UI/Widget'
for name,parent,kind in [('WBP_IC_StorageTransferPopup',u.ICStorageTransferPopup,'StorageTransferPopup'),('WBP_IC_Storage',u.ICStorageWidget,'Storage')]:
    bp=u.load_asset(folder+'/'+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=tools.create_asset(name,folder,u.WidgetBlueprint,factory)
    assert bp
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
u.log('STORAGE_UI_ASSETS_OK')
