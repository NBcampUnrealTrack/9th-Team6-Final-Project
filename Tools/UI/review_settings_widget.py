"""Review the authored settings WBP in title/gameplay and restore user preferences.

Run a rendering editor with -RenderOffscreen -ExecCmds="py <this file>".
Uses the existing isolated PIE review save namespace. No display mode is changed.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/ExpandedSettingsLiveReview'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {'phase': 'title_start', 'started': time.monotonic(), 'checks': {}}


def report():
    (OUT / 'Review.json').write_text(json.dumps({key: STATE[key] for key in ('phase', 'checks', 'error') if key in STATE},
                                              ensure_ascii=False, indent=2), encoding='utf-8')


def phase(value):
    STATE.update(phase=value, phase_at=time.monotonic())
    report()


def press(name):
    key = u.Key()
    key.set_editor_property('key_name', name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'], key), name


def screen():
    return next(widget for widget in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'], u.ICSettingsWidget, False)
                if widget.get_parent())


def control(widget, cls, name):
    return next(value for value in u.ObjectIterator(cls)
                if value.get_name() == name and value.get_outer().get_outer() == widget)


def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (name + '.png')))


def restore():
    if 'preferences' not in STATE:
        return
    settings = u.GameUserSettings.get_game_user_settings()
    for key, value in STATE['preferences'].items():
        settings.set_editor_property(key, value)
    settings.set_texture_quality(STATE['texture_quality'])
    settings.set_frame_rate_limit(STATE['frame_limit'])
    settings.set_resolution_scale_value_ex(STATE['render_scale'])
    settings.save_settings()
    if STATE.get('pc') and STATE.get('world'):
        STATE['pc'].set_menu('Pause' if STATE.get('gameplay') else 'MainMenu')
        STATE['pc'].set_menu('Settings')
        screen().apply_options()
        for key in ('MonitorDevice', 'MonitorRefreshRate'):
            settings.set_editor_property(key, STATE['preferences'][key])
        settings.save_settings()


def finish():
    try:
        restore()
    finally:
        report()
        u.unregister_slate_post_tick_callback(STATE['handle'])
        LEVELS.editor_request_end_play()
        u.SystemLibrary.quit_editor()


def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 200:
            raise RuntimeError('Timeout: ' + STATE['phase'])
        worlds = u.EditorLevelLibrary.get_pie_worlds(False)
        elapsed = time.monotonic() - STATE.get('phase_at', STATE['started'])
        if STATE['phase'] == 'title_start':
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'MainMenu':
                return
            STATE.update(world=worlds[0], pc=pc)
            settings = u.GameUserSettings.get_game_user_settings()
            STATE['preferences'] = {key: settings.get_editor_property(key) for key in ('CameraSensitivity', 'CameraFOV', 'MasterVolume', 'AntiAliasingMethod', 'UIScale', 'MonitorDevice', 'MonitorRefreshRate')}
            STATE['preferences']['KeyOverrides'] = dict(settings.get_editor_property('KeyOverrides'))
            STATE['texture_quality'] = settings.get_texture_quality()
            STATE['frame_limit'] = settings.get_frame_rate_limit()
            STATE['render_scale'] = settings.get_resolution_scale_information_ex()[1]
            pc.set_menu('Settings')
            STATE['tab'] = 0
            phase('title_tabs')
            return
        if STATE['phase'] == 'title_tabs' and elapsed > .35:
            current = screen()
            assert current.get_class().get_path_name() == '/Game/IsekaiClass/UI/Widget/WBP_IC_Settings.WBP_IC_Settings_C'
            tabs = control(current, u.WidgetSwitcher, 'SettingsTabSwitcher')
            assert tabs.get_num_widgets() == 5
            index = STATE['tab']
            current.select_tab(index)
            capture('Title_' + ('Combat', 'Controls', 'Graphics', 'Audio', 'Other')[index])
            assert tabs.get_active_widget_index() == index
            if index < 4:
                STATE['tab'] += 1
                STATE['phase_at'] = time.monotonic()
                return
            STATE['checks']['all_five_authored_tabs_render_and_switch'] = True
            assert control(current, u.VerticalBox, 'LIST_KeyBindings').get_children_count() > 15
            STATE['checks']['keyboard_mouse_bindings_populated'] = True
            press('Escape')
            assert str(STATE['pc'].get_editor_property('menu')) == 'MainMenu'
            STATE['checks']['escape_returns_to_title'] = True
            LEVELS.editor_request_end_play()
            phase('title_end')
            return
        if STATE['phase'] == 'title_end' and not worlds:
            u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
            assert u.ICCharacterSetupLibrary.begin_monster_test_review()
            phase('game_start')
            return
        if STATE['phase'] == 'game_start':
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            STATE.update(world=worlds[0], pc=pc, gameplay=True)
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            phase('creating')
            return
        if STATE['phase'] == 'creating':
            if not STATE['pc'].get_editor_property('created'):
                return
            STATE['pc'].set_menu('Pause')
            STATE['pc'].set_menu('Settings')
            phase('game_edit')
            return
        if STATE['phase'] == 'game_edit' and elapsed > .5:
            current = screen()
            current.select_tab(1)
            sensitivity = control(current, u.Slider, 'OPTION_CameraSensitivity')
            sensitivity.set_keyboard_focus()
            before_sensitivity = sensitivity.get_value()
            press('Left' if sensitivity.get_value() > 2.5 else 'Right')
            STATE['sensitivity'] = sensitivity.get_value()
            assert abs(STATE['sensitivity'] - before_sensitivity) > .001
            fov = control(current, u.Slider, 'OPTION_CameraFOV')
            fov.set_keyboard_focus()
            press('Left' if fov.get_value() > 105 else 'Right')
            STATE['fov'] = fov.get_value()
            capture('Gameplay_Controls')
            current.select_tab(3)
            volume = control(current, u.Slider, 'OPTION_MasterVolume')
            volume.set_keyboard_focus()
            press('Right' if volume.get_value() < .1 else 'Left')
            STATE['volume'] = volume.get_value()
            current.select_tab(2)
            texture = control(current, u.ComboBoxString, 'OPTION_Texture')
            STATE['new_texture'] = 1 if texture.get_selected_index() != 1 else 2
            texture.set_selected_index(STATE['new_texture'])
            control(current, u.ComboBoxString, 'OPTION_AntiAliasingMethod').set_selected_index(2)
            control(current, u.ComboBoxString, 'OPTION_FrameLimit').set_selected_index(4)
            scale = control(current, u.Slider, 'OPTION_ResolutionScale')
            scale.set_keyboard_focus()
            press('Left' if scale.get_value() > 98 else 'Right')
            STATE['new_render_scale'] = scale.get_value()
            current.select_tab(4)
            text_scale = control(current, u.Slider, 'OPTION_UIScale')
            text_scale.set_keyboard_focus()
            press('Left' if text_scale.get_value() > 1.2 else 'Right')
            STATE['text_scale'] = text_scale.get_value()
            current.apply_options()
            assert not current.is_awaiting_display_confirmation()
            phase('applied')
            return
        if STATE['phase'] == 'applied' and elapsed > .5:
            current = screen()
            player = u.GameplayStatics.get_player_pawn(STATE['world'], 0)
            settings = u.GameUserSettings.get_game_user_settings()
            assert abs(player.get_editor_property('camera_sensitivity') - STATE['sensitivity']) < .001
            assert abs(player.get_component_by_class(u.CameraComponent).get_editor_property('field_of_view') - STATE['fov']) < .001
            assert abs(settings.get_editor_property('MasterVolume') - STATE['volume']) < .001
            STATE['checks']['apply_updates_actual_camera_and_audio_preferences'] = True
            assert u.SystemLibrary.get_console_variable_int_value('sg.TextureQuality') == STATE['new_texture']
            assert u.SystemLibrary.get_console_variable_int_value('r.AntiAliasingMethod') == 2
            assert abs(u.SystemLibrary.get_console_variable_float_value('t.MaxFPS') - 120.) < .1
            assert abs(u.SystemLibrary.get_console_variable_float_value('r.ScreenPercentage') - STATE['new_render_scale']) < .1
            STATE['checks']['graphics_quality_antialiasing_fps_and_render_scale_reach_renderer'] = True
            title = control(current, u.TextBlock, 'TXT_SettingsTitle')
            assert abs(title.get_editor_property('font').size - 36 * STATE['text_scale']) < .2
            STATE['checks']['text_size_changes_actual_widget_fonts'] = True
            current.select_tab(3)
            capture('Gameplay_Audio')
            current.select_tab(1)
            keys = control(current, u.VerticalBox, 'LIST_KeyBindings')
            selector = keys.get_child_at(0).get_child_at(1).get_content()
            control(current, u.ScrollBox, 'SettingsScroll_1').scroll_widget_into_view(selector, False)
            selector.set_keyboard_focus()
            press('Enter')
            assert selector.get_is_selecting_key()
            press('H')
            assert str(selector.get_editor_property('selected_key').key.get_editor_property('key_name')) == 'H'
            current.apply_options()
            press('Escape')
            assert str(STATE['pc'].get_editor_property('menu')) == 'Pause'
            STATE['checks']['close_returns_to_paused_game_menu'] = True
            STATE['pc'].set_menu('')
            phase('key_applied')
            return
        if STATE['phase'] == 'key_applied' and elapsed > .3:
            move = u.load_asset('/Game/IsekaiClass/Input/Actions/IA_Move')
            actual = [key for subsystem in u.ObjectIterator(u.EnhancedInputLocalPlayerSubsystem)
                      if isinstance(subsystem.get_outer(), u.LocalPlayer)
                      for key in subsystem.query_keys_mapped_to_action(move)]
            assert any(str(key.get_editor_property('key_name')) == 'H' for key in actual)
            STATE['checks']['key_selector_updates_actual_enhanced_input_mapping'] = True
            STATE['pc'].set_menu('Pause')
            STATE['pc'].set_menu('Settings')
            phase('reopened')
            return
        if STATE['phase'] == 'reopened' and elapsed > .4:
            current = screen()
            assert abs(control(current, u.Slider, 'OPTION_CameraSensitivity').get_value() - STATE['sensitivity']) < .001
            assert abs(control(current, u.Slider, 'OPTION_MasterVolume').get_value() - STATE['volume']) < .001
            STATE['checks']['reopening_restores_saved_preferences'] = True
            phase('complete')
            finish()
            u.log('IC_EXPANDED_SETTINGS_LIVE_REVIEW_COMPLETE')
    except Exception:
        STATE['error'] = traceback.format_exc()
        phase('error')
        u.log_error(STATE['error'])
        finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_Title')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
