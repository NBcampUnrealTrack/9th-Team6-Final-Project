"""Use real keyboard events to select and give bag items through an NPC gift WBP."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'NPCGiftPicker'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}

def report():
    result={k:STATE[k] for k in ('phase','checks')}
    if 'error' in STATE:result['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):STATE.update(phase=name,phase_started=time.monotonic());report()
def finish():
    u.unregister_slate_post_tick_callback(STATE['handle'])
    LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def press(name):
    key=u.Key();key.set_editor_property('key_name',name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'],key),name
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def picker():
    values=[w for w in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICNPCGiftWidget,False) if w.get_parent()]
    assert len(values)<=1
    return values[0] if values else None
def dialogue_entries():
    dialogue=next(w for w in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICNPCDialogueWidget,False) if w.get_parent())
    return [e for e in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICDialogueEntryWidget,False)
            if e.get_parent() and e.get_parent().get_outer().get_outer()==dialogue]
def selected():return next(str(e.get_entry_text()) for e in dialogue_entries() if e.get_editor_property('selected'))
def seek_item(id):
    current=picker();assert current
    if str(current.get_selected_item_id())==id:return True
    index=STATE.get('grid_index',0)
    if index%5==4:
        for _ in range(4):press('A')
        press('S')
    else:press('D')
    STATE['grid_index']=index+1
    return False

def tick(delta):
    try:
        if time.monotonic()-STATE['started']>150:raise RuntimeError('Timeout in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc'];world=STATE['world'];elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(world,0)
            npc=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.ICNPCBase) if str(a.get_editor_property('npc_definition_id'))=='NPC_R01_002')
            npc.get_controller().stop_movement();npc.get_controller().set_actor_tick_enabled(False)
            npc.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            player.set_actor_location_and_rotation(npc.get_actor_location()+u.Vector(-190,0,0),u.Rotator(),False,True)
            player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();pc.set_control_rotation(u.Rotator())
            inventory=pc.get_inventory()
            for id in ('ITEM_P0_01','ITM_R01_PLANT_01','ITM_R01_ORE_01'):assert inventory.add_item(id,2,False)
            for index,item in enumerate(inventory.get_snapshot().bag.slots):
                if str(item.item_id)=='ITM_R01_ORE_01':assert inventory.toggle_lock(index)
            pc.get_world_state().set_affinity('NPC_R01_002',280)
            STATE.update(npc=npc,inventory=inventory,plant_before=inventory.count_item('ITM_R01_PLANT_01'),preferred_before=inventory.count_item('ITEM_P0_01'))
            pc.set_menu('');phase('settle');return
        if STATE['phase']=='settle' and elapsed>1:
            pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('greeting');return
        if STATE['phase']=='greeting' and elapsed>.25:
            if '선물하기' not in [str(e.get_entry_text()) for e in dialogue_entries()]:
                press('Enter');STATE['phase_started']=time.monotonic();return
            if selected()!='선물하기':press('S');return
            press('Enter');STATE['grid_index']=0;phase('choose_neutral');return
        if STATE['phase']=='choose_neutral' and elapsed>.3:
            assert picker(),'Gift option did not open a separate picker'
            if not seek_item('ITM_R01_PLANT_01'):return
            rows=[e for e in u.WidgetLibrary.get_all_widgets_of_class(world,u.ICInventorySlotWidget,False)
                  if e.get_parent() and e.get_parent().get_outer().get_outer()==picker()]
            assert len(rows)>=20
            assert not any('광석1' in str(e.get_editor_property('tool_tip_text')) for e in rows)
            STATE['checks']['locked_items_hidden']=True
            STATE['checks']['separate_gift_wbp']=True
            press('S');press('W');assert str(picker().get_selected_item_id())=='ITM_R01_PLANT_01'
            if STATE['grid_index']%5==0:press('D');press('A')
            else:press('A');press('D')
            assert str(picker().get_selected_item_id())=='ITM_R01_PLANT_01'
            STATE['checks']['wasd_moves_inventory_grid']=True
            capture('GiftPicker_SelectedPlant')
            press('Enter');phase('confirm_neutral');return
        if STATE['phase']=='confirm_neutral' and elapsed>.3:
            assert picker().get_editor_property('confirming')
            assert STATE['inventory'].count_item('ITM_R01_PLANT_01')==STATE['plant_before']
            capture('GiftPicker_ConfirmPlant')
            press('D');press('Enter')
            assert picker() and not picker().get_editor_property('confirming')
            assert STATE['inventory'].count_item('ITM_R01_PLANT_01')==STATE['plant_before']
            STATE['checks']['confirmation_cancel_preserves_item']=True
            press('Enter');press('A');press('Enter');phase('neutral_given');return
        if STATE['phase']=='neutral_given' and elapsed>.3:
            assert picker() is None
            assert STATE['inventory'].count_item('ITM_R01_PLANT_01')==STATE['plant_before']-1
            assert STATE['inventory'].count_item('ITEM_P0_01')==STATE['preferred_before']
            assert pc.get_world_state().get_snapshot('NPC_R01_002').affinity==280
            assert selected()=='선물하기'
            STATE['checks']['enter_gives_exact_selected_item_once']=True
            capture('GiftPicker_NeutralResult');press('Enter');STATE['grid_index']=0;phase('choose_preferred');return
        if STATE['phase']=='choose_preferred' and elapsed>.3:
            if not seek_item('ITEM_P0_01'):return
            capture('GiftPicker_SelectedPreferred');press('Enter');phase('confirm_preferred');return
        if STATE['phase']=='confirm_preferred' and elapsed>.3:
            assert picker().get_editor_property('confirming')
            capture('GiftPicker_ConfirmPreferred');press('W');press('Enter')
            STATE['checks']['accept_confirmation_sends_selected_gift']=True
            phase('preferred_given');return
        if STATE['phase']=='preferred_given' and elapsed>.3:
            assert picker() is None
            assert STATE['inventory'].count_item('ITEM_P0_01')==STATE['preferred_before']-1
            assert pc.get_world_state().get_snapshot('NPC_R01_002').affinity==300
            assert any(str(e.get_entry_text())=='동료 영입하기' for e in dialogue_entries())
            STATE['checks']['preferred_affinity_and_recruit_threshold_update']=True
            capture('GiftPicker_AffinityResult');assert selected()=='선물하기',selected()
            press('Enter');phase('cancel');return
        if STATE['phase']=='cancel' and elapsed>.3:
            assert picker();press('Escape');assert picker() is None
            assert STATE['inventory'].count_item('ITEM_P0_01')==STATE['preferred_before']-1
            assert str(pc.get_editor_property('menu'))=='Dialogue' and selected()=='선물하기'
            STATE['checks']['escape_returns_to_npc_without_consuming_item']=True
            phase('after_cancel');return
        if STATE['phase']=='after_cancel' and elapsed>.3:
            press('S');assert selected()=='대화 끝내기';press('Enter');phase('closing');return
        if STATE['phase']=='closing' and elapsed>.5:
            assert not str(pc.get_editor_property('menu')) and not u.GameplayStatics.is_game_paused(world)
            assert not STATE['npc'].is_interacting()
            STATE['checks']['conversation_closes_and_releases_pause']=True
            phase('complete');return
        if STATE['phase']=='complete':
            report();u.log('IC_NPC_GIFT_PICKER_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
