"""Inspect the live, unsaved map and furniture source components without changes."""
import unreal as u
import json
from pathlib import Path
ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/LifeWorldUpgrade'
OUT.mkdir(parents=True, exist_ok=True)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name() != 'L_OpenWorld':
    world=u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')

def vec(v): return [v.x, v.y, v.z]
def transform(t): return dict(location=vec(t.translation), scale=vec(t.scale3d), rotation=[t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w])
def components(actor):
    result=[]
    for c in actor.get_components_by_class(u.SceneComponent):
        item=dict(name=c.get_name(),type=c.get_class().get_name(),relative=transform(c.get_relative_transform()),world=transform(c.get_world_transform()),visible=c.is_visible())
        if isinstance(c,u.StaticMeshComponent):
            item.update(mesh=c.static_mesh.get_path_name() if c.static_mesh else None,materials=[c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials())],collision=str(c.get_collision_enabled()))
        result.append(item)
    return result

report=dict(world=world.get_path_name(),fishing=[],stations=[],sources=[],foliage_api=[n for n in dir(u) if 'Foliage' in n and 'Factory' in n])
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.ICLifeActor) and actor.life_type==u.ICDefinitionType.FISHING_POINT:
        report['fishing'].append(dict(name=actor.get_name(),label=actor.get_actor_label(),definition=str(actor.definition_id),point=str(actor.point_id),hidden=actor.get_editor_property('hidden'),collision=actor.get_actor_enable_collision(),transform=transform(actor.get_actor_transform()),components=components(actor)))
    if isinstance(actor,u.ICCraftingStation):
        report['stations'].append(dict(name=actor.get_name(),type=actor.get_class().get_path_name(),transform=transform(actor.get_actor_transform()),components=components(actor)))
source_dir='/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Meshes/Props/CraftingTables/'
for name in ('BP_Farm_CraftingTable_Cooking_01','BP_Farm_CraftingTable_Carpentry_01'):
    bp=u.load_asset(source_dir+name)
    # Transient editor actors allow SCS transforms to be inspected accurately.
    actor=actors.spawn_actor_from_class(bp.generated_class(),u.Vector(0,0,-900000),u.Rotator(),True)
    report['sources'].append(dict(path=bp.get_path_name(),components=components(actor)))
    actors.destroy_actor(actor)
(OUT/'LiveAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_WORLD_AUDIT_COMPLETE')
