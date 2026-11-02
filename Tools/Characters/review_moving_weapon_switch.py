"""Rendered PIE review: hold movement through every weapon transition, using isolated saves."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/MovingWeaponReview';OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE=dict(phase='starting',started=time.monotonic(),case=0,samples=[],complete=False)

def report():
    (OUT/'Review.json').write_text(json.dumps({k:STATE[k] for k in ['phase','samples','complete','error'] if k in STATE},indent=2),encoding='utf-8')

def key(name,pressed):
    value=u.Key();value.set_editor_property('key_name',name)
    u.ICCharacterSetupLibrary.set_ui_review_key(STATE['world'],value,pressed)

def finish():
    handle=STATE.pop('handle',None)
    if handle is not None:u.unregister_slate_post_tick_callback(handle)
    report()
    try:STATE['pc'].get_saves().delete_slot(0)
    except Exception:pass
    for name in ['W','LeftShift']:
        try:key(name,False)
        except Exception:pass
    LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        assert time.monotonic()-STATE['started']<240,'Moving weapon review timed out'
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc,phase='creating');return
        if STATE['phase']=='creating':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or not pc.get_editor_property('created'):return
            STATE.update(world=worlds[0],pc=pc)
            player=u.GameplayStatics.get_player_pawn(worlds[0],0);inventory=pc.get_inventory()
            for index,item in enumerate(['EQP_WPN_SWORD_001','EQP_WPN_BOW_001','EQP_WPN_STAFF_001','EQP_WPN_WAND_001']):
                if str(inventory.get_weapon_loadout()[index].item_id)==item:continue
                assert inventory.add_item(item,1,False)
                slot=next(i for i,value in enumerate(inventory.get_snapshot().bag.slots) if str(value.item_id)==item)
                assert inventory.equip(slot)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property('field_of_view',50)
            for enemy in u.GameplayStatics.get_all_actors_of_class(worlds[0],u.ICEnemyBase):
                enemy.set_actor_enable_collision(False);enemy.set_actor_tick_enabled(False)
                controller=enemy.get_controller()
                if controller:controller.set_actor_tick_enabled(False)
                enemy.set_actor_location(u.Vector(50000,50000,-1000),False,True)
            STATE.update(player=player,inventory=inventory,camera=camera,phase='next');return
        player=STATE['player'];body=player.get_editor_property('mesh');case=STATE['case']
        if STATE['phase'] in ['settling','switching']:
            # Feed the same CharacterMovement input queue used by the controller;
            # Slate focus in an offscreen editor must not determine the pose review.
            player.add_movement_input(u.Vector(1,0,0),1,False)
        focus=player.get_actor_location()+u.Vector(0,0,-5)
        location=focus+u.Vector(-380,420,110)
        STATE['camera'].set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True)
        if STATE['phase']=='next':
            if case>=20:
                STATE.update(phase='complete',complete=True);finish();return
            key('W',False);key('LeftShift',False)
            STATE['pc'].release_input_command(u.ICInputCommand.DODGE,0)
            player.apply_preset(case//10)
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,90),u.Rotator(),False,True)
            STATE['pc'].set_control_rotation(u.Rotator())
            if case%5==0:
                for _ in range(6):
                    if STATE['inventory'].get_equipped_weapon_family()==u.ICWeaponFamily.NONE:break
                    assert STATE['inventory'].cycle_weapon()
            STATE.update(phase='settling',move_started=False);return
        if STATE['phase']=='settling':
            clock=STATE['pc'].get_time().get_clock().play_second
            if not STATE['move_started']:
                if str(STATE['pc'].get_editor_property('menu')) not in ['', 'None']:return
                if (case//5)%2:STATE['pc'].handle_input_command(u.ICInputCommand.DODGE,0)
                STATE.update(move_started=True,ready=clock+.5,move_retry=clock+4);return
            if clock<STATE['ready']:return
            if player.get_velocity().length()<100:
                assert clock<STATE['move_retry'],str(dict(menu=str(STATE['pc'].get_editor_property('menu')),speed=player.get_velocity().length(),movement=str(player.get_component_by_class(u.CharacterMovementComponent).get_editor_property('movement_mode'))))
                key('W',False);key('W',True);STATE['ready']=clock+.2;return
            STATE['pc'].handle_input_command(u.ICInputCommand.SWITCH_WEAPON,0)
            assert player.is_switching_weapon(),'Weapon transition did not start'
            label=('M' if case<10 else 'F')+('_Sprint_' if (case//5)%2 else '_Walk_')+str(case%5)
            STATE.update(phase='switching',sample=dict(case=label,distance_cm=0,leg_steps=0,max_speed=0),
                         start=player.get_actor_location(),last_leg=None,captured=False,
                         action_started=STATE['pc'].get_time().get_clock().play_second);return
        if STATE['phase']=='switching':
            elapsed=STATE['pc'].get_time().get_clock().play_second-STATE['action_started']
            assert elapsed<6,'Weapon switch remained active'
            rotation=body.get_socket_transform('J_Bip_L_UpperLeg',u.RelativeTransformSpace.RTS_PARENT_BONE_SPACE).rotation
            previous=STATE['last_leg']
            if previous is not None and abs(rotation.x*previous.x+rotation.y*previous.y+rotation.z*previous.z+rotation.w*previous.w)<.99999:STATE['sample']['leg_steps']+=1
            STATE['last_leg']=rotation
            STATE['sample']['max_speed']=max(STATE['sample']['max_speed'],player.get_velocity().length())
            if elapsed>.25 and not STATE['captured'] and case%5==0:
                assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(STATE['sample']['case']+'.png')))
                STATE['captured']=True
            if not player.is_switching_weapon():
                sample=STATE['sample'];sample['distance_cm']=player.get_actor_location().distance(STATE['start'])
                sample['family']=str(STATE['inventory'].get_equipped_weapon_family())
                assert sample['distance_cm']>40,sample
                assert sample['leg_steps']>3,sample
                assert sample['max_speed']>(450 if (case//5)%2 else 150),sample
                if not (case//5)%2:assert sample['max_speed']<500,sample
                STATE['samples'].append(sample);STATE.update(case=case+1,phase='next');report()
    except Exception:
        STATE.update(phase='error',error=traceback.format_exc());u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
