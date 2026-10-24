"""Verify actual PIE dash movement and capture clockwise cooldown fill on the HUD."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/MovementInput';OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}

def status():
    result={k:STATE[k] for k in ('phase','checks')}
    if 'error' in STATE:result['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')

def phase(name):STATE.update(phase=name,phase_started=time.monotonic());status()

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))

def record_character_placement():
    # Capture the visible result for centering QA and record the projected canvas position.
    point=STATE['widget'].get_editor_property('slot').get_position()
    assert point.x>0 and point.y>0
    STATE['checks']['character_canvas_position']=[point.x,point.y]
    assert STATE['ring'].get_editor_property('thickness')==14

def freeze(name):
    STATE['pc'].get_time().set_pause_request(STATE['widget'],'ReviewCapture',True)
    STATE['widget'].refresh_cooldown();phase(name)

def resume():STATE['pc'].get_time().release_pause_requests(STATE['widget'])

def tick(delta):
    try:
        if time.monotonic()-STATE['started']>100:raise RuntimeError('Movement review timed out in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc'];elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('')
            pawn=u.GameplayStatics.get_player_pawn(STATE['world'],0);pawn.set_actor_rotation(u.Rotator(0,0,0),False)
            STATE.update(pawn=pawn,asc=pawn.get_editor_property('ability_system'))
            widgets=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICMovementCooldownWidget,False)
            assert len(widgets)==1,len(widgets)
            STATE['widget']=widgets[0]
            STATE['ring']=widgets[0].get_editor_property('RING_Cooldown')
            phase('settling');return
        asc=STATE['asc'];widget=STATE['widget'];pawn=STATE['pawn'];ring=STATE['ring']
        if STATE['phase']=='settling' and elapsed>.5:
            widget.refresh_cooldown();assert widget.get_editor_property('ready')
            assert ring.get_visibility()==u.SlateVisibility.HIDDEN
            STATE['checks']['ready_ring_hidden']=True
            capture('CooldownReady')
            STATE['start']=pawn.get_actor_location()
            pc.handle_input_command(u.ICInputCommand.DODGE,0)
            assert asc.get_movement_cooldown_remaining()>2.9
            phase('dash');return
        if STATE['phase']=='dash':
            progress=asc.get_movement_cooldown_progress()
            if progress<.25:return
            distance=pawn.get_actor_location().distance(STATE['start'])
            assert 490<distance<510,distance
            STATE['checks']['actual_dash_distance_cm']=distance
            STATE['checks']['quarter_fill']=progress
            assert abs(widget.get_editor_property('progress')-progress)<.08,'HUD must follow cooldown automatically'
            freeze('quarter_capture');return
        if STATE['phase']=='quarter_capture' and elapsed>.2:
            assert ring.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE
            record_character_placement()
            capture('CooldownQuarter');resume();phase('half');return
        if STATE['phase']=='half' and asc.get_movement_cooldown_progress()>=.5:
            STATE['checks']['half_fill']=asc.get_movement_cooldown_progress()
            assert abs(widget.get_editor_property('progress')-asc.get_movement_cooldown_progress())<.08
            freeze('half_capture');return
        if STATE['phase']=='half_capture' and elapsed>.2:
            record_character_placement()
            capture('CooldownHalf');resume();phase('ready');return
        if STATE['phase']=='ready' and asc.get_movement_cooldown_remaining()==0:
            widget.refresh_cooldown();assert widget.get_editor_property('ready')
            assert widget.get_editor_property('progress')==1
            assert ring.get_visibility()==u.SlateVisibility.HIDDEN
            STATE['checks']['completed_ring_hidden']=True
            capture('CooldownComplete')
            pc.handle_input_command(u.ICInputCommand.ROLL,0)
            pc.handle_input_command(u.ICInputCommand.QUEST_FOLLOW,0)
            assert str(pc.get_editor_property('menu'))==''
            pc.handle_input_command(u.ICInputCommand.DODGE,0)
            assert asc.get_movement_cooldown_remaining()>2.9
            STATE['checks']['complete_ring_allows_reuse']=True
            phase('second_dash');return
        if STATE['phase']=='second_dash' and elapsed>.4:
            assert ring.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE
            STATE['checks']['next_dash_reveals_ring']=True
            STATE['jump_z']=pawn.get_actor_location().z
            pc.handle_input_command(u.ICInputCommand.JUMP,0);phase('jump');return
        if STATE['phase']=='jump' and elapsed>.2:
            height=pawn.get_actor_location().z-STATE['jump_z']
            assert height>20,height
            STATE['checks']['jump_height_cm']=height
            pc.release_input_command(u.ICInputCommand.JUMP,0)
            phase('complete');u.log('IC_MOVEMENT_INPUT_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
