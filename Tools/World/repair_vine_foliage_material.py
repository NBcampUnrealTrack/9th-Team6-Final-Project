"""Repair the foliage usage missing from the vine instance reported by UE.

Run in a dedicated editor process after closing the interactive editor.
Only the affected material instance is saved; its parent and maps are protected.
"""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

assert '-ICFoliageMaterialRepair' in u.SystemLibrary.get_command_line()
root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved/FoliageMaterialRepair'
out.mkdir(parents=True, exist_ok=True)
target = '/Game/IsekaiClass/World/SampleLevel/Medieval_House/materials/plants_materials/MI_vine_04_Inst'
asset_file = root / ('Content/' + target.removeprefix('/Game/') + '.uasset')
backup = out / 'MI_vine_04_Inst.before.uasset'
if not backup.exists():
    shutil.copy2(asset_file, backup)
material = u.load_asset(target)
assert isinstance(material, u.MaterialInstanceConstant)
usage = u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES


def params():
    return {key: str(material.get_editor_property(key)) for key in (
        'parent', 'texture_parameter_values', 'scalar_parameter_values', 'vector_parameter_values')}


chain = []
current = material
while isinstance(current, u.MaterialInstanceConstant):
    chain.append({'path': current.get_path_name(), 'foliage_usage': u.MaterialEditingLibrary.has_material_usage(current, usage),
                  'usage_override': u.MaterialEditingLibrary.has_material_usage_override(current, usage)})
    current = current.get_editor_property('parent')
assert isinstance(current, u.Material)
chain.append({'path': current.get_path_name(), 'foliage_usage': u.MaterialEditingLibrary.has_material_usage(current, usage)})
protected = [root / 'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
             root / ('Content/' + current.get_path_name().split('.')[0].removeprefix('/Game/') + '.uasset')]
hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in protected}
before_params = params()
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
references = []
for package in registry.get_referencers(target, u.AssetRegistryDependencyOptions()):
    for data in registry.get_assets_by_package_name(package):
        references.append({'path': str(data.package_name), 'class': str(data.asset_class_path.asset_name)})
report = {'material': target, 'before_chain': chain, 'referencing_assets': references}
(out / 'Before.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')

# UE 5.8 supports per-instance usage overrides. Preserve the shared master.
u.MaterialEditingLibrary.set_material_usage_override(material, usage, True, True)
u.MaterialEditingLibrary.update_material_instance(material)
assert u.MaterialEditingLibrary.has_material_usage(material, usage)
assert u.MaterialEditingLibrary.has_material_usage_override(material, usage)
assert params() == before_params, 'Visual parameters changed unexpectedly'
assert u.EditorAssetLibrary.save_loaded_asset(material, False)
for path in protected:
    assert hashlib.sha256(path.read_bytes()).hexdigest() == hashes[str(path)], str(path)
report.update(foliage_usage=True, usage_override=True, visual_parameters_preserved=True,
              master_and_map_preserved=True, saved=True)
stats = u.MaterialEditingLibrary.get_statistics(material)
report['shader_statistics'] = {key: stats.get_editor_property(key) for key in (
    'num_vertex_shader_instructions', 'num_pixel_shader_instructions', 'num_samplers', 'num_pixel_texture_samples')}
(out / 'Applied.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FOLIAGE_VINE_MATERIAL_REPAIRED')
