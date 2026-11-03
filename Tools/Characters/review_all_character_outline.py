"""Verify 13 persisted appearances in a fresh real gameplay world, without overrides."""
import json,struct,sys,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));sys.path.insert(0,str(ROOT/'Tools/Characters'))
import apply_all_character_outline as selected
assert '-ICAllCharacterOutlineReview' in u.SystemLibrary.get_command_line()
APPLIED=json.loads((selected.OUT/'Applied.json').read_text(encoding='utf-8'))
SCOPE=json.loads((selected.OUT/'Scope.json').read_text(encoding='utf-8'));selected.check(SCOPE)
assert json.loads((selected.OUT/'SavedVerification.json').read_text(encoding='utf-8'))['complete']
ROWS={s['id']:s for s in APPLIED['subjects']}
import review_player_world_lighting as review
OUT=selected.OUT/'FinalCaptures';OUT.mkdir(parents=True,exist_ok=True);review.OUT=OUT;review.STATE['automatic_variants']=[]
POINTS={'Outside':u.Vector(-53990,629630,-5098.451662),'Inside':u.Vector(-53240,629940,-5069.725)}
ENV=[('Night_Outside',1200,'Outside'),('Night_Inside',1200,'Inside'),('Day_Outside',600,'Outside'),('Day_Inside',600,'Inside')]
CASES=[(identifier,label,minute,place,close) for identifier in ROWS for label,minute,place in ENV for close in ([False,True] if identifier.startswith('Player_') else [False])]
CASES += [(identifier,'Day_Outside',600,'Outside',True) for identifier in ROWS if identifier.startswith('NPC_')]
STATE={'stage':'waiting','index':0,'samples':[],'complete':False}

def report():
    (selected.OUT/'FinalVerification.json').write_text(json.dumps({k:v for k,v in STATE.items() if k not in ['handle','ready','actors','target']},ensure_ascii=False,indent=2),encoding='utf-8')

def pin():
    identifier,label,minute,place,close=CASES[STATE['index']]
    target=review.STATE['player'] if identifier.startswith('Player_') else STATE['actors'][identifier]
    if (target.get_actor_location()-POINTS[place]).length()>1.:target.set_actor_location_and_rotation(POINTS[place],u.Rotator(),False,True)
    return target

def validate(identifier,target):
    record=ROWS[identifier];body=target.get_editor_property('mesh');mesh=body.get_skeletal_mesh_asset()
    assert mesh.get_path_name().split('.')[0]==record['mesh']
    assert selected.npc.non_material_state(mesh)==record['non_material_state']
    assert not list(body.get_editor_property('override_materials')),'Unexpected material overrides: '+identifier
    assert body.get_editor_property('render_custom_depth') and body.get_editor_property('custom_depth_stencil_value')==249,identifier+' stencil'
    assert body.get_editor_property('cast_shadow')
    for index,slot in enumerate(mesh.get_editor_property('materials')):assert body.get_material(index)==slot.get_editor_property('material_interface')
    for row in record['changed_materials']:
        mat=body.get_material(row['slot']);assert mat.get_path_name()==row['material']
        assert u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mat,'ICResearchHairDiffuse').get_path_name()==row['diffuse']
        assert u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat,'ICHairScatter')==0.
        assert u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat,'ICHairDensity')==1.
    return {'saved_defaults_without_overrides':True,'stencil':249,'changed_materials_verified':len(record['changed_materials']),'skeleton_physics_animation_preserved':True}

