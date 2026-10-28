"""Review calendar regeneration, natural crop meshes and chicken combat in real PIE.

Disposable level actors are never saved; the existing review helper isolates save slots.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'LifeCalendarLiveReview'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'starting', 'started': time.monotonic(), 'checks': {}, 'complete': False}


def report():
    data = {k: STATE[k] for k in ('phase', 'checks', 'complete', 'error') if k in STATE}
    (OUT / 'Review.json').write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')


def phase(name):
    STATE.update(phase=name, phase_started=time.monotonic()); report()


def finish():
    report(); u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play(); u.SystemLibrary.quit_editor()


def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))


def camera(point, offset):
    location = point + offset
    STATE['camera'].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, point), False, True)


def subsystem(cls):
    return next(s for s in u.ObjectIterator(cls) if s.get_outer() == STATE['pc'].get_world_state().get_outer())


def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 200:
            raise RuntimeError('Life calendar review timeout: ' + STATE['phase'])
        if STATE['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0], pc=pc); phase('creating'); return
        pc = STATE['pc']; world = STATE['world']; elapsed = time.monotonic() - STATE['phase_started']
        if STATE['phase'] == 'creating':
            if not pc.get_editor_property('created'):
                return
            player = u.GameplayStatics.get_player_pawn(world, 0)
            view = player.get_component_by_class(u.CameraComponent)
            view.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            STATE.update(player=player, camera=view, clock=subsystem(u.ICTimeSubsystem), gather=subsystem(u.ICGatheringSubsystem))
            for enemy in u.GameplayStatics.get_all_actors_of_class(world, u.ICEnemyBase):
                ai = enemy.get_controller()
                if ai:
                    brain = ai.get_component_by_class(u.BrainComponent)
                    if brain: brain.stop_logic('Life calendar review')
                enemy.set_actor_location(u.Vector(50000, 50000, 96), False, True)
            life = u.GameplayStatics.get_all_actors_of_class(world, u.ICLifeActor)
            STATE['plots'] = [next(a for a in life if str(a.get_editor_property('point_id')) == 'REVIEW_CALENDAR_CROP_%02d' % i) for i in range(1, 6)]
            STATE['plant'] = next(a for a in life if str(a.get_editor_property('point_id')) == 'REVIEW_CALENDAR_RESOURCE')
            STATE['chicken'] = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.ICRanchAnimal) if str(a.get_editor_property('point_id')) == 'REVIEW_CALENDAR_CHICKEN')
            pc.set_menu(''); STATE['clock'].set_running(False)
            assert all(a.get_growth_stage() == 0 and not a.is_harvestable() for a in STATE['plots'])
            assert all(not a.try_interact(pc) for a in STATE['plots'])
            STATE['checks']['first_stage_and_no_early_harvest'] = True
            camera(u.Vector(0, 1500, 70), u.Vector(-1100, 0, 800)); phase('first_stage'); return
        if STATE['phase'] == 'first_stage' and elapsed > 1.5:
            capture('Crops_FirstStage')
            STATE['clock'].jump_game_minutes(1440)
            assert all(a.get_growth_stage() == 1 for a in STATE['plots'])
            STATE['checks']['one_day_one_stage'] = True; phase('day1'); return
        if STATE['phase'] == 'day1' and elapsed > 1.5:
            capture('Crops_OneDay')
            STATE['clock'].jump_game_minutes(1440 * 3)
            assert all(a.is_harvestable() for a in STATE['plots'])
            STATE['checks']['original_last_meshes_are_harvestable'] = True; phase('ripe'); return
        if STATE['phase'] == 'ripe' and elapsed > 2:
            capture('Crops_FinalStages')
            for a in STATE['plots']: assert a.try_interact(pc)
            assert all(a.get_growth_stage() == -1 for a in STATE['plots'])
            STATE['clock'].jump_game_minutes(119)
            assert all(a.get_growth_stage() == -1 for a in STATE['plots'])
            STATE['checks']['harvested_crops_hidden_for_119_minutes'] = True; phase('regrowing'); return
        if STATE['phase'] == 'regrowing' and elapsed > .3:
            capture('Crops_Harvested')
            STATE['clock'].jump_game_minutes(1)
            assert all(a.get_growth_stage() == 0 and not a.is_harvestable() for a in STATE['plots'])
            STATE['checks']['two_hours_restarts_first_stage'] = True; phase('regenerated'); return
        if STATE['phase'] == 'regenerated' and elapsed > 1:
            capture('Crops_Regenerated')
            plant = STATE['plant']; p = plant.get_actor_location()
            STATE['player'].set_actor_location_and_rotation(p + u.Vector(-150, 0, 96), u.Rotator(), False, True)
            STATE['player'].get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            pc.set_control_rotation(u.Rotator()); camera(p + u.Vector(0, 0, 70), u.Vector(-550, -200, 320))
            STATE['clock'].set_running(True)
            assert plant.try_interact(pc); phase('gathering'); return
        if STATE['phase'] == 'gathering':
            if elapsed < 5 and STATE['gather'].is_gathering():
                return
            if STATE['gather'].is_gathering():
                return
            STATE['clock'].set_running(False)
            assert not STATE['plant'].is_resource_available()
            STATE['clock'].jump_game_minutes(119)
            assert not STATE['plant'].is_resource_available()
            STATE['clock'].jump_game_minutes(1)
            assert STATE['plant'].is_resource_available()
            STATE['checks']['gathering_regenerates_on_calendar_two_hours'] = True
            inventory = pc.get_inventory()
            if inventory.get_weapon_loadout()[0].count == 0:
                assert inventory.add_item('EQP_WPN_SWORD_001', 1, False)
                index = next(i for i, item in enumerate(inventory.get_snapshot().bag.slots) if str(item.item_id) == 'EQP_WPN_SWORD_001')
                assert inventory.equip(index)
            for _ in range(6):
                if inventory.get_equipped_weapon_family() == u.ICWeaponFamily.SWORD: break
                assert inventory.cycle_weapon()
            chicken = STATE['chicken']; p = chicken.get_actor_location()
            STATE['player'].set_actor_location_and_rotation(p + u.Vector(-115, 0, 56), u.Rotator(), False, True)
            pc.set_control_rotation(u.Rotator()); camera(p + u.Vector(-40, 0, 40), u.Vector(-450, -280, 230))
            STATE.update(asc=STATE['player'].get_component_by_class(u.ICAbilitySystemComponent), before_meat=inventory.count_item('ITM_R01_CHICKEN_MEAT'), attacks=0)
            STATE['clock'].set_running(True); phase('chicken'); return
        if STATE['phase'] == 'chicken':
            if STATE['chicken'].get_editor_property('hidden'):
                assert pc.get_inventory().count_item('ITM_R01_CHICKEN_MEAT') == STATE['before_meat'] + 1
                STATE['checks']['normal_attack_kills_chicken_and_grants_meat'] = True
                STATE['checks']['egg_count'] = pc.get_inventory().count_item('ITM_R01_RANCH_01')
                phase('chicken_reward'); return
            if elapsed > (STATE['attacks'] + 1) * 1.5:
                assert STATE['attacks'] < 8, 'Chicken did not receive normal attack damage'
                result = STATE['asc'].request_action('Attack', 'None', STATE['chicken'])
                assert result.success, str(result.reason)
                STATE['attacks'] += 1
        if STATE['phase'] == 'chicken_reward' and elapsed > .15:
            capture('Chicken_NormalAttackReward'); STATE.update(phase='complete', complete=True); finish()
    except Exception:
        STATE.update(phase='error', error=traceback.format_exc())
        u.log_error(STATE['error']); finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
for i in range(1, 6):
    bp = u.load_asset('/Game/IsekaiClass/Life/Farming/BP_R01_Crop_%02d' % i)
    a = actors.spawn_actor_from_class(bp.generated_class(), u.Vector(0, 1500 + (i - 3) * 175, 0))
    a.set_editor_property('point_id', 'REVIEW_CALENDAR_CROP_%02d' % i)
for name, point, location in [('Collection/BP_R01_Collection_01', 'REVIEW_CALENDAR_RESOURCE', u.Vector(0, 3500, 0)),
                              ('Ranching/BP_R01_Chicken', 'REVIEW_CALENDAR_CHICKEN', u.Vector(0, 4500, 40))]:
    bp = u.load_asset('/Game/IsekaiClass/Life/' + name)
    a = actors.spawn_actor_from_class(bp.generated_class(), location); a.set_editor_property('point_id', point)
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
