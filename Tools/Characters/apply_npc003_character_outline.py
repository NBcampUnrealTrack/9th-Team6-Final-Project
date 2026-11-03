"""Select method 07 and suppress world outline only on NPC003's visible body."""
import hashlib,json,re,shutil
from pathlib import Path
import unreal as u
import setup_npc003_hair_only as npc

ROOT=npc.ROOT
OUT=ROOT/'Saved/CharacterLighting/NPC003CharacterOutline_20261103'
DEST=npc.FOLDER+'/SelectedRawRGB'
PP_DEST='/Game/IsekaiClass/NPC/Materials/WorldReactive/Shared/CharacterOutline'
MAP='/Game/IsekaiClass/World/Maps/L_OpenWorld'
BP=npc.original.blueprint_path('NPC_R01_003')
SOURCE_PP='/Game/IsekaiClass/World/SampleLevel/Ruined_Village/materials/postprocess_materials/MI_postprocess_Inst'
PP=PP_DEST+'/MI_ICCharacterWorldPostProcess'
MASTER=DEST+'/M_ICNPC003RawRGB'
STENCIL=249

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def json_write(name,value):(OUT/name).write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
def asset_file(path):return npc.original.asset_file(path)
def check(scope):
    for name,value in scope['protected_hashes'].items():assert digest(ROOT/name)==value,'Protected file changed: '+name

def stable(value):
    if value is None or isinstance(value,(bool,int,float,str)):return value
    if isinstance(value,u.Object):return value.get_path_name()
    return re.sub(r'\(0x[0-9a-fA-F]+\)','',str(value))

def properties(obj,names):return {name:stable(obj.get_editor_property(name)) for name in names}

def world_state(world):
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    result={'actors':sorted(a.get_path_name() for a in actors),'lights':[],'postprocess':[],'controllers':[]}
    for actor in actors:
        if isinstance(actor,(u.DirectionalLight,u.SkyLight)):
            component=actor.get_component_by_class(u.SkyLightComponent if isinstance(actor,u.SkyLight) else u.DirectionalLightComponent)
            result['lights'].append({'actor':actor.get_path_name(),'transform':stable(actor.get_actor_transform()),
                **properties(component,['intensity','visible','light_color','cast_shadows'])})
        if isinstance(actor,u.PostProcessVolume):
            settings=actor.get_editor_property('settings');weighted=settings.get_editor_property('weighted_blendables')
            entries=[{'weight':e.get_editor_property('weight'),'object':stable(e.get_editor_property('object'))} for e in weighted.get_editor_property('array')]
            isolated=settings.copy();isolated.set_editor_property('weighted_blendables',u.WeightedBlendables());rest=stable(isolated)
            result['postprocess'].append({'actor':actor.get_path_name(),**properties(actor,['enabled','priority','unbound','blend_weight']),
                'settings_except_blendables':rest,'blendables':entries})
        if isinstance(actor,u.ICDayNightController):result['controllers'].append({'actor':actor.get_path_name(),'suns':[a.get_path_name() for a in actor.sun_lights]})
    for key in ['lights','postprocess','controllers']:result[key].sort(key=lambda x:x['actor'])
    return result

def same_world(before,after):
    expected=json.loads(json.dumps(before));changed=0
    for volume in expected['postprocess']:
        for entry in volume['blendables']:
            if entry['object'].split('.')[0]==SOURCE_PP:entry['object']=PP+'.'+PP.split('/')[-1];changed+=1
    assert changed==1
    assert expected==after,'World properties changed beyond the selected postprocess material'

