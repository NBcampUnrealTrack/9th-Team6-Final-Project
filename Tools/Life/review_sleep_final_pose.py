"""Verify both players' short sleep sequence, hidden pose cuts and restored controls in PIE."""
import unreal as u,time,pathlib,json,traceback
OUT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'QuickSleep/Live';OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='start',started=time.monotonic(),results=[],gender=0);levels=u.get_editor_subsystem(u.LevelEditorSubsystem)

def phase(p):S.update(phase=p,t=time.monotonic())
def actors(cls):return u.GameplayStatics.get_all_actors_of_class(S['world'],cls)
def fade():return next((w for w in u.ObjectIterator(u.ICSleepFadeWidget) if w.get_owning_player()==S['pc'] and w.is_in_viewport()),None)
def shot(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(('Male_' if S['gender']==0 else 'Female_')+name+'.png')))
def finish():
    (OUT/'Review.json').write_text(json.dumps({k:v for k,v in S.items() if k in ['phase','results','error','complete']},ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(S['handle']);levels.editor_request_end_play();u.SystemLibrary.quit_editor()

def tick(dt):
    try:
        if time.monotonic()-S['started']>90:raise RuntimeError('Quick sleep review timeout: '+S['phase'])
        if S['phase']=='start':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('Menu'))!='Create':return
            S.update(world=worlds[0],pc=pc);assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        pc=S['pc'];elapsed=time.monotonic()-S['t']
        if S['phase']=='created':
            if not pc.get_editor_property('created'):return
            S.update(player=u.GameplayStatics.get_player_pawn(S['world'],0),bed=actors(u.ICPlayerBed)[0])
            S['life']=S['player'].get_component_by_class(u.ICPlayerLifeComponent)
            for enemy in actors(u.ICEnemyBase)+actors(u.ICNPCBase):
                c=enemy.get_controller()
                if c:
                    brain=c.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Quick sleep review')
                    c.stop_movement();c.set_actor_tick_enabled(False)
                enemy.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            phase('approach');return
        if S['phase']=='approach':
            p=S['bed'].get_actor_location()+S['bed'].get_actor_forward_vector()*170
            S['player'].set_actor_location_and_rotation(p,u.MathLibrary.find_look_at_rotation(p,S['bed'].get_actor_location()),False,True)
            S['player'].apply_preset(S['gender']);phase('ready');return
        if S['phase']=='ready' and elapsed>.7:
            life=S['life'];position=S['player'].get_actor_location();assert S['bed'].try_interact(pc)
            assert not life.is_sleep_pose_active();life.end_work()
            after=S['player'].get_actor_location()
            assert max(abs(after.x-position.x),abs(after.y-position.y),abs(after.z-position.z))<.1,'Cancelling before black must not teleport'
            S.update(before=S['player'].get_actor_transform(),view=pc.get_view_target(),minute=pc.get_time().get_clock().game_minute,game_start=u.GameplayStatics.get_time_seconds(S['world']),pose_seen=False,wake_seen=False,rest_shot=False,frames=[])
            assert S['bed'].try_interact(pc);assert life.is_sleeping() and not life.is_sleep_pose_active();phase('sleep');return
        if S['phase']=='sleep':
            life=S['life'];t=u.GameplayStatics.get_time_seconds(S['world'])-S['game_start'];pose=life.is_sleep_pose_active();w=fade()
            opacity=w.get_render_opacity() if w else 0
            S['frames'].append(dict(t=round(t,3),pose=pose,black=round(opacity,3)))
            if pose and not S['pose_seen']:
                assert opacity>.99,'Entering the bed must be fully hidden';assert pc.get_view_target()!=S['view'];S['pose_seen']=True
            if pose and t>.85 and not S['rest_shot']:
                assert opacity<.05;shot('Rest');S['rest_shot']=True
            if S['pose_seen'] and not pose and not S['wake_seen']:
                assert opacity>.99,'Standing/camera cut must be fully hidden';assert pc.get_view_target()==S['view'];S['wake_seen']=True
            if life.is_sleeping():return
            assert S['pose_seen'] and S['wake_seen'] and S['rest_shot'];assert 1.8<t<2.5,(t,'Sleep should finish in about two seconds')
            assert not pc.get_editor_property('bInEvent');assert not fade();assert pc.get_view_target()==S['view']
            minutes=pc.get_time().get_clock().game_minute-S['minute'];assert minutes>=120
            S['results'].append(dict(gender='Male' if S['gender']==0 else 'Female',duration=round(t,3),game_minutes=minutes,hidden_pose_cuts=True,controls_restored=True,cancel_before_black_preserves_position=True,frames=S['frames']))
            phase('awake');return
        if S['phase']=='awake' and elapsed>.3:
            shot('Awake')
            if S['gender']==0:S['gender']=1;phase('approach');return
            S['complete']=True;phase('complete');finish()
    except Exception:S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick);assert u.ICCharacterSetupLibrary.begin_monster_test_review()
