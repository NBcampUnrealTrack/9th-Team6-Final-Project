"""Fresh gameplay verification: eleven NPCs, four environments and eye closeups.

NPC AI and placement are fixed only in the isolated PIE review. Material overrides
are never assigned; each NPC must render its saved skeletal-mesh default materials.
"""
import json
import struct
import sys
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
import setup_npc_reactive_lighting as lighting
import review_player_world_lighting as review
OUT=lighting.OUT/'Final';OUT.mkdir(parents=True,exist_ok=True)
APPLIED=json.loads((lighting.OUT/'Applied.json').read_text(encoding='utf-8'))
AUDIT=json.loads((lighting.OUT/'Audit.json').read_text(encoding='utf-8'))
ROWS={row['id']:row for row in AUDIT['npcs']}
review.OUT=OUT;review.STATE['automatic_variants']=[]
POINTS={'Outside':u.Vector(-53990,629630,-5098.451662),'Inside':u.Vector(-53240,629940,-5069.725)}
CASES=[(identifier,label,minute,POINTS[place],closeup) for identifier in lighting.NPC_IDS
    for label,minute,place,closeup in [
        ('Night_Outside',1200,'Outside',False),('Night_Inside',1200,'Inside',False),
        ('Day_Outside',600,'Outside',False),('Day_Inside',600,'Inside',False),
        ('Day_Outside_Closeup',600,'Outside',True)]]
CHECK={'stage':'waiting','index':0,'samples':[],'graphs':{},'complete':False}

def report():
    (OUT/'Verification.json').write_text(json.dumps({key:value for key,value in CHECK.items() if key not in ['handle','ready','targets']},ensure_ascii=False,indent=2),encoding='utf-8')

