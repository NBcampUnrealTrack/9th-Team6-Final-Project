"""Real F interaction and keyboard quantity transfer using isolated PIE saves."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'StorageReview'
OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='starting',started=time.monotonic(),checks={},complete=False)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
def report():(OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):S.update(phase=name,phase_started=time.monotonic());report()
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def press(name):
    key=u.Key();key.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],key),name
def finish():
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def chest_count(identifier):return sum(v.count for v in S['inventory'].get_snapshot().storage[identifier].slots if str(v.item_id)=='ITM_CONS_001')
def tick(delta):
    try:
        if time.monotonic()-S['started']>150:raise RuntimeError('Storage review timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(pc=pc,world=worlds[0]);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(S['world'],0);chest=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICPlayerStorage) if a.get_editor_property('required_extra_storage')==0)
            for actor in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Character):
                controller=actor.get_controller()
                if controller and actor!=player:
                    brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Storage UI review')
                    controller.set_actor_tick_enabled(False)
            inventory=pc.get_inventory();assert inventory.add_item('ITM_CONS_001',20,False)
            point=chest.get_actor_location();player.set_actor_location_and_rotation(point+u.Vector(-170,0,96),u.Rotator(),False,True);player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();pc.set_control_rotation(u.Rotator())
            camera=player.get_component_by_class(u.CameraComponent);camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            position=point+u.Vector(-600,-400,350);camera.set_world_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,point+u.Vector(0,0,50)),False,True)
            pc.set_menu('');S.update(inventory=inventory,player=player,chest=chest,before=inventory.count_item('ITM_CONS_001'));phase('approach');return
        if S['phase']=='approach' and elapsed>1:
            pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('opened');return
        if S['phase']=='opened' and elapsed>.7:
            ui=pc.get_editor_property('storage_screen');assert ui and ui.is_open()
            assert str(pc.get_editor_property('menu'))=='' and u.GameplayStatics.is_game_paused(S['world'])
            assert len(S['inventory'].get_snapshot().storage['PlayerHome'].slots)==40
            S['checks']['f_opens_independent_forty_slot_screen']=True;S['ui']=ui;capture('Storage_TwoPanes')
            index=next(i for i,v in enumerate(S['inventory'].get_snapshot().bag.slots) if str(v.item_id)=='ITM_CONS_001' and v.count>0)
            assert ui.open_item(False,index)
            for _ in range(4):press('Right')
            assert ui.get_editor_property('quantity')==5;phase('deposit_popup');return
        if S['phase']=='deposit_popup' and elapsed>.7:
            capture('Storage_DepositQuantity');press('Enter');ui=S['ui'];assert not ui.get_editor_property('popup_open') and ui.is_open()
            assert S['inventory'].count_item('ITM_CONS_001')==S['before']-5 and chest_count('PlayerHome')==5
            assert ui.open_item(True,0);press('Right');assert ui.get_editor_property('quantity')==2;phase('withdraw_popup');return
        if S['phase']=='withdraw_popup' and elapsed>.7:
            capture('Storage_WithdrawQuantity');press('Enter');assert S['inventory'].count_item('ITM_CONS_001')==S['before']-3 and chest_count('PlayerHome')==3
            S['checks']['keyboard_quantity_deposit_and_withdraw']=True
            assert S['ui'].open_item(True,0);press('Escape');assert S['ui'].is_open() and not S['ui'].get_editor_property('popup_open')
            assert chest_count('PlayerHome')==3;press('Escape');assert pc.get_editor_property('storage_screen') is None
            assert not u.GameplayStatics.is_game_paused(S['world']) and str(pc.get_editor_property('menu'))==''
            S['checks']['back_keeps_items_and_escape_returns_to_world']=True;phase('reopen');return
        if S['phase']=='reopen' and elapsed>.8:
            pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('reopened');return
        if S['phase']=='reopened' and elapsed>.6:
            ui=pc.get_editor_property('storage_screen');assert ui and ui.is_open() and chest_count('PlayerHome')==3
            assert ui.open_item(True,0) and ui.get_editor_property('quantity')==1;press('Escape');press('Escape')
            S['checks']['reopening_preserves_contents_and_resets_quantity']=True;phase('complete');return
        if S['phase']=='complete':S['complete']=True;u.log('STORAGE_UI_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
