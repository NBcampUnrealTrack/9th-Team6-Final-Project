"""Rendered PIE checks for companion sprint and equipped pet followers; isolated saves."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'PartyFollow/Live'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
S=dict(phase='starting',start=time.monotonic(),checks={})

def report():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')

def phase(name):
    S.update(phase=name,at=time.monotonic());report()

def key(name,pressed):
    value=u.Key();value.set_editor_property('key_name',name)
    return u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],value,pressed)

def finish():
    if 'world' in S:
        for name in ('W','LeftShift'):key(name,False)
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))

def followers():
    return u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICPetFollower)

def tick(delta):
    try:
        if time.monotonic()-S['start']>150:raise RuntimeError('Party follow timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['at']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            pc.set_menu('')
            for command in ('ICRecruitTestParty','ICGiveAllPetEssence','ICEquipTestPets'):
                u.SystemLibrary.execute_console_command(S['world'],command,pc)
            player=u.GameplayStatics.get_player_pawn(S['world'],0)
            player.set_actor_location_and_rotation(u.Vector(-1000,-2500,100),u.Rotator(),False,True)
            S['player']=player;phase('party_spawn');return
        if S['phase']=='party_spawn' and elapsed>.5:
            player=S['player']
            actors=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCharacterBase)
            companions=sorted([a for a in actors if str(a.get_character_id()) in ('NPC_R01_001','NPC_R01_002','NPC_R01_003')],key=lambda a:str(a.get_character_id()))
            if len(companions)!=3:
                assert elapsed<15,len(companions)
                return
            for i,npc in enumerate(companions):
                npc.set_actor_location_and_rotation(u.Vector(-1300,-2650+i*150,100),u.Rotator(),False,True)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            camera.set_field_of_view(70)
            S.update(player=player,companions=companions,camera=camera,original_caps=[n.get_component_by_class(u.CharacterMovementComponent).get_editor_property('max_walk_speed') for n in companions])
            phase('pets');return
        if S['phase']=='pets' and elapsed>1:
            pets=followers();assert len(pets)==4,len(pets)
            assert len({str(p.get_editor_property('character_id')) for p in pets})==4
            assert all(p.get_editor_property('visual').get_editor_property('skeletal_mesh_asset') for p in pets)
            S['checks']['four_equipped_pets_use_existing_meshes']=True
            S['original_caps']=[n.get_component_by_class(u.CharacterMovementComponent).get_editor_property('max_walk_speed') for n in S['companions']]
            S['multiplier']=S['player'].get_editor_property('sprint_speed_multiplier')
            S['player_pet']=next(p for p in pets if str(p.get_editor_property('character_id'))=='CHR_PLAYER')
            camera=S['camera'];at=S['player'].get_actor_location()
            camera.set_world_location_and_rotation(at+u.Vector(-750,-1000,650),u.MathLibrary.find_look_at_rotation(at+u.Vector(-750,-1000,650),at+u.Vector(-150,0,0)),False,True)
            phase('idle_capture');return
        if S['phase']=='idle_capture' and elapsed>.3:
            capture('Pets_Idle')
            S['pet_before']=S['player_pet'].get_editor_property('animation_switch_count')
            key('W',True);key('LeftShift',True);phase('sprint');return
        if S['phase']=='sprint':
            at=S['player'].get_actor_location();camera=S['camera']
            camera.set_world_location_and_rotation(at+u.Vector(-750,-1050,700),u.MathLibrary.find_look_at_rotation(at+u.Vector(-750,-1050,700),at+u.Vector(-330,0,0)),False,True)
            if elapsed<3:return
            details=[]
            for npc in S['companions']:
                anim=npc.get_component_by_class(u.SkeletalMeshComponent).get_anim_instance()
                movement=npc.get_component_by_class(u.CharacterMovementComponent)
                details.append(dict(id=str(npc.get_character_id()),speed=npc.get_velocity().length(),cap=movement.get_editor_property('max_walk_speed'),sprinting=anim.get_editor_property('sprinting')))
            S['checks']['sprint_samples']=details
            assert all(abs(v['cap']-S['original_caps'][i]*S['multiplier'])<.1 for i,v in enumerate(details)),details
            assert all(v['sprinting'] for v in details),details
            assert len(followers())==4
            S['checks']['companions_apply_sprint_multiplier_to_own_stats']=True
            S['checks']['player_sprint_speed']=S['player'].get_velocity().length()
            S['pet_sprint_end']=S['player_pet'].get_editor_property('animation_switch_count')
            switches=S['pet_sprint_end']-S['pet_before']
            assert switches<=1,switches
            assert S['player_pet'].get_editor_property('moving')
            S['checks']['pet_move_loop_switches_during_sprint']=switches
            capture('Party_Sprinting_WithPets');key('LeftShift',False);phase('walk');return
        if S['phase']=='walk' and elapsed>.4:
            for npc in S['companions']:
                assert not npc.get_component_by_class(u.SkeletalMeshComponent).get_anim_instance().get_editor_property('sprinting')
            S['checks']['release_sprint_returns_to_normal']=True
            assert S['player_pet'].get_editor_property('animation_switch_count')==S['pet_sprint_end']
            S['checks']['pet_keeps_same_loop_when_owner_slows']=True
            key('W',False);phase('settle');return
        if S['phase']=='settle' and elapsed>2:
            assert not S['player_pet'].get_editor_property('moving')
            S['pet_idle']=S['player_pet'].get_editor_property('animation_switch_count')
            assert S['pet_idle']-S['pet_sprint_end']<=1
            phase('complete');return
        if S['phase']=='complete' and elapsed>.5:
            assert S['player_pet'].get_editor_property('animation_switch_count')==S['pet_idle']
            S['checks']['pet_settles_without_restarting_loops']=True
            capture('Pets_AfterMovement');u.log('IC_PARTY_FOLLOW_LIVE_OK '+json.dumps(S['checks']));finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
