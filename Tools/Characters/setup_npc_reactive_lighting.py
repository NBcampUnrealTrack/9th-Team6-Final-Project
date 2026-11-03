"""R01 NPC material audit and player-matched world lighting conversion."""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive'
DEST='/Game/IsekaiClass/NPC/Materials/WorldReactive'
NPC_IDS=[f'NPC_R01_{number:03}' for number in range(1,12)]
PLAYER_MATERIALS='/Game/IsekaiClass/Player/Materials/WorldReactive'

def mesh_path(identifier):
    return f'/Game/IsekaiClass/NPC/SampleNPC/R01/{identifier}/SK_R01_{int(identifier[-3:])}'

def blueprint_path(identifier):
    return '/Game/IsekaiClass/NPC/Blueprints/R01/BP_'+identifier

def asset_file(asset):
    path=asset.get_path_name().split('.')[0] if not isinstance(asset,str) else asset.split('.')[0]
    if path.startswith('/Game/'):return ROOT/'Content'/Path(path[6:]+'.uasset')
    if path.startswith('/VRM4U/'):return ROOT/'Plugins/VRM4U/Content'/Path(path[7:]+'.uasset')
    return None

def source_material(identifier,material):
    if material.get_path_name().startswith(DEST+'/'):
        assert material.get_name().endswith('_World')
        material=u.load_asset('/Game/IsekaiClass/NPC/SampleNPC/R01/'+identifier+'/'+material.get_name()[:-6])
    assert material
    return material

def category(name,count):
    if any(word in name for word in ['Eye','Brow','Mouth']):return 'features'
    if 'Hair' in name:return 'hair'
    if count<=3 and 'Body' in name:return 'combined'
    if 'Face_' in name or 'Body_' in name:return 'skin'
    return 'cloth'

def effective(material,name,override):
    while isinstance(material,u.MaterialInstance):
        settings=material.get_editor_property('base_property_overrides')
        if settings.get_editor_property(override):return settings.get_editor_property(name)
        material=material.get_editor_property('parent')
    return material.get_editor_property(name)

def non_material_state(mesh):
    result={}
    for name in ['skeleton','physics_asset','post_process_anim_blueprint']:
        value=mesh.get_editor_property(name)
        result[name]=value.get_path_name() if value else None
    result['slots']=[str(slot.get_editor_property('material_slot_name')) for slot in mesh.get_editor_property('materials')]
    return result

