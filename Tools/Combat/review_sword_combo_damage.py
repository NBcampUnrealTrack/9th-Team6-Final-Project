"""Measure real enemy HP for all sword stages, genders and close-range distances."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'SwordComboDamageLive'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'samples':[],'complete':False}


def report():
    (OUT/'Review.json').write_text(json.dumps({k:STATE[k] for k in ('phase','samples','complete','error') if k in STATE},indent=2),encoding='utf-8')


def phase(value):
    STATE.update(phase=value,phase_started=time.monotonic());report()


def finish():
    report();u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()


def hp():
    state=STATE['pc'].get_world_state()
    assert STATE['enemy'].flush_to_world_state(state)
    return state.export_state().spawn_slots[STATE['enemy'].get_character_id()].hp


def attack():
    result=STATE['asc'].request_action('Attack','None',None)
    assert result.success,str(result.reason)


def tick(delta):
    try:
        if time.monotonic()-STATE['started']>180:raise RuntimeError('Sword damage review timeout')
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(STATE['world'],0)
            inventory=pc.get_inventory()
            if inventory.get_weapon_loadout()[0].count==0:
                assert inventory.add_item('EQP_WPN_SWORD_001',1,False)
                slot=next(i for i,v in enumerate(inventory.get_snapshot().bag.slots) if str(v.item_id)=='EQP_WPN_SWORD_001')
                assert inventory.equip(slot)
            for _ in range(6):
                if inventory.get_equipped_weapon_family()==u.ICWeaponFamily.SWORD:break
                assert inventory.cycle_weapon()
            enemies=u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICEnemyBase)
            if len(enemies)<6:return
            enemies=sorted(enemies,key=lambda a:str(a.get_character_id()))
            for actor in enemies:
                ai=actor.get_controller()
                if ai:
                    brain=ai.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Sword combo damage review')
                    ai.set_actor_tick_enabled(False)
                actor.get_component_by_class(u.CharacterMovementComponent).set_component_tick_enabled(False)
                actor.set_actor_location(u.Vector(50000,50000,96),False,True)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            STATE.update(player=player,enemies=enemies,
                         asc=player.get_component_by_class(u.ICAbilitySystemComponent),camera=camera,case=0)
            pc.set_menu('');phase('next');return
        if STATE['phase']=='next':
            if STATE['case']>=6:
                STATE.update(phase='complete',complete=True);finish();return
            STATE['asc'].cancel_action();STATE['player'].apply_preset(STATE['case']//3)
            for enemy in STATE['enemies']:enemy.set_actor_location(u.Vector(50000,50000,96),False,True)
            STATE['enemy']=STATE['enemies'][STATE['case']]
            STATE['enemy_asc']=STATE['enemy'].get_component_by_class(u.ICAbilitySystemComponent)
            player=STATE['player'];distance=(90,130,170)[STATE['case']%3]
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True);pc.set_control_rotation(u.Rotator())
            STATE['enemy'].set_actor_location(u.Vector(-1000+distance,-2500,96),False,True)
            view=u.Vector(-1420,-2350,340);focus=u.Vector(-900,-2500,80)
            STATE['camera'].set_world_location_and_rotation(view,u.MathLibrary.find_look_at_rotation(view,focus),False,True)
            STATE.update(distance=distance,seen=0,last_hp=hp(),late_entry=False);phase('settle');return
        if STATE['phase']=='settle' and time.monotonic()-STATE['phase_started']>.9:
            attack();attack();STATE.update(stage=0,before=hp(),queued_third=False);phase('attack');return
        if STATE['phase']=='attack':
            anim=STATE['player'].get_editor_property('mesh').get_anim_instance();montage=anim.get_current_active_montage()
            stage=int(montage.get_name().rsplit('_',1)[1]) if montage and montage.get_name().startswith('AM_Combo_Sword_') else 0
            if stage==3 and STATE['distance']==170 and not STATE['late_entry'] and anim.montage_get_position(montage)>.78:
                STATE['enemy'].set_actor_location(STATE['player'].get_actor_location()+u.Vector(130,0,0),False,True)
                STATE['late_entry']=True
            if stage!=STATE['stage']:
                if STATE['stage']:
                    weapon=STATE['player'].get_editor_property('equipped_weapon_actor')
                    damage=STATE['before']-hp()
                    if STATE['distance']<170 or STATE['stage']==3:assert damage>0,(STATE['case'],STATE['stage'],damage)
                    else:assert damage==0,(STATE['case'],STATE['stage'],damage)
                    STATE['samples'].append({'gender':'Male' if STATE['case']<3 else 'Female','distance':STATE['distance'],
                                             'stage':STATE['stage'],'damage':damage,'entered_during_finisher':STATE['late_entry'],
                                             'reach':weapon.get_melee_reach_cm() if weapon else 0})
                    report();STATE['seen']+=1
                STATE.update(stage=stage,before=hp())
                if stage==2 and not STATE['queued_third']:attack();STATE['queued_third']=True
                if stage==0 and STATE['seen']==3:
                    STATE['case']+=1;phase('next')
    except Exception:
        STATE.update(phase='error',error=traceback.format_exc());u.log_error(STATE['error']);finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
