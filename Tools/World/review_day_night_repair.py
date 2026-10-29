"""Review the saved map in a dedicated -ICDayNightRepairReview editor process.

Uses a fresh visual-review save namespace and discards the PIE world on exit.
Does not save editor assets, delete save slots, or close an existing editor.
"""
import hashlib
import json
import time
import traceback
from pathlib import Path
import unreal as u

assert '-ICDayNightRepairReview' in u.SystemLibrary.get_command_line(), 'Dedicated review process required'
assert not u.EditorLevelLibrary.get_pie_worlds(False), 'An existing play session must not be used'
u.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved/DayNightRepair'
out.mkdir(parents=True, exist_ok=True)
setup = json.loads((out / 'Setup.json').read_text(encoding='utf-8'))
map_file = root / 'Content/IsekaiClass/World/Maps/L_OpenWorld.umap'
map_hash = hashlib.sha256(map_file.read_bytes()).hexdigest()
state = {'phase': 'starting', 'started': time.monotonic(), 'checks': {}, 'samples': [], 'index': 0}
probes = [('Noon_1200', 720), ('Night_2200', 1320), ('Morning_0800', 480)]


def report():
    (out / 'Runtime.json').write_text(json.dumps({k: state[k] for k in ('phase', 'checks', 'samples', 'error') if k in state}, ensure_ascii=False, indent=2), encoding='utf-8')


def finish():
    u.unregister_slate_post_tick_callback(state['handle'])
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    state['checks']['saved_map_unchanged_by_review'] = hashlib.sha256(map_file.read_bytes()).hexdigest() == map_hash
    report()
    # The ExecutePythonScript executer exits its own process when keep-alive ends.
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    # Console-launched runs use the matching, dedicated -TestExit phrase.
    u.log('IC_DAY_NIGHT_REPAIR_FINISHED')


def jump():
    _, minute = probes[state['index']]
    clock = state['pc'].get_time()
    now = clock.get_clock().game_minute
    assert clock.jump_game_minutes((minute - now % 1440 + 1440) % 1440)
    state.update(phase='capturing', at=time.monotonic())
    report()


def tick(delta):
    try:
        now = time.monotonic()
        if now - state['started'] > 240:
            raise RuntimeError('Day/night review timed out: ' + state['phase'])
        if state['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            world = worlds[0]
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(world)
            state.update(world=world, pc=pc, phase='creating')
            report()
            return
        if state['phase'] == 'creating':
            pc = state['pc']
            if not pc.get_editor_property('created') or pc.get_saves().get_editor_property('saving') or str(pc.get_editor_property('menu')):
                return
            cycles = u.GameplayStatics.get_all_actors_of_class(state['world'], u.ICDayNightController)
            assert len(cycles) == 1, 'One saved day/night controller is required'
            cycle = cycles[0]
            suns = list(cycle.get_editor_property('sun_lights'))
            skies = list(cycle.get_editor_property('sky_lights'))
            spheres = list(cycle.get_editor_property('sky_spheres'))
            assert len(suns) == len(setup['suns']) and len(skies) == len(setup['skylights']) and len(spheres) == len(setup['sky_spheres'])
            assert {s.get_path_name() for s in suns} == {s.get_path_name() for s in u.GameplayStatics.get_all_actors_of_class(state['world'], u.DirectionalLight)}, 'An unbound sun remains'
            assert {s.get_path_name() for s in skies} == {s.get_path_name() for s in u.GameplayStatics.get_all_actors_of_class(state['world'], u.SkyLight)}, 'An unbound sky light remains'
            priorities = [s.get_component_by_class(u.DirectionalLightComponent).get_editor_property('forward_shading_priority') for s in suns]
            atmosphere = [s.get_component_by_class(u.DirectionalLightComponent).get_editor_property('atmosphere_sun_light') for s in suns]
            assert priorities == [1] + [0] * (len(suns) - 1)
            assert atmosphere == [True] + [False] * (len(suns) - 1)
            player = u.GameplayStatics.get_player_pawn(state['world'], 0)
            player.get_component_by_class(u.CharacterMovementComponent).disable_movement()
            camera = player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view', 75)
            point = u.Vector(-56000, 510500, -5800)
            location = point + u.Vector(-4000, -3000, 1500)
            camera.set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, point + u.Vector(0, 0, 800)), False, True)
            pc.get_time().set_running(False)
            state.update(cycle=cycle, suns=suns, skies=skies, spheres=spheres, camera=camera)
            state['checks'].update(suns=len(suns), skylights=len(skies), sky_spheres=len(spheres), primary_sun_priorities=priorities, atmosphere_suns=atmosphere, all_lights_bound=True)
            jump()
            return
        if state['phase'] == 'capturing' and now - state['at'] > 8:
            sample = state['cycle'].get_editor_property('current_state')
            name, minute = probes[state['index']]
            assert abs(sample.minute_of_day - minute) < .1, (sample.minute_of_day, minute)
            forward = u.MathLibrary.get_forward_vector(sample.sun_rotation)
            intensities = []
            for sun in state['suns']:
                assert (sun.get_actor_forward_vector() - forward).length() < .001, 'A sun did not follow the clock'
                intensities.append(sun.get_component_by_class(u.DirectionalLightComponent).get_editor_property('intensity'))
            ambient = [s.get_component_by_class(u.SkyLightComponent).get_editor_property('intensity') for s in state['skies']]
            heights = [float(s.get_editor_property('Sun height')) for s in state['spheres']]
            assert all(h < 0 for h in heights) if minute == 1320 else all(h > 0 for h in heights), heights
            if minute == 1320:
                assert all(v == 0 for v in intensities), 'A sun still lights the night'
                baseline = state['samples'][0]['sky_intensities']
                assert all(abs(value - day * .08) < .0001 for value, day in zip(ambient, baseline)), 'Ambient light did not dim'
            else:
                assert all(v > 0 for v in intensities), 'Daytime lighting did not recover'
            image_path = out / (name + '.png')
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(state['world'], str(image_path))
            state['samples'].append({'name': name, 'minute': sample.minute_of_day, 'elevation': sample.sun_elevation_degrees, 'sun_intensities': intensities, 'sky_intensities': ambient, 'sky_scale': sample.sky_intensity_scale, 'sky_sphere_heights': heights, 'image': str(image_path)})
            state['index'] += 1
            if state['index'] == len(probes):
                state['phase'] = 'complete'
                report()
                u.log('IC_DAY_NIGHT_REPAIR_VERIFIED')
                finish()
                return
            jump()
    except Exception:
        state.update(phase='error', error=traceback.format_exc())
        report()
        u.log_error(state['error'])
        finish()


try:
    u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
    state['handle'] = u.register_slate_post_tick_callback(tick)
    assert u.ICCharacterSetupLibrary.begin_monster_test_review()
    report()
except Exception:
    if state.get('handle'):
        u.unregister_slate_post_tick_callback(state['handle'])
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.log('IC_DAY_NIGHT_REPAIR_FINISHED')
    raise
