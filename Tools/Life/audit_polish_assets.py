import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=root/'Saved/Polish';out.mkdir(exist_ok=True)
registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
data={'meshes':{},'animations':[],'profiles':{}}
for name in ['SM_chest_01','SM_chest_02','SM_chest_03']:
    mesh=u.load_asset('/Game/IsekaiClass/World/SampleLevel/Medieval_House/meshes/props/'+name)
    data['meshes'][name]={'box':str(mesh.get_bounding_box()),'materials':[str(v.material_interface.get_path_name()) for v in mesh.get_editor_property('static_materials')]}
for folder in ['/Game/IsekaiClass/Animation','/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Animations']:
    for asset in registry.get_assets_by_path(folder,True):
        if str(asset.asset_class_path.asset_name)=='AnimSequence' and any(word in str(asset.asset_name).lower() for word in ['jump','sleep','lying','lie_','get_up','getup','death','stand_up','sit_to']):
            data['animations'].append(str(asset.package_name))
for code in ['Male','Female']:
    profile=u.load_asset('/Game/IsekaiClass/Animation/Player/Main'+code+'/DA_ANIM_PLAYER_'+('M' if code=='Male' else 'F'))
    unarmed=profile.get_editor_property('Unarmed')
    data['profiles'][code]={'sleep':str(profile.get_editor_property('Sleep')),'unarmed':{key:str(unarmed.get_editor_property(key)) for key in ['Jump','JumpStart','FallLoop','Land']}}
(out/'Assets.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');u.log('POLISH_ASSET_AUDIT_OK')
