"""Read-only audit of requested NPC skill sources, their dependencies and portraits."""
import csv
import io
import json
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/NPCSkillReview/Audit'
OUT.mkdir(parents=True, exist_ok=True)
REGISTRY = u.AssetRegistryHelpers.get_asset_registry()
REGISTRY.search_all_assets(True)
ASSETS = REGISTRY.get_assets_by_path('/Game/IsekaiClass', recursive=True)
BY_NAME = {}
BY_PACKAGE = {}
for asset in ASSETS:
    BY_NAME.setdefault(str(asset.asset_name), []).append(str(asset.package_name))
    BY_PACKAGE[str(asset.package_name)] = asset
OPTIONS = u.AssetRegistryDependencyOptions()
OPTIONS.set_editor_property('include_hard_package_references', True)
OPTIONS.set_editor_property('include_soft_package_references', True)
NAMES = ['NS_LineExplosion_02', 'NS_Muzzle_Slash01', 'NS_Free_Spells_Buff_Healing',
         'NS_Free_Magic_Projectile1_Base', 'NS_ExplosionsCircle_06', 'NS_Free_Magic_Slash',
         'NS_Projectile_Arrow01', 'NS_Free_Magic_Attack1', 'NS_Free_Spells_Aura_Notes',
         'NS_Shield', 'NS_Projectile_Arrow05']
report = {'systems': [], 'missing_dependencies': {}, 'redirected_dependencies': {}, 'npc_appearance': [], 'sounds': []}
for index, name in enumerate(NAMES, 1):
    packages = [p for p in BY_NAME.get(name, []) if '/VFX/SampleVFX/' in p]
    entry = {'npc': f'NPC_R01_{index:03d}', 'source': packages, 'loaded': False, 'materials': [], 'textures': []}
    if len(packages) == 1:
        system = u.load_asset(packages[0])
        entry['loaded'] = bool(system)
        visited = set()
        queue = list(packages)
        while queue:
            package = queue.pop()
            if package in visited or package.startswith('/Script/'):
                continue
            visited.add(package)
            if not u.EditorAssetLibrary.does_asset_exist(package):
                candidates=BY_NAME.get(package.rsplit('/',1)[-1],[])
                if package.startswith('/Game/IsekaiClass/VFX/Combat/Skills/Free/') and len(candidates)==1:
                    restored=u.load_asset(package)
                    if restored:
                        report['redirected_dependencies'][package]=restored.get_path_name()
                        queue.extend(candidates)
                        continue
                report['missing_dependencies'][package] = {'candidates': BY_NAME.get(package.rsplit('/', 1)[-1], [])}
                continue
            asset = BY_PACKAGE.get(package)
            if asset:
                kind = str(asset.asset_class_path.asset_name)
                if kind.startswith('Material'):
                    entry['materials'].append(package)
                elif kind.startswith('Texture'):
                    texture = u.load_asset(package)
                    info = {'path': package, 'loaded': bool(texture)}
                    if isinstance(texture, u.Texture2D):
                        info['size'] = [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()]
                    entry['textures'].append(info)
            queue.extend(str(dep) for dep in REGISTRY.get_dependencies(package, OPTIONS)
                         if str(dep).startswith(('/Game/', '/Engine/')))
        entry['dependency_count'] = len(visited)
    report['systems'].append(entry)

for asset in ASSETS:
    if str(asset.asset_class_path.asset_name) in ('SoundWave', 'SoundCue', 'MetaSoundSource') and '/VFX/' in str(asset.package_name):
        report['sounds'].append(str(asset.package_name))

npc_table = u.load_asset('/Game/IsekaiClass/Data/DataTable/NPC/DT_IC_NPC')
for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(npc_table)):
    if row['ID'] not in [f'NPC_R01_{i:03d}' for i in range(1, 12)]:
        continue
    info = {'npc': row['ID'], 'mesh': row['Mesh'], 'portrait': row['Portrait'], 'materials': []}
    mesh = u.load_asset(row['Mesh'])
    if mesh:
        for slot in mesh.get_editor_property('materials'):
            material = slot.get_editor_property('material_interface')
            if not material:
                continue
            mat = {'path': material.get_path_name(), 'vectors': {}}
            try:
                for name in u.MaterialEditingLibrary.get_vector_parameter_names(material):
                    color = u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(material, name)
                    mat['vectors'][str(name)] = [color.r, color.g, color.b, color.a]
            except Exception as exc:
                mat['error'] = str(exc)
            info['materials'].append(mat)
    portrait = u.load_asset(row['Portrait'])
    if portrait:
        task = u.AssetExportTask()
        task.object = portrait
        task.filename = str(OUT / (row['ID'] + '.png'))
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        task.exporter = u.TextureExporterPNG()
        info['portrait_exported'] = u.Exporter.run_asset_export_task(task)
    report['npc_appearance'].append(info)

(OUT / 'Assets.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log(f'NPC_SKILL_ASSET_AUDIT_OK systems={len(report["systems"])} missing={len(report["missing_dependencies"])}')
