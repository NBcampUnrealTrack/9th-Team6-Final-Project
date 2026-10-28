"""Apply the Morpheus menu layout; leave the Firefight gameplay HUD intact."""
from pathlib import Path
import json
import hashlib
import unreal as u

project=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=project/'Saved/MorpheusUIReview';out.mkdir(parents=True,exist_ok=True)
base='/Game/IsekaiClass/UI/Widget/'
hud_names=['WBP_GameHUD','WBP_IC_CompanionGroup','WBP_IC_CompanionStatus','WBP_IC_CompanionStatusAlternate',
           'WBP_IC_PlayerStatus','WBP_IC_MonsterHealthBar','WBP_IC_MonsterDamage','WBP_IC_PlayerDamage',
           'WBP_IC_Minimap','WBP_IC_LifeLabel']
hud_hashes={name:hashlib.sha256((project/'Content/IsekaiClass/UI/Widget'/(name+'.uasset')).read_bytes()).hexdigest()
            for name in hud_names if (project/'Content/IsekaiClass/UI/Widget'/(name+'.uasset')).exists()}
def save(bp):
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False),bp.get_name()
for name,kind in [('WBP_IC_Settings','Settings'),('WBP_IC_GameMenu','GameMenu'),('WBP_MainMenu','Mainmenu'),('WBP_CharacterCreate','Questions')]:
    bp=u.load_asset(base+name);assert bp,name
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    assert u.ICBlueprintMaintenanceLibrary.apply_frontend_theme(bp)
    save(bp)

hud={'WBP_GameHUD','WBP_IC_CompanionGroup','WBP_IC_CompanionStatus','WBP_IC_CompanionStatusAlternate',
     'WBP_IC_PlayerStatus','WBP_IC_MonsterHealthBar','WBP_IC_MonsterDamage','WBP_IC_PlayerDamage',
     'WBP_IC_Minimap','WBP_IC_LifeLabel'}
themed=[]
for path in u.EditorAssetLibrary.list_assets(base,recursive=True,include_folder=False):
    bp=u.load_asset(path)
    if not isinstance(bp,u.WidgetBlueprint) or bp.get_name() in hud:continue
    if any(word in bp.get_name() for word in ['Root','Loading','Death','QuickSlot','Progress','Cooldown','Hearts','Marker','Tracker','Feedback','Target','Float','Fade']):continue
    assert u.ICBlueprintMaintenanceLibrary.apply_frontend_theme(bp)
    save(bp);themed.append(bp.get_name())

for name,file,step in [('WBP_IC_Settings','Settings_Combat',0),('WBP_IC_Settings','Settings_Graphics',2),
                       ('WBP_IC_GameMenu','Menu_Party',0),('WBP_IC_GameMenu','Menu_System',4),
                       ('WBP_MainMenu','Title',-1),('WBP_CharacterCreate','Name',0)]:
    bp=u.load_asset(base+name)
    assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(bp,str(out/(file+'.png')),1920,1080,step),name
    (out/(file+'.json')).write_text(u.ICBlueprintMaintenanceLibrary.inspect_frontend_widget(bp),encoding='utf-8')

# Inspect active trees, not package history, to ensure Firefight brushes do not remain in menus.
violations=[]
for name in themed:
    rows=json.loads(u.ICBlueprintMaintenanceLibrary.inspect_frontend_widget(u.load_asset(base+name)))
    for row in rows:
        for field in ['image','border','normal','hovered']:
            if '/SampleUI/FirefightUI/' in row.get(field,''):violations.append((name,row['name'],field))
assert not violations,violations
for name,before in hud_hashes.items():
    assert hashlib.sha256((project/'Content/IsekaiClass/UI/Widget'/(name+'.uasset')).read_bytes()).hexdigest()==before,name
(out/'Audit.json').write_text(json.dumps(dict(menu_widgets=themed,firefight_menu_brushes=violations,hud_preserved=sorted(hud)),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_MORPHEUS_UI_READY')
