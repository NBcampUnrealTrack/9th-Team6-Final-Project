"""Hair-only outline exclusion preview; original world PP and assets stay untouched."""
import json
import unreal as u
import research_npc003_hair_edges as edges

def create():
    assert '-ICNPC003HairEdgeStudy' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=json.loads((edges.OUT/'Scope.json').read_text(encoding='utf-8'));edges.prior.current.original.assert_protected(scope)
    lib=u.MaterialEditingLibrary;prefix=edges.DEST+'/14_hair_outline_filter'
    rows=json.loads((edges.OUT/'Methods.json').read_text(encoding='utf-8'));source=next(m for m in rows if m['id']=='07_raw_diffuse')
    def dup(path,dest):return u.load_asset(dest) if u.EditorAssetLibrary.does_asset_exist(dest) else u.EditorAssetLibrary.duplicate_asset(path,dest)
    master=dup(source['master'],prefix+'/M_HairTag')
    output=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    pins={str(n).replace(' ','').lower():node for n,node in zip(lib.get_material_expression_input_names(output),lib.get_inputs_for_material_expression(master,output))}
    roughness=pins['roughness']
    if output.get_editor_property('desc')!='IC Hair Outline Tag':
        value=lib.create_material_expression(master,u.MaterialExpressionConstant);value.set_editor_property('r',.9375)
        assert lib.connect_material_expressions(value,'',roughness,'B')
        output.set_editor_property('desc','IC Hair Outline Tag')
    assert not lib.recompile_material(master)
    assets=[master]
    def parent(src):
        if isinstance(src,u.Material):return master
        target=dup(src.get_path_name(),prefix+'/Parents/'+src.get_name())
        lib.set_material_instance_parent(target,parent(src.get_editor_property('parent')));lib.update_material_instance(target);assets.append(target);return target
    source_mi=u.load_asset(source['material']);material=dup(source['material'],prefix+'/MI_HairTag')
    lib.set_material_instance_parent(material,parent(source_mi.get_editor_property('parent')));lib.update_material_instance(material);assets.append(material)
    original=u.load_asset('/Game/IsekaiClass/World/SampleLevel/Ruined_Village/materials/postprocess_materials/MI_postprocess_Inst')
    pp=dup(original.get_editor_property('parent').get_path_name(),prefix+'/M_HairOutlineFilter')
    if not any(e.get_editor_property('desc')=='IC Hair Only Edge Gate' for e in lib.get_material_expressions(pp)):
        lerp=next(e for e in lib.get_material_expressions(pp) if e.get_name()=='MaterialExpressionLinearInterpolate_5')
        inputs={str(n):node for n,node in zip(lib.get_material_expression_input_names(lerp),lib.get_inputs_for_material_expression(pp,lerp))}
        custom=lib.create_material_expression(pp,u.MaterialExpressionCustom);custom.set_editor_property('desc','IC Hair Only Edge Gate')
        custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
        names=['Edge','Tag','ObjectDepth','WorldDepth','Roughness'];items=[]
        for name in names:
            value=u.CustomInput();value.set_editor_property('input_name',name);items.append(value)
        custom.set_editor_property('inputs',items)
        custom.set_editor_property('code','float Hair=(abs(Tag.r-249.0)<0.5 && ObjectDepth.r<=WorldDepth.r+1.0 && abs(Roughness.r-0.9375)<0.004)?1.0:0.0; return Edge*(1.0-Hair);')
        assert lib.connect_material_expressions(inputs['Alpha'],'',custom,'Edge')
        for name,identifier in zip(names[1:],[u.SceneTextureId.PPI_CUSTOM_STENCIL,u.SceneTextureId.PPI_CUSTOM_DEPTH,u.SceneTextureId.PPI_SCENE_DEPTH,u.SceneTextureId.PPI_ROUGHNESS]):
            texture=lib.create_material_expression(pp,u.MaterialExpressionSceneTexture);texture.set_editor_property('scene_texture_id',identifier)
            assert lib.connect_material_expressions(texture,'Color',custom,name)
        assert lib.connect_material_expressions(custom,'',lerp,'Alpha')
    assert not lib.recompile_material(pp)
    ppmi=dup(original.get_path_name(),prefix+'/MI_HairOutlineFilter');lib.set_material_instance_parent(ppmi,pp);lib.update_material_instance(ppmi);assets += [pp,ppmi]
    for asset in assets:
        assert asset.get_path_name().startswith(prefix+'/');assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    row={**source,'id':'14_hair_outline_filter','title':'7번 + 머리만 후처리 윤곽선 제외','kind':'outline_filter','material':material.get_path_name(),'master':master.get_path_name(),'postprocess':ppmi.get_path_name(),'hair_roughness_tag':.9375,'stencil_tag':249,'cvars':{'r.CustomDepth':3},'note':'Specular=0인 머리 픽셀만 roughness 표식으로 식별. 캐릭터 stencil과 깊이 검사도 함께 사용. 눈·피부·의상 및 월드 후처리 수식 유지.'}
    rows=[m for m in rows if m['id']!=row['id']]+[row]
    neighbor=dup(pp.get_path_name(),prefix+'/M_HairBoundaryFilter')
    gate=next(e for e in lib.get_material_expressions(neighbor) if e.get_editor_property('desc')=='IC Hair Only Edge Gate')
    if len(gate.get_editor_property('inputs'))==5:
        items=list(gate.get_editor_property('inputs'));item=u.CustomInput();item.set_editor_property('input_name','PixelSize');items.append(item)
        gate.set_editor_property('inputs',items)
        rough=next(e for e in lib.get_material_expressions(neighbor) if isinstance(e,u.MaterialExpressionSceneTexture) and e.get_editor_property('scene_texture_id')==u.SceneTextureId.PPI_ROUGHNESS)
        assert lib.connect_material_expressions(rough,'InvSize',gate,'PixelSize')
    gate.set_editor_property('code','''
if(Edge<0.001) return Edge;
float Hair=(abs(Tag.r-249.0)<0.5 && ObjectDepth.r<=WorldDepth.r+1.0 && abs(Roughness.r-0.9375)<0.004)?1.0:0.0;
float2 UV=GetDefaultSceneTextureUV(Parameters,PPI_Roughness);
[unroll] for(int Y=-1;Y<=1;Y++) { [unroll] for(int X=-1;X<=1;X++) {
    float2 P=UV+float2(X,Y)*PixelSize.xy*1.5;
    float TagN=SceneTextureLookup(P,PPI_CustomStencil,false).r;
    float RN=SceneTextureLookup(P,PPI_Roughness,false).r;
    float DN=SceneTextureLookup(P,PPI_CustomDepth,false).r;
    float WN=SceneTextureLookup(P,PPI_SceneDepth,false).r;
    Hair=max(Hair,(abs(TagN-249.0)<0.5 && DN<=WN+1.0 && abs(RN-0.9375)<0.004)?1.0:0.0);
}}
return Edge*(1.0-Hair);''')
    assert not lib.recompile_material(neighbor)
    neighbormi=dup(original.get_path_name(),prefix+'/MI_HairBoundaryFilter');lib.set_material_instance_parent(neighbormi,neighbor);lib.update_material_instance(neighbormi)
    for asset in [neighbor,neighbormi]:assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    nrow={**row,'id':'15_hair_boundary_filter','title':'7번 + 머리와 인접 경계의 윤곽선 제외','postprocess':neighbormi.get_path_name(),'note':'머리 픽셀 및 1.5px 인접 범위에서만 선 제거. 눈 재질과 월드 조명·후처리의 나머지 계산 유지.'}
    rows=[m for m in rows if m['id']!=nrow['id']]+[nrow]
    (edges.OUT/'Methods.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    edges.prior.current.original.assert_protected(scope)
    u.log('IC_HAIR_ONLY_OUTLINE_FILTER_CREATED')

if __name__=='__main__':create()
