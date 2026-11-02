"""Rendered PIE review using an isolated save namespace; does not save assets."""
import json
import copy
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
VFX_ONLY='-ICVFXOnly' in u.SystemLibrary.get_command_line()
OUT = ROOT / ('Saved/NPCSkillReview/VFX' if VFX_ONLY else 'Saved/NPCSkillReview/Live')
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
S = dict(phase='starting', started=time.monotonic(), checks={}, particles=[], complete=False)
SOURCE_AUDIT=json.loads((ROOT/'Saved/NPCSkillReview/Audit/Assets.json').read_text(encoding='utf-8')) if VFX_ONLY else None


def report():
    (OUT/'Review.json').write_text(json.dumps({k: S[k] for k in ('phase','checks','particles','complete','error') if k in S},
                                            ensure_ascii=False, indent=2), encoding='utf-8')


def phase(name):
    S.update(phase=name, phase_started=time.monotonic())
    report()


def finish():
    report()
    u.unregister_slate_post_tick_callback(S['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()


def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'], str(OUT/(name+'.png')))


def press(name):
    key=u.Key();key.set_editor_property('key_name',name)
    assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],key), name


def open_item(identifier):
    ui=S['inventory_ui'];ui.close_item();ui.select_item_tab(u.PlayerItemTab.NORMAL)
    items=[v for v in S['inventory'].get_snapshot().bag.slots if str(v.item_id)!='None' and v.count>0]
    index=next(i for i,v in enumerate(items) if str(v.item_id)==identifier)
    assert ui.open_item(index), (identifier,index)


def stop_ai(actor):
    ai=actor.get_controller()
    if ai:
        brain=ai.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('NPC expansion review')
        ai.set_actor_tick_enabled(False)
    movement=actor.get_component_by_class(u.CharacterMovementComponent)
    if movement:movement.set_component_tick_enabled(False)


def spawn_style(style, location):
    if not style['bEnabled'] or style['System']=='None':return None
    system=u.load_asset(style['System']);assert system
    scale=style['Scale'];scale=u.Vector(scale['X'],scale['Y'],scale['Z'])
    off=style['Offset'];location=location+u.Vector(off['X'],off['Y'],off['Z'])
    rotation=style['Rotation'];rotation=u.Rotator(rotation['Pitch'],rotation['Yaw'],rotation['Roll'])
    component=u.NiagaraFunctionLibrary.spawn_system_at_location(S['world'],system,location,rotation,scale,False,True,pre_cull_check=False)
    assert component
    color=style['Color'];intensity=style['Intensity']
    component.set_variable_linear_color('User.IC_Color',u.LinearColor(color['R']*intensity,color['G']*intensity,color['B']*intensity,color['A']))
    component.set_variable_vec3('User.IC_Scale',scale)
    duration=style['Duration'] or 2
    component.set_variable_float('User.IC_Duration',style['ReferenceDuration'] or duration)
    if style['ReferenceDuration']>0:component.set_custom_time_dilation(style['ReferenceDuration']/duration)
    return component


