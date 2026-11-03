"""Player-only, reversible Unlit -> PBR Lit conversion. Never saves world assets.

Run with -ICApplyPlayerWorldLighting or -ICRestorePlayerUnlit in an editor Python
session. Original materials/textures and all shared VRM4U parents are preserved.
"""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting'
DEST='/Game/IsekaiClass/Player/Materials/WorldLighting'
PLAYERS=[('M','Male'),('F','Female')]
PROTECTED=['Config/DefaultEngine.ini','Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
           'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset',
           'Plugins/VRM4U/Content/MaterialUtil/MI_VrmSimplePBR.uasset',
           'Plugins/VRM4U/Content/MaterialUtil/MI_VrmSimplePBRTransparent.uasset']

def mesh_path(sex,folder):
    return f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/SK_Player_{sex}'

def source_materials():
    result={}
    for sex,folder in PLAYERS:
        mesh=u.load_asset(mesh_path(sex,folder));assert mesh
        result[sex]=[]
        for slot in mesh.get_editor_property('materials'):
            material=slot.get_editor_property('material_interface');assert material
            if material.get_path_name().startswith('/Game/IsekaiClass/Player/Materials/WorldReactive/'):
                assert material.get_name().endswith('_World')
                material=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/'+material.get_name()[:-6])
                assert material
            if material.get_path_name().startswith('/Game/IsekaiClass/Player/Materials/SingleSunLighting/'):
                assert material.get_name().endswith('_Sun')
                material=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/'+material.get_name()[:-4])
                assert material
            if material.get_path_name().startswith('/Game/IsekaiClass/Player/Materials/PartLighting/'):
                assert material.get_name().endswith('_Part')
                material=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/'+material.get_name()[:-5])
                assert material
            if material.get_path_name().startswith('/Game/IsekaiClass/Player/Materials/ToonLighting/'):
                assert material.get_name().endswith('_Toon')
                material=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/'+material.get_name()[:-5])
                assert material
            if material.get_path_name().startswith(DEST+'/'):
                assert material.get_name().endswith('_Lit')
                material=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/'+material.get_name()[:-4])
                assert material
            result[sex].append(material)
    return result

def create_lit_materials(original=None):
    raise RuntimeError('PBR conversion withdrawn: it changed the original toon appearance. Original player materials are restored.')
    # Historical implementation below is retained for the rejected comparison.
    assert not u.EditorLevelLibrary.get_pie_worlds(False),'Create material assets outside PIE'
    original=original or source_materials()
    converted={}
    for sex,materials in original.items():
        converted[sex]=[]
        for source in materials:
            path=f'{DEST}/{sex}/'+source.get_name()+'_Lit'
            target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
            assert target,path
            transparent='Translucent' in source.get_editor_property('parent').get_name()
            parent=u.load_asset('/VRM4U/MaterialUtil/MI_VrmSimplePBR'+('Transparent' if transparent else ''))
            assert parent
            u.MaterialEditingLibrary.set_material_instance_parent(target,parent)
            overrides=target.get_editor_property('base_property_overrides')
            overrides.set_editor_property('override_shading_model',True)
            overrides.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
            overrides.set_editor_property('override_two_sided',True)
            overrides.set_editor_property('two_sided',source.get_editor_property('base_property_overrides').get_editor_property('two_sided'))
            target.set_editor_property('base_property_overrides',overrides)
            diffuse=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(source,'gltf_tex_diffuse')
            normal=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(source,'mtoon_tex_Normal')
            assert diffuse,source.get_path_name()
            u.MaterialEditingLibrary.set_material_instance_texture_parameter_value(target,'gltf_tex_diffuse',diffuse)
            if normal:u.MaterialEditingLibrary.set_material_instance_texture_parameter_value(target,'gltf_tex_normal',normal)
            # Real emission is disabled in the player copy, not in shared parents.
            u.MaterialEditingLibrary.set_material_instance_texture_parameter_value(target,'gltf_tex_emission',u.load_asset('/VRM4U/MaterialUtil/T_DummyBlack'))
            u.MaterialEditingLibrary.update_material_instance(target)
            converted[sex].append(target)
    return converted

def effective_shading_model(material):
    current=material
    while isinstance(current,u.MaterialInstance):
        settings=current.get_editor_property('base_property_overrides')
        if settings.get_editor_property('override_shading_model'):
            return settings.get_editor_property('shading_model')
        current=current.get_editor_property('parent')
    return current.get_editor_property('shading_model')

def apply(restore=False):
    if not restore:
        raise RuntimeError('PBR conversion withdrawn. Only -ICRestorePlayerUnlit is supported.')
    assert not u.EditorLevelLibrary.get_pie_worlds(False),'Save player assets only after PIE ends'
    OUT.mkdir(parents=True,exist_ok=True)
    before={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in PROTECTED if (ROOT/path).exists()}
    original=source_materials()
    materials=original if restore else create_lit_materials(original)
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    rows=[]
    for sex,folder in PLAYERS:
        asset_path=mesh_path(sex,folder)
        file=ROOT/'Content'/Path(asset_path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(asset_path);slots=list(mesh.get_editor_property('materials'))
        assert len(slots)==len(materials[sex])
        for index,(slot,material) in enumerate(zip(slots,materials[sex])):
            if not restore:
                assert material.get_path_name().startswith(DEST+'/')
                assert effective_shading_model(material)==u.MaterialShadingModel.MSM_DEFAULT_LIT
                assert u.EditorAssetLibrary.save_loaded_asset(material,False)
            slot.set_editor_property('material_interface',material)
            rows.append({'sex':sex,'slot':index,'source':original[sex][index].get_path_name(),'assigned':material.get_path_name(),
                         'shading_model':str(effective_shading_model(material))})
        mesh.set_editor_property('materials',slots)
        assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    for path,digest in before.items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,'World/shared shader changed: '+path
    result={'restored_unlit':restore,'player_slots':rows,'backup':str(backup),'protected_hashes':before,'protected_files_unchanged':True}
    (OUT/('Restored.json' if restore else 'Applied.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

if __name__=='__main__':
    command=u.SystemLibrary.get_command_line()
    assert '-ICApplyPlayerWorldLighting' in command or '-ICRestorePlayerUnlit' in command
    apply(restore='-ICRestorePlayerUnlit' in command)
    u.log('IC_PLAYER_WORLD_LIGHTING_APPLIED')
