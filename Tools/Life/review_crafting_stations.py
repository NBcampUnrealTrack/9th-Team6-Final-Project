"""Exercise real F interaction, keyboard recipe UI, production and loot toast in PIE.

Uses the existing isolated review save namespace, never the user's save slots.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'CraftingStationLiveReview'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'starting', 'started': time.monotonic(), 'checks': {}, 'complete': False}


def report():
    (OUT / 'Review.json').write_text(json.dumps({k: STATE[k] for k in ('phase', 'checks', 'complete', 'error') if k in STATE}, indent=2, ensure_ascii=False), encoding='utf-8')


def phase(name):
    STATE.update(phase=name, phase_started=time.monotonic())
    report()


def finish():
    report()
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def press(name):
    key = u.Key(); key.set_editor_property('key_name', name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'], key), name


def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))


def widget(cls, name, owner):
    return next(w for w in u.ObjectIterator(cls) if w.get_name() == name and w.get_outer() and w.get_outer().get_outer() == owner)


def open_ui():
    STATE['pc'].handle_input_command(u.ICInputCommand.INTERACT, 0)
    return next(w for w in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICCraftingStationWidget, False) if w.is_open())


def near_station(index):
    station = STATE['stations'][index]
    p = station.get_actor_location()
    STATE['player'].set_actor_location_and_rotation(p + u.Vector(-190, 0, 56), u.Rotator(), False, True)
    STATE['player'].get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
    STATE['pc'].set_control_rotation(u.Rotator())
    view = STATE['player'].get_component_by_class(u.CameraComponent)
    location = p + u.Vector(-620, -240, 260)
    view.set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, p), False, True)


def toast_text():
    notices = [w for w in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICRewardNoticeWidget, False) if w.get_parent()]
    return [str(w.get_text()) for w in u.ObjectIterator(u.TextBlock) if w.get_name() == 'TXT_Items'
            and w.get_outer() and w.get_outer().get_outer() in notices]


def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 180:
            raise RuntimeError('Station live review timeout in ' + STATE['phase'])
        if STATE['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0], pc=pc)
            phase('creating')
            return
        elapsed = time.monotonic() - STATE['phase_started']
        pc = STATE['pc']
        if STATE['phase'] == 'creating':
            if not pc.get_editor_property('created'):
                return
            STATE['player'] = u.GameplayStatics.get_player_pawn(STATE['world'], 0)
            for enemy in u.GameplayStatics.get_all_actors_of_class(STATE['world'], u.ICEnemyBase):
                controller = enemy.get_controller()
                if controller:
                    brain = controller.get_component_by_class(u.BrainComponent)
                    if brain: brain.stop_logic('Crafting UI review')
                enemy.set_actor_enable_collision(False)
                enemy.set_actor_location(u.Vector(50000, 50000, -1000), False, True)
            for item in ('ITM_R01_ORE_06', 'ITM_R01_PLANT_10', 'ITM_R01_CROP_04', 'ITM_R01_FISH_04', 'ITM_R01_CROP_01'):
                assert pc.get_inventory().add_item(item, 10, False)
            all_stations = u.GameplayStatics.get_all_actors_of_class(STATE['world'], u.ICCraftingStation)
            STATE['stations'] = [next(a for a in all_stations if str(a.get_editor_property('work_anchor_id')) == id)
                                 for id in ('ANC_FORGE_001', 'ANC_KITCHEN_001')]
            STATE['index'] = 0
            pc.set_menu(''); near_station(0); phase('near')
            return
        prefix = 'Crafting' if STATE['index'] == 0 else 'Cooking'
        if STATE['phase'] == 'near' and elapsed > .8:
            capture(prefix + '_Station')
            STATE['ui'] = open_ui()
            first = str(STATE['ui'].get_editor_property('selected_recipe'))
            press('S'); second = str(STATE['ui'].get_editor_property('selected_recipe'))
            assert first != second
            press('W'); assert str(STATE['ui'].get_editor_property('selected_recipe')) == first
            STATE['checks'][prefix + '_F_and_WS'] = True
            phase('list')
            return
        if STATE['phase'] == 'list' and elapsed > .25:
            capture(prefix + '_RecipeList')
            press('Enter'); assert STATE['ui'].get_editor_property('details')
            press('Escape'); assert not STATE['ui'].get_editor_property('details') and STATE['ui'].is_open()
            STATE['ui'].select_recipe('RCP_CRAFT_WAND_001' if STATE['index'] == 0 else 'RCP_COOK_001')
            phase('details')
            return
        if STATE['phase'] == 'details' and elapsed > .25:
            capture(prefix + '_RecipeDetails')
            STATE['output'] = 'EQP_WPN_WAND_001' if STATE['index'] == 0 else 'ITM_FOOD_002'
            STATE['before'] = pc.get_inventory().count_item(STATE['output'])
            press('Enter')
            STATE['job'] = STATE['ui'].get_editor_property('active_job').to_string()
            assert STATE['job'].replace('0', '').replace('-', ''), 'No production job'
            phase('working')
            return
        if STATE['phase'] == 'working':
            bar = widget(u.ProgressBar, 'BAR_Work', STATE['ui'])
            percent = bar.get_editor_property('percent')
            if .25 < percent < .7 and not STATE.get(prefix + '_progress'):
                capture(prefix + '_Progress'); STATE[prefix + '_progress'] = True
                STATE['checks'][prefix + '_progress'] = percent
                if STATE['index'] == 0:
                    STATE['ui'].close_station()
                    STATE['ui'] = open_ui()
                    assert STATE['ui'].get_editor_property('active_job').to_string() == STATE['job']
                    STATE['checks']['Reopen_resumes_same_job'] = True
            if not STATE['ui'].is_open():
                assert pc.get_inventory().count_item(STATE['output']) == STATE['before'] + 1
                notices = toast_text()
                assert any('획득' in text for text in notices), notices
                STATE['checks'][prefix + '_actual_reward_toast'] = notices
                phase('reward')
            return
        if STATE['phase'] == 'reward' and elapsed > .15:
            capture(prefix + '_ItemReceived')
            if STATE['index'] == 0:
                STATE['index'] = 1; near_station(1); phase('near')
            else:
                STATE.update(phase='complete', complete=True)
                finish()
    except Exception:
        STATE.update(phase='error', error=traceback.format_exc())
        u.log_error(STATE['error']); finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
