"""Verify method 7 in native gameplay: both presets, four times, and sun occlusion."""
import hashlib
import json
import math
import struct
import sys
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
assert '-ICCharacterLightingReview' in u.SystemLibrary.get_command_line()
OUT=ROOT/'Saved/CharacterLighting/EnvironmentLighting/TimeOfDay'
OUT.mkdir(parents=True,exist_ok=True)
MAP='/Game/IsekaiClass/World/Maps/L_OpenWorld'
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]!=MAP:
    u.EditorLoadingAndSavingUtils.load_map(MAP)
# Invisible, query-only fixture. It never changes rendered lighting or a saved map.
fixture=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,-100000),transient=False)
fixture.tags=['ICLightingReview.Occluder']
fixture.set_actor_hidden_in_game(True)
shape=fixture.get_component_by_class(u.StaticMeshComponent)
shape.set_mobility(u.ComponentMobility.MOVABLE)
shape.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
shape.set_visibility(False,True)
shape.set_editor_property('cast_shadow',False)
shape.set_collision_enabled(u.CollisionEnabled.QUERY_ONLY)
shape.set_collision_response_to_all_channels(u.CollisionResponseType.ECR_IGNORE)
shape.set_collision_response_to_channel(u.CollisionChannel.ECC_VISIBILITY,u.CollisionResponseType.ECR_BLOCK)
fixture.set_actor_scale3d(u.Vector(10,10,10))

import review_player_world_lighting as review
review.OUT=OUT
review.TIMES[:]=[('04h',240),('10h',600),('14h',840),('20h',1200)]
review.STATE['automatic_variants']=[]
CHECK={'stage':'waiting','transition_down':[],'transition_up':[]}


def write_result():
    result={k:v for k,v in CHECK.items() if k not in ['handle','occluder']}
    (OUT/'Verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')


def clock(minute):
    timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
    assert timer.jump_game_minutes((minute-current%1440+1440)%1440)
    for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()


def verify_slots():
    rows=[]
    for sex in ['F','M','F','M']:
        player=review.STATE['player'];player.apply_preset(0 if sex=='M' else 1)
        component=player.get_component_by_class(u.ICPlayerToonBrightnessComponent)
        body=player.get_editor_property('mesh');slots=body.get_skeletal_mesh_asset().get_editor_property('materials')
        assert len(body.get_editor_property('override_materials'))==len(slots)
        assert abs(component.get_brightness()-(.18+.34*math.sqrt(.08)))<.0001
        for index,slot in enumerate(slots):
            source=slot.get_editor_property('material_interface');material=body.get_material(index)
            assert material.get_editor_property('parent')==source
            assert source.get_path_name().startswith('/Game/IsekaiClass/Player/Materials/ToonLighting/')
            assert review.LIGHTING.effective_shading_model(source)==u.MaterialShadingModel.MSM_UNLIT
            assert abs(material.get_scalar_parameter_value('ICCharacterBrightness')-component.get_brightness())<.0001
            if len(rows)<19:rows.append({'sex':sex,'slot':index,'material':source.get_path_name(),'brightness':component.get_brightness(),'shader':'Unlit'})
    assert len(rows)==19
    return rows


def verify_tick(delta):
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        stage=CHECK['stage'];now=time.monotonic()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            player=review.STATE['player'];camera=review.STATE['camera'];point=u.Vector(-60465,561595,-6440)
            player.set_actor_location_and_rotation(point,u.Rotator(),False,True)
            location=point+u.Vector(350,380,75)
            camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)
            CHECK['stage']='photos';review.queue('NativeToon');return
        if stage=='photos' and review.STATE['phase']=='commands':
            assert len(review.STATE['samples'])==8
            for sample in review.STATE['samples']:
                expected=.82 if sample['minute'] in [600,840] else .18+.34*math.sqrt(.08)
                assert abs(sample['native_brightness']-expected)<.0002,str(sample)
                image=OUT/(sample['name']+'.png')
                assert struct.unpack('>II',image.read_bytes()[16:24])==(1600,900),str(image)
            CHECK['slots']=verify_slots();CHECK['samples']=review.STATE['samples']
            world=review.STATE['world']
            fixtures=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.StaticMeshActor) if 'ICLightingReview.Occluder' in [str(t) for t in a.tags]]
            assert len(fixtures)==1
            CHECK['occluder']=fixtures[0];clock(600)
            CHECK.update(stage='sun_baseline',ready=now+3.)
            write_result();return
        if stage=='sun_baseline' and now>=CHECK['ready']:
            player=review.STATE['player'];component=player.get_component_by_class(u.ICPlayerToonBrightnessComponent)
            assert abs(component.get_brightness()-.82)<.0002
            sun=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.DirectionalLight)[0]
            direction=u.MathLibrary.get_forward_vector(sun.get_actor_rotation())
            start=player.get_actor_location()+u.Vector(0,0,40)
            CHECK['occluder'].set_actor_location(start-direction*1200.,False,True)
            hit=u.SystemLibrary.line_trace_single(review.STATE['world'],start,start-direction*50000.,u.TraceTypeQuery.ECC_VISIBILITY,True,[player],u.DrawDebugTrace.NONE,True)
            assert hit and hit.to_dict().get('blocking_hit',False),'Fixture did not block the sun ray'
            CHECK['transition_down'].append(component.get_brightness())
            CHECK.update(stage='shadow',ready=now+3.);return
        if stage in ['shadow','sun_return']:
            brightness=review.STATE['player'].get_component_by_class(u.ICPlayerToonBrightnessComponent).get_brightness()
            CHECK['transition_down' if stage=='shadow' else 'transition_up'].append(brightness)
            if now<CHECK['ready']:return
            if stage=='shadow':
                assert abs(brightness-.52)<.001,str(brightness)
                assert any(.53<v<.81 for v in CHECK['transition_down']),'Shadow transition snapped'
                CHECK['occluder'].set_actor_location(u.Vector(0,0,-100000),False,True)
                CHECK.update(stage='sun_return',ready=now+3.);return
            assert abs(brightness-.82)<.001,str(brightness)
            assert any(.53<v<.81 for v in CHECK['transition_up']),'Sun transition snapped'
            for path,digest in review.HASHES.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest
            applied=json.loads((OUT.parent/'Applied.json').read_text(encoding='utf-8'))
            for path,digest in applied['protected_hashes'].items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,path
            CHECK.update(stage='complete',complete=True,protected_files_unchanged=True,
                         occlusion_result={'open_sun':.82,'blocked_sun':.52,'smooth_transition':True})
            write_result();u.unregister_slate_post_tick_callback(CHECK['handle']);review.finish()
    except Exception:
        CHECK.update(stage='error',error=traceback.format_exc())
        review.STATE['phase']='commands';write_result();u.log_error(CHECK['error'])
        u.unregister_slate_post_tick_callback(CHECK['handle'])


CHECK['handle']=u.register_slate_post_tick_callback(verify_tick)
