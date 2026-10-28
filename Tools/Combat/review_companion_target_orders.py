"""PIE: real Tab + ZR orders, current enemy first, then the queued enemy; gamepad and unarmed jumps."""
import unreal as u,time,pathlib,json,traceback
OUT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'Polish/Party';OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='start',started=time.monotonic(),checks={},complete=False);LEVEL=u.get_editor_subsystem(u.LevelEditorSubsystem)
def report():(OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ['phase','checks','complete','error','targets'] if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(v):S.update(phase=v,phase_started=time.monotonic());report()
def key(name,down=None):
    k=u.Key();k.set_editor_property('key_name',name)
    return u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],k) if down is None else u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],k,down)
def axis(name,value):
    k=u.Key();k.set_editor_property('key_name',name);return u.ICCharacterSetupLibrary.set_ui_review_analog(S['world'],k,value)
def actors(cls):return u.GameplayStatics.get_all_actors_of_class(S['world'],cls)
def stop(a):
    c=a.get_controller()
    if c:
        brain=c.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Target order review')
        c.stop_movement();c.set_actor_tick_enabled(False)
    a.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
def shot(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def finish():report();u.unregister_slate_post_tick_callback(S['handle']);LEVEL.editor_request_end_play();u.SystemLibrary.quit_editor()
def tick(dt):
    try:
        if time.monotonic()-S['started']>150:raise RuntimeError('Target order review timed out: '+S['phase'])
        if S['phase']=='start':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('Menu'))!='Create':return
            S.update(world=worlds[0],pc=pc);assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='created':
            if not pc.get_editor_property('created') or len(actors(u.ICNPCBase))<11:return
            player=u.GameplayStatics.get_player_pawn(S['world'],0);S.update(player=player,state=pc.get_world_state(),asc=player.get_component_by_class(u.ICAbilitySystemComponent),bag=pc.get_inventory())
            assert u.GameUserSettings.get_game_user_settings().get_editor_property('GamepadLayout')==1;S['checks']['Xbox_is_default_layout']=True
            for a in actors(u.ICNPCBase)+actors(u.ICEnemyBase):stop(a)
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True)
            for i,item in enumerate(['ITM_BOOK_001']):
                assert S['bag'].add_item(item,3,False);idx=next(n for n,v in enumerate(S['bag'].get_snapshot().bag.slots) if str(v.item_id)==item);assert S['bag'].bind_quick_slot(1,idx)
            phase('male_jump');return
        if S['phase']=='male_jump' and elapsed>.8:
            S['player'].apply_preset(0);key('Gamepad_FaceButton_Bottom',True);phase('male_air');return
        if S['phase']=='male_air':
            if elapsed>.15 and not S.get('male_released'):key('Gamepad_FaceButton_Bottom',False);S['male_released']=True
            if elapsed>.25 and not S.get('male_shot'):
                anim=S['player'].get_component_by_class(u.SkeletalMeshComponent).get_anim_instance();assert 'MM_' in anim.get_editor_property('Jump').get_name();shot('Male_Unarmed_Jump');S['male_shot']=True
            if elapsed<1.3:return
            S['player'].apply_preset(1);phase('female_jump');return
        if S['phase']=='female_jump' and elapsed>.8:key('Gamepad_FaceButton_Bottom',True);phase('female_air');return
        if S['phase']=='female_air':
            if elapsed>.15 and not S.get('female_released'):key('Gamepad_FaceButton_Bottom',False);S['female_released']=True
            if elapsed>.25 and not S.get('female_shot'):
                anim=S['player'].get_component_by_class(u.SkeletalMeshComponent).get_anim_instance();assert 'MainFemale' in anim.get_editor_property('Jump').get_path_name();shot('Female_Unarmed_Jump');S['female_shot']=True
            if elapsed<1.3:return
            S['checks']['male_and_female_use_new_unarmed_jump_air_and_land']=True;S['player'].apply_preset(0)
            u.SystemLibrary.execute_console_command(S['world'],'ICRecruitTestParty',pc);u.SystemLibrary.execute_console_command(S['world'],'ICEquipStarterWeapons',pc);phase('recruit');return
        if S['phase']=='recruit' and elapsed>1.5:
            party=[a for a in actors(u.ICNPCBase) if S['state'].get_snapshot(a.get_character_id()).party_slot>=0];assert len(party)==3;S['party']=party
            for a in party:stop(a)
            for _ in range(6):
                if S['bag'].get_equipped_weapon_family()==u.ICWeaponFamily.SWORD:break
                assert S['bag'].cycle_weapon()
            key('Gamepad_DPad_Right');phase('target_stick');return
        if S['phase']=='target_stick' and elapsed>.2:
            assert str(pc.get_editor_property('PendingItem'))=='ITM_BOOK_001';axis('Gamepad_LeftX',1);phase('target_stick_check');return
        if S['phase']=='target_stick_check' and elapsed>.3:
            axis('Gamepad_LeftX',0);S['selected']=str(pc.get_editor_property('PendingTargetID'));assert S['selected'].startswith('NPC_R01_'),S['selected'];key('Gamepad_FaceButton_Bottom');S['checks']['Xbox_DPad_quickslot_left_stick_party_target_A_use']=True
            enemies=actors(u.ICEnemyBase);old=enemies[0];new=enemies[1];S.update(old=old,new=new)
            old.set_actor_location(u.Vector(-650,-2400,100),False,True);new.set_actor_location(u.Vector(-250,-2500,100),False,True)
            S['player'].set_actor_location_and_rotation(u.Vector(-350,-2500,96),u.Rotator(),False,True);pc.set_control_rotation(u.Rotator())
            for i,a in enumerate(S['party']):
                snap=S['state'].get_snapshot(a.get_character_id());snap.set_editor_property('bAutoSkillsEnabled',False);assert S['state'].set_character_state(snap)
                a.set_actor_location(u.Vector(-720,-2400+(i-1)*60,96),False,True);a.get_controller().set_actor_tick_enabled(True);a.get_controller().order_attack(old)
            phase('old_target');return
        if S['phase']=='old_target' and elapsed>.35:
            assert all(a.get_controller().get_attack_target()==S['old'] for a in S['party'])
            camera=S['player'].get_component_by_class(u.CameraComponent);camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            target=S['new'].get_actor_location();position=target+u.Vector(-300,-400,200);camera.set_world_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,target),False,True)
            S['tab_attempts']=0;phase('target_view');return
        if S['phase']=='target_view' and elapsed>.3:key('Tab');phase('select_new');return
        if S['phase']=='select_new' and elapsed>.15:
            if pc.get_locked_combat_target()==S['new']:key('Gamepad_RightTrigger');phase('queued');return
            S['tab_attempts']+=1;assert S['tab_attempts']<20,'Tab target never became visible';key('Tab');phase('select_new');return
        if S['phase']=='queued' and elapsed>.3:
            S['targets']=[dict(current=a.get_controller().get_attack_target().get_name() if a.get_controller().get_attack_target() else None,queued=a.get_controller().get_queued_attack_target().get_name() if a.get_controller().get_queued_attack_target() else None) for a in S['party']]
            assert all(a.get_controller().get_attack_target()==S['old'] and a.get_controller().get_queued_attack_target()==S['new'] for a in S['party']),str(S['targets'])
            S['checks']['Tab_then_ZR_queues_target_for_all_three_without_interrupting_current_enemy']=True;shot('Party_Queued_Target');phase('finish_old');return
        if S['phase']=='finish_old':
            if all(a.get_controller().get_attack_target()==S['new'] for a in S['party']):
                S['checks']['all_three_switch_after_current_enemy_is_killed']=True;shot('Party_New_Target');key('Gamepad_LeftTrigger',True);key('Gamepad_FaceButton_Left');key('Gamepad_LeftTrigger',False);phase('chord');return
            if elapsed>35:raise RuntimeError('Companions did not defeat their current target and switch')
        if S['phase']=='chord' and elapsed>.3:
            assert S['asc'].get_movement_cooldown_remaining()==0;S['checks']['Xbox_LT_X_skill_chord_does_not_also_dodge']=True
            S['complete']=True;phase('complete');u.log('COMPANION_TARGET_ORDERS_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
monsters=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(monsters))
for row in rows:row['CombatStats']['MaxHP']=300;row['CombatStats']['Defense']=0
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(monsters,json.dumps(rows)) # in memory only; never save test balance
S['handle']=u.register_slate_post_tick_callback(tick);assert u.ICCharacterSetupLibrary.begin_monster_test_review();report()


