"""Create the inventory, item cell and confirmation WBPs, then update the menu reference."""
import unreal as u

tools=u.AssetToolsHelpers.get_asset_tools()
folder='/Game/IsekaiClass/UI/Widget'
for name,parent,kind in (
    ('WBP_IC_InventorySlot',u.ICInventorySlotWidget,'InventorySlot'),
    ('WBP_IC_ItemConfirm',u.ICItemConfirmWidget,'ItemConfirm'),
    ('WBP_IC_InventoryMenu',u.ICInventoryMenuWidget,'InventoryMenu'),
    ('WBP_IC_GameMenu',u.ICGameMenuWidget,'GameMenu'),
):
    bp=u.load_asset(folder+'/'+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=tools.create_asset(name,folder,u.WidgetBlueprint,factory)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
    names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
    if kind=='InventoryMenu':assert {'Member0','Equipment0','Equipment3','TAB_Normal','TAB_Quest','ItemGrid','QuickSlot0','QuickSlot3','BTN_Quick','BTN_Trash','ItemConfirm'}<=names
    if kind=='GameMenu':assert {'PartyContent','InventoryContent','MenuPages'}<=names
hud=u.load_asset(folder+'/WBP_GameHUD')
u.BlueprintEditorLibrary.compile_blueprint(hud)
assert u.EditorAssetLibrary.save_loaded_asset(hud,only_if_is_dirty=False)
u.log('IC_INVENTORY_MENU_SETUP_OK')
