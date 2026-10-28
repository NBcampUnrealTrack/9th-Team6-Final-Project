"""Rendered PIE review of the requested party, inventory, revival shop and forge changes."""
import json,time,traceback
from pathlib import Path
import unreal as u
OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'PartyForge/Live';OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='starting',started=time.monotonic(),checks={},complete=False);LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
def report():(OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(value):S.update(phase=value,phase_started=time.monotonic());report()
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def widgets(cls):return u.WidgetLibrary.get_all_widgets_of_class(S['world'],cls,False)
def child(root,name):
    try:return root.get_editor_property(name)
    except Exception:pass
    for w in u.ObjectIterator(u.Widget):
        if w.get_name()!=name:continue
        owner=w.get_outer()
        while owner:
            if owner==root:return w
            owner=owner.get_outer()
    raise AssertionError(name)
def press(name):
    key=u.Key();key.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],key),name
def search_inventory(ui,pattern):
    child(ui,'InventorySearch').set_text(pattern)
    # SetText updates the property without simulating a user edit; invoke its bound callback too.
    ui.call_method('SearchChanged',args=(pattern,))
def stop(actor):
    controller=actor.get_controller()
    if controller:
        brain=controller.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Party and forge review')
        controller.set_actor_tick_enabled(False)
    move=actor.get_component_by_class(u.CharacterMovementComponent)
    if move:move.stop_movement_immediately()
def approach(actor):
    point=actor.get_actor_location();position=point+u.Vector(-160,0,96 if abs(point.z)<60 else 0);S['player'].set_actor_location_and_rotation(position,u.Rotator(),False,True);S['player'].get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();S['pc'].set_control_rotation(u.Rotator());cam=S['camera'];p=point+u.Vector(-550,-380,320);cam.set_world_location_and_rotation(p,u.MathLibrary.find_look_at_rotation(p,point+u.Vector(0,0,60)),False,True)