def audit():
    OUT.mkdir(parents=True,exist_ok=True)
    result={'npcs':[],'complete':False}
    protected=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    protected+=list((ROOT/'Content/IsekaiClass/Player/Materials').rglob('*.uasset'))
    protected+=list((ROOT/'Content/IsekaiClass/Player/SamplePlayer').rglob('*.uasset'))
    for identifier in NPC_IDS:
        mesh=u.load_asset(mesh_path(identifier));assert isinstance(mesh,u.SkeletalMesh),identifier
        cls=u.EditorAssetLibrary.load_blueprint_class(blueprint_path(identifier));assert cls
        component=u.get_default_object(cls).get_editor_property('mesh')
        assert component.get_skeletal_mesh_asset()==mesh,(identifier,'Blueprint mesh mismatch')
        slots=list(mesh.get_editor_property('materials'));materials=[]
        for index,slot in enumerate(slots):
            material=source_material(identifier,slot.get_editor_property('material_interface'))
            current=material;parents=[]
            while current:
                file=asset_file(current)
                if file and file.is_file():protected.append(file)
                parents.append(current.get_path_name())
                current=current.get_editor_property('parent') if isinstance(current,u.MaterialInstance) else None
            texture=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(material,'gltf_tex_diffuse')
            tint=u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(material,'gltf_basecolor')
            materials.append({'slot':index,'source':material.get_path_name(),'category':category(material.get_name(),len(slots)),
                'texture':texture.get_path_name() if texture else None,'tint':[tint.r,tint.g,tint.b,tint.a],
                'parents':parents,'shader':str(effective(material,'shading_model','override_shading_model')),
                'blend':str(effective(material,'blend_mode','override_blend_mode'))})
        result['npcs'].append({'id':identifier,'mesh':mesh.get_path_name(),'blueprint':blueprint_path(identifier),
            'blueprint_overrides':[m.get_path_name() if m else None for m in component.get_editor_property('override_materials')],
            'non_material':non_material_state(mesh),'materials':materials})
    result['protected_hashes']={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in dict.fromkeys(protected)}
    result['complete']=True
    (OUT/'Audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result


def assert_protected(audit_result):
    for relative,digest in audit_result['protected_hashes'].items():
        assert hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()==digest,'Protected asset changed: '+relative


def create_materials(audit_result):
    assert '-ICApplyNPCReactiveLighting' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    lib=u.MaterialEditingLibrary;cache={}
    def template(source_path,name):
        path=DEST+'/Shared/'+name
        material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source_path,path)
        assert material
        cache[path]=material
        assert material.get_editor_property('shading_model')==u.MaterialShadingModel.MSM_DEFAULT_LIT
        assert material.get_editor_property('used_with_skeletal_mesh') and material.get_editor_property('used_with_morph_targets')
        return material
    body_root=template(PLAYER_MATERIALS+'/Shared/M_VrmMToonBaseOpaque_6dd88d','M_ICNPCBodyLit')
    feature_root=template(PLAYER_MATERIALS+'/Shared/M_ICAuthoredFeatureLit','M_ICNPCFeatureLit')
    def parent(source):
        if isinstance(source,u.Material):
            assert source.get_path_name()=='/VRM4U/MaterialUtil/UE5/Material/M_VrmMToonBaseOpaque.M_VrmMToonBaseOpaque'
            return body_root
        path=source.get_path_name()
        if path in cache:return cache[path]
        destination=DEST+'/Shared/'+source.get_name()+'_'+hashlib.sha256(path.encode()).hexdigest()[:6]
        target=u.load_asset(destination) if u.EditorAssetLibrary.does_asset_exist(destination) else u.EditorAssetLibrary.duplicate_asset(path,destination)
        assert target;cache[path]=target
        lib.set_material_instance_parent(target,parent(source.get_editor_property('parent')))
        lib.update_material_instance(target)
        return target
    result={}
    for npc in audit_result['npcs']:
        u.log('IC_NPC_LIGHTING_CREATE '+npc['id'])
        (OUT/'ApplyProgress.json').write_text(json.dumps({'stage':'materials','npc':npc['id']},indent=2),encoding='utf-8')
        result[npc['id']]=[]
        for row in npc['materials']:
            source=u.load_asset(row['source']);assert source
            path=DEST+'/'+npc['id']+'/'+source.get_name()+'_World'
            material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(row['source'],path)
            assert material;cache[path]=material
            features=row['category']=='features'
            lib.set_material_instance_parent(material,feature_root if features else parent(source.get_editor_property('parent')))
            overrides=material.get_editor_property('base_property_overrides')
            overrides.set_editor_property('override_shading_model',True)
            overrides.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
            if features:
                lib.clear_all_material_instance_parameters(material)
                for name,value in {'override_blend_mode':True,'blend_mode':u.BlendMode.BLEND_MASKED,
                    'override_two_sided':True,'two_sided':True,'override_opacity_mask_clip_value':True,'opacity_mask_clip_value':.333333}.items():
                    overrides.set_editor_property(name,value)
                texture=u.load_asset(row['texture']);assert texture
                lib.set_material_instance_texture_parameter_value(material,'ICFeatureAlbedoTexture',texture)
                lib.set_material_instance_vector_parameter_value(material,'ICFeatureAlbedoTint',u.LinearColor(*row['tint']))
            else:
                switches={str(name) for name in lib.get_static_switch_parameter_names(material)}
                for name,value in {'bUseLight':False,'bUseLightForce':False,'bUseFilmTonemapInverse':False,'bDivAdaptation':False,
                    'bUseMatCap':False,'bUseRimLight':False,'bUseNormalmap':True,'bUseCustomReflection':False,'bUseMRSA':True}.items():
                    assert name in switches,name
                    lib.set_material_instance_static_switch_parameter_value(material,name,value)
                for name,value in {'ColorScaleAllMat':1.,'NormalToCameraMat':0.,'NormalToLightMat':0.,'NormalFresnelScale':1.,'mtoon_NormalScale':.25}.items():
                    lib.set_material_instance_scalar_parameter_value(material,name,value)
                lib.set_material_instance_vector_parameter_value(material,'MRSA',u.LinearColor(0,.85,.03,1.))
            material.set_editor_property('base_property_overrides',overrides)
            lib.set_material_instance_scalar_parameter_value(material,'ICAlbedoScale',.65)
            lib.update_material_instance(material)
            result[npc['id']].append(material)
    for asset in reversed(list(cache.values())):
        assert asset.get_path_name().startswith(DEST+'/')
        assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    assert_protected(audit_result)
    return result


def apply(audit_result):
    assert '-ICApplyNPCReactiveLighting' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    assert len(audit_result['npcs'])==11
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    for npc in audit_result['npcs']:
        mesh=u.load_asset(npc['mesh']);assert non_material_state(mesh)==npc['non_material']
        assert not npc['blueprint_overrides'],'Unexpected Blueprint override: '+npc['id']
        shutil.copy2(asset_file(mesh),backup/asset_file(mesh).name)
    materials=create_materials(audit_result);rows=[]
    for npc in audit_result['npcs']:
        mesh=u.load_asset(npc['mesh']);slots=list(mesh.get_editor_property('materials'))
        u.log('IC_NPC_LIGHTING_SAVE_MESH '+npc['id'])
        assert len(slots)==len(materials[npc['id']])
        for index,(slot,material) in enumerate(zip(slots,materials[npc['id']])):
            previous=slot.get_editor_property('material_interface').get_path_name()
            slot.set_editor_property('material_interface',material)
            rows.append({'npc':npc['id'],'slot':index,'source':npc['materials'][index]['source'],'previous':previous,
                         'assigned':material.get_path_name(),'category':npc['materials'][index]['category']})
        mesh.set_editor_property('materials',slots)
        assert non_material_state(mesh)==npc['non_material']
        assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    assert_protected(audit_result)
    result={'npcs':NPC_IDS,'slot_count':len(rows),'slots':rows,'albedo_scale':.65,'backup':str(backup),
        'body_template':body_root_path(),'feature_template':feature_root_path(),
        'protected_hashes':audit_result['protected_hashes'],'world_player_and_original_materials_unchanged':True,
        'skeletal_physics_post_process_and_slot_layout_preserved':True}
    assert result['slot_count']==22
    (OUT/'Applied.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/'ApplyProgress.json').write_text(json.dumps({'stage':'complete','npc_count':11,'slot_count':22},indent=2),encoding='utf-8')
    return result


def body_root_path():return DEST+'/Shared/M_ICNPCBodyLit'
def feature_root_path():return DEST+'/Shared/M_ICNPCFeatureLit'


if __name__=='__main__':
    apply(audit())