def snapshot():
    OUT.mkdir(parents=True,exist_ok=True);path=OUT/'Scope.json'
    if path.exists():return json.loads(path.read_text(encoding='utf-8'))
    allowed=[asset_file(npc.BODY),asset_file(BP),ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Config/DefaultEngine.ini']
    files=set()
    for folder in ['Content/IsekaiClass/Player','Content/IsekaiClass/NPC','Plugins/VRM4U/Content','Plugins/KawaiiPhysics']:
        files.update((ROOT/folder).rglob('*.uasset'))
    files.update((ROOT/'Content/IsekaiClass/World/SampleLevel/Ruined_Village/materials/postprocess_materials').rglob('*.uasset'))
    files.update([ROOT/'IsekaiClass.uproject',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset'])
    backup=OUT/'Before';backup.mkdir(exist_ok=True)
    for p in allowed:
        target=backup/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
    scope={'protected_hashes':{str(p.relative_to(ROOT)):digest(p) for p in sorted(files) if p not in allowed},
        'allowed_changes':{str(p.relative_to(ROOT)):digest(p) for p in allowed},'backup':str(backup),
        'face_material_hash':digest(asset_file(npc.HEAD)),'target':'NPC_R01_003'}
    json_write('Scope.json',scope);return scope

def create_materials(production=False):
    lib=u.MaterialEditingLibrary
    research=npc.OUT/'Research_20261103/Methods.json'
    source=next(m for m in json.loads(research.read_text(encoding='utf-8')) if m['id']=='07_raw_diffuse')
    def dup(path,target):return u.load_asset(target) if u.EditorAssetLibrary.does_asset_exist(target) else u.EditorAssetLibrary.duplicate_asset(path,target)
    root=dup(source['master'],MASTER);assert root
    assert not lib.recompile_material(root)
    assets=[root]
    def parent(original):
        if isinstance(original,u.Material):return root
        target=dup(original.get_path_name(),DEST+'/Parents/'+original.get_name())
        lib.set_material_instance_parent(target,parent(original.get_editor_property('parent')));lib.update_material_instance(target);assets.append(target);return target
    template=u.load_asset(source['material'])
    body=u.load_asset(npc.BODY) if production else dup(npc.BODY,DEST+'/MI_NPC003FullOutlinePreview')
    before=npc.parameters(body);names={kind:list(v) for kind,v in before.items()}
    lib.set_material_instance_parent(body,parent(template.get_editor_property('parent')))
    overrides=body.get_editor_property('base_property_overrides');overrides.set_editor_property('override_shading_model',False);body.set_editor_property('base_property_overrides',overrides)
    for name,value in [('ICHairScatter',0.),('ICHairDensity',1.)]:lib.set_material_instance_scalar_parameter_value(body,name,value)
    lib.set_material_instance_texture_parameter_value(body,'ICResearchHairDiffuse',lib.get_material_instance_texture_parameter_value(template,'ICResearchHairDiffuse'))
    lib.set_material_instance_vector_parameter_value(body,'ICResearchHairTint',lib.get_material_instance_vector_parameter_value(template,'ICResearchHairTint'))
    lib.update_material_instance(body)
    expected=json.loads(json.dumps(before));expected['scalar']['ICHairScatter']=0.;expected['scalar']['ICHairDensity']=1.
    assert npc.parameters(body,names)==expected,'Unexpected original body parameter change'
    assets.append(body)
    original=u.load_asset(SOURCE_PP);pproot=dup(original.get_editor_property('parent').get_path_name(),PP_DEST+'/M_ICCharacterWorldPostProcess')
    gate=next((e for e in lib.get_material_expressions(pproot) if e.get_editor_property('desc')=='IC Visible Character Outline Gate'),None)
    if gate is None:
        lerp=next(e for e in lib.get_material_expressions(pproot) if e.get_name()=='MaterialExpressionLinearInterpolate_5')
        pins={str(n):v for n,v in zip(lib.get_material_expression_input_names(lerp),lib.get_inputs_for_material_expression(pproot,lerp))}
        gate=lib.create_material_expression(pproot,u.MaterialExpressionCustom);gate.set_editor_property('desc','IC Visible Character Outline Gate');gate.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
        inputs=[]
        for name in ['Edge','Tag','ObjectDepth','WorldDepth','PixelSize','Strength']:
            ci=u.CustomInput();ci.set_editor_property('input_name',name);inputs.append(ci)
        gate.set_editor_property('inputs',inputs)
        gate.set_editor_property('code','''
if(Edge<0.001) return Edge;
float Character=(abs(Tag.r-249.0)<0.5 && ObjectDepth.r<=WorldDepth.r+1.0)?1.0:0.0;
float2 UV=GetDefaultSceneTextureUV(Parameters,PPI_CustomStencil);
[unroll] for(int Y=-1;Y<=1;Y++) { [unroll] for(int X=-1;X<=1;X++) {
    float2 P=UV+float2(X,Y)*PixelSize.xy*1.5;
    float T=SceneTextureLookup(P,PPI_CustomStencil,false).r;
    float D=SceneTextureLookup(P,PPI_CustomDepth,false).r;
    float W=SceneTextureLookup(P,PPI_SceneDepth,false).r;
    Character=max(Character,(abs(T-249.0)<0.5 && D<=W+1.0)?1.0:0.0);
}}
return Edge*lerp(1.0,saturate(Strength),Character);''')
        assert lib.connect_material_expressions(pins['Alpha'],'',gate,'Edge')
        for name,kind in [('Tag',u.SceneTextureId.PPI_CUSTOM_STENCIL),('ObjectDepth',u.SceneTextureId.PPI_CUSTOM_DEPTH),('WorldDepth',u.SceneTextureId.PPI_SCENE_DEPTH)]:
            tex=lib.create_material_expression(pproot,u.MaterialExpressionSceneTexture);tex.set_editor_property('scene_texture_id',kind)
            assert lib.connect_material_expressions(tex,'Color',gate,name)
            if name=='Tag':assert lib.connect_material_expressions(tex,'InvSize',gate,'PixelSize')
        strength=lib.create_material_expression(pproot,u.MaterialExpressionScalarParameter);strength.set_editor_property('parameter_name','ICCharacterOutlineStrength');strength.set_editor_property('default_value',0.)
        assert lib.connect_material_expressions(strength,'',gate,'Strength')
        assert lib.connect_material_expressions(gate,'',lerp,'Alpha')
    assert not lib.recompile_material(pproot)
    ppmi=dup(SOURCE_PP,PP);lib.set_material_instance_parent(ppmi,pproot);lib.set_material_instance_scalar_parameter_value(ppmi,'ICCharacterOutlineStrength',0.);lib.update_material_instance(ppmi)
    for kind in ['scalar','vector','texture']:
        for name in getattr(lib,'get_'+kind+'_parameter_names')(original):
            a=getattr(lib,'get_material_instance_'+kind+'_parameter_value')(original,name);b=getattr(lib,'get_material_instance_'+kind+'_parameter_value')(ppmi,name)
            assert stable(a)==stable(b),'Original world postprocess parameter changed: '+str(name)
    assets += [pproot,ppmi]
    for asset in assets:assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    json_write('Materials.json',{'body':body.get_path_name(),'master':MASTER,'world_postprocess':PP,'source_method':'07_raw_diffuse','scope':'whole character, hair / skin / clothes / face','roughness_marker_removed':True,'emissive':0.,'production':production})

def create_preview():
    assert '-ICPreviewNPC003CharacterOutline' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot();check(scope);create_materials(False);check(scope)
    for name,value in scope['allowed_changes'].items():assert digest(ROOT/name)==value,'Existing target changed during preview: '+name
    u.log('IC_NPC003_WHOLE_OUTLINE_UNASSIGNED_PREVIEW_CREATED')

def _apply(scope):
    create_materials(True)
    bp=u.load_asset(BP);u.BlueprintEditorLibrary.compile_blueprint(bp)
    component=u.get_default_object(bp.generated_class()).get_editor_property('mesh')
    before=properties(component,['render_custom_depth','custom_depth_stencil_value','cast_shadow'])
    assert not list(component.get_editor_property('override_materials'))
    component.set_editor_property('render_custom_depth',True);component.set_editor_property('custom_depth_stencil_value',STENCIL)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    component=u.get_default_object(bp.generated_class()).get_editor_property('mesh')
    assert component.get_editor_property('render_custom_depth') and component.get_editor_property('custom_depth_stencil_value')==STENCIL
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    world=u.EditorLoadingAndSavingUtils.load_map(MAP);assert world
    previous=world_state(world);json_write('WorldBefore.json',previous)
    count=0
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor,u.PostProcessVolume):continue
        settings=actor.get_editor_property('settings');weighted=settings.get_editor_property('weighted_blendables');entries=list(weighted.get_editor_property('array'))
        for entry in entries:
            material=entry.get_editor_property('object')
            if material and material.get_path_name().split('.')[0]==SOURCE_PP:entry.set_editor_property('object',u.load_asset(PP));count+=1
        weighted.set_editor_property('array',entries);settings.set_editor_property('weighted_blendables',weighted);actor.set_editor_property('settings',settings)
    assert count==1,count
    after=world_state(world);same_world(previous,after);json_write('WorldAfter.json',after)
    assert u.EditorLoadingAndSavingUtils.save_map(world,MAP)
    config=ROOT/'Config/DefaultEngine.ini';data=config.read_bytes();assert b'r.CustomDepth=1' in data
    config.write_bytes(data.replace(b'r.CustomDepth=1',b'r.CustomDepth=3',1))
    check(scope);assert digest(asset_file(npc.HEAD))==scope['face_material_hash']
    json_write('Applied.json',{'target':'NPC_R01_003','body_material':npc.BODY,'postprocess':PP,'stencil':STENCIL,'outline_strength':0.,'previous_component':before,
        'world_lights_exposure_and_other_postprocess_settings_unchanged':True,'face_material_unchanged':True,'other_characters_unchanged':True,'roughness_marker_removed':True,'backup':scope['backup']})
    u.log('IC_NPC003_WHOLE_CHARACTER_OUTLINE_APPLIED')

def apply():
    assert '-ICApplyNPC003CharacterOutline' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot();check(scope)
    preview=json.loads((OUT/'PreviewVerification.json').read_text(encoding='utf-8'))
    pixels=json.loads((OUT/'WorldVisualCheck.json').read_text(encoding='utf-8'))
    assert preview['complete'] and len(preview['samples'])==16 and preview['world_control_complete']
    assert pixels['passes_static_world_check']
    for name,value in scope['allowed_changes'].items():
        assert digest(ROOT/name)==value,'Target changed after preview: '+name
        assert digest(Path(scope['backup'])/name)==value,'Backup missing or inconsistent: '+name
    try:_apply(scope)
    except Exception:
        for name,value in scope['allowed_changes'].items():
            shutil.copy2(Path(scope['backup'])/name,ROOT/name)
            assert digest(ROOT/name)==value
        json_write('RolledBack.json',{'original_target_files_restored':True})
        raise

if __name__=='__main__':apply()
