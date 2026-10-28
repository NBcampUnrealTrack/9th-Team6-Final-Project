"""Rendered PIE, isolated saves: live dialogue/movement, NPC beds, clock and audible BGM."""
import unreal as u,time,json,pathlib,traceback
out=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'LiveDialogueMusic';out.mkdir(exist_ok=True)
S=dict(phase='starting',started=time.monotonic(),checks={},complete=False);levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
def report():(out/'LiveReview.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(p):S.update(phase=p,phase_started=time.monotonic());report()
def finish():
    if 'world' in S:
        key=u.Key();key.set_editor_property('key_name','S');u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],key,False)
    report();u.unregister_slate_post_tick_callback(S['handle']);levels.editor_request_end_play();u.SystemLibrary.quit_editor()
def key(name,pressed=None):
    value=u.Key();value.set_editor_property('key_name',name)
    return u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],value) if pressed is None else u.ICCharacterSetupLibrary.set_ui_review_key(S['world'],value,pressed)
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(out/(name+'.png')))
def stop(npc):
    controller=npc.get_controller()
    if controller:
        brain=controller.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Live presentation review')
        controller.stop_movement();controller.set_actor_tick_enabled(False)
    npc.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
def approach(npc):
    point=npc.get_actor_location();position=point+npc.get_actor_forward_vector()*190
    S['player'].set_actor_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,point),False,True)
    S['pc'].set_control_rotation(u.MathLibrary.find_look_at_rotation(position,point))
