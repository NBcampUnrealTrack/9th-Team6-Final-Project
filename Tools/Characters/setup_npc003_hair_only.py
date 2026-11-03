"""NPC 003-only hair UV softening; original face/eyes stay byte-identical."""
import hashlib,json,shutil
from datetime import datetime
import unreal as u
import setup_npc_reactive_lighting as original

ROOT=original.ROOT
OUT=original.OUT/'NPC003HairOnly'
FOLDER=original.DEST+'/NPC_R01_003'
MASTER=FOLDER+'/M_ICNPC003HairOnly'
HEAD=FOLDER+'/MI_N00_000_00_FaceMouth_00_FACE__Instance__World'
BODY=FOLDER+'/MI_N00_000_00_Body_00_SKIN__Instance__World'
PREVIEW=FOLDER+'/MI_NPC003HairOnlyPreview'
RECTS=[(.5001,.5001,.7499,.9999),(.765,.52,.995,.74)]

def parameters(material,names=None):
    lib=u.MaterialEditingLibrary
    if names is None:
        names={kind:[str(n) for n in getattr(lib,'get_'+kind+'_parameter_names')(material)] for kind in ['scalar','vector','texture','static_switch']}
    result={}
    for kind,keys in names.items():
        values={}
        for name in keys:
            value=getattr(lib,'get_material_instance_'+kind+'_parameter_value')(material,name)
            if kind=='vector':value=[value.r,value.g,value.b,value.a]
            elif kind=='texture':value=value.get_path_name() if value else None
            values[name]=value
        result[kind]=values
    return result

def snapshot():
    OUT.mkdir(parents=True,exist_ok=True)
    path=OUT/'Scope.json'
    if path.exists():return json.loads(path.read_text(encoding='utf-8'))
    initial=json.loads((original.OUT/'Applied.json').read_text(encoding='utf-8'))
    files={ROOT/p for p in initial['protected_hashes']}
    files.update((ROOT/'Content/IsekaiClass/NPC/Materials/WorldReactive').rglob('*.uasset'))
    files.update((ROOT/'Content/IsekaiClass/NPC/SampleNPC/R01').rglob('*.uasset'))
    files.discard(original.asset_file(BODY))
    files.discard(original.asset_file(MASTER));files.discard(original.asset_file(PREVIEW))
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    for asset in [HEAD,BODY]:shutil.copy2(original.asset_file(asset),backup/original.asset_file(asset).name)
    result={'protected_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)},
        'backup':str(backup),'face_baseline_hash':hashlib.sha256(original.asset_file(HEAD).read_bytes()).hexdigest(),
        'hair_uv_rectangles':RECTS,'scope':'NPC_R01_003 hair UV islands only; face, eyes and other NPCs preserved'}
    path.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

