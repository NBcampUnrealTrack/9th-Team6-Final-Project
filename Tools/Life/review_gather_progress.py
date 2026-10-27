"""Exercise real F gathering in PIE without saving the disposable review session."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'GatherProgress'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'starting', 'started': time.monotonic(), 'checks': {}}

def phase(name):
    STATE.update(phase=name, phase_started=time.monotonic())
    report()

def report():
    data = {key: STATE[key] for key in ('phase', 'checks')}
    if 'error' in STATE: data['error'] = STATE['error']
    (OUT / 'LiveReview.json').write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))

def subsystem(cls):
    return next(s for s in u.ObjectIterator(cls) if s.get_outer() == STATE['pc'].get_world_state().get_outer())

def interact():
    STATE['pc'].handle_input_command(u.ICInputCommand.INTERACT, 0)

def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 150: raise RuntimeError('Review timeout in ' + STATE['phase'])
        if STATE['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds: return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create': return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0], pc=pc)
            phase('creating')
            return
        pc, world = STATE['pc'], STATE['world']
        elapsed = time.monotonic() - STATE['phase_started']
        if STATE['phase'] == 'creating':
            if not pc.get_editor_property('created'): return
            player = u.GameplayStatics.get_player_pawn(world, 0)
            plant = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.ICLifeActor)
                         if str(a.get_editor_property('point_id')) == 'TEST_R01_GATHER_01')
            point = plant.get_actor_location()
            player.set_actor_location_and_rotation(point + u.Vector(-190, 0, 96), u.Rotator(), False, True)
            player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            pc.set_control_rotation(u.Rotator())
            view = player.get_component_by_class(u.CameraComponent)
            camera = point + u.Vector(-620, -300, 350)
            view.set_world_location_and_rotation(camera, u.MathLibrary.find_look_at_rotation(camera, point + u.Vector(0, 0, 60)), False, True)
            pc.set_menu('')
            STATE.update(player=player, plant=plant, gather=subsystem(u.ICGatheringSubsystem), clock=subsystem(u.ICTimeSubsystem))
            phase('settling')
            return
        gather = STATE['gather']
        if STATE['phase'] == 'settling' and elapsed > 1.5:
            STATE.update(initial_count=pc.get_inventory().count_item('ITM_R01_PLANT_01'),
                         play_started=STATE['clock'].get_clock().play_second,
                         game_started=u.GameplayStatics.get_time_seconds(world), frames=0, samples=[])
            started = time.monotonic()
            interact()
            assert gather.is_gathering(), 'F did not start gathering'
            STATE['checks']['start_call_ms'] = round((time.monotonic() - started) * 1000, 3)
            assert gather.get_progress_widget() and not u.GameplayStatics.is_game_paused(world)
            phase('gathering')
            return
        if STATE['phase'] == 'gathering':
            STATE['frames'] += 1
            assert not u.GameplayStatics.is_game_paused(world)
            progress = gather.get_progress()
            if gather.is_gathering():
                assert pc.get_inventory().count_item('ITM_R01_PLANT_01') == STATE['initial_count']
                if not STATE['samples'] and progress >= .04:
                    capture('GatherProgress_Start')
                    STATE['samples'].append(round(progress, 3))
                elif len(STATE['samples']) == 1 and progress >= .48:
                    capture('GatherProgress_Half')
                    STATE['samples'].append(round(progress, 3))
                return
            assert pc.get_inventory().count_item('ITM_R01_PLANT_01') == STATE['initial_count'] + 1
            assert gather.get_progress_widget() is None
            game_elapsed = u.GameplayStatics.get_time_seconds(world) - STATE['game_started']
            play_elapsed = STATE['clock'].get_clock().play_second - STATE['play_started']
            assert game_elapsed >= 4.9 and play_elapsed >= 5 and STATE['frames'] > 10
            assert len(STATE['samples']) == 2, STATE['samples']
            STATE['checks'].update(five_second_reward_once=True, overlay_removed=True, world_unpaused=True,
                game_seconds=round(game_elapsed, 3), play_seconds=round(play_elapsed, 3), frames=STATE['frames'], gauge_samples=STATE['samples'])
            capture('GatherProgress_Complete')
            # The second plant remains available for cancellation checks.
            plant = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.ICLifeActor)
                         if str(a.get_editor_property('point_id')) == 'TEST_R01_GATHER_02')
            STATE['player'].set_actor_location_and_rotation(plant.get_actor_location() + u.Vector(-190, 0, 96), u.Rotator(), False, True)
            STATE.update(cancel_count=pc.get_inventory().count_item('ITM_R01_PLANT_02'))
            phase('cancel_settling')
            return
        if STATE['phase'] == 'cancel_settling' and elapsed > 1:
            interact()
            assert gather.is_gathering()
            phase('cancel_f')
            return
        if STATE['phase'] == 'cancel_f' and elapsed > 1:
            interact()
            assert not gather.is_gathering() and not str(pc.get_editor_property('menu'))
            assert pc.get_inventory().count_item('ITM_R01_PLANT_02') == STATE['cancel_count']
            STATE['checks']['f_cancel_no_reward'] = True
            interact()
            assert gather.is_gathering()
            phase('cancel_escape')
            return
        if STATE['phase'] == 'cancel_escape' and elapsed > .6:
            pc.handle_input_command(u.ICInputCommand.MENU, 0)
            assert not gather.is_gathering() and not str(pc.get_editor_property('menu'))
            assert not u.GameplayStatics.is_game_paused(world)
            STATE['checks']['escape_cancel_keeps_game_running'] = True
            phase('complete')
            return
        if STATE['phase'] == 'complete':
            report()
            u.log('IC_GATHER_REVIEW_OK ' + json.dumps(STATE['checks']))
            finish()
    except Exception:
        STATE['error'] = traceback.format_exc()
        phase('error')
        u.log_error(STATE['error'])
        finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
