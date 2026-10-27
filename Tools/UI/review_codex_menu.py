"""Review codex WBPs using real Slate keys and an isolated PIE save prefix."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'CodexMenu'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}
def status():
    data={k:STATE[k] for k in ('phase','checks')}
    if 'error' in STATE:data['error']=STATE['error']
    (OUT/'LiveReview.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):STATE.update(phase=name,phase_started=time.monotonic());status()
def finish():
    u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def press(name):
    key=u.Key();key.set_editor_property('key_name',name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'],key),name
def keys(*names):
    for name in names:press(name)
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def tick(delta):
    try:
        if time.monotonic()-STATE['started']>150:raise RuntimeError('Codex review timeout in '+STATE['phase'])
        if STATE['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            STATE.update(world=worlds[0],pc=pc);phase('creating');return
        pc=STATE['pc'];elapsed=time.monotonic()-STATE['phase_started']
        if STATE['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('Pause')
            menu=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICGameMenuWidget,False)[0]
            assert menu.select_tab('Codex')
            ui=u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICCodexMenuWidget,False)[0]
            gi=pc.get_world_state().get_outer();codex=next(c for c in u.ObjectIterator(u.ICCodexSubsystem) if c.get_outer()==gi)
            inv=u.GameplayStatics.get_player_pawn(STATE['world'],0).get_editor_property('inventory')
            if inv.get_weapon_loadout()[0].count>0:assert inv.unequip_weapon(u.ICWeaponFamily.SWORD)
            STATE.update(menu=menu,ui=ui,codex=codex,inv=inv);phase('home');return
        ui=STATE['ui'];menu=STATE['menu'];codex=STATE['codex']
        if STATE['phase']=='home' and elapsed>.5:
            capture('CodexHome');press('Enter');assert ui.get_editor_property('category')==0
            STATE['checks']['keyboard_opens_right_category']=True;phase('skills');return
        if STATE['phase']=='skills' and elapsed>.4:
            capture('CodexSkills');assert len(ui.get_editor_property('cells'))==60
            keys('S','Enter');assert ui.get_editor_property('popup')==1
            STATE['checks']['twelve_by_five_skill_grid']=True;phase('skill_detail');return
        if STATE['phase']=='skill_detail' and elapsed>.3:
            capture('CodexSkillDetail');press('Enter');assert ui.get_editor_property('notice_open')
            STATE['checks']['missing_weapon_notice']=True;phase('weapon_notice');return
        if STATE['phase']=='weapon_notice' and elapsed>.3:
            capture('CodexWeaponNotice');press('Enter');assert not ui.get_editor_property('notice_open');assert ui.get_editor_property('popup')==1
            STATE['checks']['notice_returns_to_same_skill_popup']=True
            press('Escape');ui.select_category(1);assert len(ui.get_editor_property('cells'))==55
            assert codex.add_pet_essence(0,3);phase('pets_locked');return
        if STATE['phase']=='pets_locked' and elapsed>.3:
            capture('CodexPetsLocked');press('Enter');assert ui.get_editor_property('popup')==0
            STATE['checks']['locked_pet_no_popup']=True
            assert codex.add_pet_essence(0,97);assert codex.add_pet_essence(1,100)
            assert ui.open_cell(0);phase('pet_detail');return
        if STATE['phase']=='pet_detail' and elapsed>.3:
            capture('CodexPetDetail');press('Enter');assert ui.get_editor_property('popup')==2
            STATE['checks']['selector_only_actual_player']=len(codex.get_party_members())==1
            phase('pet_targets');return
        if STATE['phase']=='pet_targets' and elapsed>.3:
            capture('CodexPetTargets');press('Enter');assert ui.get_editor_property('popup')==3
            phase('pet_confirm');return
        if STATE['phase']=='pet_confirm' and elapsed>.3:
            capture('CodexPetConfirm');keys('A','Enter');assert codex.get_equipped_pet('CHR_PLAYER')==0
            STATE['checks']['keyboard_confirms_player_pet']=True
            press('Escape');assert ui.open_cell(1);keys('Enter','Enter');phase('pet_replace');return
        if STATE['phase']=='pet_replace' and elapsed>.3:
            capture('CodexPetReplace');press('Escape');assert codex.get_equipped_pet('CHR_PLAYER')==0
            STATE['checks']['replacement_cancel_preserves_pet']=True
            keys('Enter','A','Enter');assert codex.get_equipped_pet('CHR_PLAYER')==1
            STATE['checks']['replacement_confirm_applies_pet']=True
            press('Escape');assert ui.open_cell(0);press('Enter')
            state=pc.get_world_state();assert state.ensure_character('NPC_R01_002',u.ICCharacterKind.RECRUITABLE);assert state.set_affinity('NPC_R01_002',300);assert state.set_party_slot('NPC_R01_002',0)
            assert len(codex.get_party_members())==2;phase('companion_targets');return
        if STATE['phase']=='companion_targets' and elapsed>.3:
            capture('CodexCompanionTargets');keys('D','Enter','A','Enter');assert codex.get_equipped_pet('NPC_R01_002')==0
            STATE['checks']['keyboard_assigns_real_companion_pet']=True
            press('Escape');ui.select_category(2);phase('products');return
        if STATE['phase']=='products' and elapsed>.3:
            capture('CodexProducts');press('D');assert ui.get_editor_property('sub_tab')==1
            STATE['checks']['keyboard_switches_product_tabs']=True
            ui.select_category(3)
            for tab in range(3):
                ui.select_sub_tab(tab)
                unknown=next((i for i,id in enumerate(ui.get_editor_property('cells')) if not codex.is_recipe_known(id)),None)
                if unknown is not None:
                    assert not ui.open_cell(unknown);assert ui.get_editor_property('popup')==0
                    STATE['checks']['unknown_recipe_no_popup']=True
                    break
            phase('recipes');return
        if STATE['phase']=='recipes' and elapsed>.3:
            capture('CodexRecipes');phase('complete');return
        if STATE['phase']=='complete' and elapsed>.2:
            status();u.log('IC_CODEX_MENU_REVIEW_OK '+json.dumps(STATE['checks']));finish()
    except Exception:
        if 'world' in STATE:
            try:capture('CodexReviewError')
            except Exception:pass
        STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()