def create():
    assert '-ICReviewNPC003HairOnly' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot()
    lib=u.MaterialEditingLibrary
    master=u.load_asset(MASTER) if u.EditorAssetLibrary.does_asset_exist(MASTER) else u.EditorAssetLibrary.duplicate_asset(original.body_root_path(),MASTER)
    master.set_editor_property('shading_model',u.MaterialShadingModel.MSM_FROM_MATERIAL_EXPRESSION)
    old=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if old.get_editor_property('desc')!='IC NPC003 Hair Only':
        def node(cls):return lib.create_material_expression(master,cls)
        def connect(a,out,b,inp):assert lib.connect_material_expressions(a,out,b,inp),(str(b),inp)
        def scalar(name,value):
            n=node(u.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value);return n
        split=node(u.MaterialExpressionBreakMaterialAttributes)
        connect(old,'',split,lib.get_material_expression_input_names(split)[0])
        output=node(u.MaterialExpressionMakeMaterialAttributes);output.set_editor_property('desc','IC NPC003 Hair Only')
        norm=lambda s:str(s).replace(' ','').lower()
        ins=lib.get_material_expression_input_names(output);outs=lib.get_material_expression_output_names(split)
        inp=lambda key:next(n for n in ins if norm(n)==key)
        out=lambda key:next(n for n in outs if norm(n)==key)
        for pin in ins:
            match=next((p for p in outs if norm(p)==norm(pin)),None)
            if match:connect(split,match,output,pin)
        uv=node(u.MaterialExpressionTextureCoordinate)
        mask=node(u.MaterialExpressionCustom);mask.set_editor_property('description','NPC003 hair atlas UV islands only')
        mask.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
        ci=u.CustomInput();ci.set_editor_property('input_name','UV');mask.set_editor_property('inputs',[ci])
        terms=[f'(step({x0},UV.x)*step(UV.x,{x1})*step({y0},UV.y)*step(UV.y,{y1}))' for x0,y0,x1,y1 in RECTS]
        mask.set_editor_property('code','return saturate('+ '+'.join(terms)+');')
        connect(uv,'',mask,'UV')
        diffuse=node(u.MaterialExpressionShadingModel);diffuse.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
        scattering=node(u.MaterialExpressionShadingModel);scattering.set_editor_property('shading_model',u.MaterialShadingModel.MSM_SUBSURFACE)
        select=node(u.MaterialExpressionIf);select.set_editor_property('const_b',.5)
        select_names=lib.get_material_expression_input_names(select);u.log('IC_HAIR_IF_PINS '+str(select_names))
        branch=lambda key:next(n for n in select_names if norm(n)==key)
        connect(mask,'',select,'A')
        for key,value in [('a>b',scattering),('a==b',diffuse),('a<b',diffuse)]:connect(value,'',select,branch(key))
        connect(select,'',output,inp('shadingmodel'))
        scatter=scalar('ICHairScatter',.55);density=scalar('ICHairDensity',.45)
        transmission=node(u.MaterialExpressionMultiply);connect(split,out('basecolor'),transmission,'A');connect(scatter,'',transmission,'B')
        masked=node(u.MaterialExpressionMultiply);connect(transmission,'',masked,'A');connect(mask,'',masked,'B')
        connect(masked,'',output,inp('subsurfacecolor'))
        opacity=node(u.MaterialExpressionLinearInterpolate);connect(split,out('opacity'),opacity,'A');connect(density,'',opacity,'B');connect(mask,'',opacity,'Alpha')
        connect(opacity,'',output,inp('opacity'))
        assert lib.connect_material_property(output,'',u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    errors=lib.recompile_material(master);assert not errors,str(errors)
    cache=[master]
    def parent(source):
        if isinstance(source,u.Material):return master
        dest=FOLDER+'/HairOnlyParents/'+source.get_name()
        target=u.load_asset(dest) if u.EditorAssetLibrary.does_asset_exist(dest) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),dest)
        lib.set_material_instance_parent(target,parent(source.get_editor_property('parent')))
        settings=target.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
        target.set_editor_property('base_property_overrides',settings);lib.update_material_instance(target);cache.append(target)
        return target
    body=u.load_asset(BODY);chain=parent(body.get_editor_property('parent'))
    preview=u.load_asset(PREVIEW) if u.EditorAssetLibrary.does_asset_exist(PREVIEW) else u.EditorAssetLibrary.duplicate_asset(BODY,PREVIEW)
    lib.set_material_instance_parent(preview,chain)
    settings=preview.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
    preview.set_editor_property('base_property_overrides',settings);lib.update_material_instance(preview);cache.append(preview)
    for asset in cache:assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    original.assert_protected(scope)
    assert hashlib.sha256(original.asset_file(HEAD).read_bytes()).hexdigest()==scope['face_baseline_hash']
    (OUT/'Created.json').write_text(json.dumps({'master':MASTER,'preview':PREVIEW,'body_parent':chain.get_path_name(),'face_unchanged_exact_bytes':True},indent=2),encoding='utf-8')
    u.log('IC_NPC003_HAIR_ONLY_CREATED')

def apply():
    assert '-ICApplyNPC003HairOnly' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot();original.assert_protected(scope)
    choice=json.loads((OUT/'Chosen.json').read_text(encoding='utf-8'))
    created=json.loads((OUT/'Created.json').read_text(encoding='utf-8'))
    lib=u.MaterialEditingLibrary;material=u.load_asset(BODY)
    baseline=parameters(material);names={kind:list(values) for kind,values in baseline.items()}
    assert parameters(u.load_asset(PREVIEW),names)==baseline,'Inherited body parameters differ'
    lib.set_material_instance_parent(material,u.load_asset(created['body_parent']))
    settings=material.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
    material.set_editor_property('base_property_overrides',settings)
    for name,value in [('ICHairScatter',choice['scatter']),('ICHairDensity',choice['density'])]:lib.set_material_instance_scalar_parameter_value(material,name,value)
    lib.update_material_instance(material);assert u.EditorAssetLibrary.save_loaded_asset(material,False)
    assert parameters(material,names)==baseline,'Original body parameters changed'
    (OUT/'BodyParameters.json').write_text(json.dumps(baseline,indent=2),encoding='utf-8')
    assert hashlib.sha256(original.asset_file(HEAD).read_bytes()).hexdigest()==scope['face_baseline_hash']
    original.assert_protected(scope)
    (OUT/'Applied.json').write_text(json.dumps({**choice,'face_unchanged_exact_bytes':True,'hair_uv_only':True,'other_npcs_player_world_unchanged':True,'backup':scope['backup']},indent=2),encoding='utf-8')
    u.log('IC_NPC003_HAIR_ONLY_APPLIED')

if __name__=='__main__':
    if '-ICApplyNPC003HairOnly' in u.SystemLibrary.get_command_line():apply()
    else:create()
