"""Exercise weapon-only menus, item-specific popups and the ten sample quests in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u
OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'WeaponOnlyMenu'
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
def keys(*names):
    for name in names:press(name)
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def bag_index(id):return next(i for i,item in enumerate(STATE['inv'].get_snapshot().bag.slots) if str(item.item_id)==id and item.count>0)
def open_item(id):assert STATE['inventory'].open_item(bag_index(id))
def tick(delta):
    try:
        if time.monotonic()-STATE['started']>150:raise RuntimeError('Review timeout in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc'];elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('Pause');menu=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICGameMenuWidget,False)[0]
            assert menu.select_tab('Party');inventory=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICInventoryMenuWidget,False)[0]
            inv=u.GameplayStatics.get_player_pawn(STATE['world'],0).get_editor_property('inventory');gi=pc.get_world_state().get_outer()
            quests=next(q for q in u.ObjectIterator(u.ICQuestSubsystem) if q.get_outer()==gi)
            STATE.update(menu=menu,inventory=inventory,inv=inv,quests=quests);assert len(inv.get_snapshot().equipment)==1;phase('party');return
        menu=STATE['menu'];inventory=STATE['inventory'];inv=STATE['inv']
        if STATE['phase']=='party' and elapsed>.4:
            capture('PartyStatGroups');STATE['checks']['three_stat_groups']=True
            assert menu.select_tab('Inventory')
            for id,count in (('ITM_CONS_001',3),('ITM_MAT_002',4),('EQP_WPN_SWORD_001',1)):assert inv.add_item(id,count,False)
            inventory.refresh_inventory();phase('inventory');return
        if STATE['phase']=='inventory' and elapsed>.4:
            capture('WeaponAndQuickSlots');open_item('EQP_WPN_SWORD_001');phase('weapon_popup');return
        if STATE['phase']=='weapon_popup' and elapsed>.3:
            capture('WeaponItemPopup');assert inventory.get_editor_property('detail_open');assert not menu.select_tab('Party')
            press('Enter');assert not inventory.get_editor_property('detail_open');STATE['checks']['keyboard_equips_from_popup']=True
            open_item('ITM_CONS_001');phase('consumable_popup');return
        if STATE['phase']=='consumable_popup' and elapsed>.3:
            capture('ConsumableItemPopup');keys('Enter','D','D','Enter');assert inventory.get_editor_property('confirming');phase('quick_confirm');return
        if STATE['phase']=='quick_confirm' and elapsed>.3:
            capture('QuickSlotConfirmation');press('Escape');assert inventory.get_editor_property('detail_open');assert not inventory.get_editor_property('confirming')
            STATE['checks']['cancel_returns_to_item_popup']=True
            keys('Enter','D','D','Enter','A','Enter');assert str(inv.get_snapshot().quick_slots[2])=='ITM_CONS_001';assert not inventory.get_editor_property('detail_open')
            STATE['checks']['keyboard_assigns_quick_slot']=True;open_item('ITM_MAT_002');STATE['material_count']=inv.count_item('ITM_MAT_002');phase('material_popup');return
        if STATE['phase']=='material_popup' and elapsed>.3:
            capture('MaterialItemPopup');press('Enter');assert inventory.get_editor_property('confirming');press('Escape');assert inv.count_item('ITM_MAT_002')==STATE['material_count']
            keys('Enter','A','Enter');assert inv.count_item('ITM_MAT_002')==0;STATE['checks']['trash_requires_confirmation']=True
            assert menu.select_tab('Codex');codex=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICCodexMenuWidget,False)[0];codex.select_category(0);STATE['codex']=codex;phase('skills');return
        if STATE['phase']=='skills' and elapsed>.3:
            capture('UnknownSkills');codex=STATE['codex'];known={str(id) for id in pc.get_world_state().get_snapshot('CHR_PLAYER').get_editor_property('UnlockedSkillIDs')}
            unknown=next(i for i,id in enumerate(codex.get_editor_property('cells')) if str(id) not in known);assert not codex.open_cell(unknown)
            STATE['checks']['unacquired_skills_are_locked']=True;assert codex.open_cell(1000);phase('skill_remove');return
        if STATE['phase']=='skill_remove' and elapsed>.3:
            capture('EquippedSkillRemoval');keys('Enter','Enter','Escape')
            equipped=pc.get_world_state().get_snapshot('CHR_PLAYER').equipped_weapon_skills;assert str(equipped.get(u.ICWeaponFamily.SWORD,u.Name('None')))=='None'
            STATE['checks']['keyboard_removes_equipped_skill']=True;assert menu.select_tab('Quest')
            journal=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICQuestMenuWidget,False)[0];STATE['journal']=journal
            assert len(journal.get_editor_property('QuestIDs'))==10;phase('quests');return
        if STATE['phase']=='quests' and elapsed>.3:
            capture('TenNPCQuests');assert STATE['journal'].open_quest('QST_NPC_TALK_001');STATE['checks']['ten_quests_visible_before_acceptance']=True;phase('quest_detail');return
        if STATE['phase']=='quest_detail' and elapsed>.3:
            capture('TalkQuest100Gold');STATE['gold_before']=pc.get_world_state().export_state().gold;press('Enter')
            assert pc.get_world_state().export_state().quests[u.Name('QST_NPC_TALK_001')].started
            assert STATE['quests'].report_objective_event(u.ICQuestObjectiveType.TALK,'NPC_R01_002',1,u.GuidLibrary.new_guid())
            assert pc.get_world_state().export_state().gold==STATE['gold_before']+100
            STATE['checks']['talk_quest_grants_100_gold']=True;phase('complete');return
        if STATE['phase']=='complete' and elapsed>.2:
            status();u.log('IC_WEAPON_ONLY_MENU_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        if 'world' in STATE:
            try:capture('ReviewError')
            except Exception:pass
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
