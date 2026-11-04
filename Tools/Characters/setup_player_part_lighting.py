"""Player-only hybrid toon/Lit materials with independent surface tuning."""
import sys,json
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
DEST='/Game/IsekaiClass/Player/Materials/PartLighting'
sys.path.insert(0,str(ROOT/'Tools/Characters'))
import setup_player_world_lighting as SOURCE

def category(sex,name):
    if sex=='M' and 'Body' in name:return 'combined'
    if any(n in name for n in ['Eye','Brow','Mouth']):return 'features'
    if 'Face_' in name or 'Body_' in name:return 'skin'
    if 'Hair' in name:return 'hair'
    return 'cloth'

def create_hybrid():
    """Preserve authored MToon colour with a controlled Lit term and soft fill."""
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    original=SOURCE.source_materials();cache={}
    def duplicate(source,leaf=None):
        key=source.get_path_name()
        if key in cache:return cache[key]
        import hashlib
        name=source.get_name()+'_'+hashlib.sha256(key.encode()).hexdigest()[:6]
        path=f'{DEST}/{leaf}/{source.get_name()}_Part' if leaf else f'{DEST}/Shared/{name}'
        target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(key,path)
        assert target;cache[key]=target
        if isinstance(source,u.MaterialInstanceConstant):
            u.MaterialEditingLibrary.set_material_instance_parent(target,duplicate(source.get_editor_property('parent')))
            u.MaterialEditingLibrary.update_material_instance(target)
            return target
        assert isinstance(target,u.Material)
        prop=u.MaterialProperty.MP_MATERIAL_ATTRIBUTES
        old=u.MaterialEditingLibrary.get_material_property_input_node(target,prop)
        if old.get_editor_property('desc')=='IC Part Toon Lighting':return target
        old_output=u.MaterialEditingLibrary.get_material_property_input_node_output_name(target,prop)
        def node(cls):return u.MaterialEditingLibrary.create_material_expression(target,cls)
        def connect(a,out,b,inp):assert u.MaterialEditingLibrary.connect_material_expressions(a,out,b,inp),(a.get_name(),out,b.get_name(),inp)
        def scalar(name,value):
            e=node(u.MaterialExpressionScalarParameter);e.set_editor_property('parameter_name',name);e.set_editor_property('default_value',value);return e
        def vector(name,value):
            e=node(u.MaterialExpressionVectorParameter);e.set_editor_property('parameter_name',name);e.set_editor_property('default_value',u.LinearColor(*value));return e
        split=node(u.MaterialExpressionBreakMaterialAttributes)
        connect(old,old_output,split,u.MaterialEditingLibrary.get_material_expression_input_names(split)[0])
        outs=u.MaterialEditingLibrary.get_material_expression_output_names(split)
        getout=lambda text:next(n for n in outs if n.replace(' ','').lower()==text.lower())
        profile=vector('ICPartProfile',(.55,.12,.85,.03))
        skin=vector('ICSkinProfile',(.78,.045,1.,0.))
        features=vector('ICFeatureProfile',(.85,.025,1.,0.))
        hair=vector('ICHairProfile',(.62,.09,.70,.04))
        atlas=scalar('ICAtlasMode',0.)
        uv=node(u.MaterialExpressionTextureCoordinate)
        choose=node(u.MaterialExpressionCustom)
        choose.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT4)
        choose.set_editor_property('code',"float4 p=DefaultProfile; if(Mode>1.5){if(UV.y>0.70)p=Skin;}else if(Mode>0.5){if(UV.x<0.5 && UV.y<0.5)p=Skin;else if(UV.x>0.5 && UV.y<0.23)p=Hair;else p=Features;}return p;")
        inputs=[]
        for name in ['UV','Mode','DefaultProfile','Skin','Features','Hair']:
            item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
        choose.set_editor_property('inputs',inputs)
        for name,e,out in [('UV',uv,''),('Mode',atlas,''),('DefaultProfile',profile,'RGBA'),('Skin',skin,'RGBA'),('Features',features,'RGBA'),('Hair',hair,'RGBA')]:connect(e,out,choose,name)
        def channel(letter):
            e=node(u.MaterialExpressionComponentMask)
            for c in ['r','g','b','a']:e.set_editor_property(c,c==letter)
            connect(choose,'',e,u.MaterialEditingLibrary.get_material_expression_input_names(e)[0]);return e
        fill=channel('r');response=channel('g');roughness=channel('b');specular=channel('a')
        brightness=scalar('ICCharacterBrightness',1.)
        def multiply(a,aout,b,bout=''):
            e=node(u.MaterialExpressionMultiply);connect(a,aout,e,'A');connect(b,bout,e,'B');return e
        lit=multiply(split,getout('EmissiveColor'),response)
        emissive=multiply(multiply(split,getout('EmissiveColor'),fill),'',brightness)
        output=node(u.MaterialExpressionMakeMaterialAttributes);output.set_editor_property('desc','IC Part Toon Lighting')
        ins=u.MaterialEditingLibrary.get_material_expression_input_names(output)
        skip=['basecolor','emissivecolor','roughness','specular','metallic']
        for name in ins:
            normalized=name.replace(' ','').lower()
            matching=next((n for n in outs if n.replace(' ','').lower()==normalized),None)
            if matching and normalized not in skip:connect(split,matching,output,name)
        for text,e in [('basecolor',lit),('emissivecolor',emissive),('roughness',roughness),('specular',specular)]:
            connect(e,'',output,next(n for n in ins if n.replace(' ','').lower()==text))
        assert u.MaterialEditingLibrary.connect_material_property(output,'',prop)
        target.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
        u.MaterialEditingLibrary.delete_unused_expressions(target)
        u.MaterialEditingLibrary.recompile_material(target)
        return target
    converted={};rows=[]
    profiles={'features':(.85,.025,1.,0.),'skin':(.78,.045,1.,0.),'hair':(.62,.09,.70,.04),'cloth':(.55,.12,.85,.03),'combined':(.55,.12,.85,.03)}
    for sex,materials in original.items():
        converted[sex]=[]
        for source in materials:
            target=duplicate(source,sex);part=category(sex,source.get_name())
            overrides=target.get_editor_property('base_property_overrides')
            overrides.set_editor_property('override_shading_model',True);overrides.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
            target.set_editor_property('base_property_overrides',overrides)
            scalar=lambda n,v:u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(target,n,v)
            switch=lambda n,v:u.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(target,n,v)
            switch('bUseLight',False)
            switch('bUseMatCap',False)
            switch('bUseRimLight',False)
            switch('bUseNormalmap',False if part in ['features','skin'] else True)
            scalar('NormalToCameraMat',.75 if part in ['features','skin'] else 0.)
            scalar('NormalToLightMat',0.)
            scalar('mtoon_NormalScale',.15 if part=='hair' else .25)
            scalar('ICAtlasMode',1. if sex=='M' and part=='features' else 2. if sex=='M' else 0.)
            u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(target,'ICPartProfile',u.LinearColor(*profiles[part]))
            u.MaterialEditingLibrary.update_material_instance(target)
            converted[sex].append(target);rows.append({'sex':sex,'material':target.get_path_name(),'category':part,'profile':profiles[part]})
    for asset in reversed(list(cache.values())):assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    out=ROOT/'Saved/CharacterLighting/PartLighting'
    (out/'HybridMaterials.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
    return converted,list(cache.values())


def apply_to_meshes(converted,assets):
    import hashlib,shutil
    from datetime import datetime
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    out=ROOT/'Saved/CharacterLighting/PartLighting'
    backup=out/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    protected=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    for asset in assets:
        assert asset.get_path_name().startswith(DEST+'/')
        assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    rows=[]
    for sex,folder in [('M','Male'),('F','Female')]:
        path=f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/SK_Player_{sex}'
        file=ROOT/'Content'/Path(path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(path);slots=list(mesh.get_editor_property('materials'));assert len(slots)==len(converted[sex])
        for i,(slot,material) in enumerate(zip(slots,converted[sex])):
            assert SOURCE.effective_shading_model(material)==u.MaterialShadingModel.MSM_DEFAULT_LIT
            slot.set_editor_property('material_interface',material)
            rows.append({'sex':sex,'slot':i,'material':material.get_path_name()})
        mesh.set_editor_property('materials',slots);assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    for path,digest in hashes.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest
    result={'mode':'Original MToon colour with per-part Default Lit response and controlled toon fill','night_fill_multiplier':.6,'world_files_unchanged':True,'world_hashes':hashes,'backup':str(backup),'slots':rows,'male_atlas_masks':'Face atlas UV0 skin/features/hair regions; body atlas UV0 lower skin region'}
    (out/'Applied.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    return result
