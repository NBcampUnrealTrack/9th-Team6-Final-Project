"""Review actual party WBP data, expansion and complete keyboard navigation in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'PartyMenu'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}
IDS=('NPC_R01_002','NPC_R01_008','NPC_R01_011')
def status():
    result={k:STATE[k] for k in ('phase','checks')}
    if 'error' in STATE:result['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):STATE.update(phase=name,phase_started=time.monotonic());status()
def finish():
    u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def press(name):
    key=u.Key();key.set_editor_property('key_name',name)
    return u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'],key)
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def tick(delta):
    try:
        if time.monotonic()-STATE['started']>110:raise RuntimeError('Party review timeout in '+STATE['phase'])
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
            widgets=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICPartyMenuWidget,False)
            assert len(widgets)==1,len(widgets)
            menus=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICGameMenuWidget,False)
            STATE.update(party=widgets[0],menu=menus[0]);phase('solo');return
        party=STATE['party'];menu=STATE['menu']
        if STATE['phase']=='solo' and elapsed>.4:
            assert len(party.get_editor_property('members'))==1
            assert str(party.get_editor_property('selected_character'))=='CHR_PLAYER'
            assert str(party.get_editor_property('TXT_MainSetting').get_text())=='게임 속에 떨어진 수험생'
            capture('PartyPlayer')
            STATE['checks']['solo_player_only']=True
            state=pc.get_world_state()
            for slot,id in enumerate(IDS):
                assert state.ensure_character(id,u.ICCharacterKind.RECRUITABLE)
                assert state.set_affinity(id,300)
                assert state.set_party_slot(id,slot)
            party.refresh_party();phase('recruited');return
        if STATE['phase']=='recruited' and elapsed>.3:
            assert len(party.get_editor_property('members'))==4
            assert press('S')
            assert str(party.get_editor_property('selected_character'))==IDS[0]
            STATE['checks']['real_S_selects_companion']=True
            phase('first_companion');return
        if STATE['phase']=='first_companion' and elapsed>.3:
            capture('PartyCompanions')
            assert press('S');assert press('S')
            assert str(party.get_editor_property('selected_character'))==IDS[2]
            assert press('W');assert str(party.get_editor_property('selected_character'))==IDS[1]
            STATE['checks']['real_WASD_traverses_roster']=True
            assert press('D');assert menu.get_editor_property('navigation_area')==2
            assert press('S');assert str(party.get_editor_property('selected_character'))==IDS[1]
            assert press('A');assert press('A');assert press('A')
            assert str(menu.get_selected_tab())=='Settings'
            assert press('S');assert press('S');assert press('Enter')
            assert str(pc.get_editor_property('menu'))=='Save'
            STATE['checks']['keyboard_opens_save']=True
            phase('save');return
        if STATE['phase']=='save' and elapsed>.3:
            widgets=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICSaveLoadWidget,False)
            assert len(widgets)==1
            STATE['close']=widgets[0].get_editor_property('IC_SaveLoad_Close')
            STATE['save_keys']=iter(['S']*6+['D']*8+['W']*5+['D']*5)
            phase('save_navigation');return
        if STATE['phase']=='save_navigation' and elapsed>.08:
            if STATE['close'].has_user_focus(pc):
                assert press('Enter')
                assert str(pc.get_editor_property('menu'))=='Pause'
                STATE['checks']['keyboard_save_back_button']=True
                menu.select_tab('Party');pc.get_world_state().set_party_slot(IDS[1],-1);party.refresh_party()
                assert len(party.get_editor_property('members'))==3
                STATE['checks']['dismissal_updates_roster']=True
                phase('complete');return
            name=next(STATE['save_keys'],None)
            assert name,'Could not reach save Back button with WASD'
            press(name);STATE['phase_started']=time.monotonic();return
        if STATE['phase']=='complete' and elapsed>.2:
            capture('PartyAfterDismissal');u.log('IC_PARTY_MENU_REVIEW_OK '+json.dumps(STATE['checks']));status();finish()
    except Exception:
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
