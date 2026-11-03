"""Preserve VRM toon colour/face geometry, with physical light and shadow only."""
import hashlib,json,shutil
from datetime import datetime
from pathlib import Path
import unreal as u
import setup_player_world_lighting as SOURCE
import setup_player_part_lighting as PART
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
DEST='/Game/IsekaiClass/Player/Materials/WorldReactive'
OUT=ROOT/'Saved/CharacterLighting/WorldReactive'


def use_diffuse_albedo(sex,name):
    # The male head is one atlas containing eyes, lashes, mouth and skin.
    return PART.category(sex,name)=='features'


def remove_failed_feature_option(root,output):
    lib=u.MaterialEditingLibrary
    names=lib.get_material_expression_input_names(output)
    base_name=next(name for name in names if name.replace(' ','').lower()=='basecolor')
    choose=lib.get_inputs_for_material_expression(root,output)[names.index(base_name)]
    if not isinstance(choose,u.MaterialExpressionStaticSwitchParameter):return
    assert str(choose.get_editor_property('parameter_name'))=='ICUseDiffuseAlbedo'
    albedo,original=lib.get_inputs_for_material_expression(root,choose)
    colour=lib.get_inputs_for_material_expression(root,albedo)[0]
    texture,tint=lib.get_inputs_for_material_expression(root,colour)
    assert lib.connect_material_expressions(original,'',output,base_name)
    for expression in [choose,albedo,colour,texture,tint]:lib.delete_material_expression(root,expression)
    errors=lib.recompile_material(root);assert not errors,str(errors)


def create_feature_surface(albedo_scale):
    lib=u.MaterialEditingLibrary
    path=DEST+'/Shared/M_ICAuthoredFeatureLit'
    root=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_ICAuthoredFeatureLit',DEST+'/Shared',u.Material,u.MaterialFactoryNew())
    root.set_editor_property('used_with_skeletal_mesh',True)
    root.set_editor_property('used_with_morph_targets',True)
    root.set_editor_property('two_sided',True)
    root.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED)
    root.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    root.set_editor_property('use_material_attributes',True)
    prop=u.MaterialProperty.MP_MATERIAL_ATTRIBUTES
    old=lib.get_material_property_input_node(root,prop)
    if old and old.get_editor_property('desc')=='IC Authored Feature Surface':return root
    def node(cls):return lib.create_material_expression(root,cls)
    def connect(a,out,b,inp):assert lib.connect_material_expressions(a,out,b,inp)
    texture=node(u.MaterialExpressionTextureSampleParameter2D)
    texture.set_editor_property('parameter_name','ICFeatureAlbedoTexture')
    texture.set_editor_property('texture',u.load_asset('/VRM4U/MaterialUtil/T_DummyWhite'))
    tint=node(u.MaterialExpressionVectorParameter)
    tint.set_editor_property('parameter_name','ICFeatureAlbedoTint');tint.set_editor_property('default_value',u.LinearColor(1,1,1,1))
    scale=node(u.MaterialExpressionScalarParameter)
    scale.set_editor_property('parameter_name','ICAlbedoScale');scale.set_editor_property('default_value',albedo_scale)
    colour=node(u.MaterialExpressionMultiply);connect(texture,'RGB',colour,'A');connect(tint,'RGB',colour,'B')
    albedo=node(u.MaterialExpressionMultiply);connect(colour,'',albedo,'A');connect(scale,'',albedo,'B')
    opacity=node(u.MaterialExpressionMultiply);connect(texture,'A',opacity,'A');connect(tint,'A',opacity,'B')
    zero=node(u.MaterialExpressionConstant);zero.set_editor_property('r',0.)
    one=node(u.MaterialExpressionConstant);one.set_editor_property('r',1.)
    output=node(u.MaterialExpressionMakeMaterialAttributes);output.set_editor_property('desc','IC Authored Feature Surface')
    names=lib.get_material_expression_input_names(output)
    for name,expression in [('basecolor',albedo),('emissivecolor',zero),('metallic',zero),('specular',zero),('roughness',one),('opacitymask',opacity),('opacity',opacity)]:
        connect(expression,'',output,next(pin for pin in names if pin.replace(' ','').lower()==name))
    assert lib.connect_material_property(output,'',prop)
    errors=lib.recompile_material(root);assert not errors,str(errors)
    return root


