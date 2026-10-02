import unreal, json
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/IsekaiClass/Characters/Player'], True)
result=[]
for data in registry.get_assets_by_path('/Game/IsekaiClass/Characters/Player', recursive=True):
    kind=str(data.asset_class_path.asset_name)
    if kind not in ['SkeletalMesh','Skeleton','AnimBlueprint','AnimSequence','Blueprint']:
        continue
    entry={'path':str(data.package_name),'class':kind}
    if kind=='SkeletalMesh':
        mesh=data.get_asset()
        entry['skeleton']=mesh.get_editor_property('skeleton').get_path_name()
        entry['bounds']=str(mesh.get_bounds())
        entry['materials']=[str(x.material_interface.get_path_name()) if x.material_interface else None for x in mesh.get_editor_property('materials')]
    result.append(entry)
with open(unreal.Paths.project_saved_dir()+'LocalPlayerAssets.json','w',encoding='utf-8') as f:
    json.dump(result,f,ensure_ascii=False,indent=2)
