"""Author missing WBP designs and prologue DTs. Existing prologue DT edits are preserved."""
from pathlib import Path
import json
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
game='/Game/IsekaiClass';base=game+'/UI/Widget/'
out=root/'Saved/ManagedUIReview';out.mkdir(parents=True,exist_ok=True)
assets=u.AssetToolsHelpers.get_asset_tools()

def save(asset):
    if isinstance(asset,u.Blueprint):u.BlueprintEditorLibrary.compile_blueprint(asset)
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()

def widget(name,parent,kind,rebuild=False):
    bp=u.load_asset(base+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=assets.create_asset(name,base.rstrip('/'),u.WidgetBlueprint,factory);rebuild=True
    assert bp,name
    if rebuild:assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind),name
    if kind not in ('Prologue','Root','LoadingMovie','StatusEffect'):assert u.ICBlueprintMaintenanceLibrary.apply_frontend_theme(bp)
    save(bp);return bp

def table(name,struct,source,defaults=None):
    path=game+'/Data/DataTable/UI/'+name
    result=u.load_asset(path)
    if not result:
        u.EditorAssetLibrary.make_directory(game+'/Data/DataTable/UI')
        factory=u.DataTableFactory();factory.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.'+struct))
        result=assets.create_asset(name,game+'/Data/DataTable/UI',u.DataTable,factory)
        if defaults is None:assert u.DataTableFunctionLibrary.fill_data_table_from_csv_file(result,str(root/source))
        else:assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(result,json.dumps(defaults,ensure_ascii=False))
        save(result)
    (root/source).parent.mkdir(parents=True,exist_ok=True)
    (root/source).write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(result),encoding='utf-8-sig')
    return result

script=table('DT_IC_Prologue','ICPrologueRow','Data/UI/DT_IC_Prologue.csv')
presentation=table('DT_IC_ProloguePresentation','ICProloguePresentationRow','Data/UI/DT_IC_ProloguePresentation.csv',[
    dict(Name='Default',NextPrompt='G · 다음 대사',GamepadNextPrompt='확인 버튼 · 다음 대사',SkipPrompt='H 길게 누르기 · 프롤로그 건너뛰기',GamepadSkipPrompt='Y / △ 길게 누르기 · 프롤로그 건너뛰기',CharacterInterval=.045,FadeSeconds=1.2,SkipHoldSeconds=1.5,InitialMusicRow='BGM_Prologue')])
prologue=widget('WBP_IC_Prologue',u.ICPrologueWidget,'Prologue')
assert u.ICBlueprintMaintenanceLibrary.prepare_managed_widget(prologue,'Prologue')
default=u.get_default_object(prologue.generated_class());default.set_editor_property('ScriptTable',script);default.set_editor_property('PresentationTable',presentation);save(prologue)
widget('WBP_IC_Root',u.ICUIRootWidget,'Root')
widget('WBP_IC_LoadingMovie',u.UserWidget,'LoadingMovie')
for name,parent,kind in [('WBP_IC_RecipeChoice',u.ICRecipeChoiceButton,'ActionRow'),('WBP_IC_EnhancementChoice',u.ICEnhancementChoiceButton,'ActionRow'),('WBP_IC_LifeChoice',u.ICLifeChoiceButton,'ActionRow'),('WBP_IC_KeyBindingRow',u.UserWidget,'KeyBindingRow'),('WBP_IC_QuestReward',u.UserWidget,'QuestReward'),('WBP_IC_StatusEffect',u.UserWidget,'StatusEffect'),('WBP_IC_TextLine',u.UserWidget,'TextLine'),('WBP_IC_Crosshair',u.UserWidget,'Crosshair'),('WBP_IC_MapPortalMarker',u.UserWidget,'MapPortalMarker'),('WBP_IC_MapPlayerMarker',u.UserWidget,'MapPlayerMarker')]:
    bp=widget(name,parent,kind)
    if kind=='MapPortalMarker':
        u.get_default_object(bp.generated_class()).set_render_transform_pivot(u.Vector2D(0,0));save(bp)
for name,kind in [('WBP_IC_Map','Map'),('WBP_IC_Minimap','Minimap')]:
    widget(name,u.ICMapWidget,kind,True)
widget('WBP_IC_QuestDetail',u.ICQuestDetailWidget,'QuestDetail',True)
for name,kind in [('WBP_IC_SaveSlot','SaveSlot'),('WBP_IC_SaveLoad','SaveLoad')]:
    widget(name,u.ICSaveSlotWidget if kind=='SaveSlot' else u.ICSaveLoadWidget,kind,True)
for name,kind in [('WBP_GameHUD','HUD'),('WBP_IC_PlayerStatus','Status'),('WBP_IC_CompanionStatus','Status'),('WBP_IC_CompanionStatusAlternate','Status'),('WBP_IC_CombatFeedback','Feedback')]:
    bp=u.load_asset(base+name);assert bp,name
    assert u.ICBlueprintMaintenanceLibrary.prepare_managed_widget(bp,kind),name
    save(bp)

audit={};native_children=[]
for file in (root/'Content/IsekaiClass').rglob('WBP_*.uasset'):
    relative=file.relative_to(root/'Content').as_posix()
    if '/SampleUI/' in relative or '/SampleLevel/' in relative:continue
    path='/Game/'+relative.removesuffix('.uasset')
    bp=u.load_asset(path)
    if not isinstance(bp,u.WidgetBlueprint):continue
    rows=json.loads(u.ICBlueprintMaintenanceLibrary.inspect_frontend_widget(bp));audit[bp.get_name()]=rows
    for row in rows:
        if row['class'].startswith('/Script/IsekaiClass.'):
            cls=u.load_object(None,row['class'])
            if isinstance(u.get_default_object(cls),u.UserWidget):native_children.append(dict(asset=bp.get_name(),**row))
(out/'WidgetAudit.json').write_text(json.dumps(dict(widgets=audit,native_user_widget_children=native_children),ensure_ascii=False,indent=2),encoding='utf-8')
assert not native_children,native_children
assert len(u.DataTableFunctionLibrary.get_data_table_row_names(script))>0
u.log('IC_MANAGED_UI_FLOW_READY')
