"""Unassigned NPC003 hair lighting experiments. Existing game assets are read-only."""
import hashlib,json
from pathlib import Path
import unreal as u
import setup_npc003_hair_only as current

ROOT=current.ROOT
OUT=current.OUT/'Research_20261103'
DEST='/Game/IsekaiClass/NPC/Materials/LightingResearch/NPC_R01_003'
METHODS=[
    {'id':'01_current','title':'현재 상태','kind':'current','scatter':.25,'density':.65,'cast_shadow':True,'note':'현재 저장된 머리카락 보정. 모든 비교의 기준.'},
    {'id':'02_geometry_normal','title':'머리카락 노멀맵 제거','kind':'geometry','scatter':.25,'density':.65,'cast_shadow':True,'note':'머리카락만 기본 메시 노멀 사용. 겹친 메시 자체의 그림자는 유지.'},
    {'id':'03_backface_normal','title':'머리카락 뒷면 노멀 보정','kind':'backface','scatter':.25,'density':.65,'cast_shadow':True,'note':'얇은 머리카락 뒷면의 노멀 반전을 보상. 원본 RGB와 그림자는 유지.'},
    {'id':'04_wide_scatter','title':'빛 산란 확대','kind':'scatter','scatter':.55,'density':.25,'cast_shadow':True,'note':'산란을 넓혀 짙은 음영 완화. 밝은 곳에서 머리가 더 하얗게 보일 수 있음.'},
    {'id':'05_thin_surface','title':'얇은 면용 빛 전달','kind':'foliage','scatter':.35,'density':.65,'cast_shadow':True,'note':'머리카락 UV만 Two Sided Foliage. 앞·뒷면 빛 전달을 비교.'},
    {'id':'06_preintegrated','title':'Preintegrated Skin 조명','kind':'skin','scatter':.35,'density':.5,'cast_shadow':True,'note':'머리카락 UV만 Preintegrated Skin. 다른 확산·투과 계산을 비교.'},
    {'id':'07_raw_diffuse','title':'원본 RGB의 단순 표면','kind':'raw','scatter':0.,'density':1.,'cast_shadow':True,'note':'머리카락의 MToon 색·노멀 계산을 건너뛰고 원본 RGB × 0.65를 Default Lit에 사용.'},
    {'id':'08_no_cast_shadow','title':'진단: 캐릭터 그림자 끄기','kind':'diagnostic','scatter':.25,'density':.65,'cast_shadow':False,'note':'그림자 원인 분리용. 캐릭터 전체의 바닥 그림자와 얼굴 자기 그림자도 없어짐. 머리카락 전용 후보가 아님.'},
]

def snapshot():
    OUT.mkdir(parents=True,exist_ok=True)
    path=OUT/'Scope.json'
    if path.exists():return json.loads(path.read_text(encoding='utf-8'))
    files=set()
    original=json.loads((current.original.OUT/'Applied.json').read_text(encoding='utf-8'))
    files.update(ROOT/rel for rel in original['protected_hashes'])
    for folder in ['Content/IsekaiClass/NPC/Materials','Content/IsekaiClass/NPC/SampleNPC/R01','Content/IsekaiClass/NPC/Blueprints/R01']:
        files.update(p for p in (ROOT/folder).rglob('*.uasset') if 'LightingResearch' not in p.parts)
    result={'protected_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)},
        'existing_assets_read_only':True,'trial_assets':DEST,'target':'NPC_R01_003'}
    path.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

