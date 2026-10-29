"""Create editable dialogue/BGM DTs, HUD clock, and NPC bed interaction assets."""
import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=root/'Saved/LiveDialogueMusic';out.mkdir(exist_ok=True)
assets=u.AssetToolsHelpers.get_asset_tools();actors=u.get_editor_subsystem(u.EditorActorSubsystem);game='/Game/IsekaiClass'
def save(asset):
    if isinstance(asset,u.Blueprint):u.BlueprintEditorLibrary.compile_blueprint(asset)
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()
def table(path,struct,defaults,relative):
    result=u.load_asset(path)
    if not result:
        folder,name=path.rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
        factory=u.DataTableFactory();factory.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.'+struct))
        result=assets.create_asset(name,folder,u.DataTable,factory)
    rows={r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(result))}
    for row in defaults:rows.setdefault(row['Name'],row)
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(result,json.dumps(list(rows.values()),ensure_ascii=False))
    save(result);source=root/'Data'/relative;source.parent.mkdir(parents=True,exist_ok=True)
    source.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(result),encoding='utf-8-sig');return result
def blueprint(path,parent):
    bp=u.load_asset(path)
    if not bp:
        folder,name=path.rsplit('/',1);u.EditorAssetLibrary.make_directory(folder);factory=u.BlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=assets.create_asset(name,folder,u.Blueprint,factory)
    assert bp;return bp
dialogue=table(game+'/Data/DataTable/NPC/DT_IC_DialoguePresentation','ICDialoguePresentationRow',[
    dict(Name='Default',bUseCloseUpCamera=True,bAllowPlayerMovement=False,CloseDistanceCm=500,HeadBone='J_Bip_C_Head',CameraDistanceCm=220,CameraHeightCm=10,CameraSideOffsetCm=0,FocusOffset='(X=0,Y=0,Z=-12)',FieldOfView=40,FollowSpeed=12,CameraCollisionRadiusCm=8,bHidePlayerInCamera=True)
],'NPC/DT_IC_DialoguePresentation.csv')
tracks=[('BGM_Village','마을','Merchant'),('BGM_Outside','마을 밖','Flying_Through_Mountains'),('BGM_Area_01','다른 구역 1','Ocean_Waves'),('BGM_Area_02','다른 구역 2','Inner_Dreams'),('BGM_Combat','전투 중','Enemy_Scrolls')]
music=table(game+'/Data/DataTable/Audio/DT_IC_BGM','ICBGMRow',[
    dict(Name=key,DisplayName=label,Music=game+'/Audio/SampleAudio/OrchestralLoops/'+sound+'.'+sound,Volume=.6,FadeInSeconds=1.5,FadeOutSeconds=1.5,bLoop=True) for key,label,sound in tracks
],'Audio/DT_IC_BGM.csv')
for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(music)):
    assert u.load_asset(row['Music']),row['Music']
hud=u.load_asset(game+'/UI/Widget/WBP_GameHUD');assert u.ICBlueprintMaintenanceLibrary.add_game_clock_hud(hud);save(hud)
folder=game+'/NPC/LifeObject/Definitions';u.EditorAssetLibrary.make_directory(folder)
definition=u.load_asset(folder+'/SO_IC_NPCBed')
if not definition:
    factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.SmartObjectDefinition)
    definition=assets.create_asset('SO_IC_NPCBed',folder,u.SmartObjectDefinition,factory)
    slot=u.SmartObjectSlotDefinition()
    definition.set_editor_property('slots',[slot])
    behavior=u.new_object(u.ICLifeObjectBehaviorDefinition,outer=definition)
    definition.set_editor_property('default_behavior_definitions',[behavior]);save(definition)
bed=blueprint(game+'/NPC/Blueprints/Interaction/BP_IC_NPCBed',u.ICLifeObjectActor);default=u.get_default_object(bed.generated_class())
default.set_editor_property('ObjectType',u.ICLifeObjectType.BED);default.set_editor_property('bPlayerCanSleep',True);default.set_editor_property('PlayerRequiredAffinity',200)
assert u.ICBlueprintMaintenanceLibrary.set_life_object_definition(default,definition)
shape=default.get_editor_property('Shape');shape.set_editor_property('static_mesh',u.load_asset('/Engine/BasicShapes/Cube'))
shape.set_visibility(False,False);shape.set_relative_scale3d(u.Vector(2.3,1.7,1));shape.set_relative_location(u.Vector(0,0,-35),False,False)
default.get_editor_property('Label').set_visibility(False);save(bed)
zone=blueprint(game+'/Audio/Blueprints/BP_IC_BGMZone',u.ICBGMZone);save(zone)
u.EditorLoadingAndSavingUtils.load_map(game+'/World/L_TestMap')
layout=next(a for a in actors.get_all_level_actors() if isinstance(a,u.ICNPCTestLayout));layout.set_editor_property('InteractionBedClass',bed.generated_class())
layout_bp=u.load_asset(game+'/World/Blueprints/BP_R01TestLayout');u.get_default_object(layout_bp.generated_class()).set_editor_property('InteractionBedClass',bed.generated_class());save(layout_bp)
for key,position,extent,priority in [('BGM_Village',(1000,0,0),(4500,2500,2000),10),('BGM_Area_01',(2000,-5000,0),(1600,1500,2000),20),('BGM_Area_02',(2000,-11000,0),(1600,1500,2000),20)]:
    tag='IC_'+key;actor=next((a for a in actors.get_all_level_actors() if a.actor_has_tag(tag)),None)
    actor=actor or actors.spawn_actor_from_class(zone.generated_class(),u.Vector(*position))
    actor.set_actor_location(u.Vector(*position),False,True);actor.set_editor_property('MusicRowID',key);actor.set_editor_property('Priority',priority)
    actor.get_editor_property('Bounds').set_box_extent(u.Vector(*extent),False);actor.set_editor_property('tags',[u.Name(tag)])
    actor.set_actor_label(tag);actor.set_folder_path('IC_BGM_Zones')
assert u.EditorLoadingAndSavingUtils.save_current_level()
(out/'Assets.json').write_text(json.dumps({'dialogue':dialogue.get_path_name(),'bgm':music.get_path_name(),'npc_bed':bed.get_path_name(),'zones':3},ensure_ascii=False,indent=2),encoding='utf-8')
u.log('LIVE_DIALOGUE_MUSIC_SETUP_OK')
