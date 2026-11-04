"""Isolated visual experiments. Saves reports/screenshots, never player or world assets."""
import sys,json,hashlib,time,traceback,math
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting/MethodComparison';OUT.mkdir(parents=True,exist_ok=True)
assert '-ICCharacterLightingReview' in u.SystemLibrary.get_command_line()
sys.path.insert(0,str(ROOT/'Tools/Characters'))
import setup_player_world_lighting as SOURCE
import setup_player_part_lighting as PART
TRIAL='/Game/Developer/ICLightingComparison'
METHODS=[('00_current','현재 부위별 혼합 재질'),('01_unlit_scale','Unlit 고정 명도'),('02_tonemap','Unlit 톤매핑 역보정 해제'),('03_mtoon_lit','MToon Lit'),('04_sss_skin','피부 Subsurface Profile'),('05_character_fill','MToon Lit와 캐릭터 전용 보조광'),('06_postprocess','캐릭터만 화면 명도 보정'),('07_environment','광원과 가림 검사로 조도 추정'),('08_single_sun','단일 태양광 진단')]
TIMES=[('04h',240),('10h',600),('14h',840),('20h',1200)]
protected=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
protected+=list((ROOT/'Content/IsekaiClass/Player/Materials').rglob('*.uasset'))
protected+=[ROOT/f'Content/IsekaiClass/Player/SamplePlayer/{folder}/SK_Player_{sex}.uasset' for sex,folder in [('M','Male'),('F','Female')]]
HASHES={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
(OUT/'ProtectionBefore.json').write_text(json.dumps(HASHES,indent=2),encoding='utf-8')
STATE={'stage':'setup','samples':[],'complete':False,'commands':[]}
if (OUT/'Command.json').exists():STATE['commands'].append(json.loads((OUT/'Command.json').read_text(encoding='utf-8'))['id'])


def save_state():
    (OUT/'Status.json').write_text(json.dumps({k:v for k,v in STATE.items() if k in ['stage','samples','complete','error','command_result']},ensure_ascii=False,indent=2),encoding='utf-8')

def duplicate(source,folder,name):
    path=f'{TRIAL}/{folder}/{name}'
    return u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)