def tick(delta):
    try:
        now=time.monotonic()
        if now-S['started']>240:raise RuntimeError('Review timeout in '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=now-S['phase_started']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(S['world'],0)
            inventory=pc.get_inventory();state=pc.get_world_state()
            for command in ('ICRecruitTestParty','ICEquipStarterWeapons','ICGiveGold'):
                u.SystemLibrary.execute_console_command(S['world'],command,pc)
            assert all(v.count>0 for v in inventory.get_weapon_loadout())
            for i in range(3):
                npc=state.get_snapshot(f'NPC_R01_{i+1:03d}')
                assert npc.affinity==300 and npc.party_slot==i
            S['checks']['party_cheat_and_four_equipped_weapons']=True
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            camera.set_world_location_and_rotation(u.Vector(-1500,-2860,310),u.MathLibrary.find_look_at_rotation(u.Vector(-1500,-2860,310),u.Vector(-850,-2500,95)),False,True)
            S.update(player=player,inventory=inventory,state=state,camera=camera)
            pc.set_menu('Pause')
            menu=u.WidgetLibrary.get_all_widgets_of_class(S['world'],u.ICGameMenuWidget,False)[0]
            assert menu.select_tab('Inventory')
            ui=u.WidgetLibrary.get_all_widgets_of_class(S['world'],u.ICInventoryMenuWidget,False)[0]
            S.update(menu=menu,inventory_ui=ui)
            for identifier,count in [('ITM_ELIXIR_001',2),('ITM_R01_FOOD_01',2),('ITM_CONS_001',2)]:assert inventory.add_item(identifier,count,False)
            assert len(ui.get_editor_property('members'))==4
            gold=str(ui.get_editor_property('TXT_Gold').get_text())
            assert int(''.join(c for c in gold if c.isdigit()))==state.export_state().gold,gold
            S['checks']['inventory_live_gold']=gold
            if VFX_ONLY:
                pc.set_menu('')
                for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCharacterBase):
                    if a!=player:stop_ai(a);a.set_actor_location(u.Vector(50000,50000,100),False,True)
                table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX')
                S['visuals']={r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))}
                S.update(vfx_index=1,components=[]);phase('vfx_next');return
            phase('inventory');return
        ui=S['inventory_ui'];inv=S['inventory'];state=S['state']
        if S['phase']=='inventory' and elapsed>.6:
            capture('InventoryGoldAndParty');open_item('ITM_ELIXIR_001');phase('book');return
        if S['phase']=='book' and elapsed>.35:
            capture('GrowthElixirUse');press('Enter')
            assert ui.get_editor_property('picking_use_target')
            assert len(ui.get_editor_property('UseTargets').get_editor_property('members'))==4
            S['book_str']=state.get_snapshot('NPC_R01_002').main_stats.str
            S['book_count']=inv.count_item('ITM_ELIXIR_001')
            phase('targets');return
        if S['phase']=='targets' and elapsed>.4:
            capture('UsePartyTargets');press('D');press('D');press('Enter')
            assert not ui.get_editor_property('picking_use_target')
            assert state.get_snapshot('NPC_R01_002').main_stats.str==S['book_str']+1
            assert inv.count_item('ITM_ELIXIR_001')==S['book_count']-1
            S['checks']['book_use_keyboard_selects_companion']=True
            open_item('ITM_R01_FOOD_01');press('Enter')
            before=inv.count_item('ITM_R01_FOOD_01')
            assert not ui.choose_use_target(2)
            assert inv.count_item('ITM_R01_FOOD_01')==before
            S['checks']['ineffective_food_is_not_consumed']=True
            ui.cancel_use_target();ui.close_item();open_item('ITM_CONS_001')
            press('D');press('Enter');assert ui.get_editor_property('picking_quick_slot')
            press('Three');assert ui.get_editor_property('confirming');press('A');press('Enter')
            assert str(inv.get_snapshot().quick_slots[2])=='ITM_CONS_001'
            S['checks']['quick_slot_number_registration']=True
            assert S['menu'].select_tab('Codex')
            codex=u.WidgetLibrary.get_all_widgets_of_class(S['world'],u.ICCodexMenuWidget,False)[0]
            codex.select_category(4);S['codex_ui']=codex;phase('codex');return
        if S['phase']=='codex' and elapsed>.5:
            codex=S['codex_ui'];assert len(codex.get_editor_property('cells'))>=11
            index=next(i for i,v in enumerate(codex.get_editor_property('cells')) if str(v)=='NPC_R01_001')
            capture('NPCCodexAffinity');assert codex.open_cell(index);phase('npc_detail');return
        if S['phase']=='npc_detail' and elapsed>.4:
            popup=S['codex_ui'].get_editor_property('DetailPopup')
            description=str(popup.get_editor_property('TXT_Description').get_text());assert '300' in description,description
            capture('NPCCodexDetail');S['checks']['npc_codex_current_affinity']=True
            pc.set_menu('')
            enemies=list(u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICEnemyBase))
            assert len(enemies)>=2
            for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCharacterBase):
                if a!=S['player']:stop_ai(a);a.set_actor_location(u.Vector(50000,50000,100),False,True)
            S.update(enemies=enemies)
            for i,a in enumerate(enemies[:2]):a.set_actor_location(u.Vector(-550,-2500+i*160,96),False,True)
            pc.set_control_rotation(u.Rotator())
            phase('lock_ready');return
        if S['phase']=='lock_ready' and elapsed>.4:
            pc.handle_input_command(u.ICInputCommand.NEXT_TARGET,0)
            assert pc.get_locked_combat_target()
            S['lock_first']=pc.get_locked_combat_target();phase('lock');return
        if S['phase']=='lock' and elapsed>.5:
            capture('MonsterLockOn')
            pc.handle_input_command(u.ICInputCommand.NEXT_TARGET,0)
            assert pc.get_locked_combat_target() and pc.get_locked_combat_target()!=S['lock_first']
            pc.handle_input_command(u.ICInputCommand.NEXT_TARGET,0);assert not pc.get_locked_combat_target()
            S['checks']['lock_cycles_two_monsters_then_clears']=True
            for a in S['enemies']:a.set_actor_location(u.Vector(50000,50000,100),False,True)
            table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX')
            S['visuals']={r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))}
            S.update(vfx_index=1,components=[]);phase('vfx_next');return
        if S['phase']=='vfx_next':
            for c in S['components']:c.destroy_component(c)
            S['components']=[]
            if S['vfx_index']>11:
                S.update(complete=True);phase('complete');return
            i=S['vfx_index'];row=S['visuals'][f'NPC_R01_{i:03d}']
            styles=[row['Cast']]+row['CastLayers']+[row['Impact']]+row['ImpactLayers']
            if VFX_ONLY:
                original=copy.deepcopy(row['Cast']);original.update(System=SOURCE_AUDIT['systems'][i-1]['source'][0],Color=dict(R=1,G=1,B=1,A=1),Intensity=1,Scale=dict(X=1,Y=1,Z=1))
                styles=[original,row['Cast'],row['Impact']]
            origin=u.Vector(-750,-2500,120)
            view=u.Vector(-1160,-2750,290) if VFX_ONLY else u.Vector(-1550,-3100,460)
            S['camera'].set_world_location_and_rotation(view,u.MathLibrary.find_look_at_rotation(view,origin),False,True)
            for index,style in enumerate(styles):
                c=spawn_style(style,origin+u.Vector(0,(index-1)*180 if VFX_ONLY else 150 if index==len(styles)-1 else -100,0))
                if c:S['components'].append(c)
            S['early_capture']=False;phase('vfx_wait');return
        if S['phase']=='vfx_wait' and elapsed>.08 and not S['early_capture']:
            capture(f'VFX_R01_{S["vfx_index"]:02d}_Early');S['early_capture']=True
        if S['phase']=='vfx_wait' and elapsed>.5:
            counts=[u.ICCombatVFXSetupLibrary.get_combat_particle_count(c) for c in S['components']]
            descriptions=[u.ICCombatVFXSetupLibrary.describe_combat_particles(c) for c in S['components']]
            S['particles'].append(dict(npc=S['vfx_index'],counts=counts,descriptions=descriptions))
            capture(f'VFX_R01_{S["vfx_index"]:02d}')
            S['vfx_index']+=1;phase('vfx_hold');return
        if S['phase']=='vfx_hold' and elapsed>.4:phase('vfx_next');return
        if S['phase']=='complete' and elapsed>.4:
            u.log('NPC_EXPANSION_LIVE_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error'])
        if 'world' in S:
            try:capture('Error')
            except Exception:pass
        finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
