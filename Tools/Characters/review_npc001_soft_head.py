"""Isolated selected-NPC hair/head trials or saved-default four-environment review."""
import json
import struct
import sys
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
NPC003='-ICNPC003HairOnlyReview' in u.SystemLibrary.get_command_line()
TARGET_ID='NPC_R01_003' if NPC003 else 'NPC_R01_001'
HAIR_ONLY=NPC003 or '-ICNPC001HairOnlyReview' in u.SystemLibrary.get_command_line()
if NPC003:import setup_npc003_hair_only as soft
elif HAIR_ONLY:import setup_npc001_hair_only as soft
else:import setup_npc001_soft_head as soft
import review_player_world_lighting as review
FINAL=any(flag in u.SystemLibrary.get_command_line() for flag in ['-ICNPC001SoftHeadFinal','-ICNPC001HairOnlyFinal','-ICNPC003HairOnlyFinal'])
OUT=soft.OUT/('Final' if FINAL else 'Trials');OUT.mkdir(parents=True,exist_ok=True)
review.OUT=OUT;review.STATE['automatic_variants']=[]
SCOPE=json.loads((soft.OUT/'Scope.json').read_text(encoding='utf-8'))
MASTER=u.load_asset(soft.MASTER)
assert MASTER
POINTS={'Outside':u.Vector(-53990,629630,-5098.451662),'Inside':u.Vector(-53240,629940,-5069.725)}
ENVIRONMENTS=[('Night_Outside',1200,'Outside'),('Night_Inside',1200,'Inside'),('Day_Outside',600,'Outside'),('Day_Inside',600,'Inside')]
VARIANTS=[('Baseline',None,None,True),('NoSelfShadow',None,None,False),('SoftLow',.25,.65,True),('SoftMedium',.5,.45,True),('SoftHigh',.8,.25,True)]
if HAIR_ONLY:VARIANTS=[('Baseline',None,None,True),('HairSoft',.4,.6,True),('HairSoftPlus',.65,.45,True),('HairMore',.85,.3,True)]
if NPC003:VARIANTS=[('Baseline',None,None,True),('HairSubtle',.15,.7,True),('HairMild',.25,.65,True),('HairSoft',.4,.6,True)]
CASES=([('Saved',None,None,True,label,minute,place,close) for close in [False,True] for label,minute,place in ENVIRONMENTS] if FINAL else
    [(name,scatter,density,shadow,label,minute,place,True) for name,scatter,density,shadow in VARIANTS
        for label,minute,place in [ENVIRONMENTS[2],ENVIRONMENTS[1]]])
STATE={'stage':'waiting','index':0,'samples':[],'complete':False}

def report():
    (OUT/'Verification.json').write_text(json.dumps({k:v for k,v in STATE.items() if k not in ['handle','ready','targets','target','dynamic']},ensure_ascii=False,indent=2),encoding='utf-8')

def pin():
    point=POINTS[CASES[STATE['index']][6]]
    target=STATE['target']
    if (target.get_actor_location()-point).length()>1.:target.set_actor_location_and_rotation(point,u.Rotator(),False,True)
    return target

