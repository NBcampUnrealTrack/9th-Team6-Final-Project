"""Exercise death -> load -> back -> successful load -> death -> title in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/DeathScreen'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}

def status():
    data={key:STATE[key] for key in ('phase','checks')}
    if 'error' in STATE:data['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')

def phase(name):
    STATE['phase']=name;STATE['phase_started']=time.monotonic();status()

def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()

def die():
    pc=STATE['pc'];p=pc.get_player_snapshot()
    assert pc.get_world_state().set_vitals('CHR_PLAYER',0,p.mp,p.sp)

def tick(delta):
    try:
        if time.monotonic()-STATE['started']>100:raise RuntimeError('Death review timed out in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='title':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            controller=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not controller or not controller.is_main_menu_level():return
            assert str(controller.get_editor_property('menu'))=='MainMenu'
            STATE['checks']['main_menu_button']='MainMenu'
            STATE.update(world=worlds[0],pc=controller);phase('complete');return
        pc=STATE['pc'];menu=str(pc.get_editor_property('menu'))
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('');die();phase('dying');return
        if STATE['phase']=='dying':
            if not pc.is_dead() or menu!='Death':return
            assert pc.get_editor_property('death_screen')
            assert not pc.get_time().is_running()
            STATE['checks']['death_menu']=menu
            phase('death_capture');return
        if STATE['phase']=='death_capture' and elapsed>.4:
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/'Death.png'))
            pc.get_editor_property('death_screen').load_game();phase('load');return
        if STATE['phase']=='load' and elapsed>.2:
            assert menu=='Load'
            pc.close_session_menu();assert str(pc.get_editor_property('menu'))=='Death'
            STATE['checks']['load_back']='Death'
            pc.handle_input_command(u.ICInputCommand.MENU,0)
            assert str(pc.get_editor_property('menu'))=='Death'
            STATE['checks']['escape_stays_dead']=True
            pc.get_editor_property('death_screen').load_game()
            pc.select_session_slot(0);pc.request_load_player()
            assert str(pc.get_editor_property('menu'))=='Confirm'
            pc.confirm_modal(True);phase('restoring');return
        if STATE['phase']=='restoring':
            if pc.is_dead() or menu!='':return
            assert pc.get_player_snapshot().hp>0
            assert not pc.get_editor_property('death_screen')
            STATE['checks']['loaded_alive_hp']=pc.get_player_snapshot().hp
            die();phase('second_death');return
        if STATE['phase']=='second_death' and menu=='Death' and elapsed>.2:
            pc.get_editor_property('death_screen').main_menu();phase('title');return
        if STATE['phase']=='complete' and elapsed>.4:
            u.log('IC_DEATH_REVIEW_OK '+json.dumps(STATE['checks']));status();finish()
    except Exception:
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
