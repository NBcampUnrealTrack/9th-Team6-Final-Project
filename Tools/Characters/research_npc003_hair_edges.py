"""Target remaining lines: UV coverage, alpha fringes, mips, and GI diagnostics."""
import hashlib,json
import unreal as u
import research_npc003_hair_setup as prior
current=prior.current
ROOT=prior.ROOT
OUT=prior.current.OUT/'EdgeStudy_20261103'
DEST=prior.DEST+'/EdgeStudy'
METHODS=[
 {'id':'02_geometry_normal','title':'2번 기준','kind':'baseline','source':'02_geometry_normal'},
 {'id':'07_raw_diffuse','title':'7번 기준','kind':'baseline','source':'07_raw_diffuse'},
 {'id':'09_mask_coverage','title':'7번 + UV 경계 보정','kind':'coverage','source':'07_raw_diffuse'},
 {'id':'10_alpha_colour','title':'7번 + 투명 가장자리 색 보정','kind':'alpha','source':'07_raw_diffuse'},
 {'id':'11_alpha_clip','title':'7번 + 가장자리 잘라내기','kind':'clip','source':'07_raw_diffuse'},
 {'id':'12_finer_mip','title':'7번 + 더 선명한 텍스처 샘플','kind':'mip','source':'07_raw_diffuse'},
 {'id':'13_thin_normal','title':'7번 + 얇은 면 빛 전달','kind':'thin','source':'07_raw_diffuse'},
 {'id':'D1_unlit_view','title':'진단: 조명 없는 재질색 보기','kind':'diagnostic','source':'07_raw_diffuse','view_mode':'unlit'},
 {'id':'D2_short_ao_off','title':'진단: 근거리 간접광 차폐 끄기','kind':'diagnostic','source':'07_raw_diffuse','cvars':{'r.Lumen.ScreenProbeGather.ShortRangeAO':0}},
 {'id':'D3_screen_trace_off','title':'진단: 화면 기반 간접광 추적 끄기','kind':'diagnostic','source':'07_raw_diffuse','cvars':{'r.Lumen.ScreenProbeGather.ScreenTraces':0}},
]

