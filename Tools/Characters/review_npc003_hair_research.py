"""Native hair/edge trial captures; never assigns or saves game asset defaults."""
import json,struct,sys,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
CHARACTER_OUTLINE='-ICNPC003CharacterOutlineReview' in u.SystemLibrary.get_command_line()
SAVED_OUTLINE='-ICNPC003CharacterOutlineSavedReview' in u.SystemLibrary.get_command_line()
EDGE_STUDY=CHARACTER_OUTLINE or '-ICNPC003HairEdgeStudy' in u.SystemLibrary.get_command_line()
if EDGE_STUDY:import research_npc003_hair_edges as research
else:import research_npc003_hair_setup as research
assert EDGE_STUDY or '-ICNPC003HairResearch' in u.SystemLibrary.get_command_line()
METHODS=json.loads((research.OUT/'Methods.json').read_text(encoding='utf-8'))
if CHARACTER_OUTLINE:
    import apply_npc003_character_outline as selected
    hair=next(m for m in METHODS if m['id']=='15_hair_boundary_filter')
    record=json.loads((selected.OUT/'Materials.json').read_text(encoding='utf-8'))
    whole={'id':'Character_Whole','title':'7번 + 전신 후처리 선 제외','kind':'character_outline','material':record['body'],
        'postprocess':selected.PP,'stencil_tag':249,'cvars':{'r.CustomDepth':3},'cast_shadow':True,'scatter':0.,'density':1.,'emissive':0.}
    if SAVED_OUTLINE:whole.update(kind='current',postprocess=None,material=selected.npc.BODY,cvars={})
    METHODS=[whole] if SAVED_OUTLINE else [hair,whole]
    research.OUT=selected.OUT
EDGE_FINAL='-ICNPC003HairEdgeFinal' in u.SystemLibrary.get_command_line()
EDGE_PROBE='-ICNPC003HairEdgeProbe' in u.SystemLibrary.get_command_line()
if EDGE_PROBE:METHODS=[m for m in METHODS if m['id'] in ['07_raw_diffuse','16_tag_debug','17_all_edge_off']]
OUTLINE_STUDY='-ICNPC003HairOutlineStudy' in u.SystemLibrary.get_command_line()
if OUTLINE_STUDY:METHODS=[m for m in METHODS if m['id'].startswith(('D4_','D5_','D6_','D7_','D8_')) or m['id']=='07_raw_diffuse']
if EDGE_FINAL:
    chosen=json.loads((research.OUT/'Shortlist.json').read_text(encoding='utf-8'))['methods']
    METHODS=[m for m in METHODS if m['id'] in chosen]
MATERIALS={m['id']:u.load_asset(m['material']) for m in METHODS}
assert all(MATERIALS.values())
SCOPE=json.loads((research.OUT/'Scope.json').read_text(encoding='utf-8'))
if CHARACTER_OUTLINE and not SAVED_OUTLINE:SCOPE['protected_hashes'].update(SCOPE['allowed_changes'])
research.current.original.assert_protected(SCOPE)
import review_player_world_lighting as review
OUT=research.OUT/('AppliedCaptures' if SAVED_OUTLINE else 'PreviewCaptures' if CHARACTER_OUTLINE else 'ProbeCaptures' if EDGE_PROBE else 'OutlineCaptures' if OUTLINE_STUDY else 'FinalCaptures' if EDGE_FINAL else 'Captures');OUT.mkdir(parents=True,exist_ok=True)
review.OUT=OUT;review.STATE['automatic_variants']=[]
POINTS={'Outside':u.Vector(-53990,629630,-5098.451662),'Inside':u.Vector(-53240,629940,-5069.725)}
ENVIRONMENTS=[('Night_Outside',1200,'Outside'),('Night_Inside',1200,'Inside'),('Day_Outside',600,'Outside'),('Day_Inside',600,'Inside')]
if EDGE_STUDY and not EDGE_FINAL and not CHARACTER_OUTLINE:ENVIRONMENTS=[ENVIRONMENTS[1],ENVIRONMENTS[2]]
if EDGE_PROBE:ENVIRONMENTS=[ENVIRONMENTS[0]]
CASES=[(label,minute,place,method,closeup) for label,minute,place in ENVIRONMENTS for method in METHODS for closeup in ([True] if EDGE_STUDY and not EDGE_FINAL and not CHARACTER_OUTLINE else [False,True])]
STATE={'stage':'waiting','index':0,'samples':[],'complete':False,'methods':[{k:v for k,v in m.items() if k!='material'} for m in METHODS]}
PP_ORIGINAL=None
PP_ACTOR=None
MESH_OUTLINE_ORIGINAL=None

