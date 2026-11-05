"""Render blinking and explicit speech on both player presets and all eleven NPCs.

Run a dedicated editor with -ICCharacterFaceReview -ExecutePythonScript=<this file>.
Uses an isolated L_TestMap PIE/save namespace and never saves content packages.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

assert '-ICCharacterFaceReview' in u.SystemLibrary.get_command_line()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
# The settings class has no generated Python type in UE5.8; access its reflected CDO.
# Offscreen review windows must continue rendering; -NoSaveConfig keeps this process-local.
performance = u.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings')
if performance:
    performance.set_editor_property('bThrottleCPUWhenNotForeground', False)
u.EditorPythonScripting.set_keep_python_script_alive(True)
OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'CharacterFaces/Visual'
OUT.mkdir(parents=True, exist_ok=True)
S = {'stage': 'starting', 'started': time.monotonic(), 'subjects': {}, 'complete': False, 'index': -1}
MOUTHS = ['Fcl_MTH_' + vowel for vowel in 'AIUEO']


def report():
    (OUT / 'Review.json').write_text(json.dumps({k: S[k] for k in ('stage', 'subjects', 'complete', 'error') if k in S},
                                               ensure_ascii=False, indent=2), encoding='utf-8')


def finish():
    report()
    u.unregister_slate_post_tick_callback(S['handle'])
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()


def capture(label):
    path = OUT / (S['id'] + '_' + label + '.png')
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'], str(path)), 'Viewport capture failed'
    S['subjects'][S['id']]['captures'][label] = str(path)
    report()


def advance():
    if S.get('face'):
        S['face'].stop_dialogue()
    S['index'] += 1
    if S['index'] == len(S['cases']):
        S.update(stage='complete', complete=True)
        finish()
        return
    identifier, target, preset = S['cases'][S['index']]
    if preset is not None:
        target.apply_preset(preset)
    face = target.get_editor_property('facial_animation')
    assert face and face.is_face_ready(), identifier + ' must support blinking'
    assert face.can_animate_speech(), identifier + ' speech support'
    S['subjects'][identifier] = {'speech_enabled': face.can_animate_speech(), 'captures': {}}
    S.update(id=identifier, target=target, face=face, mesh=target.get_editor_property('mesh'),
             stage='idle', ready=time.monotonic(), camera_frames=0)
    report()


def tick(delta):
    try:
        now = time.monotonic()
        if now - S['started'] > 300:
            raise RuntimeError('Face review timed out at ' + S['stage'])
        if S['stage'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            world = worlds[0]
            u.SystemLibrary.execute_console_command(world, 't.IdleWhenNotForeground 0')
            u.SystemLibrary.execute_console_command(world, 't.MaxFPS 30')
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(world)
            S.update(world=world, pc=pc, stage='creating')
            report()
            return
        if S['stage'] == 'creating':
            if not S['pc'].get_editor_property('created'):
                return
            npcs = {str(npc.get_character_id()): npc for npc in u.GameplayStatics.get_all_actors_of_class(S['world'], u.ICNPCBase)
                    if str(npc.get_character_id()).startswith('NPC_R01_0')}
            if len(npcs) != 11:
                return
            player = u.GameplayStatics.get_player_pawn(S['world'], 0)
            S['pc'].set_menu('')
            for npc in npcs.values():
                controller = npc.get_controller()
                if controller:
                    controller.stop_movement()
                    brain = controller.get_component_by_class(u.BrainComponent)
                    if brain:
                        brain.stop_logic('CharacterFaceReview')
                    controller.set_actor_tick_enabled(False)
                for name in ('HomeBehavior', 'LifeObjectUser'):
                    component = npc.get_editor_property(name)
                    if component:
                        component.set_component_tick_enabled(False)
                npc.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            camera = player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view', 35)
            S['pc'].set_view_target_with_blend(player, 0)
            S['camera'] = camera
            S['cases'] = [('PLAYER_M', player, 0), ('PLAYER_F', player, 1)] + [(key, npcs[key], None) for key in sorted(npcs)]
            advance()
            return
        focus = S['mesh'].get_socket_location('J_Bip_C_Head') + u.Vector(0, 0, -3)
        location = focus + S['target'].get_actor_forward_vector() * 110
        S['camera'].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)
        S['camera_frames'] += 1
        if now - S['ready'] < 1 or S['camera_frames'] < 4:
            return
        captures = S['subjects'][S['id']]['captures']
        if S['stage'] == 'idle':
            blink = S['mesh'].get_morph_target('Fcl_EYE_Close')
            if 'neutral' not in captures and blink == 0:
                capture('neutral')
            if 'blink' not in captures and blink > .8:
                capture('blink')
            assert all(S['mesh'].get_morph_target(name) == 0 for name in MOUTHS), 'Blinking changed the mouth'
            if 'neutral' in captures and 'blink' in captures:
                S['face'].play_dialogue_line(u.Text('아' * 16 + '.'), S['target'].get_character_id())
                S.update(stage='speaking', speech_started=now)
                report()
            return
        if S['stage'] == 'speaking':
            if 'speech' not in captures and S['mesh'].get_morph_target('Fcl_MTH_A') > .4:
                capture('speech')
            if not S['face'].is_speaking() and now - S['speech_started'] > 3:
                assert 'speech' in captures, 'Mouth must visibly move during speech'
                assert all(S['mesh'].get_morph_target(name) == 0 for name in MOUTHS), 'Mouth did not close'
                S['subjects'][S['id']]['finished_mouth_closed'] = True
                advance()
    except Exception:
        S.update(stage='error', error=traceback.format_exc())
        u.log_error(S['error'])
        finish()


u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
