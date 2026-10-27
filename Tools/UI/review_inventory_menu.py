"""Exercise the actual inventory WBPs with Slate key events in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'InventoryMenu'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}
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
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def select_item(id):
    ui=STATE['ui'];ui.select_item_tab(u.PlayerItemTab.NORMAL)
    slots=STATE['inv'].get_snapshot().bag.slots
    index=next(i for i,s in enumerate(slots) if str(s.item_id)==id and s.count>0)
    assert ui.select_item(index)
def keys(*names):
    for name in names:press(name)
def tick(delta):
    try:
        if time.monotonic()-STATE['started']>110:raise RuntimeError('Inventory review timeout in '+STATE['phase'])
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
            assert menu.select_tab('Inventory')
            ui=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICInventoryMenuWidget,False)[0]
            inv=u.GameplayStatics.get_player_pawn(STATE['world'],0).get_editor_property('inventory')
            STATE.update(menu=menu,ui=ui,inv=inv)
            for id,count in (('ITM_CONS_001',7),('ITM_FOOD_001',3),('ITM_QST_001',2)):assert inv.add_item(id,count,False)
            STATE['food_count']=inv.count_item('ITM_FOOD_001')
            snapshot=inv.get_snapshot();snapshot.set_editor_property('quick_slots',[u.Name('None')]*4)
            assert pc.get_world_state().commit_inventory_state(snapshot)
            assert len(ui.get_editor_property('members'))==1
            STATE['checks']['solo_player_only']=True
            for slot,id in enumerate(('NPC_R01_002','NPC_R01_008','NPC_R01_011')):
                state=pc.get_world_state();assert state.ensure_character(id,u.ICCharacterKind.RECRUITABLE)
                assert state.set_affinity(id,300);assert state.set_party_slot(id,slot)
            ui.refresh_inventory();phase('player');return
        ui=STATE['ui'];inv=STATE['inv'];menu=STATE['menu']
        if STATE['phase']=='player' and elapsed>.4:
            assert len(ui.get_editor_property('members'))==4
            capture('InventoryPlayer');press('S')
            assert str(ui.get_editor_property('selected_character'))=='NPC_R01_002'
            STATE['checks']['keyboard_companion_selection']=True;phase('companion');return
        if STATE['phase']=='companion' and elapsed>.3:
            capture('InventoryCompanion')
            gear=pc.get_world_state().get_snapshot('NPC_R01_002').equipment
            assert gear and str(gear[0].item_id)=='EQP_WPN_SWORD_001'
            assert str(ui.get_editor_property('Equipment0').get_editor_property('TXT_Name').get_text())
            STATE['checks']['companion_uses_own_equipment']=True
            ui.select_member(0);menu.select_tab('Inventory')
            keys('D','D','S');select_item('ITM_CONS_001');press('Enter');press('Enter')
            assert ui.get_editor_property('picking_quick_slot')
            keys('D','D','Enter')
            assert ui.get_editor_property('confirming');assert str(inv.get_snapshot().quick_slots[2])=='None'
            STATE['checks']['chooses_slot_before_confirmation']=True;phase('register_confirmation');return
        if STATE['phase']=='register_confirmation' and elapsed>.3:
            capture('InventoryRegisterConfirm');keys('A','Enter')
            assert str(inv.get_snapshot().quick_slots[2])=='ITM_CONS_001'
            STATE['checks']['keyboard_registration']=True
            keys('W');select_item('ITM_FOOD_001');keys('Enter','Enter','D','D','Enter')
            assert ui.get_editor_property('confirming');phase('replace_confirmation');return
        if STATE['phase']=='replace_confirmation' and elapsed>.3:
            capture('InventoryReplaceConfirm');press('Escape')
            assert not ui.get_editor_property('confirming');assert str(pc.get_editor_property('menu'))=='Pause'
            assert str(inv.get_snapshot().quick_slots[2])=='ITM_CONS_001'
            STATE['checks']['escape_cancels_preserving_old_item']=True
            keys('Enter','D','D','Enter','A','Enter')
            assert str(inv.get_snapshot().quick_slots[2])=='ITM_FOOD_001'
            STATE['checks']['confirmed_replacement']=True
            keys('D','Enter');assert ui.get_editor_property('confirming');phase('trash_confirmation');return
        if STATE['phase']=='trash_confirmation' and elapsed>.3:
            capture('InventoryTrashConfirm');press('Enter')
            assert inv.count_item('ITM_FOOD_001')==STATE['food_count']
            STATE['checks']['default_cancel_preserves_stack']=True
            keys('Enter','A','Enter')
            assert inv.count_item('ITM_FOOD_001')==0;assert str(inv.get_snapshot().quick_slots[2])=='None'
            STATE['checks']['confirmed_trash_removes_stack_and_binding']=True
            keys('W');ui.select_item_tab(u.PlayerItemTab.QUEST);phase('quest');return
        if STATE['phase']=='quest' and elapsed>.3:
            capture('InventoryQuest');assert not ui.begin_quick_slot();assert not ui.begin_trash()
            STATE['checks']['quest_items_protected']=True
            pc.get_world_state().set_party_slot('NPC_R01_008',-1);assert len(ui.get_editor_property('members'))==3
            STATE['checks']['dismissal_updates_inventory_roster']=True
            phase('complete');return
        if STATE['phase']=='complete' and elapsed>.2:
            status();u.log('IC_INVENTORY_MENU_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        if 'world' in STATE:
            try:capture('InventoryReviewError')
            except Exception:pass
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
