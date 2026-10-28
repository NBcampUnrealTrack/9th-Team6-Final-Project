"""Place three existing affinity-gated NPC container variants behind each test bed."""
import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.project_dir());out=root/'Saved/NPCFishing';out.mkdir(exist_ok=True)
game='/Game/IsekaiClass';assets=u.AssetToolsHelpers.get_asset_tools();actors=u.get_editor_subsystem(u.EditorActorSubsystem)
u.EditorLoadingAndSavingUtils.load_map(game+'/World/L_TestMap')
layouts=[a for a in actors.get_all_level_actors() if isinstance(a,u.ICNPCTestLayout)]
assert len(layouts)==1,'Expected one NPC test layout'
layout=layouts[0];forward=layout.get_actor_forward_vector();right=layout.get_actor_right_vector()
variants=[('Chest','개인 상자','SM_chest_01',(-235,0),95),('Wardrobe','옷장','SM_commode_01',(-420,-85),145),('Bookshelf','책장','SM_bookcase_01',(-420,85),145)]
report={'existing_native_system':'ICNPCContainerActor','required_affinity':200,'containers':[]}
for kind,title,mesh_name,offset,target_height in variants:
    path=game+'/NPC/Blueprints/Containers/BP_IC_NPC'+kind;bp=u.load_asset(path)
    if not bp:
        folder,name=path.rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
        factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.ICNPCContainerActor)
        bp=assets.create_asset(name,folder,u.Blueprint,factory)
    assert bp;default=u.get_default_object(bp.generated_class());mesh=u.load_asset(game+'/World/SampleLevel/Medieval_House/meshes/props/'+mesh_name);assert mesh
    shape=default.get_editor_property('shape');shape.set_editor_property('static_mesh',mesh)
    bounds=mesh.get_bounding_box();scale=target_height/(bounds.max.z-bounds.min.z)
    shape.set_relative_scale3d(u.Vector(scale,scale,scale));default.set_editor_property('interaction_text',u.Text(title+' 열기 · 호감도 200'))
    label=default.get_editor_property('label');label.set_world_size(14);label.set_relative_location(u.Vector(0,0,bounds.max.z+22),False,False)
    u.BlueprintEditorLibrary.compile_blueprint(bp);assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    for index,npc_id in enumerate(layout.get_editor_property('NPCIDs')):
        key=f'IC_NPC_{str(npc_id)}_{kind}';existing=next((a for a in actors.get_all_level_actors() if a.actor_has_tag(key)),None)
        bed=layout.bed_transform(index).translation;position=bed+forward*offset[0]+right*offset[1];position.z-=bounds.min.z*scale
        actor=existing or actors.spawn_actor_from_class(bp.generated_class(),position,layout.get_actor_rotation())
        actor.set_actor_location_and_rotation(position,layout.get_actor_rotation(),False,True)
        actor.set_editor_property('OwnerNPCID',npc_id);actor.set_editor_property('ContainerID',f'CNT_{str(npc_id)}_{kind.upper()}')
        actor.set_editor_property('interaction_text',u.Text(str(npc_id).replace('NPC_','')+' '+title+' · 호감도 200'))
        actor.get_editor_property('label').set_text(u.Text(str(npc_id).replace('NPC_','')+' '+title))
        if not actor.actor_has_tag(key):actor.set_editor_property('tags',list(actor.get_editor_property('tags'))+[u.Name(key)])
        actor.set_actor_label(key);actor.set_folder_path('IC_NPC_Containers')
        report['containers'].append({'npc':str(npc_id),'kind':kind,'blueprint':path,'id':str(actor.get_editor_property('container_id')),'position':[position.x,position.y,position.z]})
assert len(report['containers'])==len(layout.get_editor_property('NPCIDs'))*3
assert u.EditorLoadingAndSavingUtils.save_current_level()
(out/'NPCContainers.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NPC_CONTAINERS_SETUP_OK')