def create():
    assert '-ICNPC003HairResearch' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot();current.original.assert_protected(scope)
    lib=u.MaterialEditingLibrary;assets=[];rows=[]
    source_body=u.load_asset(current.BODY)
    baseline=current.parameters(source_body);names={kind:list(values) for kind,values in baseline.items()}
    for method in METHODS:
        prefix=DEST+'/'+method['id'];master_path=prefix+'/M_HairResearch'
        master=u.load_asset(master_path) if u.EditorAssetLibrary.does_asset_exist(master_path) else u.EditorAssetLibrary.duplicate_asset(current.MASTER,master_path)
        assert master
        output=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        norm=lambda s:str(s).replace(' ','').lower()
        input_names=lib.get_material_expression_input_names(output)
        pins={norm(name):node for name,node in zip(input_names,lib.get_inputs_for_material_expression(master,output))}
        inp=lambda key:next(name for name in input_names if norm(name)==key)
        select=pins['shadingmodel'];assert isinstance(select,u.MaterialExpressionIf)
        select_inputs={norm(name):node for name,node in zip(lib.get_material_expression_input_names(select),lib.get_inputs_for_material_expression(master,select))}
        mask=select_inputs['a'];hair_model=select_inputs['a>b']
        assert isinstance(mask,u.MaterialExpressionCustom) and isinstance(hair_model,u.MaterialExpressionShadingModel)
        def node(cls):return lib.create_material_expression(master,cls)
        def connect(a,out,b,pin):assert lib.connect_material_expressions(a,out,b,pin),(str(b),pin)
        def split_output(expression,key):
            assert isinstance(expression,u.MaterialExpressionBreakMaterialAttributes)
            return next(name for name in lib.get_material_expression_output_names(expression) if norm(name)==key)
        def constant(value):
            n=node(u.MaterialExpressionConstant);n.set_editor_property('r',value);return n
        def colour(value):
            n=node(u.MaterialExpressionConstant3Vector);n.set_editor_property('constant',u.LinearColor(*value,1));return n
        def hair_lerp(key,expression,expression_output=''):
            original=pins[key];lerp=node(u.MaterialExpressionLinearInterpolate)
            source_out=split_output(original,key) if isinstance(original,u.MaterialExpressionBreakMaterialAttributes) else ''
            connect(original,source_out,lerp,'A');connect(expression,expression_output,lerp,'B');connect(mask,'',lerp,'Alpha')
            connect(lerp,'',output,inp(key))
        if output.get_editor_property('desc')!='IC Hair Research '+method['id']:
            kind=method['kind']
            if kind=='geometry':hair_lerp('normal',colour((0,0,1)))
            if kind=='backface':
                sign=node(u.MaterialExpressionTwoSidedSign);multiply=node(u.MaterialExpressionMultiply)
                connect(pins['normal'],split_output(pins['normal'],'normal'),multiply,'A');connect(sign,'',multiply,'B')
                hair_lerp('normal',multiply)
            if kind in ['foliage','skin','raw']:
                hair_model.set_editor_property('shading_model',{'foliage':u.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,
                    'skin':u.MaterialShadingModel.MSM_PREINTEGRATED_SKIN,'raw':u.MaterialShadingModel.MSM_DEFAULT_LIT}[kind])
            if kind=='raw':
                texture=node(u.MaterialExpressionTextureSampleParameter2D);texture.set_editor_property('parameter_name','ICResearchHairDiffuse')
                texture.set_editor_property('texture',lib.get_material_instance_texture_parameter_value(source_body,'gltf_tex_diffuse'))
                tint=node(u.MaterialExpressionVectorParameter);tint.set_editor_property('parameter_name','ICResearchHairTint')
                tint.set_editor_property('default_value',lib.get_material_instance_vector_parameter_value(source_body,'gltf_basecolor'))
                multiply=node(u.MaterialExpressionMultiply);connect(texture,'RGB',multiply,'A');connect(tint,'RGB',multiply,'B')
                scaled=node(u.MaterialExpressionMultiply);connect(multiply,'',scaled,'A');connect(constant(.65),'',scaled,'B')
                hair_lerp('basecolor',scaled);hair_lerp('normal',colour((0,0,1)))
                for key,value in [('roughness',1.),('specular',0.),('metallic',0.),('ambientocclusion',1.)]:hair_lerp(key,constant(value))
            output.set_editor_property('desc','IC Hair Research '+method['id'])
        errors=lib.recompile_material(master);assert not errors,str(errors)
        assets.append(master)
        def parent(source):
            if isinstance(source,u.Material):return master
            path=prefix+'/Parents/'+source.get_name()
            target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
            lib.set_material_instance_parent(target,parent(source.get_editor_property('parent')))
            settings=target.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
            target.set_editor_property('base_property_overrides',settings);lib.update_material_instance(target);assets.append(target)
            return target
        path=prefix+'/MI_HairResearch'
        material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(current.BODY,path)
        lib.set_material_instance_parent(material,parent(source_body.get_editor_property('parent')))
        settings=material.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
        material.set_editor_property('base_property_overrides',settings)
        assert current.parameters(material,names)==baseline,'Inherited parameters differ: '+method['id']
        for name,value in [('ICHairScatter',method['scatter']),('ICHairDensity',method['density'])]:lib.set_material_instance_scalar_parameter_value(material,name,value)
        lib.update_material_instance(material);assets.append(material)
        rows.append({**method,'material':path,'master':master_path,'emissive':0.})
    for asset in assets:
        assert asset.get_path_name().startswith(DEST+'/')
        assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    current.original.assert_protected(scope)
    (OUT/'Methods.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_NPC003_HAIR_RESEARCH_CREATED '+str(len(rows)))

if __name__=='__main__':create()
