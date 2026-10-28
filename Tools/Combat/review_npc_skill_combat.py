"""Real casts and sixteen weapon appearances in isolated PIE. Never saves DT edits."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/NPCSkillReview/Combat';OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
S=dict(phase='starting',started=time.monotonic(),weapons=[],skills=[],complete=False)
SUPPORT_ONLY='-ICSupportOnly' in u.SystemLibrary.get_command_line()
SKILLS={r['id']:r for r in json.loads((ROOT/'Saved/NPCSkillReview/Setup.json').read_text(encoding='utf-8'))['skills']}
FAMILIES=(u.ICWeaponFamily.SWORD,u.ICWeaponFamily.BOW,u.ICWeaponFamily.STAFF,u.ICWeaponFamily.WAND)
NAMES=('SWORD','BOW','STAFF','WAND')


def report():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','weapons','skills','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')


def phase(name):S.update(phase=name,phase_started=time.monotonic());report()


def finish():
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()


def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))


def hp():
    assert S['enemy'].flush_to_world_state(S['state'])
    return S['state'].export_state().spawn_slots[S['enemy'].get_character_id()].hp


def stop(actor):
    ai=actor.get_controller()
    if ai:
        brain=ai.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('NPC skill damage review')
        ai.set_actor_tick_enabled(False)
    movement=actor.get_component_by_class(u.CharacterMovementComponent)
    if movement:movement.set_component_tick_enabled(False)


def arm(family):
    inv=S['inventory']
    for _ in range(6):
        if inv.get_equipped_weapon_family()==family:return
        assert inv.cycle_weapon()
    raise AssertionError('Could not arm '+str(family))


def tick(delta):
    try:
        now=time.monotonic()
        if now-S['started']>240:raise RuntimeError('Combat review timeout '+S['phase'])
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
            enemies=list(u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICEnemyBase))
            if not enemies:return
            state=pc.get_world_state();player=u.GameplayStatics.get_player_pawn(S['world'],0)
            inv=pc.get_inventory()
            u.SystemLibrary.execute_console_command(S['world'],'ICEquipStarterWeapons',pc)
            player_id=player.get_character_id()
            for stat in (u.ICMainStat.STR,u.ICMainStat.DEX,u.ICMainStat.INT,u.ICMainStat.CON):assert state.set_main_stat(player_id,stat,40)
            snap=state.get_snapshot(player_id)
            snap.set_editor_property('UnlockedSkillIDs',set(list(snap.get_editor_property('UnlockedSkillIDs'))+[u.Name(i) for i in SKILLS]))
            snap.set_editor_property('Equipment',[])
            assert state.set_character_state(snap)
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            focus=u.Vector(-1000,-2500,95);location=focus+u.Vector(310,300,70)
            camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,focus),False,True)
            S.update(state=state,player=player,inventory=inv,camera=camera,weapon_index=0,asc=player.get_component_by_class(u.ICAbilitySystemComponent))
            for actor in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCharacterBase):
                if actor!=player:stop(actor);actor.set_actor_location(u.Vector(50000,50000,100),False,True)
            S['enemy']=enemies[0]
            if SUPPORT_ONLY:S['skill_index']=3;phase('skill_prepare')
            else:phase('weapon_next')
            return
        inv=S['inventory'];player=S['player'];state=S['state']
        if S['phase']=='weapon_next':
            i=S['weapon_index']
            if i==16:S.update(skill_index=1);phase('skill_prepare');return
            family=FAMILIES[i//4];tier=i%4+2;identifier=f'EQP_WPN_{NAMES[i//4]}_{tier:03d}'
            assert inv.add_item(identifier,1,False),identifier
            slot=next(i for i,v in enumerate(inv.get_snapshot().bag.slots) if str(v.item_id)==identifier)
            assert inv.equip(slot),identifier
            arm(family);player.refresh_weapon_appearance();S['weapon_id']=identifier
            # Keep the isolated review bag small while checking all sixteen meshes.
            for index,value in enumerate(inv.get_snapshot().bag.slots):
                if str(value.item_id).startswith('EQP_WPN_'):inv.trash_item(index)
            phase('weapon_capture');return
        if S['phase']=='weapon_capture' and elapsed>.25:
            weapon=player.get_editor_property('equipped_weapon_actor');assert weapon and not weapon.get_editor_property('hidden')
            mesh=weapon.get_component_by_class(u.StaticMeshComponent).get_editor_property('static_mesh');assert mesh
            S['weapons'].append(dict(id=S['weapon_id'],mesh=mesh.get_path_name(),length=weapon.get_scaled_length_cm()))
            capture(S['weapon_id']);S['weapon_index']+=1;phase('weapon_next');return
        if S['phase']=='skill_prepare':
            index=S['skill_index']
            if index>11:S.update(complete=True);phase('complete');return
            identifier=f'SKL_NPC_R01_{index:03d}';asc=S['asc'];asc.cancel_action()
            arm(asc.get_skill_weapon_family(identifier));player.refresh_weapon_appearance()
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,96),u.Rotator(),False,True)
            S['enemy'].set_actor_location(u.Vector(-730,-2500,96),False,True)
            loc=u.Vector(-1450,-2850,310);focus=u.Vector(-780,-2500,100)
            S['camera'].set_world_location_and_rotation(loc,u.MathLibrary.find_look_at_rotation(loc,focus),False,True)
            pc.set_control_rotation(u.Rotator());pc.handle_input_command(u.ICInputCommand.CLEAR_TARGET,0)
            snap=state.get_snapshot(player.get_character_id());assert state.set_vitals(player.get_character_id(),snap.max_hp*.25,snap.max_mp,snap.max_sp)
            S.update(skill_id=identifier,last_hp=hp(),damage_events=[],follow_seen=False)
            phase('skill_ready');return
        if S['phase']=='skill_ready' and elapsed>.3:
            pc.handle_input_command(u.ICInputCommand.NEXT_TARGET,0)
            assert pc.get_locked_combat_target()==S['enemy']
            target=player if S['skill_index'] in (3,9,10) else S['enemy']
            result=S['asc'].request_action('Skill',S['skill_id'],target);assert result.success,str(result.reason)
            assert not S['asc'].notify_combat_event(result.action_id,'AN_HitStart',0),'Early montage bypassed DT cast time'
            assert player.flush_to_world_state(state)
            S['player_before_hp']=state.get_snapshot(player.get_character_id()).hp
            S['before_hp']=hp();phase('casting');return
        if S['phase']=='casting':
            current=hp()
            if current<S['last_hp']:
                S['damage_events'].append(S['last_hp']-current);S['last_hp']=current
                effects=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICAttackEffect)
                S['follow_seen']|=any(e.get_attach_parent_actor()==S['enemy'] for e in effects)
            if elapsed>1.9:
                index=S['skill_index'];damage=S['before_hp']-current
                if index not in (3,9,10):assert damage>0,(S['skill_id'],damage)
                assert S['asc'].get_skill_cooldown_remaining(S['skill_id'])>0
                if index in (8,11):assert len(S['damage_events'])==3,(index,S['damage_events'])
                if index not in (3,9,10):assert S['follow_seen'],'No attached hit effect'
                assert player.flush_to_world_state(state)
                snap=state.get_snapshot(player.get_character_id())
                effects={str(e.effect_id):e.remaining_shield for e in snap.combat.effects}
                if index==3:assert snap.hp>S['player_before_hp'],'Healing did not restore HP'
                if index==9:assert 'STE_NPC_R01_009' in effects,'Heal-over-time missing'
                if index==10:assert effects.get('STE_NPC_R01_010',0)>0,'Shield missing'
                capture(S['skill_id']);S['skills'].append(dict(id=S['skill_id'],damage=damage,events=S['damage_events'],hit_follows=S['follow_seen'],player_hp_before=S['player_before_hp'],player_hp_after=snap.hp,statuses=effects))
                S['skill_index']={3:9,9:10,10:12}[index] if SUPPORT_ONLY else index+1
                phase('skill_prepare');return
        if S['phase']=='complete' and elapsed>.3:u.log('NPC_SKILL_COMBAT_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error'])
        if 'world' in S:
            try:capture('Error')
            except Exception:pass
        finish()


assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
# Give review targets enough HP for the three-hit ultimate. These edits stay in memory.
monsters=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(monsters))
for row in rows:row['CombatStats']['MaxHP']=100000;row['CombatStats']['Defense']=0
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(monsters,json.dumps(rows))
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