def tick(delta):
    try:
        if time.monotonic()-S['started']>200:raise RuntimeError('Timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='creating':
            npcs=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase)
            if not pc.get_editor_property('created') or len(npcs)<11:return
            player=u.GameplayStatics.get_player_pawn(S['world'],0)
            for npc in npcs:stop(npc)
            npc=next(a for a in npcs if str(a.get_editor_property('NPCDefinitionID'))=='NPC_R01_001')
            beds=[a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICLifeObjectActor) if a.get_editor_property('ObjectType')==u.ICLifeObjectType.BED]
            assert len(beds)==11;bed=next(a for a in beds if str(a.get_editor_property('OwnerNPCID'))=='NPC_R01_001')
            assert bed.get_slot_count()==1 and bed.is_smart_object_ready()
            music=next(o for o in u.ObjectIterator(u.ICBGMSubsystem) if o.get_outer()==S['world'])
            S.update(player=player,npc=npc,bed=bed,music=music,clock=pc.get_time(),state=pc.get_world_state(),standing=npc.get_actor_location())
            pc.set_menu('');approach(npc);phase('approach');return
        if S['phase']=='approach' and elapsed>1:
            S['clock_before']=S['clock'].get_clock().play_second;pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('dialogue');return
        if S['phase']=='dialogue' and elapsed>2:
            assert str(pc.get_editor_property('menu'))=='Dialogue' and not u.GameplayStatics.is_game_paused(S['world'])
            assert S['clock'].get_clock().play_second>S['clock_before']+1
            assert isinstance(pc.get_view_target(),u.CameraActor)
            S['checks']['dialogue_keeps_world_and_clock_running']=True;S['camera_before']=pc.get_view_target().get_actor_location()
            S['npc'].set_actor_location(S['npc'].get_actor_location()+u.Vector(0,30,0),False,True);phase('follow');return
        if S['phase']=='follow' and elapsed>.8:
            assert (pc.get_view_target().get_actor_location()-S['camera_before']).length()>10
            S['checks']['face_camera_follows_live_npc']=True;capture('LiveDialogue_Clock');S['walk_start']=S['player'].get_actor_location();key('S',True);phase('walking');return
        if S['phase']=='walking':
            if str(pc.get_editor_property('menu'))=='Dialogue':
                if elapsed>5:raise RuntimeError('Held S did not move player away from dialogue')
                return
            key('S',False);assert (S['player'].get_actor_location()-S['walk_start']).length()>250
            assert pc.get_view_target()==S['player'];S['checks']['real_s_key_walks_and_distance_closes_dialogue']=True;capture('Dialogue_ReturnToPlay')
            S['music'].set_music_override('BGM_Area_01');phase('music');return
        if S['phase']=='music' and elapsed>1:
            music=S['music'];assert str(music.get_current_music_row())=='BGM_Area_01'
            audio=music.get_music_component();assert audio and audio.is_playing(),'BGM audio must actually play'
            assert audio.get_editor_property('sound_class_override')==u.load_asset('/Game/IsekaiClass/Audio/Settings/SC_Music')
            S['checks']['dt_music_plays_on_music_volume_channel']=True
            pc.set_actor_tick_enabled(False);pc.set_editor_property('bInCombat',True);phase('combat_music');return
        if S['phase']=='combat_music' and elapsed>.7:
            assert str(S['music'].get_current_music_row())=='BGM_Combat';pc.set_editor_property('bInCombat',False);pc.set_actor_tick_enabled(True);phase('music_return');return
        if S['phase']=='music_return' and elapsed>.8:
            assert str(S['music'].get_current_music_row())=='BGM_Area_01';S['music'].clear_music_override();S['checks']['combat_bgm_overrides_and_returns_to_area']=True
            minute=S['clock'].get_clock().game_minute%1440;assert S['clock'].jump_game_minutes((1440+60-minute)%1440)
            for npc in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase):stop(npc)
            S['npc'].set_actor_location(S['bed'].get_actor_location(),False,True);approach(S['npc']);phase('sleeping_npc');return
        if S['phase']=='sleeping_npc' and elapsed>1.2:
            npc=S['npc'];bed=S['bed'];assert npc.is_sleeping_for_interaction()
            assert str(npc.get_interaction_text())=='(자는 중)';assert str(pc.get_editor_property('InteractionHint'))=='(자는 중)'
            pc.handle_input_command(u.ICInputCommand.INTERACT,0);assert str(pc.get_editor_property('menu'))==''
            assert not bed.try_interact(pc);S['checks']['sleeping_npc_only_hint_and_occupied_bed_rejected']=True;capture('SleepingNPC_Hint')
            npc.get_life_object_user().release_all('ReviewEmptyBed');stop(npc);npc.set_actor_location(S['standing'],False,True)
            assert S['state'].set_affinity('NPC_R01_001',200);phase('empty_bed');return
        if S['phase']=='empty_bed' and elapsed>.7:
            bed=S['bed'];npc=S['npc'];npc.get_life_object_user().release_all('ReviewEmptyBed');stop(npc)
            S['player'].set_actor_location(bed.get_actor_location()+bed.get_actor_forward_vector()*190,False,True)
            assert not bed.is_occupied_by_any_npc();assert not bed.try_interact(pc),'Exactly 200 must remain locked'
            assert S['state'].set_affinity('NPC_R01_001',201);stop(npc);npc.get_life_object_user().release_all('ReviewSleep')
            assert bed.try_interact(pc);assert S['player'].get_component_by_class(u.ICPlayerLifeComponent).is_sleeping();S['sleep_before']=S['clock'].get_clock().game_minute;phase('player_sleep');return
        if S['phase']=='player_sleep':
            life=S['player'].get_component_by_class(u.ICPlayerLifeComponent)
            if elapsed>.6 and not S.get('sleep_shot'):capture('NPCBed_PlayerSleepFade');S['sleep_shot']=True
            if life.is_sleeping():return
            assert S['clock'].get_clock().game_minute>=S['sleep_before']+120 and not S['bed'].is_player_locked()
            S['checks']['affinity_200_locked_201_empty_bed_sleep_and_wake']=True;phase('wake_settled');return
        if S['phase']=='wake_settled' and elapsed>1:
            capture('NPCBed_AfterWake');S['complete']=True;phase('complete');u.log('LIVE_DIALOGUE_MUSIC_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick);assert u.ICCharacterSetupLibrary.begin_monster_test_review();report()
