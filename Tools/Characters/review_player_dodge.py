"""Review full dodge/recovery in rendered PIE, using isolated saves and real movement input."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
COMBAT = '-ICCombatDodge' in u.SystemLibrary.get_command_line()
OUT = ROOT / ('Saved/PlayerCombatDodgeReview' if COMBAT else 'Saved/PlayerDodgeReview')
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
SETTINGS=u.load_asset('/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CombatSettings')
PLAY_RATE=float(json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(SETTINGS))[0]['DodgeAnimationPlayRate'])
DISTANCE_SCALE=float(json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(SETTINGS))[0]['PlayerDodgeDistanceScale'])
FAMILIES = [u.ICWeaponFamily.NONE, u.ICWeaponFamily.SWORD, u.ICWeaponFamily.BOW, u.ICWeaponFamily.STAFF, u.ICWeaponFamily.WAND]
NAMES = ['Unarmed', 'Sword', 'Bow', 'Staff', 'Wand']
# The automated GAS check covers all eight directions; PIE exercises both
# no-input retreat and held diagonal input in every gender/weapon state.
STATE = dict(phase='starting', started=time.monotonic(), samples=[], complete=False, case=0)


def report():
    (OUT / 'Review.json').write_text(json.dumps({k: STATE[k] for k in ['phase', 'samples', 'complete', 'error'] if k in STATE}, indent=2), encoding='utf-8')


def key(name, pressed):
    value = u.Key()
    value.set_editor_property('key_name', name)
    u.ICCharacterSetupLibrary.set_ui_review_key(STATE['world'], value, pressed)


def finish():
    handle=STATE.pop('handle',None)
    if handle is not None:u.unregister_slate_post_tick_callback(handle)
    report()
    try:STATE['pc'].get_saves().delete_slot(0)
    except Exception:pass
    if 'world' in STATE:
        for name in ['W', 'D']:
            try:key(name, False)
            except Exception:pass
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def capture(label):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'], str(OUT / (label + '.png')))


def player_hp():
    assert STATE['player'].flush_to_world_state(STATE['pc'].get_world_state())
    return STATE['pc'].get_world_state().get_snapshot(STATE['player'].get_character_id()).hp


def authored_dodge():
    case=STATE['case'];pack=['Sword_Animations','Sword_Animations','Archer','Mage','Mage'][(case//2)%5]
    suffix=('F_R_45' if case%2 else 'B') if pack=='Sword_Animations' else ('F_R_45' if case%2 else 'B_180')
    name=('Dodge_Combat_' if pack=='Sword_Animations' else 'AS_Dodge_Combat_')+suffix+'_Seq'
    gender='Male' if case<10 else 'Female'
    clip=u.load_asset(f'/Game/IsekaiClass/Animation/Player/Main{gender}/{pack}/RT_{pack}_{name}')
    assert clip,name
    locked=clip.get_editor_property('force_root_lock')
    try:
        clip.set_editor_property('force_root_lock',False)
        options=u.AnimPoseEvaluationOptions(extract_root_motion=False,incorporate_root_motion_into_pose=True)
        start=u.AnimPoseExtensions.get_bone_pose(clip.get_anim_pose_at_time(0,options),'Root',u.AnimPoseSpaces.WORLD).translation
        end=u.AnimPoseExtensions.get_bone_pose(clip.get_anim_pose_at_time(clip.get_play_length(),options),'Root',u.AnimPoseSpaces.WORLD).translation
        return clip.get_play_length()/(clip.get_editor_property('rate_scale')*PLAY_RATE),start.distance(end)*DISTANCE_SCALE
    finally:clip.set_editor_property('force_root_lock',locked)


def tick(delta):
    try:
        if time.monotonic() - STATE['started'] > 240:
            raise RuntimeError('Dodge PIE review timeout in ' + STATE['phase'])
        if STATE['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(pc.get_editor_property('menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0], pc=pc, phase='creating')
            u.SystemLibrary.execute_console_command(worlds[0], 'r.MotionBlurQuality 0')
            return
        if STATE['phase'] == 'creating':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            current_pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not current_pc:return
            STATE.update(world=worlds[0],pc=current_pc)
            if not STATE['pc'].get_editor_property('created'):
                return
            player = u.GameplayStatics.get_player_pawn(STATE['world'], 0)
            enemies=list(u.GameplayStatics.get_all_actors_of_class(STATE['world'], u.ICEnemyBase))
            if COMBAT:
                # Use an isolated monster whose authored origin is in the clear
                # review area. Moving a map monster away from its own origin can
                # legitimately activate Returning and reject the test attack.
                if not STATE.get('review_enemy'):
                    enemy=u.ICCharacterSetupLibrary.spawn_live_review_enemy(STATE['world'],u.Vector(-890,-2500,90))
                    assert enemy
                    STATE['review_enemy']=enemy
                    return
                enemies=[STATE['review_enemy'],*[enemy for enemy in enemies if enemy!=STATE['review_enemy']]]
            if COMBAT and not enemies[0].flush_to_world_state(STATE['pc'].get_world_state()):return
            inventory = STATE['pc'].get_inventory()
            for index, item in enumerate(['EQP_WPN_SWORD_001', 'EQP_WPN_BOW_001', 'EQP_WPN_STAFF_001', 'EQP_WPN_WAND_001']):
                if inventory.get_weapon_loadout()[index].count > 0:
                    continue
                assert inventory.add_item(item, 1, False)
                slot = next(i for i, v in enumerate(inventory.get_snapshot().bag.slots) if str(v.item_id) == item)
                assert inventory.equip(slot)
            view = player.get_component_by_class(u.CameraComponent)
            view.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            view.set_editor_property('field_of_view', 65)
            STATE.update(player=player, inventory=inventory, camera=view, asc=player.get_component_by_class(u.ICAbilitySystemComponent), phase='next')
            if COMBAT:
                assert enemies
                STATE.update(enemy=enemies[0], enemy_origin=u.Vector(-890,-2500,90))
                assert inventory.add_item('ITM_CONS_001',30,False)
            for enemy in enemies:
                enemy.set_actor_tick_enabled(False)
                controller = enemy.get_controller()
                if controller:
                    brain = controller.get_component_by_class(u.BrainComponent)
                    if brain:
                        brain.stop_logic('Dodge review')
                    controller.set_actor_tick_enabled(False)
                enemy.set_actor_enable_collision(False)
                enemy.set_actor_location(u.Vector(50000, 50000, -1000), False, True)
            if COMBAT:
                STATE['enemy'].set_actor_location(STATE['enemy_origin'],False,True)
                STATE['enemy'].set_actor_enable_collision(True)
                STATE['enemy'].get_component_by_class(u.CharacterMovementComponent).set_component_tick_enabled(False)
                STATE['enemy_asc']=STATE['enemy'].get_component_by_class(u.ICAbilitySystemComponent)
            return
        asc, player = STATE['asc'], STATE['player']
        focus=player.get_actor_location()+u.Vector(0,0,-5)
        camera_location=focus+u.Vector(-560,540,210)
        STATE['camera'].set_world_location_and_rotation(camera_location,u.MathLibrary.find_look_at_rotation(camera_location,focus),False,True)
        if COMBAT:
            controller=STATE['enemy'].get_controller()
            if controller:
                controller.set_actor_tick_enabled(False)
                brain=controller.get_component_by_class(u.BrainComponent)
                if brain:
                    brain.stop_logic('Controlled dodge hit review')
                    brain.set_component_tick_enabled(False)
        if STATE['phase'] == 'next':
            if STATE['case'] >= 20:
                STATE.update(phase='complete', complete=True)
                finish()
                return
            if COMBAT:STATE['enemy_asc'].cancel_action()
            if asc.get_movement_cooldown_remaining() > 0:
                return
            if COMBAT:
                # Repeated recovery hits must not kill the review player. Use
                # the real consumable path; changing stored vitals alone does
                # not refill the live ASC in this fixture.
                if player_hp()<60:
                    STATE['pc'].use_inventory_item('ITM_CONS_001',player.get_character_id())
                if player_hp()<20:return
            case = STATE['case']
            player.apply_preset(case // 10)
            if COMBAT:
                STATE['enemy_asc'].cancel_action()
                STATE['enemy'].set_actor_location(STATE['enemy_origin'],False,True)
                for _ in range(6):
                    if STATE['inventory'].get_equipped_weapon_family()==u.ICWeaponFamily.SWORD:break
                    assert STATE['inventory'].cycle_weapon()
                warmup=asc.request_action('Attack','None',STATE['enemy'])
                if not warmup.success:return
                assert asc.notify_combat_event(warmup.action_id,'AN_EffectCommit',0)
                asc.cancel_action()
            family = FAMILIES[(case // 2) % 5]
            for _ in range(6):
                if STATE['inventory'].get_equipped_weapon_family() == family:
                    break
                assert STATE['inventory'].cycle_weapon()
            assert STATE['inventory'].get_equipped_weapon_family() == family
            origin=STATE['enemy_origin']+u.Vector(-110,0,8) if COMBAT else u.Vector(-1000,-2500,90)
            player.set_actor_location_and_rotation(origin, u.Rotator(), False, True)
            player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            STATE['pc'].set_control_rotation(u.Rotator())
            if COMBAT:
                world_state=STATE['pc'].get_world_state();assert player.flush_to_world_state(world_state)
                snap=world_state.get_snapshot(player.get_character_id())
                world_state.set_vitals(player.get_character_id(),snap.max_hp,snap.max_mp,snap.max_sp)
                # The detached review camera must keep its input yaw; explicit
                # camera lock would turn WASD relative to the oblique capture view.
                STATE['pc'].handle_input_command(u.ICInputCommand.CLEAR_TARGET,0)
            focus = player.get_actor_location() + u.Vector(0, 50, 0)
            location = focus + u.Vector(-560, 540, 210)
            STATE['camera'].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)
            STATE.update(phase='settling', ready=time.monotonic() + .25)
            return
        if STATE['phase'] == 'settling':
            if time.monotonic() < STATE['ready']:
                return
            if STATE['case'] % 2:
                key('W', True)
                key('D', True)
            STATE.update(phase='input', ready=time.monotonic() + .05)
            return
        if STATE['phase'] == 'input':
            if time.monotonic() < STATE['ready']:
                return
            # Input may turn a noncombat character; reset facing so both reviews
            # select a known direction from the same camera-relative keys.
            player.set_actor_rotation(u.Rotator(), False)
            start = player.get_actor_location()
            expected_duration,expected_distance=authored_dodge()
            if COMBAT:
                STATE['hp_before']=player_hp()
                STATE['pc'].handle_input_command(u.ICInputCommand.DODGE,0)
                action_id=asc.get_action_id()
                assert asc.notify_combat_event(action_id,'IFrameStart',0),'Combat Shift did not start a dodge'
                attack=STATE['enemy_asc'].request_action('Attack','None',player)
                assert attack.success,str(attack.reason)
            else:
                action = asc.request_action('Dodge', 'None', None)
                assert action.success, str(action.reason)
                action_id=action.action_id
            key('W', False)
            key('D', False)
            anim = player.get_editor_property('mesh').get_anim_instance()
            montage = anim.get_current_active_montage()
            assert montage and montage.get_name().endswith('_' + ('1' if STATE['case'] % 2 else '4')), montage
            duration = montage.get_play_length() / (anim.montage_get_play_rate(montage) * montage.get_editor_property('rate_scale'))
            assert abs(duration - expected_duration) < .001, (duration,expected_duration)
            assert abs(anim.montage_get_play_rate(montage)-PLAY_RATE)<.001
            label = ('M' if STATE['case'] < 10 else 'F') + '_' + NAMES[(STATE['case'] // 2) % 5] + ('_Diagonal' if STATE['case'] % 2 else '_Back')
            STATE.update(phase='dodging', action_id=action_id, anim=anim, montage=montage, start=start,
                         action_started=STATE['pc'].get_time().get_clock().play_second,
                         sample=dict(case=label, duration=duration, authored_distance_cm=expected_distance/DISTANCE_SCALE, configured_distance_cm=expected_distance, distance_scale=DISTANCE_SCALE, montage=montage.get_path_name()), captured=set())
            return
        if STATE['phase'] == 'dodging':
            elapsed = STATE['pc'].get_time().get_clock().play_second - STATE['action_started']
            ongoing = asc.notify_combat_event(STATE['action_id'], 'IFrameStart', 0)
            if elapsed > .29 and 'mid' not in STATE['captured']:
                assert ongoing and STATE['anim'].montage_is_playing(STATE['montage'])
                STATE['sample']['active_after_old_cutoff'] = True
                if COMBAT:
                    assert player_hp()==STATE['hp_before'],'A hit during the dodge push bypassed invulnerability'
                    STATE['sample']['enemy_hit_avoided_during_push']=True
                if STATE['case'] in [1, 3, 5, 7, 11, 13, 15, 17]:
                    capture(STATE['sample']['case'] + '_Mid')
                STATE['captured'].add('mid')
            if elapsed > STATE['sample']['duration']-.05 and 'recovery' not in STATE['captured']:
                distance = player.get_actor_location().distance(STATE['start'])
                assert abs(distance-STATE['sample']['configured_distance_cm'])<2, (distance,STATE['sample']['configured_distance_cm'])
                assert ongoing
                STATE['sample']['distance_cm'] = distance
                STATE['sample']['recovery_location'] = [player.get_actor_location().x, player.get_actor_location().y]
                if STATE['case'] in [3, 13]:
                    capture(STATE['sample']['case'] + '_Recovery')
                STATE['captured'].add('recovery')
            if elapsed > STATE['sample']['duration']+.12:
                assert not ongoing
                assert not STATE['anim'].montage_is_playing(STATE['montage'])
                distance = player.get_actor_location().distance(STATE['start'])
                assert abs(distance - STATE['sample']['distance_cm']) < 1, distance
                STATE['sample']['no_recovery_slide'] = True
                if COMBAT:
                    # Re-enter melee reach after the completed dodge, so the
                    # second attack tests expired immunity rather than a range miss.
                    STATE['enemy'].set_actor_location(player.get_actor_location()+u.Vector(100,0,0),False,True)
                    attack=STATE['enemy_asc'].request_action('Attack','None',player)
                    assert attack.success,str(attack.reason)
                    STATE.update(phase='after_dodge_hit',hit_started=STATE['pc'].get_time().get_clock().play_second)
                    return
                STATE['samples'].append(STATE['sample'])
                STATE.update(case=STATE['case'] + 1, phase='next')
                report()
        if STATE['phase']=='after_dodge_hit' and STATE['pc'].get_time().get_clock().play_second-STATE['hit_started']>.35:
            STATE['sample']['hp_before_attacks']=STATE['hp_before']
            STATE['sample']['hp_after_recovery_hit']=player_hp()
            STATE['sample']['recovery_hit_distance_cm']=player.get_actor_location().distance(STATE['enemy'].get_actor_location())
            assert player_hp()<STATE['hp_before'],str(STATE['sample'])
            STATE['sample']['enemy_hit_received_after_dodge']=True
            STATE['samples'].append(STATE['sample'])
            STATE.update(case=STATE['case']+1,phase='next')
            report()
    except Exception:
        STATE.update(phase='error', error=traceback.format_exc())
        u.log_error(STATE['error'])
        finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
