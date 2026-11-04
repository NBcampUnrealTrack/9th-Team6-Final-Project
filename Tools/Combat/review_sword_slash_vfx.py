"""Render all five sword contacts on both presets in an isolated PIE session.

Run a dedicated editor with -ICSwordSlashReview -NoSaveConfig -RenderOffscreen
and -UseFixedTimeStep -FPS=30
and -ExecutePythonScript=<this file>. No content packages or player saves change.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

assert '-ICSwordSlashReview' in u.SystemLibrary.get_command_line()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
performance = u.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings')
if performance:
    performance.set_editor_property('bThrottleCPUWhenNotForeground', False)
u.EditorPythonScripting.set_keep_python_script_alive(True)
OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'SwordSlashVFX/Visual'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
S = dict(phase='starting', started=time.monotonic(), complete=False, samples=[], case=-1)


def report():
    (OUT / 'Review.json').write_text(json.dumps({k: S[k] for k in ('phase', 'step', 'case', 'complete', 'samples', 'seen', 'error') if k in S},
                                              indent=2), encoding='utf-8')


def finish():
    report()
    S['phase'] = 'ending'
    LEVELS.editor_request_end_play()


def attack():
    result = S['asc'].request_action('Attack', 'None', None)
    assert result.success, str(result.reason)


def advance():
    S['asc'].cancel_action()
    S['case'] += 1
    if S['case'] == 2:
        S.update(phase='complete', complete=True)
        finish()
        return
    S['player'].apply_preset(S['case'])
    S['player'].set_actor_location_and_rotation(u.Vector(-1000, -2500, 96), u.Rotator(), False, True)
    S['pc'].set_control_rotation(u.Rotator())
    view, focus = u.Vector(-720, -2870, 280), u.Vector(-965, -2500, 95)
    S['camera'].set_world_location_and_rotation(view, u.MathLibrary.find_look_at_rotation(view, focus), False, True)
    S.update(phase='settle', ready=time.monotonic(), seen={}, queued=False)
    report()


def tick(delta):
    try:
        if S['phase'] == 'ending':
            if u.EditorLevelLibrary.get_pie_worlds(False):
                return
            u.unregister_slate_post_tick_callback(S['handle'])
            u.EditorPythonScripting.set_keep_python_script_alive(False)
            u.SystemLibrary.quit_editor()
            return
        assert time.monotonic() - S['started'] < 300, 'Sword slash review timed out'
        if S['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            u.SystemLibrary.execute_console_command(worlds[0], 't.IdleWhenNotForeground 0')
            u.SystemLibrary.execute_console_command(worlds[0], 't.MaxFPS 30')
            u.SystemLibrary.execute_console_command(worlds[0], 'r.MotionBlurQuality 0')
            S.update(world=worlds[0], pc=pc, phase='creating')
            report()
            return
        if S['phase'] == 'creating':
            if not S['pc'].get_editor_property('created'):
                return
            player = u.GameplayStatics.get_player_pawn(S['world'], 0)
            S['step'] = 'equip'
            report()
            inventory = S['pc'].get_inventory()
            if inventory.get_weapon_loadout()[0].count == 0:
                assert inventory.add_item('EQP_WPN_SWORD_001', 1, False)
                slot = next(i for i, v in enumerate(inventory.get_snapshot().bag.slots) if str(v.item_id) == 'EQP_WPN_SWORD_001')
                assert inventory.equip(slot)
            for _ in range(6):
                if inventory.get_equipped_weapon_family() == u.ICWeaponFamily.SWORD:
                    break
                assert inventory.cycle_weapon()
            for cls in (u.ICEnemyBase, u.ICNPCBase):
                for actor in u.GameplayStatics.get_all_actors_of_class(S['world'], cls):
                    S['step'] = 'isolate ' + actor.get_name()
                    report()
                    ai = actor.get_controller()
                    if ai:
                        brain = ai.get_component_by_class(u.BrainComponent)
                        if brain:
                            brain.stop_logic('Sword slash VFX review')
                        ai.set_actor_tick_enabled(False)
                    actor.get_component_by_class(u.CharacterMovementComponent).set_component_tick_enabled(False)
                    actor.set_actor_location(u.Vector(50000, 50000, 96), False, True)
                    actor.set_actor_hidden_in_game(True)
                    actor.set_actor_tick_enabled(False)
                    mesh = actor.get_component_by_class(u.SkeletalMeshComponent)
                    if mesh:
                        mesh.set_component_tick_enabled(False)
            camera = player.get_component_by_class(u.CameraComponent)
            S['step'] = 'camera'
            report()
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view', 45)
            S['pc'].set_view_target_with_blend(player, 0)
            S['pc'].set_menu('')
            u.GameplayStatics.set_global_time_dilation(S['world'], .3)
            S.update(player=player, asc=player.get_component_by_class(u.ICAbilitySystemComponent), camera=camera)
            advance()
            return
        if S['phase'] == 'settle':
            if time.monotonic() - S['ready'] < 2:
                return
            attack()
            attack()
            S['phase'] = 'attack'
            return
        anim = S['player'].get_editor_property('mesh').get_anim_instance()
        origin = S['player'].get_actor_location()
        view, focus = origin + u.Vector(280, -370, 184), origin + u.Vector(35, 0, -1)
        S['camera'].set_world_location_and_rotation(view, u.MathLibrary.find_look_at_rotation(view, focus), False, True)
        montage = anim.get_current_active_montage()
        stage = int(montage.get_name().rsplit('_', 1)[1]) if montage and montage.get_name().startswith('AM_Combo_Sword_') else 0
        if stage == 2 and not S['queued']:
            attack()
            S['queued'] = True
        effects = u.GameplayStatics.get_all_actors_of_class(S['world'], u.ICAttackEffect)
        for effect in effects:
            if not effect.actor_has_tag('IC.SwordSlash'):
                continue
            name = effect.get_name()
            if name not in S['seen']:
                S['seen'][name] = {'stage': stage, 'time': anim.montage_get_position(montage), 'frames': 0}
            sample = S['seen'][name]
            sample['frames'] += 1
            if sample.get('capture') or sample['frames'] < 2:
                continue
            component = effect.get_component_by_class(u.NiagaraComponent)
            count = u.ICCombatVFXSetupLibrary.get_combat_particle_count(component)
            sample['last_particle_count'] = count
            if count < 1:
                continue
            gender = 'Male' if S['case'] == 0 else 'Female'
            path = OUT / (gender + '_Contact' + str(len(S['seen'])) + '.png')
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'], str(path))
            sample.update(capture=str(path), particles=count, system=component.get_asset().get_path_name(),
                          transform=str(effect.get_actor_transform()),
                          particle_values=u.ICCombatVFXSetupLibrary.describe_combat_particles(component))
            S['samples'].append(dict(gender=gender, **sample))
            report()
        if stage == 0 and S['queued']:
            contacts = list(S['seen'].values())
            assert len(contacts) == 5, str(contacts)
            assert [v['stage'] for v in contacts] == [1, 2, 2, 3, 3], str(contacts)
            if S.get('warmed'):
                assert all(v.get('capture') for v in contacts), 'Each contact must render particles'
            expected = [.15, .15, .45, 11 / 60, 44 / 60]
            assert all(abs(v['time'] - t) < .055 for v, t in zip(contacts, expected)), str(contacts)
            # The first editor activation can compile the mesh material/PSO.
            # Warm that path once before keeping the two measured presets.
            if not S.get('warmed'):
                S.update(warmed=True, case=-1, samples=[])
            advance()
    except Exception:
        S.update(phase='error', error=traceback.format_exc())
        u.log_error(S['error'])
        finish()


u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
report()
S['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
