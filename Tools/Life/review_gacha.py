"""Exercise gold cheat, real machine F interaction and cube overflow in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'GachaLiveReview'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{},'complete':False}


def report():
    (OUT/'Review.json').write_text(json.dumps({k:STATE[k] for k in ('phase','checks','complete','error') if k in STATE},ensure_ascii=False,indent=2),encoding='utf-8')


def phase(name):
    STATE.update(phase=name,phase_started=time.monotonic());report()


def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))


def finish():
    report();u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()


def aim(point,offset):
    location=point+offset
    STATE['camera'].set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)


def tick(delta):
    try:
        if time.monotonic()-STATE['started']>180:raise RuntimeError('Gacha live review timeout: '+STATE['phase'])
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
            player=u.GameplayStatics.get_player_pawn(world,0);view=player.get_component_by_class(u.CameraComponent)
            view.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            machine=u.GameplayStatics.get_all_actors_of_class(world,u.ICGachaMachine)[0]
            for enemy in u.GameplayStatics.get_all_actors_of_class(world,u.ICEnemyBase):
                ai=enemy.get_controller()
                if ai:
                    brain=ai.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Gacha review')
                enemy.set_actor_location(u.Vector(50000,50000,96),False,True)
            STATE.update(player=player,camera=view,machine=machine)
            pc.set_menu('');p=machine.get_actor_location()
            player.set_actor_location_and_rotation(p+u.Vector(-180,0,21),u.Rotator(),False,True)
            player.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            pc.set_control_rotation(u.Rotator());aim(p+u.Vector(0,0,40),u.Vector(-520,-220,220))
            before=pc.get_world_state().export_state().gold
            u.SystemLibrary.execute_console_command(world,'ICGiveGold',pc)
            assert pc.get_world_state().export_state().gold==before+10000
            STATE['checks']['console_gold_adds_10000']=True
            inventory=pc.get_inventory();before_weapons={id:inventory.count_item(id) for id in ('EQP_WPN_SWORD_001','EQP_WPN_BOW_001','EQP_WPN_STAFF_001','EQP_WPN_WAND_001')}
            u.SystemLibrary.execute_console_command(world,'ICGiveStarterWeapons',pc)
            assert all(inventory.count_item(id)==count+1 for id,count in before_weapons.items())
            STATE['checks']['console_gives_four_starter_weapons']=True;phase('near');return
        if STATE['phase']=='near' and elapsed>1:
            capture('Machine_And_Gold');STATE['before_gold']=pc.get_world_state().export_state().gold
            pc.handle_input_command(u.ICInputCommand.INTERACT,0)
            assert pc.get_world_state().export_state().gold==STATE['before_gold']-1000
            STATE['checks']['F_draw_charges_1000']=True;phase('reward');return
        if STATE['phase']=='reward' and elapsed>1:
            capture('Machine_ItemReceived')
            inventory=pc.get_inventory();inventory.reset_inventory()
            for _ in range(len(inventory.get_snapshot().bag.slots)):
                assert inventory.add_item('ITM_R01_SEED_01',99,False)
            STATE['before_gold']=pc.get_world_state().export_state().gold
            pc.handle_input_command(u.ICInputCommand.INTERACT,0)
            assert pc.get_world_state().export_state().gold==STATE['before_gold']-1000
            drops=u.GameplayStatics.get_all_actors_of_class(world,u.ICDroppedItem)
            assert len(drops)==1,len(drops)
            drop=drops[0];assert drop.get_editor_property('mesh').get_editor_property('static_mesh').get_name()=='Cube'
            assert (drop.get_actor_location()-STATE['machine'].get_actor_location()).length()>120
            STATE['checks']['full_bag_creates_visible_cube_outside_machine']=True
            STATE['drop']=drop;aim(drop.get_actor_location(),u.Vector(260,-430,220));phase('overflow');return
        if STATE['phase']=='overflow' and elapsed>1:
            capture('Machine_FullBag_CubeDrop')
            STATE.update(phase='complete',complete=True);finish()
    except Exception:
        STATE.update(phase='error',error=traceback.format_exc());u.log_error(STATE['error']);finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
