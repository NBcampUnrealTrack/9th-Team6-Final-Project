"""PIE review of existing world skies; run via full editor -ExecCmds=py <path>."""
import json,math,time,traceback
from pathlib import Path
import unreal as u

out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'DayNightReview'
out.mkdir(parents=True,exist_ok=True)
state={'phase':'starting','started':time.monotonic(),'checks':{},'samples':[],'index':0}
probes=[('Dawn_0500',300),('Sunrise_0600',360),('Noon_1200',720),('Sunset_1800',1080),('Night_2200',1320)]
def report():
    (out/'Runtime.json').write_text(json.dumps({k:state[k] for k in ('phase','checks','samples','error') if k in state},ensure_ascii=False,indent=2),encoding='utf-8')
def finish():
    u.unregister_slate_post_tick_callback(state['handle'])
    if state.get('pc'):
        saves=state['pc'].get_saves()
        if not saves.get_editor_property('saving'):saves.delete_slot(0)
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    u.SystemLibrary.quit_editor()
def jump():
    name,minute=probes[state['index']]
    clock=state['pc'].get_time();now=clock.get_clock().game_minute
    assert clock.jump_game_minutes((minute-now%1440+1440)%1440)
    state['phase']='capturing';state['at']=time.monotonic();report()
def tick(delta):
    try:
        now=time.monotonic()
        if now-state['started']>120:raise RuntimeError('Day/night review timed out: '+state['phase'])
        if state['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            world=worlds[0];pc=u.GameplayStatics.get_player_controller(world,0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(world)
            state.update(world=world,pc=pc,phase='creating');report();return
        if state['phase']=='creating':
            pc=state['pc']
            if not pc.get_editor_property('created') or pc.get_saves().get_editor_property('saving'):return
            cycles=u.GameplayStatics.get_all_actors_of_class(state['world'],u.ICDayNightController)
            assert len(cycles)==1,'One saved day/night controller is required'
            cycle=cycles[0];suns=cycle.get_editor_property('sun_lights');skies=cycle.get_editor_property('sky_lights');spheres=cycle.get_editor_property('sky_spheres')
            assert len(suns)==3 and len(skies)==3 and len(spheres)==2
            priorities=[s.get_component_by_class(u.DirectionalLightComponent).get_editor_property('forward_shading_priority') for s in suns]
            atmosphere_flags=[s.get_component_by_class(u.DirectionalLightComponent).get_editor_property('atmosphere_sun_light') for s in suns]
            assert priorities==[1,0,0] and atmosphere_flags==[True,False,False]
            player=u.GameplayStatics.get_player_pawn(state['world'],0)
            player.get_component_by_class(u.CharacterMovementComponent).disable_movement()
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view',75)
            point=u.Vector(-56000,510500,-5800);location=point+u.Vector(-4000,-3000,1500)
            camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point+u.Vector(0,0,800)),False,True)
            pc.get_time().set_running(False)
            state.update(cycle=cycle,suns=suns,skies=skies,spheres=spheres,camera=camera)
            state['checks'].update(suns=len(suns),skylights=len(skies),sky_spheres=len(spheres),primary_sun_priorities=priorities,atmosphere_suns=atmosphere_flags)
            jump();return
        if state['phase']=='capturing' and now-state['at']>2.5:
            cycle=state['cycle'];sample=cycle.get_editor_property('current_state');name,minute=probes[state['index']]
            assert abs(sample.minute_of_day-minute)<.1,(sample.minute_of_day,minute)
            rotation=sample.sun_rotation;forward=u.MathLibrary.get_forward_vector(rotation)
            intensities=[]
            for sun in state['suns']:
                actual=sun.get_actor_forward_vector()
                assert (actual-forward).length()<.001,'An existing sun did not follow the clock'
                intensities.append(sun.get_component_by_class(u.DirectionalLightComponent).get_editor_property('intensity'))
            heights=[float(s.get_editor_property('Sun height')) for s in state['spheres']]
            assert (all(h<0 for h in heights) if minute in (300,1320) else all(h>0 for h in heights) if minute==720 else all(abs(h)<.001 for h in heights)),heights
            if minute==1320:assert all(v==0 for v in intensities),'A fixed sun still lights the night'
            image_path=out/(name+'.png')
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(state['world'],str(image_path))
            state['samples'].append({'name':name,'minute':sample.minute_of_day,'elevation':sample.sun_elevation_degrees,'sun_intensities':intensities,'sky_scale':sample.sky_intensity_scale,'sky_sphere_heights':heights,'image':str(image_path)})
            state['index']+=1
            if state['index']==len(probes):
                state['phase']='complete';report();u.log('IC_OPEN_WORLD_DAY_NIGHT_VERIFIED');finish();return
            jump()
    except Exception:
        state.update(phase='error',error=traceback.format_exc());report();u.log_error(state['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
state['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
