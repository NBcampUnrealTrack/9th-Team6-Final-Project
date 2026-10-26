"""Review the quest list, detail controls, active HUD and restart behavior with real keys."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'QuestMenu'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}
IDS=('QST_RING_001','QST_TEST_TALK_001','QST_TEST_DELIVER_001','QST_TEST_COLLECT_001','QST_TEST_AFFINITY_001','QST_TEST_STAT_001','QST_TEST_VISIT_001','QST_TEST_POSSESS_001')
def status():
    data={k:STATE[k] for k in ('phase','checks')}
    if 'error' in STATE:data['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):STATE.update(phase=name,phase_started=time.monotonic());status()
def finish():
    u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def press(name):
    key=u.Key();key.set_editor_property('key_name',name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'],key),name
def keys(*names):
    for name in names:press(name)
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))

def running():
    return sorted(str(id) for id,q in STATE['pc'].get_world_state().export_state().quests.items() if q.accepted and q.started and not q.completed)

def list_titles():
    entries=[entry for entry in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICQuestMenuEntryWidget,False)
             if entry.get_parent() and entry.get_outer().get_outer()==STATE['ui']]
    # Entries are owned by the journal's WidgetTree; read the live text widgets rather than cached labels.
    return [str(text.get_text()) for text in u.ObjectIterator(u.TextBlock)
            if text.get_name()=='TXT_Title' and text.get_outer().get_outer() in entries]
def tick(delta):
    try:
        if time.monotonic()-STATE['started']>120:raise RuntimeError('Quest review timeout in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc'];elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('Pause')
            menu=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICGameMenuWidget,False)[0]
            assert menu.select_tab('Quest')
            ui=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICQuestMenuWidget,False)[0]
            game_instance=pc.get_world_state().get_outer()
            quests=next(q for q in u.ObjectIterator(u.ICQuestSubsystem) if q.get_outer()==game_instance)
            STATE.update(menu=menu,ui=ui,quests=quests)
            assert not ui.get_editor_property('showing_details')
            phase('empty');return
        ui=STATE['ui'];quests=STATE['quests'];menu=STATE['menu']
        if STATE['phase']=='empty' and elapsed>.3:
            capture('QuestEmpty')
            assert len(ui.get_editor_property('QuestIDs'))==0
            STATE['checks']['empty_journal']=True
            observed=sorted({widget.get_class().get_path_name() for widget in
                u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.UserWidget,False)
                if widget.get_parent() or widget.is_in_viewport()})
            (OUT/'RuntimeUI.json').write_text(json.dumps({'live_widget_classes':observed,
                'native_user_widget_classes':[path for path in observed if path.startswith('/Script/')]},indent=2),encoding='utf-8')
            for index,id in enumerate(IDS):
                assert quests.accept_quest(id)
                assert running()==[IDS[0]]
                if index:assert not pc.get_world_state().export_state().quests[u.Name(id)].started
            STATE['checks']['first_acceptance_automatically_tracks_quest']=True
            STATE['checks']['later_acceptances_preserve_current_hud_quest']=True
            ui.refresh_quests();phase('list');return
        if STATE['phase']=='list' and elapsed>.3:
            titles=list_titles()
            assert len(titles)==8,titles
            assert sum('(진행중)' in title for title in titles)==1,titles
            STATE['checks']['only_hud_quest_is_marked_running_in_list']=True
            capture('QuestList');press('S')
            assert ui.get_editor_property('selected_index')==1
            STATE['checks']['keyboard_selects_highlighted_row']=True
            keys('S','S','S','S','S','S')
            STATE['checks']['keyboard_scrolls_to_last_quest']=ui.get_editor_property('selected_index')==7
            phase('scrolled');return
        if STATE['phase']=='scrolled' and elapsed>.4:
            capture('QuestListScrolled')
            assert ui.open_quest('QST_TEST_DELIVER_001');phase('details');return
        if STATE['phase']=='details' and elapsed>.3:
            capture('QuestDetails');press('Enter')
            progress=pc.get_world_state().export_state().quests
            assert progress[u.Name('QST_TEST_DELIVER_001')].started
            assert quests.deliver_item('QST_TEST_DELIVER_001','OBJ_DELIVER')
            STATE['checks']['keyboard_starts_real_quest']=True
            keys('S','Enter')
            assert not ui.get_editor_property('showing_details')
            STATE['checks']['bottom_back_returns_to_list']=True
            assert ui.open_quest('QST_TEST_TALK_001');press('Enter')
            progress=pc.get_world_state().export_state().quests
            assert not progress[u.Name('QST_TEST_DELIVER_001')].started
            assert len(progress[u.Name('QST_TEST_DELIVER_001')].objectives)==0
            assert progress[u.Name('QST_TEST_TALK_001')].started
            STATE['checks']['switch_resets_previous_quest']=True
            phase('started');return
        if STATE['phase']=='started' and elapsed>.3:
            capture('QuestStarted');press('Escape')
            assert not ui.get_editor_property('showing_details');assert str(pc.get_editor_property('menu'))=='Pause'
            STATE['checks']['escape_returns_to_list']=True
            press('Escape');assert str(pc.get_editor_property('menu'))==''
            phase('hud');return
        if STATE['phase']=='hud' and elapsed>.3:
            capture('QuestHUD')
            tracker=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICQuestTrackerWidget,False)[0]
            assert tracker.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE
            active=[str(id) for id,q in pc.get_world_state().export_state().quests.items() if q.started]
            assert active==['QST_TEST_TALK_001']
            STATE['checks']['hud_only_shows_started_quest']=True
            pc.set_menu('Pause');menu.select_tab('Quest');ui.open_quest('QST_TEST_TALK_001')
            phase('abandon_ready');return
        if STATE['phase']=='abandon_ready' and elapsed>.4:
            keys('A','Enter');assert not ui.get_editor_property('showing_details')
            tracker=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICQuestTrackerWidget,False)[0]
            assert tracker.get_visibility()==u.SlateVisibility.COLLAPSED
            assert len(ui.get_editor_property('QuestIDs'))==7
            progress=pc.get_world_state().export_state().quests
            assert not progress[u.Name('QST_TEST_TALK_001')].accepted
            STATE['checks']['keyboard_abandon_removes_quest_and_hud']=True
            phase('abandoned');return
        if STATE['phase']=='abandoned' and elapsed>.3:
            capture('QuestAfterAbandon')
            assert not running()
            assert quests.accept_quest('QST_NPC_TALK_001')
            assert running()==['QST_NPC_TALK_001']
            ui.refresh_quests()
            STATE['checks']['empty_hud_starts_new_acceptance_while_other_quests_wait']=True
            phase('new_acceptance');return
        if STATE['phase']=='new_acceptance' and elapsed>.3:
            titles=list_titles();assert sum('(진행중)' in title for title in titles)==1,titles
            capture('QuestNewAcceptance');phase('complete');return
        if STATE['phase']=='complete' and elapsed>.2:
            status();u.log('IC_QUEST_MENU_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        if 'world' in STATE:
            try:capture('QuestReviewError')
            except Exception:pass
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