def create():
    assert '-ICNPC003HairEdgeStudy' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    OUT.mkdir(parents=True,exist_ok=True)
    scope=json.loads((prior.OUT/'Scope.json').read_text(encoding='utf-8'))
    prior.current.original.assert_protected(scope)
    (OUT/'Scope.json').write_text(json.dumps(scope,ensure_ascii=False,indent=2),encoding='utf-8')
    old={m['id']:m for m in json.loads((prior.OUT/'Methods.json').read_text(encoding='utf-8'))}
    lib=u.MaterialEditingLibrary;assets=[];rows=[]
    for method in METHODS:
        source=old[method['source']]
        row={**method,'cast_shadow':True,'scatter':source['scatter'],'density':source['density'],'note':method['title'],'emissive':0.,'view_mode':method.get('view_mode','lit')}
        if method['kind'] in ['baseline','diagnostic']:
            rows.append({**row,'material':source['material'],'master':source['master']});continue
        prefix=DEST+'/'+method['id'];master_path=prefix+'/M_HairEdgeStudy'
        master=u.load_asset(master_path) if u.EditorAssetLibrary.does_asset_exist(master_path) else u.EditorAssetLibrary.duplicate_asset(source['master'],master_path)
        output=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        norm=lambda s:str(s).replace(' ','').lower()
        def pin_map(expression):return {norm(name):node for name,node in zip(lib.get_material_expression_input_names(expression),lib.get_inputs_for_material_expression(master,expression))}
        pins=pin_map(output);select=pins['shadingmodel'];mask=pin_map(select)['a']
        def node(cls):return lib.create_material_expression(master,cls)
        def connect(a,out,b,inp):assert lib.connect_material_expressions(a,out,b,inp),(str(b),inp)
        def scalar(value):
            n=node(u.MaterialExpressionConstant);n.set_editor_property('r',value);return n
        if output.get_editor_property('desc')!='IC Hair Edge '+method['id']:
            kind=method['kind']
            if kind=='coverage':
                rects=[(.4985,.4985,.7515,1.0015),(.755,.51,1.001,.75)]
                terms=[f'(step({x0},UV.x)*step(UV.x,{x1})*step({y0},UV.y)*step(UV.y,{y1}))' for x0,y0,x1,y1 in rects]
                mask.set_editor_property('code','return saturate('+'+'.join(terms)+');')
            if kind in ['alpha','mip']:
                texture=next(e for e in lib.get_material_expressions(master) if isinstance(e,u.MaterialExpressionTextureSampleParameter2D) and str(e.get_editor_property('parameter_name'))=='ICResearchHairDiffuse')
                if kind=='alpha':
                    raw_lerp=pins['basecolor'];colour=pin_map(raw_lerp)['b']
                    clean=node(u.MaterialExpressionCustom);clean.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
                    inputs=[]
                    for name in ['C','Alpha']:
                        value=u.CustomInput();value.set_editor_property('input_name',name);inputs.append(value)
                    clean.set_editor_property('inputs',inputs)
                    clean.set_editor_property('code','return min(C / max(Alpha, 0.333333), float3(0.65,0.65,0.65));')
                    connect(colour,'',clean,'C');connect(texture,'A',clean,'Alpha');connect(clean,'',raw_lerp,'B')
                else:
                    texture.set_editor_property('mip_value_mode',u.TextureMipValueMode.TMVM_MIP_BIAS)
                    texture.set_editor_property('const_mip_value',-1)
            if kind=='clip':
                original=pins['opacitymask'];multiply=node(u.MaterialExpressionMultiply)
                out=next(p for p in lib.get_material_expression_output_names(original) if norm(p)=='opacitymask')
                connect(original,out,multiply,'A');connect(scalar(.333333/.65),'',multiply,'B')
                lerp=node(u.MaterialExpressionLinearInterpolate);connect(original,out,lerp,'A');connect(multiply,'',lerp,'B');connect(mask,'',lerp,'Alpha')
                target=next(p for p in lib.get_material_expression_input_names(output) if norm(p)=='opacitymask');connect(lerp,'',output,target)
            if kind=='thin':pin_map(select)['a>b'].set_editor_property('shading_model',u.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
            output.set_editor_property('desc','IC Hair Edge '+method['id'])
        errors=lib.recompile_material(master);assert not errors,str(errors)
        assets.append(master)
        def parent(src):
            if isinstance(src,u.Material):return master
            path=prefix+'/Parents/'+src.get_name()
            target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(src.get_path_name(),path)
            lib.set_material_instance_parent(target,parent(src.get_editor_property('parent')))
            settings=target.get_editor_property('base_property_overrides');settings.set_editor_property('override_shading_model',False)
            target.set_editor_property('base_property_overrides',settings);lib.update_material_instance(target);assets.append(target);return target
        src=u.load_asset(source['material']);path=prefix+'/MI_HairEdgeStudy'
        material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source['material'],path)
        lib.set_material_instance_parent(material,parent(src.get_editor_property('parent')))
        if method['kind']=='thin':
            row.update(scatter=.35,density=.65)
            lib.set_material_instance_scalar_parameter_value(material,'ICHairScatter',row['scatter']);lib.set_material_instance_scalar_parameter_value(material,'ICHairDensity',row['density'])
        lib.update_material_instance(material);assets.append(material)
        rows.append({**row,'material':path,'master':master_path})
    for asset in assets:
        assert asset.get_path_name().startswith(DEST+'/')
        assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    prior.current.original.assert_protected(scope)
    (OUT/'Methods.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_NPC003_HAIR_EDGE_STUDY_CREATED '+str(len(rows)))

if __name__=='__main__':create()
