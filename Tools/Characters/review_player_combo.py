"""Capture actual player combo animations and outgoing effects in isolated PIE."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/PlayerComboLiveReview'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'starting', 'started': time.monotonic(), 'samples': [], 'complete': False}
FAMILIES = (u.ICWeaponFamily.SWORD, u.ICWeaponFamily.BOW, u.ICWeaponFamily.STAFF, u.ICWeaponFamily.WAND)
NAMES = ('Sword', 'Bow', 'Staff', 'Wand')
TIMINGS = {(item['gender'], item['family'], item['stage']): item['hit_time']
           for item in json.loads((ROOT / 'Saved/PlayerComboSetupReport.json').read_text(encoding='utf-8'))}


def report():
    (OUT / 'Review.json').write_text(json.dumps({key: STATE[key] for key in ('phase', 'samples', 'complete', 'error') if key in STATE}, indent=2), encoding='utf-8')


def finish():
    report()
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def attack():
    result = STATE['asc'].request_action('Attack', 'None', None)
    if not result.success:
        raise RuntimeError(str(result.reason))


def camera():
    player = STATE['player']
    focus = player.get_actor_location() + u.Vector(1500, 0, 110)
    location = player.get_actor_location() + u.Vector(-420, 100, 100)
    STATE['camera'].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)


def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 240:
            raise RuntimeError('Combo review timeout')
        if STATE['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0], pc=pc, phase='creating')
            return
        if STATE['phase'] == 'creating':
            if not STATE['pc'].get_editor_property('created'):
                return
            player = u.GameplayStatics.get_player_pawn(STATE['world'], 0)
            inventory = STATE['pc'].get_inventory()
            for index, item in enumerate(('EQP_WPN_SWORD_001', 'EQP_WPN_BOW_001', 'EQP_WPN_STAFF_001', 'EQP_WPN_WAND_001')):
                if inventory.get_weapon_loadout()[index].count > 0:
                    continue
                assert inventory.add_item(item, 1, False)
                snapshot = inventory.get_snapshot()
                slot = next(i for i, value in enumerate(snapshot.bag.slots) if str(value.item_id) == item)
                assert inventory.equip(slot)
            player.set_actor_location_and_rotation(u.Vector(-1000, -2500, 90), u.Rotator(), False, True)
            STATE['pc'].set_control_rotation(u.Rotator())
            view = player.get_component_by_class(u.CameraComponent)
            view.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            view.set_editor_property('field_of_view', 70)
            STATE.update(player=player, inventory=inventory, camera=view,
                         asc=player.get_component_by_class(u.ICAbilitySystemComponent), case=0, phase='next')
            STATE['aim_point'] = player.get_actor_location() + u.Vector(1500, 0, 110)
            for enemy in u.GameplayStatics.get_all_actors_of_class(STATE['world'], u.ICEnemyBase):
                controller = enemy.get_controller()
                if controller:
                    controller.set_actor_tick_enabled(False)
                    brain = controller.get_component_by_class(u.BrainComponent)
                    if brain:
                        brain.stop_logic('Combo visual review')
                enemy.set_actor_enable_collision(False)
                enemy.set_actor_location(u.Vector(50000, 50000, -1000), False, True)
            return
        camera()
        if STATE['phase'] == 'next':
            case = STATE['case']
            if case >= 8:
                assert len(STATE['samples']) == 24
                STATE.update(phase='complete', complete=True)
                finish()
                return
            STATE['asc'].cancel_action()
            STATE['player'].apply_preset(case // 4)
            for _ in range(6):
                if STATE['inventory'].get_equipped_weapon_family() == FAMILIES[case % 4]:
                    break
                assert STATE['inventory'].cycle_weapon()
            STATE.update(phase='settling', ready=time.monotonic() + 1.1)
            return
        if STATE['phase'] == 'settling':
            if time.monotonic() < STATE['ready']:
                return
            attack()
            attack()  # Queue stage two early, as a real extra click would.
            STATE.update(phase='attacking', captured=set(), queued_third=False, attack_started=time.monotonic())
            return
        if STATE['phase'] == 'attacking':
            if time.monotonic() - STATE['attack_started'] > 12:
                raise RuntimeError('Combo/effect did not finish: ' + NAMES[STATE['case'] % 4])
            anim = STATE['player'].get_editor_property('mesh').get_anim_instance()
            montage = anim.get_current_active_montage()
            if not montage or not montage.get_name().startswith('AM_Combo_'):
                if len(STATE['captured']) == 3:
                    STATE['case'] += 1
                    STATE['phase'] = 'next'
                return
            stage = int(montage.get_name().rsplit('_', 1)[1])
            if stage == 2 and not STATE['queued_third']:
                attack()
                STATE['queued_third'] = True
            visual_type = u.ICAttackEffect if STATE['case'] % 4 == 0 else u.ICProjectile
            visuals = u.GameplayStatics.get_all_actors_of_class(STATE['world'], visual_type)
            hit_time = TIMINGS[('Male' if STATE['case'] < 4 else 'Female', NAMES[STATE['case'] % 4], stage)]
            if stage not in STATE['captured'] and visuals and anim.montage_get_position(montage) >= hit_time + .03:
                label = f'{"M" if STATE["case"] < 4 else "F"}_{NAMES[STATE["case"] % 4]}_{stage}'
                assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (label + '.png')))
                sample = {'case': label, 'montage': montage.get_path_name(), 'visible_effects': len(visuals)}
                if STATE['case'] % 4:
                    shot = max(visuals, key=lambda value: int(value.get_name().rsplit('_', 1)[1]))
                    velocity = shot.get_component_by_class(u.ProjectileMovementComponent).get_editor_property('velocity')
                    aim = STATE['aim_point'] - shot.get_actor_location()
                    dot = sum(getattr(velocity, axis) * getattr(aim, axis) for axis in ('x', 'y', 'z'))
                    length = math.sqrt(sum(getattr(velocity, axis) ** 2 for axis in ('x', 'y', 'z')) * sum(getattr(aim, axis) ** 2 for axis in ('x', 'y', 'z')))
                    sample['screen_centre_alignment'] = dot / max(length, .001)
                    assert sample['screen_centre_alignment'] > .995, sample
                STATE['samples'].append(sample)
                STATE['captured'].add(stage)
                report()
    except Exception:
        STATE.update(phase='error', error=traceback.format_exc())
        u.log_error(STATE['error'])
        finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
