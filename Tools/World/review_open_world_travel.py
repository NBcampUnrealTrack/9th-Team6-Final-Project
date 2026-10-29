"""PIE-only travel review with an isolated save namespace and real HUD screenshots."""
import json
import time
import traceback
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/OpenWorldUpgrade/Live'
OUT.mkdir(parents=True,exist_ok=True)
LEVEL=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'start','since':time.monotonic(),'started':time.monotonic(),'index':0,'checks':{},'trips':[]}


def phase(value):
    STATE.update(phase=value,since=time.monotonic())
    u.log('IC_WARP_REVIEW_PHASE '+value)


def shot(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))


def map_ui():
    return next(w for w in u.ObjectIterator(u.ICMapWidget) if w.get_owning_player()==STATE['pc'] and not w.get_editor_property('bMinimap') and (w.get_parent() or w.is_in_viewport()))


def finish(error=None):
    if error:STATE['error']=error
    report={k:STATE[k] for k in ('phase','checks','trips','complete','error') if k in STATE}
    (OUT/'Review.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(STATE['callback'])
    LEVEL.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        if time.monotonic()-STATE['started']>300:raise RuntimeError('Travel review timeout: '+STATE['phase'])
        elapsed=time.monotonic()-STATE['since']
        if STATE['phase']=='start':
            world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
            if world.get_name()!='L_OpenWorld':u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
            phase('create');assert u.ICCharacterSetupLibrary.begin_monster_test_review();return
        if STATE['phase']=='create':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or not pc.get_editor_property('Screen') or str(pc.get_editor_property('Menu'))!='Create':return
            STATE.update(world=worlds[0],pc=pc)
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        pc=STATE['pc']
        if STATE['phase']=='created':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(STATE['world'],0)
            STATE.update(player=player,warp=player.get_editor_property('PartyWarp'))
            STATE['portals']=sorted([a for a in u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICWarpStone) if str(a.get_editor_property('WarpID')).startswith('WRP_OW_')],key=lambda a:str(a.get_editor_property('WarpID')))
            assert len(STATE['portals'])==4
            unlocked=[STATE['warp'].is_warp_unlocked(a.get_editor_property('WarpID')) for a in STATE['portals']]
            assert unlocked==[True,False,False,False],unlocked
            assert pc.set_gold(6000)
            for enemy in u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICEnemyBase):
                controller=enemy.get_controller()
                if controller:
                    brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Isolated warp UI review')
                    controller.stop_movement();controller.set_actor_tick_enabled(False)
            u.SystemLibrary.execute_console_command(STATE['world'],'ICRecruitTestParty',pc)
            STATE['checks']['initial_home_only']=unlocked;phase('party');return
        if STATE['phase']=='party' and elapsed>2 and not pc.get_editor_property('bInCombat'):
            pc.set_menu('Map');phase('locked_map');return
        if STATE['phase']=='locked_map' and elapsed>.7:
            shot('01_Map_HomeOnly')
            assert not map_ui().request_travel('WRP_OW_02_INN')
            STATE['checks']['locked_destination_rejected']=True
            phase('approach');return
        if STATE['phase']=='approach':
            portal=STATE['portals'][STATE['index']]
            pc.set_menu('')
            point=portal.get_actor_location()+u.Vector(180,0,100)
            STATE['player'].set_actor_location_and_rotation(point,u.Rotator(yaw=180),False,True)
            assert portal.try_interact(pc)
            assert STATE['warp'].is_warp_unlocked(portal.get_editor_property('WarpID'))
            phase('opened');return
        if STATE['phase']=='opened' and elapsed>.5:
            shot('Map_Opened_'+str(STATE['index']+1))
            assert map_ui().request_travel(STATE['portals'][STATE['index']].get_editor_property('WarpID'))
            phase('travel');return
        if STATE['phase']=='travel':
            if elapsed>.15 and not STATE.get('loading_captured'):
                shot('02_Travel_Loading');STATE['loading_captured']=True
            if str(pc.get_editor_property('Menu'))=='Map':
                if elapsed>8:
                    shot('Travel_Failure')
                    labels=[str(w.get_text()) for w in u.ObjectIterator(u.TextBlock) if w.get_name()=='TXT_MapStatus' and w.get_world()==STATE['world']]
                    raise RuntimeError('Travel did not finish: '+str(dict(traveling=map_ui().is_traveling(),status=labels,notice=str(pc.get_editor_property('Notice')))))
                return
            if elapsed<1.4:return
            point=STATE['player'].get_actor_location()
            expected=json.loads((ROOT/'Saved/OpenWorldUpgrade/Authored.json').read_text(encoding='utf-8'))['destinations'][STATE['index']]['arrival']
            assert point.distance(u.Vector(*expected))<180,(point,expected)
            STATE['trips'].append(dict(id=str(STATE['portals'][STATE['index']].get_editor_property('WarpID')),arrival=[point.x,point.y,point.z],gold=pc.get_editor_property('Gold')))
            pc.set_control_rotation(u.Rotator(pitch=-20,yaw=180));phase('arrival');return
        if STATE['phase']=='arrival' and elapsed>.8:
            shot('Arrival_'+str(STATE['index']+1))
            STATE['index']+=1
            if STATE['index']<4:phase('approach')
            else:pc.set_menu('Map');phase('complete_map')
            return
        if STATE['phase']=='complete_map' and elapsed>.7:
            shot('03_Map_AllUnlocked')
            assert pc.get_editor_property('Gold')==4000
            STATE['checks']['four_successful_trips']=True
            STATE['complete']=True;finish()
    except Exception:
        error=traceback.format_exc();u.log_error(error);finish(error)
    finally:STATE['busy']=False


STATE['callback']=u.register_slate_post_tick_callback(tick)
