import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
lib = u.EditorAssetLibrary
report = {'samples': [], 'animations': [], 'map': []}
world = u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
for number in ('07','03','164','153','150','114','107','10','38','08'):
    path = '/Game/IsekaiClass/Enemy/SampleEnemies/MEGA_Tiny_Dragon/Blueprints/BP_DragonSD_' + number
    bp = u.load_asset(path)
    sample = actors.spawn_actor_from_class(bp.generated_class(), u.Vector(0,0,5000))
    components = sample.get_components_by_class(u.SkeletalMeshComponent)
    report['samples'].append({'number': number, 'components': [
        {'name': c.get_name(), 'mesh': str(c.get_editor_property('skeletal_mesh_asset')), 'materials': [str(m) for m in c.get_materials()],
         'transform': str(c.get_relative_transform()), 'animation_class': str(c.get_editor_property('anim_class'))} for c in components]})
    actors.destroy_actor(sample)
for path in lib.list_assets('/Game/IsekaiClass/Enemy/SampleEnemies/MEGA_Tiny_Dragon', recursive=True, include_folder=False):
    if any(s in path.lower() for s in ('idle','hit','attack','walk','run','skeleton')):
        report['animations'].append(path)
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    report['map'].append({'name': actor.get_actor_label(), 'class': actor.get_class().get_name(), 'location': str(actor.get_actor_location()), 'bounds': str(actor.get_actor_bounds(False))})
out = root / 'Saved/MonsterPrototype'
out.mkdir(parents=True, exist_ok=True)
(out/'Inspection.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_MONSTER_INSPECTION_OK ' + str(out))