def validate():
    body=STATE['target'].get_editor_property('mesh');mesh=body.get_skeletal_mesh_asset()
    assert mesh.get_path_name().split('.')[0]==soft.original.mesh_path(TARGET_ID)
    assert not list(body.get_editor_property('override_materials'))
    head=body.get_material(0)
    assert head.get_path_name().split('.')[0]==soft.HEAD
    if HAIR_ONLY:
        import hashlib
        assert hashlib.sha256(soft.original.asset_file(soft.HEAD).read_bytes()).hexdigest()==SCOPE['face_baseline_hash']
        assert head.get_editor_property('parent').get_path_name().split('.')[0]==soft.original.feature_root_path()
        assert soft.original.effective(head,'shading_model','override_shading_model')==u.MaterialShadingModel.MSM_DEFAULT_LIT
        material=body.get_material(1)
        assert material==mesh.get_editor_property('materials')[1].get_editor_property('material_interface')
        assert soft.original.effective(material,'shading_model','override_shading_model')==u.MaterialShadingModel.MSM_FROM_MATERIAL_EXPRESSION
        root=material
        while isinstance(root,u.MaterialInstance):root=root.get_editor_property('parent')
        assert root==MASTER
        lib=u.MaterialEditingLibrary
        output=lib.get_material_property_input_node(root,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        assert output.get_editor_property('desc')==f'IC NPC{TARGET_ID[-3:]} Hair Only'
        pins={str(n).replace(' ','').lower():node for n,node in zip(lib.get_material_expression_input_names(output),lib.get_inputs_for_material_expression(root,output))}
        assert isinstance(pins['shadingmodel'],u.MaterialExpressionIf)
        assert isinstance(pins['subsurfacecolor'],u.MaterialExpressionMultiply)
        models=[e.get_editor_property('shading_model').name for e in lib.get_material_expressions(root) if isinstance(e,u.MaterialExpressionShadingModel)]
        assert set(models)=={'MSM_DEFAULT_LIT','MSM_SUBSURFACE'}
        old=lib.get_inputs_for_material_expression(root,pins['emissivecolor'])[0]
        old_pins={str(n).replace(' ','').lower():node for n,node in zip(lib.get_material_expression_input_names(old),lib.get_inputs_for_material_expression(root,old))}
        assert isinstance(old_pins['emissivecolor'],u.MaterialExpressionConstant) and old_pins['emissivecolor'].get_editor_property('r')==0.
        return {'face_restored_exact_bytes':not NPC003,'face_baseline_identical_bytes':True,'eyes_and_face_shader':'Default Lit baseline','hair_shader':'UV-selected Subsurface; other body pixels Default Lit',
            'hair_scatter':lib.get_material_instance_scalar_parameter_value(material,'ICHairScatter'),'hair_density':lib.get_material_instance_scalar_parameter_value(material,'ICHairDensity'),
            'emissive':0,'runtime_overrides':False}
    assert head.get_editor_property('parent')==MASTER
    assert soft.original.effective(head,'shading_model','override_shading_model')==u.MaterialShadingModel.MSM_SUBSURFACE
    assert soft.original.effective(head,'blend_mode','override_blend_mode')==u.BlendMode.BLEND_MASKED
    source=u.load_asset('/Game/IsekaiClass/NPC/SampleNPC/R01/NPC_R01_001/MI_N00_000_00_FaceMouth_00_FACE__Instance_')
    lib=u.MaterialEditingLibrary
    assert lib.get_material_instance_texture_parameter_value(head,'ICFeatureAlbedoTexture')==lib.get_material_instance_texture_parameter_value(source,'gltf_tex_diffuse')
    assert abs(lib.get_material_instance_scalar_parameter_value(head,'ICAlbedoScale')-.65)<.00001
    assert body.get_material(1)==mesh.get_editor_property('materials')[1].get_editor_property('material_interface')
    output=lib.get_material_property_input_node(MASTER,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    pins={str(n).replace(' ','').lower():node for n,node in zip(lib.get_material_expression_input_names(output),lib.get_inputs_for_material_expression(MASTER,output))}
    assert isinstance(pins['emissivecolor'],u.MaterialExpressionConstant) and pins['emissivecolor'].get_editor_property('r')==0.
    assert isinstance(pins['opacitymask'],u.MaterialExpressionMultiply)
    assert MASTER.get_editor_property('used_with_skeletal_mesh') and MASTER.get_editor_property('used_with_morph_targets')
    return {'head':head.get_path_name(),'scatter':lib.get_material_instance_scalar_parameter_value(head,'ICHeadScatter'),
        'density':lib.get_material_instance_scalar_parameter_value(head,'ICHeadDensity'),'emissive':0,'original_texture_and_alpha':True,'runtime_overrides':False}

def tick(delta):
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        stage=STATE['stage'];now=time.monotonic()
        if stage in ['warmup','camera','settle']:pin()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            review.STATE['phase']='npc001_soft_head'
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            review.STATE['pc'].get_time().set_running(True)
            STATE.update(stage='find',ready=now+35);report();return
        if stage=='find':
            actors=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICNPCBase)
            targets={str(a.get_character_id()):a for a in actors if str(a.get_character_id()) in soft.original.NPC_IDS}
            if len(targets)!=11:
                assert now<STATE['ready'],'Missing NPCs: '+str(list(targets))
                return
            review.STATE['pc'].get_time().set_running(False)
            STATE['targets']=targets;STATE['target']=targets[TARGET_ID]
            for actor in actors:
                actor.set_actor_hidden_in_game(actor!=STATE['target'])
                controller=actor.get_controller()
                if controller:
                    controller.stop_movement();brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('NPC001 head material review')
                    controller.set_actor_tick_enabled(False)
                actor.set_actor_tick_enabled(False);actor.set_actor_enable_collision(False)
                move=actor.get_component_by_class(u.CharacterMovementComponent);move.stop_movement_immediately();move.disable_movement()
            if NPC003:
                body=STATE['target'].get_editor_property('mesh')
                body.set_editor_property('visibility_based_anim_tick_option',u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
                animation=body.get_anim_instance()
                if animation:animation.montage_stop(.15)
            review.STATE['player'].set_actor_hidden_in_game(True)
            STATE['target'].set_actor_location_and_rotation(POINTS[CASES[0][6]],u.Rotator(),False,True)
            STATE.update(stage='warmup',ready=now+5.);report();return
        if stage=='warmup' and now>=STATE['ready']:
            STATE.update(stage='next');report();return
        if stage=='next':
            if STATE['index']==len(CASES):
                soft.original.assert_protected(SCOPE)
                STATE.update(stage='complete',complete=True,protected_files_unchanged=True,saved_default_materials=FINAL)
                report();u.unregister_slate_post_tick_callback(STATE['handle']);review.finish();return
            name,scatter,density,shadow,label,minute,place,close=CASES[STATE['index']]
            body=STATE['target'].get_editor_property('mesh')
            body.set_editor_property('override_materials',[]);body.set_cast_shadow(shadow)
            if scatter is not None:
                if HAIR_ONLY:
                    dynamic=body.create_dynamic_material_instance(1,u.load_asset(soft.PREVIEW))
                    dynamic.set_scalar_parameter_value('ICHairScatter',scatter);dynamic.set_scalar_parameter_value('ICHairDensity',density)
                    STATE['dynamic']=dynamic
                else:
                    head=body.get_material(0);lib=u.MaterialEditingLibrary
                    dynamic=body.create_dynamic_material_instance(0,MASTER)
                    dynamic.set_texture_parameter_value('ICFeatureAlbedoTexture',lib.get_material_instance_texture_parameter_value(head,'ICFeatureAlbedoTexture'))
                    dynamic.set_vector_parameter_value('ICFeatureAlbedoTint',lib.get_material_instance_vector_parameter_value(head,'ICFeatureAlbedoTint'))
                    for param,value in [('ICHeadScatter',scatter),('ICHeadDensity',density),('ICAlbedoScale',.65)]:dynamic.set_scalar_parameter_value(param,value)
                    STATE['dynamic']=dynamic;body.set_material(0,dynamic)
            elif FINAL:STATE['validation']=validate()
            timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
            if current%1440!=minute:assert timer.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            STATE['target'].set_actor_location_and_rotation(POINTS[place],u.Rotator(),False,True)
            STATE.update(stage='camera',ready=now+.35);report();return
        if stage=='camera' and now>=STATE['ready']:
            name,scatter,density,shadow,label,minute,place,close=CASES[STATE['index']]
            target=pin();body=target.get_editor_property('mesh')
            if 'head_offset' not in STATE:
                relative=body.get_socket_location('J_Bip_C_Head')-target.get_actor_location()+u.Vector(0,0,-5)
                assert relative.length()<150.,str(relative)
                STATE['head_offset']=[relative.x,relative.y,relative.z]
            focus=POINTS[place]+u.Vector(*STATE['head_offset']) if close else POINTS[place]
            location=focus+(u.Vector(160,95,5) if close else u.Vector(350,380,75))
            camera=review.STATE['camera'];camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True)
            camera.set_editor_property('field_of_view',35 if close else 40)
            STATE.update(stage='settle',ready=now+5.);report();return
        if stage=='settle' and now>=STATE['ready']:
            name,scatter,density,shadow,label,minute,place,close=CASES[STATE['index']]
            target=pin();world=review.STATE['world']
            assert review.STATE['pc'].get_time().get_clock().game_minute%1440==minute
            assert (target.get_actor_location()-POINTS[place]).length()<2.
            suns=[sun for sun in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight) if sun.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
            assert [sun.get_name() for sun in suns]==['DirectionalLight_0']
            start=POINTS[place]+u.Vector(0,0,40)
            hit=u.SystemLibrary.line_trace_single(world,start,start-suns[0].get_actor_forward_vector()*50000.,u.TraceTypeQuery.ECC_VISIBILITY,True,
                list(STATE['targets'].values())+[review.STATE['player']],u.DrawDebugTrace.NONE,True)
            blocked=bool(hit and hit.to_dict().get('blocking_hit',False))
            if minute==600:assert blocked==(place=='Inside')
            filename=(TARGET_ID+'_' if FINAL else name+'_')+label+('_Closeup' if close else '')+'.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(world,str(OUT/filename))
            assert struct.unpack('>II',(OUT/filename).read_bytes()[16:24])==(1600,900)
            STATE['samples'].append({'image':filename,'label':label,'variant':name,'scatter':scatter,'density':density,'cast_shadow':shadow,
                'closeup':close,'minute':minute,'sun_ray_blocked':blocked,'lighting':review.light_state(),
                'camera_location':str(review.STATE['camera'].get_world_location()),'npc_location':str(target.get_actor_location())})
            STATE.update(index=STATE['index']+1,stage='next');report()
    except Exception:
        STATE.update(stage='error',error=traceback.format_exc());report();u.log_error(STATE['error'])
        review.STATE['phase']='commands';u.unregister_slate_post_tick_callback(STATE['handle'])

STATE['handle']=u.register_slate_post_tick_callback(tick)
report()