def create_materials(albedo_scale=.65,save=True):
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    cache={}
    feature_surface=create_feature_surface(albedo_scale);cache[feature_surface.get_path_name()]=feature_surface
    def duplicate(source,sex=None):
        key=source.get_path_name()
        if key in cache:return cache[key]
        suffix=hashlib.sha256(key.encode()).hexdigest()[:6]
        path=f'{DEST}/{sex}/{source.get_name()}_World' if sex else f'{DEST}/Shared/{source.get_name()}_{suffix}'
        target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(key,path)
        assert target,path
        cache[key]=target
        if isinstance(source,u.MaterialInstanceConstant):
            u.MaterialEditingLibrary.set_material_instance_parent(target,duplicate(source.get_editor_property('parent')))
            u.MaterialEditingLibrary.update_material_instance(target)
            return target
        assert isinstance(target,u.Material) and target.get_editor_property('use_material_attributes')
        prop=u.MaterialProperty.MP_MATERIAL_ATTRIBUTES
        old=u.MaterialEditingLibrary.get_material_property_input_node(target,prop)
        if old.get_editor_property('desc')=='IC World Reactive Lit':
            remove_failed_feature_option(target,old)
            return target
        output_name=u.MaterialEditingLibrary.get_material_property_input_node_output_name(target,prop)
        def node(cls):return u.MaterialEditingLibrary.create_material_expression(target,cls)
        def connect(a,out,b,inp):assert u.MaterialEditingLibrary.connect_material_expressions(a,out,b,inp)
        split=node(u.MaterialExpressionBreakMaterialAttributes)
        connect(old,output_name,split,u.MaterialEditingLibrary.get_material_expression_input_names(split)[0])
        output=node(u.MaterialExpressionMakeMaterialAttributes);output.set_editor_property('desc','IC World Reactive Lit')
        scale=node(u.MaterialExpressionScalarParameter);scale.set_editor_property('parameter_name','ICAlbedoScale');scale.set_editor_property('default_value',albedo_scale)
        multiply=node(u.MaterialExpressionMultiply)
        zero=node(u.MaterialExpressionConstant);zero.set_editor_property('r',0.)
        outs=u.MaterialEditingLibrary.get_material_expression_output_names(split)
        ins=u.MaterialEditingLibrary.get_material_expression_input_names(output)
        norm=lambda n:n.replace(' ','').lower()
        outpin=lambda n:next(x for x in outs if norm(x)==n)
        inpin=lambda n:next(x for x in ins if norm(x)==n)
        for inp in ins:
            if norm(inp) in ['basecolor','emissivecolor']:continue
            matching=next((name for name in outs if norm(name)==norm(inp)),None)
            if matching:connect(split,matching,output,inp)
        # The original Unlit graph supplies authored colour; it no longer adds light itself.
        connect(split,outpin('emissivecolor'),multiply,'A');connect(scale,'',multiply,'B')
        connect(multiply,'',output,inpin('basecolor'))
        connect(zero,'',output,inpin('emissivecolor'))
        assert u.MaterialEditingLibrary.connect_material_property(output,'',prop)
        target.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
        errors=u.MaterialEditingLibrary.recompile_material(target);assert not errors,str(errors)
        return target
    result={}
    for sex,materials in SOURCE.source_materials().items():
        result[sex]=[]
        for source in materials:
            part=PART.category(sex,source.get_name());face=part in ['features','skin']
            if use_diffuse_albedo(sex,source.get_name()):
                path=f'{DEST}/{sex}/{source.get_name()}_World'
                material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
                cache[source.get_path_name()]=material
                u.MaterialEditingLibrary.set_material_instance_parent(material,feature_surface)
                overrides=material.get_editor_property('base_property_overrides')
                for name,value in {'override_shading_model':True,'shading_model':u.MaterialShadingModel.MSM_DEFAULT_LIT,
                    'override_blend_mode':True,'blend_mode':u.BlendMode.BLEND_MASKED,'override_two_sided':True,'two_sided':True,
                    'override_opacity_mask_clip_value':True,'opacity_mask_clip_value':.333333}.items():overrides.set_editor_property(name,value)
                material.set_editor_property('base_property_overrides',overrides)
                texture=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(source,'gltf_tex_diffuse');assert texture
                tint=u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(source,'gltf_basecolor')
                u.MaterialEditingLibrary.set_material_instance_texture_parameter_value(material,'ICFeatureAlbedoTexture',texture)
                u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material,'ICFeatureAlbedoTint',tint)
                u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material,'ICAlbedoScale',albedo_scale)
                u.MaterialEditingLibrary.update_material_instance(material)
                result[sex].append(material)
                continue
            material=duplicate(source,sex)
            overrides=material.get_editor_property('base_property_overrides')
            overrides.set_editor_property('override_shading_model',True);overrides.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
            material.set_editor_property('base_property_overrides',overrides)
            available={str(n) for n in u.MaterialEditingLibrary.get_static_switch_parameter_names(material)}
            for name,value in {'bUseLight':False,'bUseLightForce':False,'bUseFilmTonemapInverse':False,'bDivAdaptation':False,'bUseMatCap':False,'bUseRimLight':False,'bUseNormalmap':not face,'bUseCustomReflection':False,'bUseMRSA':True}.items():
                if name in available:u.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(material,name,value)
            for name,value in {'ICAlbedoScale':albedo_scale,'ColorScaleAllMat':1.,'NormalToCameraMat':.95 if face else 0.,'NormalToLightMat':0.,'NormalFresnelScale':2. if face else 1.,'mtoon_NormalScale':.15 if part=='hair' else .25}.items():
                u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material,name,value)
            u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material,'MRSA',u.LinearColor(0,1. if face else .85,0. if face else .03,1.))
            u.MaterialEditingLibrary.update_material_instance(material)
            result[sex].append(material)
    if save:
        for asset in reversed(list(cache.values())):assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    return result,list(cache.values())