def configure_lit(source,index,sss=False):
    target=duplicate(source,'SSS' if sss else 'MToon',source.get_name()+'_Trial')
    part=PART.category('F',source.get_name());face=part in ['features','skin']
    path=source.get_editor_property('parent').get_path_name().split('.')[0].replace('Unlit','Lit')
    if sss and part=='skin':path='/VRM4U/MaterialUtil/UE5/Material/MI_VrmMToonOptSSSProfileTwoSided'
    parent=u.load_asset(path);assert parent,path
    u.MaterialEditingLibrary.set_material_instance_parent(target,parent)
    overrides=target.get_editor_property('base_property_overrides');overrides.set_editor_property('override_shading_model',False);target.set_editor_property('base_property_overrides',overrides)
    scalars={str(n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(target)}
    switches={str(n) for n in u.MaterialEditingLibrary.get_static_switch_parameter_names(target)}
    for name,value in {'bUseLight':True,'bUseMRSA':True,'bUseMatCap':False,'bUseRimLight':False,'bUseNormalmap':not face,'bUseCustomReflection':False}.items():
        if name in switches:u.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(target,name,value)
    for name,value in {'NormalToCameraMat':.95 if face else 0.,'NormalFresnelScale':2. if face else 1.,'NormalToLightMat':.10,'mtoon_ReceiveShadowRate':0. if face else .65,'mtoon_NormalScale':.15 if part=='hair' else .25}.items():
        if name in scalars:u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(target,name,value)
    u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(target,'MRSA',u.LinearColor(0,1. if face else .85,0. if face else .05,1.))
    if sss and part=='skin':
        try:target.set_editor_property('subsurface_profile',u.load_asset('/VRM4U/MaterialUtil/MToonUtil/SSS/SP_VrmSSS'))
        except Exception:pass
    u.MaterialEditingLibrary.update_material_instance(target)
    return target

def create_postprocess():
    path=TRIAL+'/PostProcess/M_CharacterBrightness'
    material=u.load_asset(path)
    if material:
        u.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:material=u.AssetToolsHelpers.get_asset_tools().create_asset('M_CharacterBrightness',TRIAL+'/PostProcess',u.Material,u.MaterialFactoryNew())
    material.set_editor_property('material_domain',u.MaterialDomain.MD_POST_PROCESS)
    for name in ['BL_SCENE_COLOR_AFTER_TONEMAPPING','BL_AFTER_TONEMAPPING']:
        if hasattr(u.BlendableLocation,name):material.set_editor_property('blendable_location',getattr(u.BlendableLocation,name));break
    def node(cls):return u.MaterialEditingLibrary.create_material_expression(material,cls)
    custom=node(u.MaterialExpressionCustom);custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property('code','float visible=(abs(Tag.r-247.0)<0.5 && ObjectDepth.r<=WorldDepth.r+1.0)?1.0:0.0;return lerp(Color.rgb,Color.rgb*0.65,visible);')
    inputs=[]
    for name in ['Color','Tag','ObjectDepth','WorldDepth']:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    custom.set_editor_property('inputs',inputs)
    ids=[u.SceneTextureId.PPI_POST_PROCESS_INPUT0,u.SceneTextureId.PPI_CUSTOM_STENCIL,u.SceneTextureId.PPI_CUSTOM_DEPTH,u.SceneTextureId.PPI_SCENE_DEPTH]
    for name,identifier in zip(['Color','Tag','ObjectDepth','WorldDepth'],ids):
        texture=node(u.MaterialExpressionSceneTexture);texture.set_editor_property('scene_texture_id',identifier)
        assert u.MaterialEditingLibrary.connect_material_expressions(texture,'Color',custom,name)
    assert u.MaterialEditingLibrary.connect_material_property(custom,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    errors=u.MaterialEditingLibrary.recompile_material(material);assert not errors,str(errors)
    return material


def prepare():
    original=SOURCE.source_materials()['F']
    STATE['original']=original
    STATE['current']=[s.get_editor_property('material_interface') for s in u.load_asset('/Game/IsekaiClass/Player/SamplePlayer/Female/SK_Player_F').get_editor_property('materials')]
    STATE['unlit']=[u.load_asset('/Game/IsekaiClass/Player/Materials/ToonLighting/F/'+m.get_name()+'_Toon') for m in original];assert all(STATE['unlit'])
    STATE['tone']=[]
    for m in STATE['unlit']:
        target=duplicate(m,'Tone',m.get_name()+'_NoInverse')
        u.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(target,'bUseFilmTonemapInverse',False)
        u.MaterialEditingLibrary.update_material_instance(target);STATE['tone'].append(target)
    STATE['lit']=[configure_lit(m,i) for i,m in enumerate(original)]
    STATE['sss']=[configure_lit(m,i,True) if PART.category('F',m.get_name())=='skin' else STATE['lit'][i] for i,m in enumerate(original)]
    STATE['post']=create_postprocess()
    info={key:[{'path':m.get_path_name(),'effective_shader':str(SOURCE.effective_shading_model(m))} for m in STATE[key]] for key in ['current','unlit','tone','lit','sss']}
    (OUT/'Methods.json').write_text(json.dumps({'methods':METHODS,'times':TIMES,'materials':info,'parameters':{'constant_unlit':.35,'postprocess_multiplier':.65,'fill_light_day_lux':.25,'fill_light_night_lux':2.,'environment_formula':'clamp(.18 + .34*sqrt(sky_scale) + .30*sqrt(visible_sun_scale), .18, .82)'}},ensure_ascii=False,indent=2),encoding='utf-8')


def property_value(obj,name):
    try:
        v=obj.get_editor_property(name)
        if v is None or isinstance(v,(bool,int,float,str)):return v
        if isinstance(v,u.Object):return v.get_path_name()
        return str(v)
    except Exception:return None


def begin_review():
    u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
    fill=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.DirectionalLight,u.Vector(),u.Rotator(),False)
    assert fill
    fill.set_editor_property('tags',['ICLightingComparison.Fill'])
    fill.get_component_by_class(u.DirectionalLightComponent).set_visibility(False,True)
    STATE['editor_fill']=fill
    import review_player_world_lighting as review
    STATE['review']=review;review.OUT=OUT
    review.STATE['automatic_variants']=[]
    STATE['stage']='waiting'

def audit_world(world,cam):
    result={'directional':[],'skylight':[],'postprocess':[],'cvars':{},'camera_postprocess':{}}
    for light in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight):
        c=light.get_component_by_class(u.DirectionalLightComponent)
        result['directional'].append({'actor':light.get_path_name(),'properties':{n:property_value(c,n) for n in ['intensity','light_color','cast_shadows','visible','lighting_channels','atmosphere_sun_light','mobility']}})
    for light in u.GameplayStatics.get_all_actors_of_class(world,u.SkyLight):
        c=light.get_component_by_class(u.SkyLightComponent)
        result['skylight'].append({'actor':light.get_path_name(),'properties':{n:property_value(c,n) for n in ['intensity','light_color','real_time_capture','source_type','cubemap','visible']}})
    fields=['override_auto_exposure_method','auto_exposure_method','override_auto_exposure_bias','auto_exposure_bias','override_auto_exposure_min_brightness','auto_exposure_min_brightness','override_auto_exposure_max_brightness','auto_exposure_max_brightness','override_ambient_cubemap_intensity','ambient_cubemap_intensity','override_bloom_intensity','bloom_intensity']
    for actor in u.GameplayStatics.get_all_actors_of_class(world,u.PostProcessVolume):
        result['postprocess'].append({'actor':actor.get_path_name(),'priority':property_value(actor,'priority'),'unbound':property_value(actor,'unbound'),'enabled':property_value(actor,'enabled'),'blend_weight':property_value(actor,'blend_weight'),'settings':{n:property_value(actor.get_editor_property('settings'),n) for n in fields}})
    result['camera_postprocess']={n:property_value(cam.get_editor_property('post_process_settings'),n) for n in fields}
    for name in ['r.DefaultFeature.AutoExposure','r.CustomDepth','r.DefaultFeature.AutoExposure.Bias','r.Substrate']:
        try:result['cvars'][name]=u.SystemLibrary.get_console_variable_float_value(name)
        except Exception:pass
    (OUT/'WorldAudit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')


