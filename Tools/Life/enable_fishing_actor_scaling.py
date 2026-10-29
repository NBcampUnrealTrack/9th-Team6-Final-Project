"""Let actor scaling resize fishing water areas, preserving all current geometry."""
import json
from pathlib import Path
import shutil
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
output = root / 'Saved/FishingScaling'
output.mkdir(parents=True, exist_ok=True)
asset_path = '/Game/IsekaiClass/Life/Fishing/Blueprints/BP_R01_RandomFishingZone'
asset_file = root / 'Content/IsekaiClass/Life/Fishing/Blueprints/BP_R01_RandomFishingZone.uasset'
backup = output / 'BP_R01_RandomFishingZone.before_relative_scale.uasset'
if not backup.exists():
    shutil.copy2(asset_file, backup)


def regions(actor):
    return [component for component in actor.get_components_by_class(u.StaticMeshComponent)
            if component != actor.shape and component.static_mesh]


def xyz(vector):
    return [vector.x, vector.y, vector.z]


def inherit_scale(component, world_scale):
    component.modify()
    component.set_absolute(component.get_editor_property('absolute_location'),
                           component.get_editor_property('absolute_rotation'), False)
    component.set_world_scale3d(world_scale)


actors = u.get_editor_subsystem(u.EditorActorSubsystem)
snapshots = {}
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.ICLifeActor) and actor.life_type == u.ICDefinitionType.FISHING_POINT:
        snapshots[actor.get_path_name()] = {
            component.get_name(): {
                'world': component.get_world_transform(),
                'absolute_scale': component.get_editor_property('absolute_scale'),
            } for component in regions(actor)
        }

bp = u.load_asset(asset_path)
assert bp, asset_path
default = u.get_default_object(bp.generated_class())
for component in regions(default):
    inherit_scale(component, component.get_world_transform().scale3d)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp, False), asset_path

report = {'blueprint': asset_path, 'instances': [], 'level_saved': False}
with u.ScopedEditorTransaction('Enable fishing area actor scaling'):
    for actor in actors.get_all_level_actors():
        before = snapshots.get(actor.get_path_name())
        if before is None:
            continue
        actor.modify()
        entries = []
        for component in regions(actor):
            old = before[component.get_name()]
            inherit_scale(component, old['world'].scale3d)
            now = component.get_world_transform()
            assert not component.get_editor_property('absolute_scale')
            assert max(abs(a - b) for a, b in zip(xyz(now.translation), xyz(old['world'].translation))) < .1
            assert max(abs(a - b) for a, b in zip(xyz(now.scale3d), xyz(old['world'].scale3d))) < .001
            entries.append({'component': component.get_name(), 'was_absolute_scale': old['absolute_scale'],
                            'absolute_scale': False, 'world_scale': xyz(now.scale3d)})
        report['instances'].append({'actor': actor.get_actor_label(), 'regions': entries})

default = u.get_default_object(bp.generated_class())
assert all(not component.get_editor_property('absolute_scale') for component in regions(default))
(output / 'Applied.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FISHING_ACTOR_SCALING_ENABLED: world geometry preserved; BP saved, current map left unsaved.')
