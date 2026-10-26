"""Run with a full editor's -ExecCmds="py <path>"; uses an isolated PIE save namespace."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/MonsterPrototype'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{},'screenshots':[]}

def status(**values):
    STATE.update(values)
    report={key:STATE[key] for key in ('phase','checks','screenshots')}
    if 'error' in STATE:report['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

def camera_to(point,offset):
    location=point+offset
    STATE['camera'].set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)

def capture(name):
    # The native helper captures the actual running viewport.
    u.ICCharacterSetupLibrary.capture_live_character_review(STATE['world'],'Monster_'+name)
    STATE['screenshots'].append('Saved/LiveCharacterReview/Monster_'+name+'.png')

def capture_ui(name):
    path=OUT/('HUD_'+name+'.png')
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(path))
    STATE['screenshots'].append(str(path.relative_to(ROOT)))

def phase(name):
    STATE['phase']=name;STATE['phase_started']=time.monotonic();status()

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        now=time.monotonic()
        if now-STATE['started']>110:raise RuntimeError('PIE monster review timed out in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            world=worlds[0];pc=u.GameplayStatics.get_player_controller(world,0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(world),'Could not create isolated review player'
            STATE.update(world=world,pc=pc);phase('creating');return
        if STATE['phase']=='creating':
            pc=STATE['pc']
            if not pc.get_editor_property('created'):return
            enemies=u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICEnemyBase)
            if len(enemies)!=11:return
            enemies=sorted(enemies,key=lambda a:int(str(a.get_character_id()).rsplit(':',1)[1]))
            player=u.GameplayStatics.get_player_pawn(STATE['world'],0)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view',60)
            STATE.update(enemies=enemies,player=player,camera=camera,origins=[e.get_actor_location() for e in enemies])
            STATE['checks']['enemy_gas_tick_enabled']=enemies[0].get_component_by_class(u.ICAbilitySystemComponent).is_component_tick_enabled()
            STATE['checks']['player_gas_tick_enabled']=player.get_component_by_class(u.ICAbilitySystemComponent).is_component_tick_enabled()
            STATE['checks']['spawn_count']=len(enemies)
            STATE['checks']['row_positions']=[{'id':str(e.get_character_id()),'x':round(e.get_actor_location().x),'y':round(e.get_actor_location().y)} for e in enemies]
            assert all(abs(p.x-2000)<5 for p in STATE['origins']),'Monster row is misplaced'
            pc.set_menu('')
            camera_to(u.Vector(2000,6500,100),u.Vector(-6400,-1200,6800))
            phase('overview');return
        elapsed=now-STATE['phase_started']
        if STATE['phase']=='overview' and elapsed>2:
            capture('Row')
            origin=STATE['origins'][0]
            STATE['player'].set_actor_location_and_rotation(origin+u.Vector(-550,0,10),u.Rotator(0,0,0),False,True)
            STATE['hp_before']=STATE['pc'].get_world_state().get_snapshot('CHR_PLAYER').hp
            camera_to(origin+u.Vector(-250,0,20),u.Vector(-500,-650,360))
            phase('chase');return
        if STATE['phase']=='chase':
            enemy=STATE['enemies'][0]
            if elapsed>3.8 and not STATE.get('combat_capture'):
                capture_ui('Combat');STATE['combat_capture']=True
            if elapsed>5:
                hp=STATE['pc'].get_world_state().get_snapshot('CHR_PLAYER').hp
                moved=(enemy.get_actor_location()-STATE['origins'][0]).length()
                STATE['checks'].update(chase_distance_cm=round(moved,2),player_hp_before=STATE['hp_before'],player_hp_after=hp)
                assert moved>100,'Monster did not chase through the real NavMesh'
                assert hp<STATE['hp_before'],'Monster did not attack nearby player'
                STATE['player'].set_actor_location(u.Vector(0,0,90),False,True)
                phase('return');return
        if STATE['phase']=='return':
            distance=(STATE['enemies'][0].get_actor_location()-STATE['origins'][0]).length()
            if distance<100 and elapsed>1:
                STATE['checks']['return_distance_cm']=round(distance,2)
                capture('Returned')
                player=STATE['player'];origin=STATE['enemies'][0].get_actor_location()
                state=STATE['pc'].get_world_state()
                STATE['gold_before']=state.export_state().gold
                assert state.set_main_stat('CHR_PLAYER',u.ICMainStat.STR,100),'Could not prepare isolated test attack'
                p=state.get_snapshot('CHR_PLAYER')
                assert state.set_vitals('CHR_PLAYER',p.max_hp,p.mp,p.sp)
                if STATE['pc'].get_inventory().is_weapon_sheathed():
                    STATE['checks']['draw_weapon']=player.request_cycle_weapon()
                camera_to(origin,u.Vector(-500,-650,360))
                phase('arming');return
            if elapsed>12:raise RuntimeError('Monster did not return to its original position: '+str(distance))
        if STATE['phase']=='arming' and elapsed>2:
            origin=STATE['enemies'][0].get_actor_location()
            STATE['player'].set_actor_location_and_rotation(origin+u.Vector(-130,0,10),u.Rotator(0,0,0),False,True)
            STATE['pc'].set_control_rotation(u.Rotator(0,0,0))
            STATE['checks']['weapon_family']=str(STATE['pc'].get_inventory().get_equipped_weapon_family())
            phase('reward');return
        if STATE['phase']=='reward':
            state=STATE['pc'].get_world_state();gold=state.export_state().gold
            if gold>STATE['gold_before']:
                STATE['checks']['kill_gold']=gold-STATE['gold_before']
                phase('reward_pending_capture');return
            if now-STATE.get('last_attack',0)>1:
                STATE['last_attack']=now
                result=STATE['player'].get_component_by_class(u.ICAbilitySystemComponent).request_action('Attack','None',STATE['enemies'][0])
                STATE['checks']['player_attack_result']=str(result)
            if elapsed>10:raise RuntimeError('Player GAS attack did not kill monster / grant loot')
        if STATE['phase']=='reward_pending_capture' and elapsed>.25:
            capture_ui('Reward')
            phase('reward_capture');return
        if STATE['phase']=='reward_capture' and elapsed>1:
            boss=STATE['enemies'][10]
            camera_to(boss.get_actor_location(),u.Vector(-850,-850,550))
            phase('boss');return
        if STATE['phase']=='boss' and elapsed>1.5:
            capture('BossGreen')
            STATE['checks']['boss_green']=True
            phase('complete');return
        if STATE['phase']=='complete' and elapsed>.8:
            status();u.log('IC_MONSTER_LIVE_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        status(phase='error',error=traceback.format_exc());u.log_error(traceback.format_exc());finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False),'Use a fresh isolated review editor'
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