def apply(materials):
    assert '-ICApplyReactiveLighting' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    OUT.mkdir(parents=True,exist_ok=True)
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    files=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    rows=[]
    for sex,folder in SOURCE.PLAYERS:
        path=SOURCE.mesh_path(sex,folder);file=ROOT/'Content'/Path(path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(path);slots=list(mesh.get_editor_property('materials'))
        assert len(slots)==len(materials[sex])
        for index,(slot,material) in enumerate(zip(slots,materials[sex])):
            rows.append({'sex':sex,'slot':index,'previous':slot.get_editor_property('material_interface').get_path_name(),'material':material.get_path_name(),
                         'direct_diffuse_albedo':use_diffuse_albedo(sex,material.get_name())})
            slot.set_editor_property('material_interface',material)
        mesh.set_editor_property('materials',slots);assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    for rel,digest in hashes.items():assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==digest,rel
    result={'mode':'Default Lit, authored toon albedo, emissive zero','albedo_scale':.65,
            'feature_colour':'Original diffuse RGB * gltf_basecolor * ICAlbedoScale; original alpha retained; emissive zero',
            'slots':rows,'backup':str(backup),'world_hashes':hashes,'world_files_unchanged':True}
    (OUT/'Applied.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result


if __name__=='__main__':
    assert '-ICApplyReactiveLighting' in u.SystemLibrary.get_command_line()
    materials,assets=create_materials(.65)
    apply(materials)
    u.log('IC_PLAYER_REACTIVE_LIGHTING_APPLIED')