def environment_brightness():
    r=STATE['review'];world=r.STATE['world'];player=r.STATE['player']
    start=player.get_actor_location()+u.Vector(0,0,40)
    visible=0.;total=0.;traces=[]
    for light in STATE['suns']:
        intensity=light.get_component_by_class(u.DirectionalLightComponent).get_editor_property('intensity')
        if intensity<=0:continue
        direction=u.MathLibrary.get_forward_vector(light.get_actor_rotation())
        hit=u.SystemLibrary.line_trace_single(world,start,start-direction*50000.,u.TraceTypeQuery.ECC_VISIBILITY,True,[player],u.DrawDebugTrace.NONE,True)
        blocked=bool(hit and hit.to_dict().get('blocking_hit',False))
        total+=intensity
        if not blocked:visible+=intensity
        traces.append({'light':light.get_name(),'intensity':intensity,'blocked':blocked})
    skies=u.GameplayStatics.get_all_actors_of_class(world,u.SkyLight)
    sky=skies[0].get_component_by_class(u.SkyLightComponent).get_editor_property('intensity') if skies else 0.
    value=max(.18,min(.82,.18+.34*math.sqrt(max(0.,sky))+.30*math.sqrt(max(0.,visible/13.7))))
    return value,{'sky':sky,'visible_sun':visible,'sun_total':total,'traces':traces}


def start_trials():
    r=STATE['review'];world=r.STATE['world'];player=r.STATE['player'];cam=r.STATE['camera']
    point=u.Vector(-60465,561595,-6440)
    player.set_actor_location_and_rotation(point,u.Rotator(),False,True)
    location=point+u.Vector(350,380,75)
    cam.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)
    STATE['suns']=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight) if 'ICLightingComparison.Fill' not in [str(t) for t in a.tags]]
    audit_world(world,cam)
    if 'old_depth_mode' not in STATE:STATE['old_depth_mode']=u.SystemLibrary.get_console_variable_int_value('r.CustomDepth')
    u.SystemLibrary.execute_console_command(world,'r.CustomDepth 3')
    light=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight) if 'ICLightingComparison.Fill' in [str(t) for t in a.tags])
    light.set_actor_rotation(cam.get_world_rotation(),False)
    component=light.get_component_by_class(u.DirectionalLightComponent)
    component.set_mobility(u.ComponentMobility.MOVABLE)
    light.set_actor_rotation(u.Rotator(),False)  # Fixed world-X fill used in the captured comparison.
    for name,value in {'cast_shadows':False,'atmosphere_sun_light':False,'indirect_lighting_intensity':0.,'volumetric_scattering_intensity':0.,'lighting_channels':u.LightingChannels(channel0=False,channel1=True,channel2=False)}.items():component.set_editor_property(name,value)
    component.set_visibility(False,True);STATE['fill_light']=component
    STATE['cases']=[(key,hour,minute) for key,_ in METHODS for hour,minute in TIMES]
    STATE.update(index=0,stage='next')
    r.STATE['phase']='experiments'
    save_state()


