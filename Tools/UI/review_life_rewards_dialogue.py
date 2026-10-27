"""Verify real gathering/mining HUD rewards and keyboard-only NPC interaction in PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'LifeRewardsDialogue'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'starting', 'started': time.monotonic(), 'checks': {}}

def report():
    result = {key: STATE[key] for key in ('phase', 'checks')}
    if 'error' in STATE: result['error'] = STATE['error']
    (OUT / 'LiveReview.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')

def phase(name):
    STATE.update(phase=name, phase_started=time.monotonic())
    report()

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))

def press(name):
    key = u.Key()
    key.set_editor_property('key_name', name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'], key), name

def subsystem(cls):
    return next(s for s in u.ObjectIterator(cls) if s.get_outer() == STATE['pc'].get_world_state().get_outer())

def interact(): STATE['pc'].handle_input_command(u.ICInputCommand.INTERACT, 0)

def entries():
    dialogues = [d for d in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICNPCDialogueWidget, False) if d.get_parent()]
    if not dialogues: return []
    return [e for e in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICDialogueEntryWidget, False)
            if e.get_parent() and e.get_parent().get_outer().get_outer() == dialogues[0]]

def labels(): return [str(e.get_entry_text()) for e in entries()]

def selected():
    values = [str(e.get_entry_text()) for e in entries() if e.get_editor_property('selected')]
    assert len(values) == 1, values
    return values[0]

def toast_text():
    toasts = [w for w in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICRewardNoticeWidget, False) if w.get_parent()]
    return [str(t.get_text()) for t in u.ObjectIterator(u.TextBlock)
            if t.get_name() == 'TXT_Items' and t.get_outer() and t.get_outer().get_outer() in toasts]

def near(actor):
    point = actor.get_actor_location()
    STATE['player'].set_actor_location_and_rotation(point + u.Vector(-190, 0, 96 if isinstance(actor, u.ICLifeActor) else 0), u.Rotator(), False, True)
    STATE['player'].get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
    STATE['pc'].set_control_rotation(u.Rotator())
    view = STATE['player'].get_component_by_class(u.CameraComponent)
    position = point + u.Vector(-620, -300, 350)
    view.set_world_location_and_rotation(position, u.MathLibrary.find_look_at_rotation(position, point + u.Vector(0, 0, 60)), False, True)

def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 180: raise RuntimeError('Review timeout in ' + STATE['phase'])
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
            STATE.update(player=u.GameplayStatics.get_player_pawn(world, 0), gather=subsystem(u.ICGatheringSubsystem))
            pc.set_menu('')
            STATE['resources'] = {str(a.get_editor_property('point_id')): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.ICLifeActor)}
            near(STATE['resources']['TEST_R01_GATHER_01'])
            phase('gather_settle')
            return
        if STATE['phase'] == 'gather_settle' and elapsed > 1:
            STATE['before_plant'] = pc.get_inventory().count_item('ITM_R01_PLANT_01')
            interact()
            assert STATE['gather'].is_gathering()
            phase('gather')
            return
        if STATE['phase'] == 'gather' and elapsed > .2 and not STATE['gather'].is_gathering():
            assert pc.get_inventory().count_item('ITM_R01_PLANT_01') == STATE['before_plant'] + 1
            assert any(t.startswith('+ ') and '들꽃 줄기' in t and '1개' in t for t in toast_text()), toast_text()
            STATE['checks']['gathering_real_item_hud_notice'] = toast_text()
            capture('Gathering_HUD_ItemReward')
            near(STATE['resources']['TEST_R01_ORE_01'])
            phase('mining_settle')
            return
        if STATE['phase'] == 'mining_settle' and elapsed > 1:
            STATE['before_ore'] = pc.get_inventory().count_item('ITM_R01_ORE_01')
            interact()
            assert STATE['gather'].is_gathering()
            phase('mining')
            return
        if STATE['phase'] == 'mining' and elapsed > .2 and not STATE['gather'].is_gathering():
            assert pc.get_inventory().count_item('ITM_R01_ORE_01') == STATE['before_ore'] + 1
            assert '+ 광석1 1개' in toast_text(), toast_text()
            STATE['checks']['mining_real_item_hud_notice'] = toast_text()
            capture('Mining_HUD_ItemReward')
            npc = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.ICNPCBase)
                       if str(a.get_editor_property('npc_definition_id')) == 'NPC_R01_002')
            npc.get_controller().stop_movement()
            npc.get_controller().set_actor_tick_enabled(False)
            npc.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            STATE['npc'] = npc
            pc.get_world_state().set_affinity('NPC_R01_002', 300)
            near(npc)
            phase('npc_settle')
            return
        if STATE['phase'] == 'npc_settle' and elapsed > 1:
            interact()
            phase('greeting')
            return
        if STATE['phase'] == 'greeting' and elapsed > .25:
            assert selected()
            if selected() == '동료 영입하기':
                STATE['checks']['enter_advances_without_mouse'] = True
                capture('NPC_Keyboard_FirstSelection')
                press('W')
                assert selected() == '대화 끝내기'
                phase('wrap')
            else:
                if 'greeting_captured' not in STATE:
                    capture('NPC_Keyboard_Greeting')
                    STATE['greeting_captured'] = True
                press('Enter')
                STATE['phase_started'] = time.monotonic()
            return
        if STATE['phase'] == 'wrap' and elapsed > .2:
            press('S')
            assert selected() == '동료 영입하기'
            press('S')
            assert selected() == '퀘스트'
            STATE['checks']['w_s_selection_and_wrap'] = True
            phase('quest_selected')
            return
        if STATE['phase'] == 'quest_selected' and elapsed > .2:
            capture('NPC_Keyboard_QuestSelected')
            press('Enter')
            phase('quest_list')
            return
        if STATE['phase'] == 'quest_list' and elapsed > .2:
            assert selected() and '동료 영입하기' not in labels()
            STATE['checks']['enter_opens_selected_quest_list'] = labels()
            capture('NPC_Keyboard_QuestList')
            press('W')
            assert selected() == '대화 끝내기'
            press('Enter')
            assert not str(pc.get_editor_property('menu'))
            phase('reopen_recruit')
            return
        if STATE['phase'] == 'reopen_recruit' and elapsed > .6:
            near(STATE['npc'])
            interact()
            assert str(pc.get_editor_property('menu')) == 'Dialogue'
            phase('recruit_greeting')
            return
        if STATE['phase'] == 'recruit_greeting' and elapsed > .25:
            if selected() != '동료 영입하기':
                press('Enter')
                STATE['phase_started'] = time.monotonic()
                return
            press('Enter')
            snapshot = pc.get_world_state().get_snapshot('NPC_R01_002')
            assert snapshot.mode == u.ICCharacterMode.COMPANION and snapshot.party_slot >= 0
            assert not str(pc.get_editor_property('menu'))
            STATE['checks']['enter_recruits_actual_companion'] = True
            phase('reopen_companion')
            return
        if STATE['phase'] == 'reopen_companion' and elapsed > .4:
            near(STATE['npc'])
            interact()
            phase('dismiss_greeting')
            return
        if STATE['phase'] == 'dismiss_greeting' and elapsed > .25:
            if selected() != '동료 해제하기':
                press('Enter')
                STATE['phase_started'] = time.monotonic()
                return
            capture('NPC_Keyboard_CompanionDismiss')
            press('S')
            press('S')
            assert selected() == '선물하기'
            press('Enter')
            phase('gift')
            return
        if STATE['phase'] == 'gift' and elapsed > .25:
            pickers = [w for w in u.WidgetLibrary.get_all_widgets_of_class(world, u.ICNPCGiftWidget, False) if w.get_parent()]
            assert len(pickers) == 1
            press('Escape')
            assert selected() == '선물하기'
            STATE['checks']['keyboard_activates_gift_option'] = True
            press('W')
            press('W')
            assert selected() == '동료 해제하기'
            press('Enter')
            snapshot = pc.get_world_state().get_snapshot('NPC_R01_002')
            assert snapshot.mode != u.ICCharacterMode.COMPANION and snapshot.party_slot == -1
            assert not str(pc.get_editor_property('menu'))
            STATE['checks']['enter_dismisses_actual_companion'] = True
            phase('complete')
            return
        if STATE['phase'] == 'complete':
            report()
            u.log('IC_LIFE_REWARDS_DIALOGUE_REVIEW_OK ' + json.dumps(STATE['checks'], ensure_ascii=False))
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
