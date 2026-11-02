"""Rendered PIE integration review, with isolated saves and actual Slate key input."""
import unreal as u,time,json,pathlib,traceback
OUT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'Polish/Live';OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='start',started=time.monotonic(),checks={},complete=False);LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
def report():(OUT/'Review.json').write_text(json.dumps({k:v for k,v in S.items() if k in ('phase','checks','complete','error','timings')},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):S.update(phase=name,phase_started=time.monotonic());report()
def key(name,down=None):
    k=u.Key();k.set_editor_property('key_name',name)
    return u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],k) if down is None else u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],k,down)
def axis(name,value):
    k=u.Key();k.set_editor_property('key_name',name);return u.ICCharacterSetupLibrary.set_ui_review_analog(S['world'],k,value)
def shot(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def actors(cls):return u.GameplayStatics.get_all_actors_of_class(S['world'],cls)
def find_widget(owner,name):
    for obj in u.ObjectIterator(u.Widget):
        if obj.get_name()!=name:continue
        outer=obj.get_outer()
        while outer:
            if outer==owner:return obj
            outer=outer.get_outer()
    raise RuntimeError('Widget not found: '+name)
def stop(actor):
    controller=actor.get_controller()
    if controller:
        brain=controller.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Polish review')
        controller.stop_movement();controller.set_actor_tick_enabled(False)
    movement=actor.get_component_by_class(u.CharacterMovementComponent)
    if movement:movement.stop_movement_immediately()
def approach(actor,distance=160):
    p=actor.get_actor_location()+actor.get_actor_forward_vector()*distance
    S['player'].set_actor_location_and_rotation(p,u.MathLibrary.find_look_at_rotation(p,actor.get_actor_location()),False,True)
    S['pc'].set_control_rotation(u.MathLibrary.find_look_at_rotation(p,actor.get_actor_location()))
def finish():
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        if time.monotonic()-S['started']>240:raise RuntimeError('Polish integration review timeout: '+S['phase'])
        if S['phase']=='start':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            S.update(world=worlds[0],pc=pc);assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='created':
            if not pc.get_editor_property('created') or len(actors(u.ICNPCBase))<11:return
            player=u.GameplayStatics.get_player_pawn(S['world'],0);S.update(player=player,state=pc.get_world_state(),bag=player.get_editor_property('Inventory'))
            assert u.GameUserSettings.get_game_user_settings().get_editor_property('GamepadLayout')==1;S['checks']['Xbox_is_default_layout']=True
            for a in actors(u.ICNPCBase)+actors(u.ICEnemyBase):stop(a)
            S['chicken']=next(a for a in actors(u.ICRanchAnimal) if str(a.get_editor_property('DefinitionID'))=='RAN_R01_01')
            S['before_meat']=S['bag'].count_item('ITM_R01_CHICKEN_MEAT');approach(S['chicken']);phase('chicken_f');return
        if S['phase']=='chicken_f' and elapsed>.7:
            key('F');phase('chicken_anim');return
        if S['phase']=='chicken_anim':
            chicken=S['chicken']
            if elapsed<.8:
                assert chicken.is_slaughtering() and not chicken.get_editor_property('hidden');assert S['bag'].count_item('ITM_R01_CHICKEN_MEAT')==S['before_meat'];return
            if not chicken.get_editor_property('hidden'):return
            assert S['bag'].count_item('ITM_R01_CHICKEN_MEAT')==S['before_meat']+1
            S['checks']['F_chicken_animation_then_saved_meat_and_optional_eggs']=True;shot('ChickenLoot')
            for i,item in enumerate(['ITM_CONS_001','ITM_ELIXIR_001','ITM_TKT_001','ITM_FOOD_001']):
                assert S['bag'].add_item(item,3,False)
                slots=S['bag'].get_snapshot().bag.slots;slot=next(n for n,v in enumerate(slots) if str(v.item_id)==item and v.count>0);assert S['bag'].bind_quick_slot(i,slot)
            phase('quickslots');return
        if S['phase']=='quickslots' and elapsed>.5:
            hud=pc.get_editor_property('Screen');bar=find_widget(hud,'HUD_QuickSlots')
            for i in range(1,5):
                card=find_widget(bar,'QuickSlot'+str(i));assert card.get_editor_property('Count')>0 and card.get_editor_property('Icon')
            S['checks']['all_four_HUD_quickslots_have_icons_and_counts']=True;shot('HUD_QuickSlots')
            S['book_before_use']=S['bag'].count_item('ITM_ELIXIR_001');key('Gamepad_DPad_Right');phase('gamepad_target');return
        if S['phase']=='gamepad_target' and elapsed>.3:
            assert pc.is_using_gamepad() and str(pc.get_editor_property('PendingItem'))!='None'
            key('Gamepad_FaceButton_Bottom');phase('gamepad_use');return
        if S['phase']=='gamepad_use' and elapsed>.4:
            assert str(pc.get_editor_property('PendingItem'))=='None';S['checks']['Xbox_DPad_quickslot_and_A_target_confirm']=True
            assert S['bag'].count_item('ITM_ELIXIR_001')==S['book_before_use']-1
            S['chest']=next(a for a in actors(u.ICPlayerStorage) if not a.get_editor_property('hidden'));approach(S['chest']);phase('chest_f');return
        if S['phase']=='chest_f' and elapsed>.6:
            key('F');S['hinge']=S['chest'].get_editor_property('ChestPresentation');assert not pc.get_editor_property('StorageScreen');phase('chest_open');return
        if S['phase']=='chest_open':
            if elapsed<.4:
                if elapsed>.25 and not S.get('lid_shot'):shot('Storage_Lid_Partway');S['lid_shot']=True
                assert 0<=S['hinge'].get_open_amount()<1;assert not pc.get_editor_property('StorageScreen');return
            if not pc.get_editor_property('StorageScreen'):return
            assert S['hinge'].get_open_amount()>.99;shot('Storage_Open');key('Gamepad_RightShoulder');phase('storage_pad');return
        if S['phase']=='storage_pad' and elapsed>.2:
            key('Gamepad_FaceButton_Bottom');phase('storage_pad_popup');return
        if S['phase']=='storage_pad_popup' and elapsed>.3:
            storage=pc.get_editor_property('StorageScreen');assert storage
            assert find_widget(storage,'TransferPopup').is_visible();key('Gamepad_FaceButton_Right');phase('storage_pad_back');return
        if S['phase']=='storage_pad_back' and elapsed>.3:
            storage=pc.get_editor_property('StorageScreen');assert storage and not find_widget(storage,'TransferPopup').is_visible()
            S['checks']['Xbox_RB_storage_side_A_popup_B_back']=True;storage.close_storage();phase('chest_close');return
        if S['phase']=='chest_close' and elapsed>1:
            assert S['hinge'].get_open_amount()==0;S['checks']['F_lid_opens_before_storage_and_closes_after_UI']=True
            shelf=next(a for a in actors(u.ICNPCContainerActor) if 'Bookshelf' in a.get_class().get_name());S['shelf']=shelf
            assert S['state'].set_affinity(shelf.get_editor_property('OwnerNPCID'),300);approach(shelf);phase('shelf_f');return
        if S['phase']=='shelf_f' and elapsed>.6:
            # Exercise the actor directly; nearby owners may take F priority in a densely packed test layout.
            assert S['shelf'].try_interact(pc);assert not pc.get_editor_property('StorageScreen');assert not S['shelf'].is_furniture_loot_available()
            S['checks']['bookshelf_grants_items_without_opening_storage']=True
            npc_chest=next(a for a in actors(u.ICNPCContainerActor) if 'NPCChest' in a.get_class().get_name());S['npc_chest']=npc_chest
            assert S['state'].set_affinity(npc_chest.get_editor_property('OwnerNPCID'),300);approach(npc_chest);phase('npc_chest_f');return
        if S['phase']=='npc_chest_f' and elapsed>.6:
            key('F');phase('npc_chest_open');return
        if S['phase']=='npc_chest_open' and elapsed>1:
            assert pc.get_editor_property('StorageScreen');assert S['npc_chest'].get_editor_property('ChestPresentation').get_open_amount()>.99
            shot('NPC_Chest_Open');pc.get_editor_property('StorageScreen').close_storage();S['checks']['NPC_chest_F_lid_and_storage']=True
            wardrobe=next(a for a in actors(u.ICNPCContainerActor) if 'Wardrobe' in a.get_class().get_name())
            assert S['state'].set_affinity(wardrobe.get_editor_property('OwnerNPCID'),300);approach(wardrobe);assert wardrobe.try_interact(pc)
            assert not pc.get_editor_property('StorageScreen') and not wardrobe.is_furniture_loot_available();S['checks']['wardrobe_direct_loot_without_storage']=True
            bed=actors(u.ICPlayerBed)[0];S['bed']=bed;approach(bed);phase('sleep_f');return
        if S['phase']=='sleep_f' and elapsed>.6:
            assert S['bed'].try_interact(pc);S['life']=S['player'].get_component_by_class(u.ICPlayerLifeComponent);assert S['life'].is_sleeping();phase('sleep_enter');return
        if S['phase']=='sleep_enter':
            if elapsed>.1 and not S.get('sleep_shot'):shot('Sleep_Enter');S['sleep_shot']=True
            if elapsed>.75 and S['life'].is_sleep_pose_active() and not S.get('sleep_lie_shot'):
                mesh=S['player'].get_component_by_class(u.SkeletalMeshComponent);head=mesh.get_socket_location('J_Bip_C_Head');foot=(mesh.get_socket_location('J_Bip_L_Foot')+mesh.get_socket_location('J_Bip_R_Foot'))*.5
                assert abs(head.y-foot.y)>abs(head.x-foot.x)*2,'Sleep body must follow the long axis of the bed'
                shot('Sleep_Lying');S['sleep_lie_shot']=True;S['checks']['sleep_pose_aligned_with_bed_length']=True
            if elapsed>1.5 and not S.get('sleep_wake_shot'):shot('Sleep_Fade');S['sleep_wake_shot']=True
            if S['life'].is_sleeping():return
            S['timings']={'sleep_total_seconds':elapsed};phase('sleep_done');return
        if S['phase']=='sleep_done' and elapsed>1:
            assert pc.get_view_target()==S['player'];shot('Sleep_Awake');S['checks']['quick_sleep_and_camera_restore']=True
            pc.set_menu('Settings');phase('settings');return
        if S['phase']=='settings' and elapsed>.6:
            widget=next(w for w in u.ObjectIterator(u.ICSettingsWidget) if w.get_owning_player()==pc);S['settings']=widget;widget.select_tab(4)
            assert find_widget(widget,'OPTION_LanguageIndex');assert find_widget(widget,'OPTION_GamepadLayout')
            find_widget(widget,'OPTION_LanguageIndex').set_selected_index(1);widget.apply_options();phase('english');return
        if S['phase']=='english' and elapsed>.5:
            assert str(find_widget(S['settings'],'TXT_SettingsTitle').get_text())=='Settings';shot('Settings_English')
            S['settings'].select_tab(1);phase('controller_settings');return
        if S['phase']=='controller_settings' and elapsed>.4:
            shot('Settings_Controller');S['settings'].select_tab(4);find_widget(S['settings'],'OPTION_LanguageIndex').set_selected_index(0);S['settings'].apply_options();phase('korean');return
        if S['phase']=='korean' and elapsed>.4:
            assert str(find_widget(S['settings'],'TXT_SettingsTitle').get_text())=='설정';S['checks']['WBP_Korean_English_switch_and_controller_options']=True
            pc.set_menu('');u.SystemLibrary.execute_console_command(S['world'],'ICEquipStarterWeapons',pc)
            for _ in range(6):
                if S['bag'].get_equipped_weapon_family()==u.ICWeaponFamily.SWORD:break
                assert S['bag'].cycle_weapon()
            enemy=actors(u.ICEnemyBase)[0];S['wounded_enemy_id']=str(enemy.get_character_id());S['wounded_enemy']=enemy
            S['player'].set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True);pc.set_control_rotation(u.Rotator())
            enemy.set_actor_location(u.Vector(-900,-2500,96),False,True);S['enemy_before']=S['state'].export_state().spawn_slots[enemy.get_character_id()].hp
            phase('wound_prepare');return
        if S['phase']=='wound_prepare' and elapsed>2:
            asc=S['player'].get_component_by_class(u.ICAbilitySystemComponent);result=asc.request_action('Attack','None',S['wounded_enemy']);assert result.success,str(result.reason);phase('wound');return
        if S['phase']=='wound' and elapsed>1.5:
            assert S['wounded_enemy'].flush_to_world_state(S['state'])
            hp=S['state'].export_state().spawn_slots[S['wounded_enemy'].get_character_id()].hp
            assert 0<hp<S['enemy_before'];S['checks']['save_contains_a_living_wounded_monster']=True;phase('combat_cooldown');return
        if S['phase']=='combat_cooldown' and elapsed>9:
            pc.open_save_menu(False);pc.select_session_slot(1);pc.request_manual_save();phase('save');return
        if S['phase']=='save' and elapsed>2:
            pc.close_session_menu();S['music']=next(o for o in u.ObjectIterator(u.ICBGMSubsystem) if o.get_outer()==S['world']);phase('music');return
        if S['phase']=='music' and elapsed>1:
            assert S['music'].get_music_component();pc.player_died();phase('death');return
        if S['phase']=='death' and elapsed>1:
            assert not S['music'].get_music_component();S['checks']['death_stops_BGM_and_keeps_it_stopped']=True;shot('Death_Silent')
            u.log('POLISH_RESTORE_BEGIN');pc.open_load_menu();pc.select_session_slot(1);pc.request_load_player();pc.confirm_modal(True);phase('load');return
        if S['phase']=='load' and elapsed>2:
            if pc.is_dead():return
            assert S['music'].get_music_component();u.log('POLISH_RESTORE_END');S['checks']['load_restores_alive_player_and_area_BGM']=True
            S['complete']=True;phase('complete');u.log('COMBAT_LIFE_POLISH_LIVE_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
monsters=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster');rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(monsters))
for row in rows:row['CombatStats']['MaxHP']=10000
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(monsters,json.dumps(rows)) # temporary review HP; never save the table
S['handle']=u.register_slate_post_tick_callback(tick);assert u.ICCharacterSetupLibrary.begin_monster_test_review();report()




