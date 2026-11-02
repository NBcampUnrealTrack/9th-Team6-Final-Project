"""Capture the elixir inventory/filter and exercise permanent growth in isolated PIE."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/GrowthElixirs/Live';OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
S=dict(phase='starting',started=time.monotonic(),complete=False,checks={})

def report():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ['phase','complete','checks','error'] if k in S},ensure_ascii=False,indent=2),encoding='utf-8')

def finish():
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))

def tick(delta):
    try:
        if time.monotonic()-S['started']>90:raise RuntimeError('Elixir review timeout '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            S.update(world=worlds[0],pc=pc,phase='creating');return
        pc=S['pc']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(S['world'],0);inv=pc.get_inventory()
            for n in list(range(1,13))+[16,17,18]:assert inv.add_item(f'ITM_ELIXIR_{n:03d}',1,False)
            pc.set_menu('Pause')
            menu=u.WidgetLibrary.get_all_widgets_of_class(S['world'],u.ICGameMenuWidget,False)[0]
            assert menu.select_tab('Inventory')
            ui=u.WidgetLibrary.get_all_widgets_of_class(S['world'],u.ICInventoryMenuWidget,False)[0]
            ui.refresh_inventory()
            index=next(i for i,v in enumerate(inv.get_snapshot().bag.slots) if str(v.item_id)=='ITM_ELIXIR_010')
            assert ui.open_item(index)
            S.update(player=player,inv=inv,ui=ui,phase='detail',ready=time.monotonic()+.4);return
        if time.monotonic()<S['ready']:return
        if S['phase']=='detail':
            capture('ConstitutionElixir')
            state=pc.get_world_state();identifier=S['player'].get_character_id()
            before=state.get_snapshot(identifier).main_stats.con;count=S['inv'].count_item('ITM_ELIXIR_010')
            pc.use_inventory_item('ITM_ELIXIR_010',identifier)
            assert state.get_snapshot(identifier).main_stats.con==before+1
            assert S['inv'].count_item('ITM_ELIXIR_010')==count-1
            S['checks']['constitution_elixir_increases_CON_once']=True
            S['ui'].close_item()
            for _ in range(4):S['ui'].cycle_filter()
            S.update(phase='filter',ready=time.monotonic()+.3);return
        if S['phase']=='filter':
            capture('ElixirCategory')
            S['checks']['fifteen_elixir_types_in_inventory']=True
            S.update(phase='complete',complete=True);finish()
    except Exception:
        S.update(phase='error',error=traceback.format_exc());u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