def tick(delta):
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        stage=STATE['stage'];now=time.monotonic()
        if stage in ['warmup','settle']:pin()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            review.STATE['phase']='all_character_outline';assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            assert u.SystemLibrary.get_console_variable_int_value('r.CustomDepth')==3
            review.STATE['pc'].get_time().set_running(True);STATE.update(stage='find',ready=now+40);report();return
        if stage=='find':
            actors=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICNPCBase);targets={str(a.get_character_id()):a for a in actors}
            if not all(n in targets for n in selected.npc.NPC_IDS):assert now<STATE['ready'],'NPCs did not spawn';return
            review.STATE['pc'].get_time().set_running(False);STATE['actors']=targets
            pp=next(a for a in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.PostProcessVolume) if a.get_name()=='PostProcessVolume_1')
            entries=pp.get_editor_property('settings').get_editor_property('weighted_blendables').get_editor_property('array')
            assert len(entries)==1 and entries[0].get_editor_property('object').get_path_name().split('.')[0]==selected.approved.PP
            STATE['saved_postprocess']=entries[0].get_editor_property('object').get_path_name()
            for actor in list(actors)+[review.STATE['player']]:
                actor.set_actor_hidden_in_game(True);actor.set_actor_tick_enabled(False);actor.set_actor_enable_collision(False)
                controller=actor.get_controller()
                if isinstance(controller,u.AIController):
                    controller.stop_movement();brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Character outline saved review')
                    controller.set_actor_tick_enabled(False)
                move=actor.get_component_by_class(u.CharacterMovementComponent)
                if move:move.stop_movement_immediately();move.disable_movement()
                body=actor.get_editor_property('mesh');body.set_editor_property('visibility_based_anim_tick_option',u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
                if body.get_anim_instance():body.get_anim_instance().montage_stop(.15)
            STATE.update(stage='next');report();return
        if stage=='next':
            if STATE['index']==len(CASES):
                selected.check(SCOPE);assert len(STATE['samples'])==71
                STATE.update(stage='complete',complete=True,appearance_count=13,capture_count=71,actual_saved_defaults=True,protected_files_unchanged=True)
                report();u.unregister_slate_post_tick_callback(STATE['handle']);review.finish();return
            identifier,label,minute,place,close=CASES[STATE['index']]
            player=review.STATE['player']
            if identifier.startswith('Player_') and STATE.get('player_preset')!=identifier:
                player.apply_preset(1 if identifier=='Player_F' else 0);STATE['player_preset']=identifier
                move=player.get_component_by_class(u.CharacterMovementComponent);move.stop_movement_immediately();move.disable_movement()
                brightness=player.get_component_by_class(u.ICPlayerToonBrightnessComponent)
                assert not brightness,'Brightness controller must not replace saved defaults'
                if player.get_editor_property('mesh').get_anim_instance():player.get_editor_property('mesh').get_anim_instance().montage_stop(.15)
            target=player if identifier.startswith('Player_') else STATE['actors'][identifier]
            for actor in list(STATE['actors'].values())+[player]:actor.set_actor_hidden_in_game(actor!=target)
            timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
            if current%1440!=minute:assert timer.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            pin();STATE.update(stage='warmup',ready=now+2.,pose_deadline=now+45.);report();return
        if stage=='warmup' and now>=STATE['ready']:
            identifier,label,minute,place,close=CASES[STATE['index']];target=pin();body=target.get_editor_property('mesh')
            relative=body.get_socket_location('J_Bip_C_Head')-target.get_actor_location()
            if not(abs(relative.x)<15 and abs(relative.y)<15 and 30<relative.z<110):
                assert now<STATE['pose_deadline'],'Unsettled pose: '+identifier+' '+str(relative);STATE['ready']=now+.5;return
            focus=POINTS[place]+relative+u.Vector(0,0,-5) if close else POINTS[place]
            location=focus+(u.Vector(160,95,5) if close else u.Vector(350,380,75))
            camera=review.STATE['camera'];camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True);camera.set_editor_property('field_of_view',35 if close else 40)
            STATE.update(stage='settle',ready=now+4.5);report();return
        if stage=='settle' and now>=STATE['ready']:
            identifier,label,minute,place,close=CASES[STATE['index']];target=pin();world=review.STATE['world']
            result=validate(identifier,target);assert review.STATE['pc'].get_time().get_clock().game_minute%1440==minute
            suns=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight) if a.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
            assert [a.get_name() for a in suns]==['DirectionalLight_0']
            start=POINTS[place]+u.Vector(0,0,40)
            hit=u.SystemLibrary.line_trace_single(world,start,start-suns[0].get_actor_forward_vector()*50000.,u.TraceTypeQuery.ECC_VISIBILITY,True,list(STATE['actors'].values())+[review.STATE['player']],u.DrawDebugTrace.NONE,True)
            blocked=bool(hit and hit.to_dict().get('blocking_hit',False))
            if minute==600:assert blocked==(place=='Inside')
            name=identifier+'_'+label+('_Closeup' if close else '_Full')+'.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(world,str(OUT/name));assert struct.unpack('>II',(OUT/name).read_bytes()[16:24])==(1600,900)
            STATE['samples'].append({'subject':identifier,'environment':label,'minute':minute,'closeup':close,'image':name,'sun_ray_blocked':blocked,'validation':result,'lighting':review.light_state()})
            STATE.update(stage='next',index=STATE['index']+1);report()
    except Exception:
        STATE.update(stage='error',error=traceback.format_exc());report();u.log_error(STATE['error']);review.STATE['phase']='commands';u.unregister_slate_post_tick_callback(STATE['handle'])

STATE['handle']=u.register_slate_post_tick_callback(tick);report()
