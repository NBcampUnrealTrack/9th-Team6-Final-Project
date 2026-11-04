"""Compare material-only sunlight compensation against the selected method-8 baseline."""
import sys,json,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
assert '-ICCharacterLightingReview' in u.SystemLibrary.get_command_line()
import setup_player_single_sun_lighting as single
MATERIALS=single.create_materials(1.)
import review_player_world_lighting as review
OUT=single.OUT/'SunlightTuning';OUT.mkdir(parents=True,exist_ok=True)
review.OUT=OUT;review.STATE['automatic_variants']=[]
TEST={'stage':'waiting','samples':[],'index':0}
CASES=[(scale,hour,minute) for scale in [1.,.75,.55,.4] for hour,minute in [('10h',600),('14h',840)]]


def report():
    (OUT/'Results.json').write_text(json.dumps({k:v for k,v in TEST.items() if k in ['stage','samples','index','error']},ensure_ascii=False,indent=2),encoding='utf-8')


def tick(delta):
    try:
        if review.STATE.get('error'):raise RuntimeError(review.STATE['error'])
        stage=TEST['stage'];now=time.monotonic()
        if stage=='waiting' and review.STATE['phase']=='freeze':
            player=review.STATE['player'];camera=review.STATE['camera'];point=u.Vector(-60465,561595,-6440)
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
            player.set_actor_location_and_rotation(point,u.Rotator(),False,True)
            location=point+u.Vector(350,380,75)
            camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)
            for sun in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.DirectionalLight):
                sun.get_component_by_class(u.DirectionalLightComponent).set_visibility(sun.get_name()=='DirectionalLight_0',True)
            review.STATE['phase']='single_sun_tuning';TEST['stage']='next';return
        if stage=='next':
            if TEST['index']==len(CASES):
                TEST['stage']='complete';report();review.STATE['phase']='commands';review.report();return
            scale,hour,minute=CASES[TEST['index']]
            player=review.STATE['player'];player.apply_preset(1)
            assert not player.get_component_by_class(u.ICPlayerToonBrightnessComponent),'Legacy brightness controller is still attached'
            body=player.get_editor_property('mesh')
            for i,m in enumerate(MATERIALS['F']):
                dynamic=body.create_dynamic_material_instance(i,m)
                dynamic.set_scalar_parameter_value('PostLightScaleMat',scale)
            timer=review.STATE['pc'].get_time();current=timer.get_clock().game_minute
            assert timer.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            TEST.update(stage='settle',ready=now+6.);return
        if stage=='settle' and now>=TEST['ready']:
            scale,hour,minute=CASES[TEST['index']];name=f'Post{int(scale*100):03}_{hour}.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(review.STATE['world'],str(OUT/name))
            TEST['samples'].append({'post_light_scale':scale,'hour':hour,'minute':minute,'image':name,'lighting':review.light_state()})
            TEST.update(stage='next',index=TEST['index']+1);report()
    except Exception:
        TEST.update(stage='error',error=traceback.format_exc());report();u.log_error(TEST['error']);review.STATE['phase']='commands'
        u.unregister_slate_post_tick_callback(TEST['handle'])
TEST['handle']=u.register_slate_post_tick_callback(tick)