def report():
    (research.OUT/('AppliedVerification.json' if SAVED_OUTLINE else 'PreviewVerification.json' if CHARACTER_OUTLINE else 'ProbeVerification.json' if EDGE_PROBE else 'OutlineVerification.json' if OUTLINE_STUDY else 'FinalVerification.json' if EDGE_FINAL else 'Verification.json')).write_text(json.dumps({k:v for k,v in STATE.items() if k not in ['handle','ready','targets','target','body','original_shadow']},ensure_ascii=False,indent=2),encoding='utf-8')

def audit_rendering():
    def props(obj,names):
        result={}
        for name in names:
            try:
                value=obj.get_editor_property(name)
                result[name]=value if value is None or isinstance(value,(bool,int,float,str)) else str(value)
            except Exception:pass
        return result
    world=review.STATE['world'];components=[]
    for c in STATE['target'].get_components_by_class(u.MeshComponent):
        row={'path':c.get_path_name(),**props(c,['overlay_material','overlay_material_max_draw_distance','visible','hidden_in_game','cast_shadow','cast_contact_shadow','cast_capsule_indirect_shadow','affect_distance_field_lighting','render_custom_depth','custom_depth_stencil_value']),
            'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())]}
        if isinstance(c,u.SkeletalMeshComponent):row['asset']=props(c.get_skeletal_mesh_asset(),['overlay_material'])
        components.append(row)
    fields=['weighted_blendables','override_ambient_occlusion_intensity','ambient_occlusion_intensity','override_ambient_occlusion_radius','ambient_occlusion_radius','override_lumen_short_range_ao_intensity','lumen_short_range_ao_intensity']
    volumes=[{'path':a.get_path_name(),**props(a,['enabled','priority','unbound','blend_weight']),'settings':props(a.get_editor_property('settings'),fields)} for a in u.GameplayStatics.get_all_actors_of_class(world,u.PostProcessVolume)]
    result={'components':components,'volumes':volumes,'camera':props(review.STATE['camera'].get_editor_property('post_process_settings'),fields)}
    (research.OUT/'RenderingAudit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')

def render_mode(method=None):
    world=review.STATE['world']
    for name,value in STATE.get('console_defaults',{}).items():u.SystemLibrary.execute_console_command(world,name+' '+str(value))
    u.SystemLibrary.execute_console_command(world,'viewmode '+(method.get('view_mode','lit') if method else 'lit'))
    for name,value in (method.get('cvars',{}) if method else {}).items():
        u.SystemLibrary.execute_console_command(world,name+' '+str(value))
        assert u.SystemLibrary.get_console_variable_int_value(name)==value,name

def outline_mode(method=None):
    if PP_ACTOR is None:return
    body=STATE['body']
    if method and method.get('postprocess'):
        settings=PP_ACTOR.get_editor_property('settings')
        blendables=u.WeightedBlendables();entry=u.WeightedBlendable()
        entry.set_editor_property('weight',1.);entry.set_editor_property('object',u.load_asset(method['postprocess']))
        blendables.set_editor_property('array',[entry]);settings.set_editor_property('weighted_blendables',blendables)
        PP_ACTOR.set_editor_property('settings',settings)
        body.set_render_custom_depth(True);body.set_custom_depth_stencil_value(method['stencil_tag'])
    else:
        PP_ACTOR.set_editor_property('settings',PP_ORIGINAL)
        body.set_render_custom_depth(MESH_OUTLINE_ORIGINAL[0]);body.set_custom_depth_stencil_value(MESH_OUTLINE_ORIGINAL[1])

def pin():
    point=POINTS[CASES[STATE['index']][2]];target=STATE['target']
    if (target.get_actor_location()-point).length()>1.:target.set_actor_location_and_rotation(point,u.Rotator(),False,True)
    return target

def validate(method):
    body=STATE['body'];mesh=body.get_skeletal_mesh_asset()
    defaults=list(mesh.get_editor_property('materials'))
    assert mesh.get_path_name().split('.')[0]==research.current.original.mesh_path('NPC_R01_003')
    assert body.get_material(0)==defaults[0].get_editor_property('material_interface')
    assert body.get_material(0).get_path_name().split('.')[0]==research.current.HEAD
    expected=defaults[1].get_editor_property('material_interface') if method['kind']=='current' else MATERIALS[method['id']]
    assert body.get_material(1)==expected
    assert bool(body.get_editor_property('cast_shadow'))==method['cast_shadow']
    assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(expected,'ICHairScatter')-method['scatter'])<.00001
    assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(expected,'ICHairDensity')-method['density'])<.00001
    if method.get('postprocess'):
        assert body.get_editor_property('render_custom_depth') and body.get_editor_property('custom_depth_stencil_value')==249
        if method['id'] in ['14_hair_outline_filter','15_hair_boundary_filter']:assert u.SystemLibrary.get_console_variable_int_value('r.CustomDepth')==3
    return {'face_material':body.get_material(0).get_path_name(),'body_material':expected.get_path_name(),
        'baseline_uses_saved_defaults':method['kind']=='current','cast_shadow':method['cast_shadow'],'emissive':0.,
        'custom_depth_mode':u.SystemLibrary.get_console_variable_int_value('r.CustomDepth'),
        'render_custom_depth':body.get_editor_property('render_custom_depth'),'stencil_value':body.get_editor_property('custom_depth_stencil_value')}