def finish():report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        if time.monotonic()-S['started']>240:raise RuntimeError('Review timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            if not u.WidgetLibrary.get_all_widgets_of_class(worlds[0],u.ICGameplayWidget,False):return
            if 'ready_at' not in S:S['ready_at']=time.monotonic();return
            if time.monotonic()-S['ready_at']<1:return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(S['world'],0)
            for actor in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Character):
                if actor!=player:stop(actor)
            u.SystemLibrary.execute_console_command(S['world'],'ICRecruitTestParty',pc);u.SystemLibrary.execute_console_command(S['world'],'ICEquipStarterWeapons',pc)
            camera=player.get_component_by_class(u.CameraComponent);camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            S.update(player=player,camera=camera,inventory=pc.get_inventory(),state=pc.get_world_state());assert pc.set_gold(20000)
            for n in range(1,7):assert S['inventory'].add_item(f'ITM_R01_ORE_{n:02d}',5,False)
            assert S['inventory'].add_item('ITM_CONS_001',10,False);S['potion_count']=S['inventory'].count_item('ITM_CONS_001')
            player.set_actor_location(u.Vector(-1000,-2500,110),False,True);player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();pc.set_menu('');phase('party_ready');return
        if S['phase']=='party_ready':
            npcs=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase);target=next((a for a in npcs if str(a.get_character_id())=='NPC_R01_002'),None)
            if not target or not any(str(a.get_character_id())=='NPC_R01_006' for a in npcs):return
            for actor in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Character):
                if actor!=S['player']:stop(actor)
            target.set_actor_location(u.Vector(-1000,-2200,110),False,True)
            pc.set_menu('');dead=S['state'].get_snapshot('NPC_R01_002');dead.set_editor_property('hp',0);dead.set_editor_property('life_state',u.ICCharacterLifeState.DOWNED);assert S['state'].set_character_state(dead);phase('gray_hud');return
        if S['phase']=='gray_hud' and elapsed>1:
            dead_ui=next(w for w in widgets(u.ICCharacterStatusWidget) if str(w.get_editor_property('character_id'))=='NPC_R01_002' and w.is_visible());assert dead_ui.get_editor_property('downed');assert child(dead_ui,'BAR_MP').get_editor_property('percent')==0;assert not child(dead_ui,'StatusBuffBadge').is_visible();assert any(w.get_editor_property('has_buff') for w in widgets(u.ICCharacterStatusWidget) if w.is_visible());S['checks']['green_plus_badge_only_on_living_buffed_members']=True
            dead_actor=next((a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase) if str(a.get_character_id())=='NPC_R01_002'),None)
            if dead_actor:assert not dead_actor.can_dismiss()
            assert S['state'].get_snapshot('NPC_R01_002').party_slot==1;capture('DownedHUD');S['checks']['downed_hud_and_dismiss_guard']=True
            npc=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase) if str(a.get_character_id())=='NPC_R01_006');S['merchant']=npc;approach(npc);phase('shop_npc');return
        if S['phase']=='shop_npc' and elapsed>.8:pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('shop_dialogue');return
        if S['phase']=='shop_dialogue' and elapsed>.8:
            choices=[w for w in widgets(u.ICDialogueEntryWidget) if '상점' in str(w.get_entry_text())]
            if not choices:press('Enter');phase('shop_options');return
            choices[0].focus_entry();press('Enter');phase('shop_open');return
        if S['phase']=='shop_options' and elapsed>.4:
            trade=next(w for w in widgets(u.ICDialogueEntryWidget) if '상점' in str(w.get_entry_text()));trade.focus_entry();press('Enter');phase('shop_open');return
        if S['phase']=='shop_open' and elapsed>.6:
            shop=widgets(u.ICShopWidget)[0];shop.item_picked(5);S.update(shop=shop,gold_before=pc.get_editor_property('gold'));capture('RevivalShop');phase('shop_buy');return
        if S['phase']=='shop_buy' and elapsed>.5:
            assert S['shop'].submit_cart();assert pc.get_editor_property('gold')==S['gold_before']-5000;assert S['inventory'].count_item('ITM_CONS_REVIVE_SCROLL')==1;S['shop'].close_shop();S['checks']['revival_scroll_shop_5000G']=True
            pc.set_menu('Pause');menu=widgets(u.ICGameMenuWidget)[0];assert menu.select_tab('Inventory');phase('revive_inventory_ready');return
        if S['phase']=='revive_inventory_ready' and elapsed>.4:
            ui=widgets(u.ICInventoryMenuWidget)[0];search_inventory(ui,'ITM_CONS_REVIVE_SCROLL');phase('revive_search_ready');return
        if S['phase']=='revive_search_ready' and elapsed>.3:
            ui=widgets(u.ICInventoryMenuWidget)[0];assert '부활서' in str(child(ui,'TXT_SelectedItem').get_text());assert ui.open_item(0);assert ui.begin_use_selected();targets=child(ui,'UseTargets');members=[str(n) for n in targets.get_editor_property('members')];index=members.index('NPC_R01_002');assert ui.choose_use_target(index),str(child(targets,'TargetsTitle').get_text());phase('revived');return
        if S['phase']=='revived' and elapsed>.6:
            alive=S['state'].get_snapshot('NPC_R01_002');assert alive.hp>0 and alive.life_state==u.ICCharacterLifeState.ALIVE;assert S['inventory'].count_item('ITM_CONS_REVIVE_SCROLL')==0;S['checks']['inventory_revives_selected_companion']=True
            widgets(u.ICInventoryMenuWidget)[0].close_item();menu=widgets(u.ICGameMenuWidget)[0];assert menu.select_tab('Party');S['menu']=menu;phase('party_status');return
        if S['phase']=='party_status' and elapsed>.6:
            party=widgets(u.ICPartyMenuWidget)[0];status=str(child(party,'TXT_CurrentStatuses').get_text());assert '용사파티 트리플' in status and '30' in status;capture('PartyCurrentStatus');S['checks']['party_status_shows_hero_triple_30percent']=status
            assert S['menu'].select_tab('Inventory');ui=widgets(u.ICInventoryMenuWidget)[0];S['ui']=ui;search_inventory(ui,'ITM_CONS_001');assert ui.open_item(0);ui.lock_selected();assert any(str(v.item_id)=='ITM_CONS_001' and v.locked for v in S['inventory'].get_snapshot().bag.slots);ui.lock_selected();child(child(ui,'ItemPopup'),'SplitQuantity').set_value(3);capture('InventoryOrganization');phase('inventory_split');return
        if S['phase']=='inventory_split' and elapsed>.5:
            S['ui'].split_selected();assert S['inventory'].count_item('ITM_CONS_001')==S['potion_count'];assert sum(str(v.item_id)=='ITM_CONS_001' for v in S['inventory'].get_snapshot().bag.slots)==2;S['ui'].sort_items();S['checks']['inventory_search_lock_split_sort']=True;pc.set_menu('')
            station=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCraftingStation) if a.get_editor_property('recipe_type')==u.PlayerRecipeType.CRAFT);S['station']=station;approach(station);phase('forge_approach');return
        if S['phase']=='forge_approach' and elapsed>.8:
            assert S['station'].try_interact(pc);ui=next(w for w in widgets(u.ICCraftingStationWidget) if w.is_open());ui.enhance_tab();S['forge']=ui;phase('forge_list');return
        if S['phase']=='forge_list' and elapsed>.4:
            enhancement=child(S['forge'],'EnhancementContent');targets=S['inventory'].get_enhancement_targets();weapon=next(t for t in targets if str(t.item.item_id)=='EQP_WPN_SWORD_001' and str(t.owner_id)=='CHR_PLAYER');enhancement.select_weapon(weapon.item.instance_id);enhancement.select_ore('ITM_R01_ORE_01');S.update(enhancement=enhancement,weapon=weapon.item.instance_id.to_string(),ore_before=S['inventory'].count_item('ITM_R01_ORE_01'),gold_before=pc.get_editor_property('gold'));capture('EnhancementDetail');phase('enhance_attempt');return
        if S['phase']=='enhance_attempt' and elapsed>.6:
            S['enhancement'].attempt();weapon=next(t for t in S['inventory'].get_enhancement_targets() if t.item.instance_id.to_string()==S['weapon']);assert weapon.item.upgrade==1;assert S['inventory'].count_item('ITM_R01_ORE_01')==S['ore_before']-1;assert pc.get_editor_property('gold')==S['gold_before']-100;capture('EnhancementSuccess');S['checks']['equipped_weapon_enhances_ore_and_gold_once']=True;S['enhancement'].cancel();S['forge'].craft_tab();assert child(S['forge'],'StationPages').get_active_widget_index()==0;S['forge'].close_station();phase('complete');return
        if S['phase']=='complete':S['complete']=True;u.log('PARTY_FORGE_REVIEW_OK');finish()
    except Exception:S['error']=traceback.format_exc();phase('error');u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap');S['handle']=u.register_slate_post_tick_callback(tick);assert u.ICCharacterSetupLibrary.begin_monster_test_review();report()