def validate(identifier,body):
    mesh=body.get_skeletal_mesh_asset()
    assert mesh.get_path_name()==ROWS[identifier]['mesh']
    assert lighting.non_material_state(mesh)==ROWS[identifier]['non_material']
    assert not list(body.get_editor_property('override_materials')),'Runtime override on '+identifier
    result=[]
    for index,slot in enumerate(mesh.get_editor_property('materials')):
        material=body.get_material(index);source=ROWS[identifier]['materials'][index]
        assert material==slot.get_editor_property('material_interface')
        assert material.get_path_name().startswith(lighting.DEST+'/'+identifier+'/')
        assert lighting.effective(material,'shading_model','override_shading_model')==u.MaterialShadingModel.MSM_DEFAULT_LIT
        scale=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material,'ICAlbedoScale');assert abs(scale-.65)<.00001
        features=source['category']=='features'
        if features:
            texture=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(material,'ICFeatureAlbedoTexture')
            assert texture.get_path_name()==source['texture']
            assert material.get_editor_property('parent').get_path_name().split('.')[0]==lighting.feature_root_path()
            assert lighting.effective(material,'blend_mode','override_blend_mode')==u.BlendMode.BLEND_MASKED
        else:
            for name in ['bUseLight','bUseLightForce','bUseFilmTonemapInverse','bDivAdaptation','bUseMatCap','bUseRimLight','bUseCustomReflection']:
                assert not u.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(material,name),(identifier,name)
        root=material
        while isinstance(root,u.MaterialInstance):root=root.get_editor_property('parent')
        if root.get_path_name() not in CHECK['graphs']:
            output=u.MaterialEditingLibrary.get_material_property_input_node(root,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
            names=u.MaterialEditingLibrary.get_material_expression_input_names(output)
            inputs=u.MaterialEditingLibrary.get_inputs_for_material_expression(root,output)
            pins={str(name).replace(' ','').lower():node for name,node in zip(names,inputs)}
            assert isinstance(pins['emissivecolor'],u.MaterialExpressionConstant) and pins['emissivecolor'].get_editor_property('r')==0.
            assert isinstance(pins['basecolor'],u.MaterialExpressionMultiply)
            assert root.get_editor_property('used_with_skeletal_mesh') and root.get_editor_property('used_with_morph_targets')
            if features:assert isinstance(pins['opacitymask'],u.MaterialExpressionMultiply)
            CHECK['graphs'][root.get_path_name()]={'emissive':0.,'feature_surface':features,'skeletal_mesh_and_morph_usage':True}
        result.append({'slot':index,'material':material.get_path_name(),'category':source['category'],'albedo_scale':scale})
    assert len(result)==2
    return result

def pin_target():
    identifier,label,minute,point,closeup=CASES[CHECK['index']]
    target=CHECK['targets'][identifier]
    if (target.get_actor_location()-point).length()>1.:target.set_actor_location_and_rotation(point,u.Rotator(),False,True)
    return target

def tick(delta):
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        now=time.monotonic();stage=CHECK['stage']
        if stage in ['camera','settle']:pin_target()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            review.STATE['phase']='npc_native_lighting_review'
            review.STATE['pc'].get_time().set_running(True)
            CHECK.update(stage='find_npcs',ready=now+35.);report();return
        if stage=='find_npcs':
            actors=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICNPCBase)
            targets={str(actor.get_character_id()):actor for actor in actors if str(actor.get_character_id()) in lighting.NPC_IDS}
            if len(targets)!=11:
                assert now<CHECK['ready'],'Missing NPCs: '+str(sorted(set(lighting.NPC_IDS)-set(targets)))
                return
            review.STATE['pc'].get_time().set_running(False)
            CHECK['targets']=targets
            for actor in actors:actor.set_actor_hidden_in_game(True)
            for actor in targets.values():
                controller=actor.get_controller()
                if controller:
                    controller.stop_movement();brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('NPC material review')
                    controller.set_actor_tick_enabled(False)
                actor.set_actor_tick_enabled(False);actor.set_actor_enable_collision(False)
                move=actor.get_component_by_class(u.CharacterMovementComponent);move.stop_movement_immediately();move.disable_movement()
                validate(str(actor.get_character_id()),actor.get_editor_property('mesh'))
            for actor in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICWeaponActor):actor.set_actor_hidden_in_game(True)
            review.STATE['player'].set_actor_hidden_in_game(True)
            CHECK.update(stage='next');report();return
        if stage=='next':
            if CHECK['index']==len(CASES):
                assert len(CHECK['samples'])==55
                assert set(row['npc'] for row in CHECK['samples'])==set(lighting.NPC_IDS)
                lighting.assert_protected(APPLIED)
                CHECK.update(stage='complete',complete=True,saved_default_materials_without_runtime_overrides=True,
                    world_player_and_original_materials_unchanged=True,all_11_npcs_verified=True)
                report();u.unregister_slate_post_tick_callback(CHECK['handle']);review.finish();return
            identifier,label,minute,point,closeup=CASES[CHECK['index']]
            for key,actor in CHECK['targets'].items():actor.set_actor_hidden_in_game(key!=identifier)
            timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
            if current%1440!=minute:timer.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            target=CHECK['targets'][identifier];target.set_actor_location_and_rotation(point,u.Rotator(),False,True)
            CHECK.update(stage='camera',ready=now+.35);return
        if stage=='camera' and now>=CHECK['ready']:
            identifier,label,minute,point,closeup=CASES[CHECK['index']]
            target=pin_target();body=target.get_editor_property('mesh')
            focus=body.get_socket_location('J_Bip_C_Head')+u.Vector(0,0,-5) if closeup else point
            location=focus+(u.Vector(160,95,5) if closeup else u.Vector(350,380,75))
            review.STATE['camera'].set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True)
            review.STATE['camera'].set_editor_property('field_of_view',35 if closeup else 40)
            CHECK.update(stage='settle',ready=now+4.5);return
        if stage=='settle' and now>=CHECK['ready']:
            identifier,label,minute,point,closeup=CASES[CHECK['index']];target=pin_target();world=review.STATE['world']
            assert (target.get_actor_location()-point).length()<2.
            assert review.STATE['pc'].get_time().get_clock().game_minute%1440==minute
            suns=[actor for actor in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight)
                  if actor.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
            assert [actor.get_name() for actor in suns]==['DirectionalLight_0']
            cycles=u.GameplayStatics.get_all_actors_of_class(world,u.ICDayNightController)
            assert len(cycles)==1 and [actor.get_name() for actor in cycles[0].sun_lights]==['DirectionalLight_0']
            start=point+u.Vector(0,0,40)
            ignored=list(CHECK['targets'].values())+[review.STATE['player']]
            hit=u.SystemLibrary.line_trace_single(world,start,start-suns[0].get_actor_forward_vector()*50000.,
                u.TraceTypeQuery.ECC_VISIBILITY,True,ignored,u.DrawDebugTrace.NONE,True)
            blocked=bool(hit and hit.to_dict().get('blocking_hit',False))
            if minute==600:assert blocked==('Inside' in label),(identifier,label,blocked)
            rows=validate(identifier,target.get_editor_property('mesh'))
            image=identifier+'_'+label+'.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(world,str(OUT/image))
            assert struct.unpack('>II',(OUT/image).read_bytes()[16:24])==(1600,900)
            CHECK['samples'].append({'npc':identifier,'label':label,'minute':minute,'image':image,'slots':rows,
                'location':[point.x,point.y,point.z],'sun_ray_blocked':blocked,'lighting':review.light_state()})
            CHECK.update(stage='next',index=CHECK['index']+1);report()
    except Exception:
        CHECK.update(stage='error',error=traceback.format_exc());report();u.log_error(CHECK['error'])
        review.STATE['phase']='commands';u.unregister_slate_post_tick_callback(CHECK['handle'])

CHECK['handle']=u.register_slate_post_tick_callback(tick)
report()
