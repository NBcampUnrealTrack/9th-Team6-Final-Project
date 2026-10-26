"""Check the actual settings WBP in title and gameplay using Slate keyboard input."""
import json
import time
import traceback
import runpy
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'SettingsWidgetReview'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'title_start', 'started': time.monotonic(), 'checks': {}}

def phase(value):
    STATE.update(phase=value, phase_started=time.monotonic())
    report()

def report():
    (OUT / 'LiveReview.json').write_text(json.dumps({k: v for k, v in STATE.items()
        if k in ('phase', 'checks', 'error')}, ensure_ascii=False, indent=2), encoding='utf-8')

def press(key_name):
    key = u.Key()
    key.set_editor_property('key_name', key_name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'], key), key_name

def screen():
    values = [widget for widget in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICSettingsWidget, False)
              if widget.get_parent()]
    assert len(values) == 1
    result = values[0]
    assert result.get_class().get_path_name() == '/Game/IsekaiClass/UI/Widget/WBP_IC_Settings.WBP_IC_Settings_C'
    return result

def control(current, cls, name):
    return next(widget for widget in u.ObjectIterator(cls)
                if widget.get_name() == name and widget.get_outer().get_outer() == current)

def has_screen():
    return any(widget.get_parent() for widget in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICSettingsWidget, False))

def game_menu():
    return next(widget for widget in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICGameMenuWidget, False)
                if widget.get_parent())

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 130:
            raise RuntimeError('Timeout in ' + STATE['phase'])
        elapsed = time.monotonic() - STATE.get('phase_started', STATE['started'])
        worlds = u.EditorLevelLibrary.get_pie_worlds(False)
        if STATE['phase'] == 'title_start':
            if not worlds: return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'MainMenu': return
            STATE.update(world=worlds[0], pc=pc)
            pc.set_menu('Settings')
            phase('title_settings')
            return
        if STATE['phase'] == 'title_settings' and elapsed > .5:
            current = screen()
            sensitivity = control(current, u.Slider, 'SLIDER_Sensitivity')
            fullscreen = control(current, u.CheckBox, 'CHECK_Fullscreen')
            quality = control(current, u.ComboBoxString, 'COMBO_Quality')
            assert sensitivity and fullscreen and quality and quality.get_option_count() == 4
            before = sensitivity.get_value()
            press('D')
            assert abs(sensitivity.get_value() - min(3., before + .1)) < .001
            press('A')
            sensitivity.set_value(before)
            press('S')
            checked = fullscreen.is_checked()
            press('Enter')
            assert fullscreen.is_checked() != checked
            press('Enter')
            assert fullscreen.is_checked() == checked
            press('S')
            index = quality.get_selected_index()
            press('A')
            assert quality.get_selected_index() == max(0, index - 1)
            press('D')
            quality.set_selected_index(index)
            STATE['checks']['title_uses_wbp_and_wasd_controls'] = True
            capture('TitleSettings')
            press('Escape')
            assert str(STATE['pc'].get_editor_property('menu')) == 'MainMenu'
            assert not has_screen()
            STATE['checks']['close_returns_to_title'] = True
            LEVELS.editor_request_end_play()
            phase('title_end')
            return
        if STATE['phase'] == 'title_end' and not worlds:
            u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
            assert u.ICCharacterSetupLibrary.begin_monster_test_review()
            phase('game_start')
            return
        if STATE['phase'] == 'game_start':
            if not worlds: return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create': return
            STATE.update(world=worlds[0], pc=pc)
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            phase('creating')
            return
        if STATE['phase'] == 'creating':
            if not STATE['pc'].get_editor_property('created'): return
            STATE['pc'].set_menu('Pause')
            phase('game_menu')
            return
        if STATE['phase'] == 'game_menu' and elapsed > .4:
            menu = game_menu()
            press('A'); press('A'); press('S')
            assert str(menu.get_selected_tab()) == 'Settings'
            press('S'); press('S'); press('S')
            assert menu.get_editor_property('settings_action_index') == 3
            button = control(menu, u.Button, 'BTN_GameSettings')
            assert str(button.get_child_at(0).get_text()) == '게임 설정'
            assert button.get_visibility() == u.SlateVisibility.VISIBLE
            capture('GameplaySettingsMenu')
            press('Enter')
            assert str(STATE['pc'].get_editor_property('menu')) == 'Settings'
            STATE['checks']['menu_game_settings_button_opens_wbp_by_keyboard'] = True
            phase('game_settings')
            return
        if STATE['phase'] == 'game_settings' and elapsed > .5:
            current = screen()
            sensitivity = control(current, u.Slider, 'SLIDER_Sensitivity')
            STATE['old_sensitivity'] = sensitivity.get_value()
            # Exercise Apply with a real pawn and keep graphics/window mode unchanged.
            sensitivity.set_value(1.4 if sensitivity.get_value() < 1.3 else 1.2)
            STATE['new_sensitivity'] = sensitivity.get_value()
            press('S'); press('S'); press('S'); press('Enter')
            phase('applied')
            return
        if STATE['phase'] == 'applied' and elapsed > .5:
            player = u.GameplayStatics.get_player_pawn(STATE['world'], 0)
            assert abs(player.get_editor_property('camera_sensitivity') - STATE['new_sensitivity']) < .001
            STATE['checks']['apply_updates_actual_player_sensitivity'] = True
            capture('GameplaySettings')
            press('S'); press('Enter')
            assert str(STATE['pc'].get_editor_property('menu')) == 'Pause'
            STATE['checks']['close_button_returns_to_game_menu'] = True
            phase('returned_menu')
            return
        if STATE['phase'] == 'returned_menu' and elapsed > .4:
            menu = game_menu()
            assert str(menu.get_selected_tab()) == 'Settings'
            assert menu.get_editor_property('settings_action_index') == 3
            assert u.GameplayStatics.is_game_paused(STATE['world'])
            STATE['checks']['return_preserves_selected_tab_action_and_pause'] = True
            press('Enter')
            phase('reopened')
            return
        if STATE['phase'] == 'reopened' and elapsed > .4:
            current = screen()
            assert abs(control(current, u.Slider, 'SLIDER_Sensitivity').get_value() - STATE['new_sensitivity']) < .001
            STATE['checks']['reopen_reads_saved_setting'] = True
            control(current, u.Slider, 'SLIDER_Sensitivity').set_value(STATE['old_sensitivity'])
            press('S'); press('S'); press('S'); press('Enter')
            press('Equals')
            assert str(STATE['pc'].get_editor_property('menu')) == 'Pause'
            STATE['checks']['equals_closes_settings'] = True
            phase('complete')
            return
        if STATE['phase'] == 'complete':
            u.log('IC_SETTINGS_WIDGET_REVIEW_OK ' + json.dumps(STATE['checks']))
            finish()
    except Exception:
        STATE['error'] = traceback.format_exc()
        phase('error')
        u.log_error(STATE['error'])
        finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
if '-ICWidgetUsageAudit' in u.SystemLibrary.get_command_line():
    runpy.run_path(str(Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / 'Tools/UI/audit_widget_usage.py'))
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_Title')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
