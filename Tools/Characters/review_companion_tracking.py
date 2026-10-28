"""Exercise long travel, close formation and stalled-path recovery in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'CompanionTracking/Live'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
S=dict(phase='starting',start=time.monotonic(),checks={},samples=[])

def report():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','samples','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):S.update(phase=name,at=time.monotonic());report()
def key(name,pressed):
    value=u.Key();value.set_editor_property('key_name',name)
    u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],value,pressed)
def distances():return [(n.get_actor_location()-S['player'].get_actor_location()).length() for n in S['party']]
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def finish():
    if 'world' in S:
        for name in ('W','LeftShift'):key(name,False)
    if 'blocked' in S:S['blocked'].set_component_tick_enabled(True)
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def camera():
    at=S['player'].get_actor_location();pos=at+u.Vector(-850,650,650)
    S['camera'].set_world_location_and_rotation(pos,u.MathLibrary.find_look_at_rotation(pos,at+u.Vector(0,150,0)),False,True)
def tick(delta):
    try:
        if time.monotonic()-S['start']>180:raise RuntimeError('Tracking timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['at']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('');u.SystemLibrary.execute_console_command(S['world'],'ICRecruitTestParty',pc)
            player=u.GameplayStatics.get_player_pawn(S['world'],0)
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(0,-90,0),False,True)
            pc.set_control_rotation(u.Rotator(0,-90,0));S['player']=player;phase('party_spawn');return
        if S['phase']=='party_spawn' and elapsed>.5:
            party=[n for n in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase) if str(n.get_character_id()) in ('NPC_R01_001','NPC_R01_002','NPC_R01_003')]
            if len(party)!=3:
                assert elapsed<15,len(party)
                return
            S['party']=sorted(party,key=lambda a:str(a.get_character_id()))
            for i,npc in enumerate(S['party']):npc.set_actor_location_and_rotation(u.Vector(-1300+i*180,-2400,96),u.Rotator(0,-90,0),False,True)
            # Isolate follow navigation from monster aggro during this route.
            for enemy in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICEnemyBase):
                ai=enemy.get_controller()
                if ai:
                    brain=ai.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Companion tracking review')
                    ai.stop_movement();ai.set_actor_tick_enabled(False)
            c=S['player'].get_component_by_class(u.CameraComponent)
            c.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD);c.set_field_of_view(70);S['camera']=c
            phase('formation');return
        if 'camera' not in S:return
        camera()
        if S['phase']=='formation' and elapsed>3:
            ds=distances();assert max(ds)<400,ds
            S['checks']['close_formation_distances']=ds;capture('CloseFormation')
            key('W',True);key('LeftShift',True);S['last_sample']=-1;phase('long_sprint');return
        if S['phase']=='long_sprint':
            if elapsed-S['last_sample']>.5:
                S['last_sample']=elapsed
                S['samples'].append(dict(seconds=round(elapsed,2),player=[S['player'].get_actor_location().x,S['player'].get_actor_location().y],distances=distances(),speeds=[n.get_velocity().length() for n in S['party']],move_status=[str(n.get_controller().get_move_status()) for n in S['party']]))
                report()
            if elapsed<9:return
            ds=distances();assert max(ds)<700,ds
            S['checks']['long_travel_final_distances']=ds
            S['checks']['no_companion_left_behind_during_long_travel']=True
            capture('LongTravel');key('LeftShift',False);key('W',False);phase('stopping');return
        if S['phase']=='stopping' and elapsed>2:
            ds=distances();assert max(ds)<400,ds
            S['checks']['stopped_close_formation_distances']=ds
            npc=S['party'][0];movement=npc.get_component_by_class(u.CharacterMovementComponent)
            movement.stop_movement_immediately();movement.set_component_tick_enabled(False);S['blocked']=movement
            at=S['player'].get_actor_location();npc.set_actor_location_and_rotation(at+u.Vector(900,0,0),u.Rotator(),False,True)
            phase('stalled');return
        if S['phase']=='stalled' and elapsed>4:
            ds=distances();assert ds[0]<350,ds
            S['checks']['stalled_path_recovers_on_safe_ground']=True
            S['blocked'].set_component_tick_enabled(True)
            at=S['player'].get_actor_location()
            for i,npc in enumerate(S['party']):npc.set_actor_location_and_rotation(at+u.Vector(3500+i*120,0,0),u.Rotator(),False,True)
            phase('distant');return
        if S['phase']=='distant' and elapsed>1.5:
            ds=distances();assert max(ds)<400,ds
            S['checks']['all_three_recover_from_large_separation']=True
            capture('RecoveredParty');phase('complete');return
        if S['phase']=='complete' and elapsed>.3:
            u.log('IC_COMPANION_TRACKING_LIVE_OK '+json.dumps(S['checks']));finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
