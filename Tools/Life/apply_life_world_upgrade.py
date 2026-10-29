"""Author furniture appearances, real-actor foliage assets, and the warp list UI."""
import unreal as u
import json, shutil
from pathlib import Path
from collections import Counter

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/LifeWorldUpgrade'
OUT.mkdir(parents=True,exist_ok=True)
TOOLS=u.AssetToolsHelpers.get_asset_tools()
ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem)
REPORT={'furniture':[],'foliage':[],'widgets':[]}

def backup(path):
    source=ROOT/('Content/'+path.removeprefix('/Game/')+'.uasset')
    target=OUT/'Backup'/source.relative_to(ROOT)
    if source.exists() and not target.exists():
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)

def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()

def mesh_records(actor):
    records=[]
    for c in actor.get_components_by_class(u.StaticMeshComponent):
        if not c.static_mesh:continue
        t=c.get_world_transform();q=t.rotation
        sign=1 if q.w>=0 else -1
        records.append((c.static_mesh.get_path_name(),tuple(round(n,2) for n in (t.translation.x,t.translation.y,t.translation.z,t.scale3d.x,t.scale3d.y,t.scale3d.z,q.x*sign,q.y*sign,q.z*sign,q.w*sign)),tuple(c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials()))))
    return Counter(records)

source_root='/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Meshes/Props/CraftingTables/'
for source_name,target_path in (
    ('BP_Farm_CraftingTable_Cooking_01','/Game/IsekaiClass/Life/Cooking/BP_IC_Kitchen'),
    ('BP_Farm_CraftingTable_Carpentry_01','/Game/IsekaiClass/Life/Crafting/BP_IC_CraftingTable')):
    backup(target_path)
    source=u.load_asset(source_root+source_name);target=u.load_asset(target_path)
    cdo=u.get_default_object(target.generated_class())
    before=(str(cdo.recipe_type),str(cdo.work_anchor_id),str(cdo.action),cdo.station_widget_class.get_path_name())
    count=u.ICBlueprintMaintenanceLibrary.copy_furniture_appearance(target,source)
    assert count>0
    save(target)
    cdo=u.get_default_object(target.generated_class())
    after=(str(cdo.recipe_type),str(cdo.work_anchor_id),str(cdo.action),cdo.station_widget_class.get_path_name())
    assert before==after,(before,after)
    previews=[]
    try:
        for bp in (source,target):previews.append(ACTORS.spawn_actor_from_class(bp.generated_class(),u.Vector(0,0,-900000),u.Rotator(),True))
        expected,actual=map(mesh_records,previews)
        missing=list((expected-actual).elements());extra=list((actual-expected).elements())
        entry=dict(source=source.get_path_name(),target=target_path,parts=count,function_defaults=after,appearance_matches=not missing and not extra,missing=missing[:3],extra=extra[:3])
        REPORT['furniture'].append(entry)
        (OUT/'Partial.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
        assert entry['appearance_matches'],entry
    finally:
        for actor in previews:ACTORS.destroy_actor(actor)

for folder,prefix,count in (('Collection','Collection',10),('Mining','Ore',6)):
    path='/Game/IsekaiClass/Life/'+folder
    foliage_path=path+'/Foliage';u.EditorAssetLibrary.make_directory(foliage_path)
    for index in range(1,count+1):
        name=f'BP_R01_{prefix}_{index:02d}';bp=u.load_asset(path+'/'+name);assert bp,name
        cdo=u.get_default_object(bp.generated_class());assert str(cdo.point_id)=='None',name
        asset_name=f'FT_R01_{prefix}_{index:02d}'
        foliage=u.load_asset(foliage_path+'/'+asset_name)
        if foliage:backup(foliage_path+'/'+asset_name)
        else:foliage=TOOLS.create_asset(asset_name,foliage_path,u.FoliageType_Actor,u.FoliageType_ActorFactory())
        assert foliage
        foliage.set_editor_property('actor_class',bp.generated_class())
        foliage.set_editor_property('static_mesh_only',False)
        foliage.set_editor_property('density',7.5)
        foliage.set_editor_property('radius',200.)
        foliage.set_editor_property('random_yaw',True)
        save(foliage)
        REPORT['foliage'].append(dict(asset=foliage.get_path_name(),actor=bp.generated_class().get_path_name(),density=foliage.get_editor_property('density'),radius=foliage.get_editor_property('radius'),static_mesh_only=foliage.get_editor_property('static_mesh_only')))

path='/Game/IsekaiClass/UI/Widget/WBP_IC_Map';backup(path)
bp=u.load_asset(path);assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Map')
u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
assert {'WarpDestinations','WarpConfirm','BTN_WarpConfirm','BTN_WarpCancel'}<=names
assert 'IMG_Map' not in names
REPORT['widgets'].append(dict(asset=path,has_image='IMG_Map' in names,confirmation=True))
(OUT/'Authored.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_WORLD_UPGRADE_ASSETS_COMPLETE')
