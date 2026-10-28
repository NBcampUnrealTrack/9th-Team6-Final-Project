import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=root/'Saved/MapTravel';out.mkdir(parents=True,exist_ok=True)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    p=a.get_actor_location();b,e=a.get_actor_bounds(False)
    rows.append(dict(name=a.get_actor_label(),cls=a.get_class().get_name(),p=[p.x,p.y,p.z],center=[b.x,b.y,b.z],extent=[e.x,e.y,e.z],path=a.get_path_name()))
bp=u.load_asset('/Game/IsekaiClass/World/Blueprints/BP_ICWarpStone')
cdo=u.get_default_object(bp.generated_class())
components=[]
for c in cdo.get_components_by_class(u.ActorComponent):
    components.append(dict(name=c.get_name(),cls=c.get_class().get_name(),mesh=c.get_editor_property('static_mesh').get_path_name() if isinstance(c,u.StaticMeshComponent) and c.get_editor_property('static_mesh') else ''))
(out/'Audit.json').write_text(json.dumps(dict(actors=rows,portal_components=components),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('MAP_AUDIT_OK')
