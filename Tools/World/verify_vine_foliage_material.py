"""Reload L_OpenWorld and verify the repaired material on its foliage instances."""
import hashlib
import json
from pathlib import Path
import unreal as u

assert '-ICFoliageMaterialVerify' in u.SystemLibrary.get_command_line()
root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved/FoliageMaterialRepair'
target = '/Game/IsekaiClass/World/SampleLevel/Medieval_House/materials/plants_materials/MI_vine_04_Inst'
map_file = root / 'Content/IsekaiClass/World/Maps/L_OpenWorld.umap'
map_hash = hashlib.sha256(map_file.read_bytes()).hexdigest()
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name() != 'L_OpenWorld':
    world = u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
material = u.load_asset(target)
usage = u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
assert u.MaterialEditingLibrary.has_material_usage(material, usage), 'Saved usage was not restored'
assert u.MaterialEditingLibrary.has_material_usage_override(material, usage)
matches = []
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    for component in actor.get_components_by_class(u.InstancedStaticMeshComponent):
        for index, assigned in enumerate(component.get_materials()):
            if assigned and assigned.get_path_name().split('.')[0] == target:
                assert u.MaterialEditingLibrary.has_material_usage(assigned, usage)
                mesh = component.get_editor_property('static_mesh')
                matches.append({'actor': actor.get_path_name(), 'component': component.get_name(),
                                'mesh': mesh.get_path_name() if mesh else None,
                                'instances': component.get_instance_count(), 'material_slot': index})
assert matches, 'The material was not found on the saved map foliage'
stats = u.MaterialEditingLibrary.get_statistics(material)
assert stats.num_vertex_shader_instructions > 0 and stats.num_pixel_shader_instructions > 0
assert hashlib.sha256(map_file.read_bytes()).hexdigest() == map_hash
report = {'world': world.get_path_name(), 'material': target, 'saved_foliage_usage': True,
          'compiled_shader_instructions': {'vertex': stats.num_vertex_shader_instructions, 'pixel': stats.num_pixel_shader_instructions},
          'affected_components': matches, 'affected_instances': sum(match['instances'] for match in matches),
          'map_file_preserved': True}
(out / 'Verified.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FOLIAGE_VINE_MATERIAL_VERIFIED')