def next_case():
    r=STATE['review'];key,hour,minute=STATE['cases'][STATE['index']]
    player=r.STATE['player'];player.apply_preset(1);body=player.get_editor_property('mesh');world=r.STATE['world'];cam=r.STATE['camera']
    body.set_editor_property('lighting_channels',u.LightingChannels(channel0=True,channel1=key=='05_character_fill',channel2=False))
    body.set_editor_property('render_custom_depth',key=='06_postprocess')
    body.set_editor_property('custom_depth_stencil_value',247 if key=='06_postprocess' else 0)
    cam.add_or_update_blendable(STATE['post'],1. if key=='06_postprocess' else 0.)
    STATE['fill_light'].set_visibility(key=='05_character_fill',True)
    for i,light in enumerate(STATE['suns']):light.get_component_by_class(u.DirectionalLightComponent).set_visibility(key!='08_single_sun' or i==0,True)
    material_key={'00_current':'current','01_unlit_scale':'unlit','02_tonemap':'tone','03_mtoon_lit':'lit','04_sss_skin':'sss','05_character_fill':'lit','06_postprocess':'unlit','07_environment':'unlit','08_single_sun':'lit'}[key]
    STATE['mids']=[body.create_dynamic_material_instance(i,m) for i,m in enumerate(STATE[material_key])]
    clock=r.STATE['pc'].get_time();current=clock.get_clock().game_minute
    assert clock.jump_game_minutes((minute-current%1440+1440)%1440)
    for cycle in u.GameplayStatics.get_all_actors_of_class(world,u.ICDayNightController):cycle.refresh_from_clock()
    STATE['fill_light'].set_intensity(2. if hour in ['04h','20h'] else .25)
    STATE.update(case=(key,hour,minute),stage='settle',ready=time.monotonic()+7.)
    save_state()


def update_trial():
    key,hour,minute=STATE['case'];extra={}
    if key=='00_current':value=STATE['review'].STATE['player'].get_component_by_class(u.ICPlayerToonBrightnessComponent).get_brightness()
    elif key=='01_unlit_scale':value=.35
    elif key=='07_environment':value,extra=environment_brightness()
    else:value=1.
    for material in STATE['mids']:material.set_scalar_parameter_value('ICCharacterBrightness',value)
    STATE['current_value']=value;STATE['environment']=extra


def complete():
    r=STATE['review'];world=r.STATE['world']
    STATE['fill_light'].set_visibility(False,True)
    for light in STATE['suns']:light.get_component_by_class(u.DirectionalLightComponent).set_visibility(True,True)
    r.STATE['camera'].remove_blendable(STATE['post'])
    u.SystemLibrary.execute_console_command(world,'r.CustomDepth '+str(STATE['old_depth_mode']))
    for path,digest in HASHES.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,'Protected file changed '+path
    assert len(STATE['samples'])==len(METHODS)*4
    STATE.update(stage='complete',complete=True)
    (OUT/'Results.json').write_text(json.dumps({'methods':METHODS,'samples':STATE['samples'],'protected_files_unchanged':True,'protected_file_count':len(HASHES)},ensure_ascii=False,indent=2),encoding='utf-8')
    save_state();r.STATE['samples']=STATE['samples'];r.finish()
    u.unregister_slate_post_tick_callback(STATE['handle'])


def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        stage=STATE['stage']
        if stage=='setup':
            prepare();begin_review();save_state();return
        if stage=='waiting':
            r=STATE['review']
            if r.STATE.get('error'):raise RuntimeError(r.STATE['error'])
            if r.STATE['phase']=='freeze':start_trials()
            return
        if stage=='next':
            if STATE['index']>=len(STATE['cases']):complete();return
            next_case();return
        if stage=='settle':
            update_trial()
            if time.monotonic()<STATE['ready']:return
            key,hour,minute=STATE['case'];path=OUT/key/(hour+'.png');path.parent.mkdir(parents=True,exist_ok=True)
            r=STATE['review'];assert u.ICCharacterSetupLibrary.capture_monster_test_hud(r.STATE['world'],str(path))
            STATE['samples'].append({'method':key,'hour':hour,'minute':minute,'clock_minute':r.STATE['pc'].get_time().get_clock().game_minute,'brightness_control':STATE['current_value'],'environment':STATE['environment'],'lighting':r.light_state(),'suns_visible':[light.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible') for light in STATE['suns']],'fill_visible':STATE['fill_light'].get_editor_property('visible'),'fill_intensity':STATE['fill_light'].get_editor_property('intensity'),'camera_location':str(r.STATE['camera'].get_world_location()),'fov':r.STATE['camera'].get_editor_property('field_of_view'),'image':str(path.relative_to(OUT))})
            STATE.update(index=STATE['index']+1,stage='next');save_state();return
        if stage=='error':
            p=OUT/'Command.json'
            if not p.exists():return
            cmd=json.loads(p.read_text(encoding='utf-8'))
            if cmd['id'] in STATE['commands']:return
            STATE['commands'].append(cmd['id'])
            script=ROOT/cmd['script'];exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'),globals());STATE['command_result']=cmd['id'];save_state()
    except Exception:
        STATE.update(stage='error',error=traceback.format_exc())
        if STATE.get('review'):STATE['review'].STATE['phase']='idle_comparison'
        u.log_error(STATE['error']);save_state()
    finally:STATE['busy']=False

u.EditorPythonScripting.set_keep_python_script_alive(True)
STATE['handle']=u.register_slate_post_tick_callback(tick)
save_state()
