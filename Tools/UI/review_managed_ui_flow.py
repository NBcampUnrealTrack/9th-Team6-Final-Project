"""Rendered PIE review with isolated saves: minimap, map markers, and four save portraits."""
from pathlib import Path
import json,time,traceback
import unreal as u

out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'ManagedUIReview'
out.mkdir(parents=True,exist_ok=True)
state=dict(phase='starting',started=time.monotonic(),checks={})
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)

def phase(name):state.update(phase=name,at=time.monotonic())
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(state['world'],str(out/(name+'.png')))
def finish():
    (out/'LiveReview.json').write_text(json.dumps({k:state[k] for k in ('phase','checks','error') if k in state},ensure_ascii=False,indent=2),encoding='utf-8')
    if 'pc' in state:
        saves=state['pc'].get_saves()
        if not saves.get_editor_property('saving'):
            saves.delete_slot(0);saves.delete_slot(1)
    u.unregister_slate_post_tick_callback(state['handle']);levels.editor_request_end_play();u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        now=time.monotonic()
        if now-state['started']>120:raise RuntimeError('Review timed out: '+state['phase'])
        if state['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            state.update(world=worlds[0],pc=pc);phase('ready');return
        pc=state['pc'];elapsed=now-state['at']
        if state['phase']=='ready' and elapsed>2:
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(state['world']);phase('creating');return
        if state['phase']=='creating':
            if not pc.get_editor_property('created') or pc.get_saves().get_editor_property('saving'):return
            pc.set_menu('');phase('recruit');return
        if state['phase']=='recruit' and elapsed>.6:
            player=u.GameplayStatics.get_player_pawn(state['world'],0)
            npcs=u.GameplayStatics.get_all_actors_of_class(state['world'],u.ICNPCBase)
            for npc_id in ['NPC_R01_003','NPC_R01_004','NPC_R01_005']:
                npc=next(a for a in npcs if str(a.get_editor_property('NPCDefinitionID'))==npc_id)
                assert pc.get_world_state().set_affinity(npc_id,300)
                player.set_actor_location(npc.get_actor_location()+npc.get_actor_forward_vector()*180,False,True)
                assert npc.try_interact(pc);assert npc.try_recruit(pc);pc.set_menu('')
            state['checks']['three_companions_recruited']=True;phase('hud');return
        if state['phase']=='hud' and elapsed>1:
            capture('HUD_Minimap');pc.set_menu('Map');phase('map');return
        if state['phase']=='map' and elapsed>1:
            assert str(pc.get_editor_property('menu'))=='Map';capture('Map_WBP')
            state['checks']['map_opened']=True;pc.set_menu('');pc.open_save_menu(False);pc.select_session_slot(1);pc.request_manual_save();phase('saving');return
        if state['phase']=='saving' and elapsed>1:
            if pc.get_saves().get_editor_property('saving'):return
            capture('Save_FourPortraits');state['checks']['four_portrait_save_captured']=True;phase('complete');u.log('IC_MANAGED_UI_LIVE_READY');finish()
    except Exception:
        state['error']=traceback.format_exc();u.log_error(state['error']);phase('error');finish()

u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
state['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
