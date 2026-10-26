"""Author the codex WBPs without inserting sample player progress or monster mappings."""
import unreal as u
tools=u.AssetToolsHelpers.get_asset_tools()
folder='/Game/IsekaiClass/UI/Widget'
for name,parent,kind in (
    ('WBP_IC_CodexCell',u.ICCodexCellWidget,'CodexCell'),
    ('WBP_IC_CodexPopup',u.ICCodexPopupWidget,'CodexPopup'),
    ('WBP_IC_CodexTargets',u.ICCodexTargetsWidget,'CodexTargets'),
    ('WBP_IC_CodexMenu',u.ICCodexMenuWidget,'CodexMenu'),
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
    if kind=='CodexMenu':assert {'CellsGrid','EquippedSkills','DetailPopup','TargetPopup','ConfirmPopup','NoticePopup'}<=names
    if kind=='GameMenu':assert 'CodexContent' in names
for name in ('WBP_GameHUD','WBP_IC_PartyMenu'):
    bp=u.load_asset(folder+'/'+name);u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
u.log('IC_CODEX_MENU_SETUP_OK')