def tick(delta):
    global PP_ORIGINAL,PP_ACTOR,MESH_OUTLINE_ORIGINAL
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        stage=STATE['stage'];now=time.monotonic()
        if stage=='world_control_original' and now>=STATE['ready']:
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(review.STATE['world'],str(OUT/'World_Original.png'))
            whole=next(m for m in METHODS if m['id']=='Character_Whole')
            render_mode(whole);outline_mode(whole)
            STATE.update(stage='world_control_selected',ready=now+4.5);report();return
        if stage=='world_control_selected' and now>=STATE['ready']:
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(review.STATE['world'],str(OUT/'World_Selected.png'))
            STATE.update(stage='next',world_control_complete=True,world_control_images=['World_Original.png','World_Selected.png']);report();return
        if stage in ['warmup','camera','settle']:pin()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            review.STATE['phase']='npc003_hair_research'
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            review.STATE['pc'].get_time().set_running(True)
            STATE.update(stage='find',ready=now+35);report();return
        if stage=='find':
            actors=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICNPCBase)
            targets={str(a.get_character_id()):a for a in actors if str(a.get_character_id()) in research.current.original.NPC_IDS}
            if len(targets)!=11:
                assert now<STATE['ready'],'Missing NPCs: '+str(list(targets))
                return
            review.STATE['pc'].get_time().set_running(False)
            STATE['targets']=targets;STATE['target']=targets['NPC_R01_003']
            for actor in actors:
                actor.set_actor_hidden_in_game(actor!=STATE['target'])
                controller=actor.get_controller()
                if controller:
                    controller.stop_movement();brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Hair research capture')
                    controller.set_actor_tick_enabled(False)
                actor.set_actor_tick_enabled(False);actor.set_actor_enable_collision(False)
                move=actor.get_component_by_class(u.CharacterMovementComponent);move.stop_movement_immediately();move.disable_movement()
            body=STATE['target'].get_editor_property('mesh');STATE['body']=body
            if EDGE_STUDY:audit_rendering()
            if any(m.get('postprocess') for m in METHODS):
                PP_ACTOR=next(a for a in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.PostProcessVolume) if a.get_name()=='PostProcessVolume_1')
                PP_ORIGINAL=PP_ACTOR.get_editor_property('settings')
                MESH_OUTLINE_ORIGINAL=(body.get_editor_property('render_custom_depth'),body.get_editor_property('custom_depth_stencil_value'))
            if EDGE_STUDY:
                names={name for method in METHODS for name in method.get('cvars',{})}
                STATE['console_defaults']={name:u.SystemLibrary.get_console_variable_int_value(name) for name in names}
            STATE['original_shadow']=bool(body.get_editor_property('cast_shadow'))
            body.set_editor_property('visibility_based_anim_tick_option',u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
            if body.get_anim_instance():body.get_anim_instance().montage_stop(.15)
            review.STATE['player'].set_actor_hidden_in_game(True)
            STATE['target'].set_actor_location_and_rotation(POINTS[CASES[0][2]],u.Rotator(),False,True)
            STATE.update(stage='warmup',ready=now+5.);report();return
        if stage=='warmup' and now>=STATE['ready']:
            relative=STATE['body'].get_socket_location('J_Bip_C_Head')-STATE['target'].get_actor_location()+u.Vector(0,0,-5)
            assert relative.length()<150.
            STATE['head_offset']=[relative.x,relative.y,relative.z]
            assert abs(relative.x)<10 and abs(relative.y)<10 and 35<relative.z<75,'Pose did not settle: '+str(relative)
            STATE.update(stage='next');report();return
        if stage=='next':
            if STATE['index']==len(CASES):
                if CHARACTER_OUTLINE and not SAVED_OUTLINE and not STATE.get('world_control_complete'):
                    STATE['target'].set_actor_hidden_in_game(True);render_mode();outline_mode()
                    STATE.update(stage='world_control_original',ready=now+4.5);report();return
                if EDGE_STUDY:render_mode()
                outline_mode()
                STATE['body'].set_editor_property('override_materials',[]);STATE['body'].set_cast_shadow(STATE['original_shadow'])
                assert not list(STATE['body'].get_editor_property('override_materials'))
                research.current.original.assert_protected(SCOPE)
                assert len(STATE['samples'])==len(CASES)
                STATE.update(stage='complete',complete=True,existing_assets_unchanged=True,original_runtime_materials_restored=True,permanent_selection_applied=SAVED_OUTLINE)
                report();u.unregister_slate_post_tick_callback(STATE['handle']);review.finish();return
            label,minute,place,method,closeup=CASES[STATE['index']]
            if EDGE_STUDY:render_mode(method)
            outline_mode(method)
            body=STATE['body'];body.set_editor_property('override_materials',[])
            if method['kind']!='current':body.set_material(1,MATERIALS[method['id']])
            body.set_cast_shadow(method['cast_shadow'])
            timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
            if current%1440!=minute:assert timer.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            STATE['target'].set_actor_location_and_rotation(POINTS[place],u.Rotator(),False,True)
            STATE.update(stage='camera',ready=now+.3);report();return
        if stage=='camera' and now>=STATE['ready']:
            label,minute,place,method,closeup=CASES[STATE['index']]
            focus=POINTS[place]+u.Vector(*STATE['head_offset']) if closeup else POINTS[place]
            location=focus+(u.Vector(160,95,5) if closeup else u.Vector(350,380,75))
            camera=review.STATE['camera'];camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True)
            camera.set_editor_property('field_of_view',35 if closeup else 40)
            STATE.update(stage='settle',ready=now+4.5);report();return
        if stage=='settle' and now>=STATE['ready']:
            label,minute,place,method,closeup=CASES[STATE['index']];target=pin();world=review.STATE['world']
            assert review.STATE['pc'].get_time().get_clock().game_minute%1440==minute
            assert (target.get_actor_location()-POINTS[place]).length()<2.
            suns=[sun for sun in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight) if sun.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
            assert [sun.get_name() for sun in suns]==['DirectionalLight_0']
            start=POINTS[place]+u.Vector(0,0,40)
            hit=u.SystemLibrary.line_trace_single(world,start,start-suns[0].get_actor_forward_vector()*50000.,u.TraceTypeQuery.ECC_VISIBILITY,True,
                list(STATE['targets'].values())+[review.STATE['player']],u.DrawDebugTrace.NONE,True)
            blocked=bool(hit and hit.to_dict().get('blocking_hit',False))
            if minute==600:assert blocked==(place=='Inside')
            validation=validate(method)
            filename=method['id']+'_'+label+('_Closeup' if closeup else '_Full')+'.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(world,str(OUT/filename))
            assert struct.unpack('>II',(OUT/filename).read_bytes()[16:24])==(1600,900)
            camera=review.STATE['camera'];loc=camera.get_world_location()
            STATE['samples'].append({'image':filename,'method':method['id'],'environment':label,'closeup':closeup,'minute':minute,
                'position':[POINTS[place].x,POINTS[place].y,POINTS[place].z],'camera':[loc.x,loc.y,loc.z],
                'field_of_view':35 if closeup else 40,'sun_ray_blocked':blocked,'lighting':review.light_state(),'validation':validation,
                'view_mode':method.get('view_mode','lit'),'diagnostic_console_changes':method.get('cvars',{}),
                'hair_only_postprocess_filter':method.get('postprocess'),'stencil_tag':method.get('stencil_tag')})
            STATE.update(stage='next',index=STATE['index']+1);report()
    except Exception:
        if EDGE_STUDY and 'world' in review.STATE:render_mode()
        outline_mode()
        STATE.update(stage='error',error=traceback.format_exc());report();u.log_error(STATE['error'])
        review.STATE['phase']='commands';u.unregister_slate_post_tick_callback(STATE['handle'])

STATE['handle']=u.register_slate_post_tick_callback(tick)
report()
