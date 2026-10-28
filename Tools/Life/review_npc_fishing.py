"""Rendered PIE: real H input, 20-second catch, close-up dialogue, NPC container access."""
import unreal as u,time,json,traceback,pathlib
out=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'NPCFishing';out.mkdir(exist_ok=True)
S={'phase':'starting','started':time.monotonic(),'checks':{},'complete':False};levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
NPC_ONLY='-ICNPCReviewOnly' in u.SystemLibrary.get_command_line()
def report():(out/('LiveNPCReview.json' if NPC_ONLY else 'LiveReview.json')).write_text(json.dumps({k:S[k] for k in ('phase','checks','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):S.update(phase=name,phase_started=time.monotonic());report()
def finish():report();u.unregister_slate_post_tick_callback(S['handle']);levels.editor_request_end_play();u.SystemLibrary.quit_editor()
def press(name):
    key=u.Key();key.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],key),name
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(out/(name+'.png')))
def system(kind):return next(o for o in u.ObjectIterator(kind) if o.get_outer()==S['pc'].get_world_state().get_outer())
def fish_count():return sum(S['bag'].count_item(f'ITM_R01_FISH_{i:02d}') for i in range(1,11))
def aim(position,target):S['camera'].set_world_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,target),False,True)
def approach_npc():
    npc=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase) if str(a.get_editor_property('NPCDefinitionID'))=='NPC_R01_001')
    user=npc.get_component_by_class(u.ICLifeObjectUserComponent)
    if user:user.release_all('Review');user.set_component_tick_enabled(False)
    controller=npc.get_controller()
    if controller:controller.stop_movement()
    point=npc.get_actor_location();front=npc.get_actor_forward_vector()
    position=point+front*190;S['pawn'].set_actor_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,point),False,True)
    S['pc'].set_control_rotation(u.MathLibrary.find_look_at_rotation(position,point));aim(point+front*550+u.Vector(0,200,180),point+u.Vector(0,0,45))
    S['npc']=npc;S['checks']['npc_approach_location']=str(point);phase('approach_npc')
def tick(delta):
    try:
        if time.monotonic()-S['started']>180:raise RuntimeError('Review timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=time.monotonic()-S['phase_started']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            if NPC_ONLY and len(u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase))<11:return
            pawn=u.GameplayStatics.get_player_pawn(S['world'],0);camera=pawn.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Character):
                controller=a.get_controller()
                if controller and a!=pawn:
                    brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('NPC fishing review')
                    controller.stop_movement()
                    controller.set_actor_tick_enabled(False)
                    a.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            S.update(pawn=pawn,camera=camera,bag=pc.get_inventory(),gather=system(u.ICGatheringSubsystem),life=pawn.get_component_by_class(u.ICPlayerLifeComponent))
            if NPC_ONLY:pc.set_menu('');approach_npc();return
            zone=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICLifeActor) if str(a.get_editor_property('point_id'))=='TEST_R01_FISH_RANDOM')
            origin,extent,_=u.SystemLibrary.get_component_bounds(zone.get_editor_property('visual_mesh'));point=origin+u.Vector(0,0,extent.z)
            pawn.set_actor_location_and_rotation(point+u.Vector(-650,0,96),u.Rotator(),False,True);pc.set_control_rotation(u.Rotator())
            aim(point+u.Vector(-950,-150,350),point);pc.set_menu('');S.update(zone=zone,point=point,before=fish_count());phase('aim_water');return
        if S['phase']=='aim_water' and elapsed>1:
            assert not S['zone'].try_interact(pc),'F must not start fishing'
            capture('Fishing_Aim_H');press('H');phase('cast');return
        if S['phase']=='cast' and elapsed>.8:
            assert S['gather'].is_gathering(),'Real H key must begin fishing'
            line=S['life'].get_fishing_line();assert line and line.has_valid_landing()
            S['checks']['h_starts_and_f_does_not']=True;S['checks']['line_float_lands_inside_zone']=True;S['cast_started']=time.monotonic()-.8
            capture('Fishing_LineAndFloat');phase('fishing');return
        if S['phase']=='fishing':
            if S['gather'].is_gathering():
                if elapsed>21:raise RuntimeError('Fishing exceeded twenty seconds')
                if elapsed>9 and not S.get('half_shot'):capture('Fishing_HalfProgress');S['half_shot']=True
                return
            assert fish_count()==S['before']+1
            S['checks']['one_fish_after_twenty_seconds']=time.monotonic()-S['cast_started']>19.5;capture('Fishing_Reward');phase('repeat');return
        if S['phase']=='repeat' and elapsed>.8:press('H');phase('recast');return
        if S['phase']=='recast' and elapsed>.6:
            assert S['gather'].is_gathering(),'No fishing cooldown';press('H');phase('canceled');return
        if S['phase']=='canceled' and elapsed>.5:
            assert not S['gather'].is_gathering() and S['life'].get_fishing_line() is None
            assert fish_count()==S['before']+1;S['checks']['immediate_repeat_and_h_cancel']=True
            point=S['point'];aim(point+u.Vector(-950,-150,350),point+u.Vector(0,2200,0));press('H');phase('miss');return
        if S['phase']=='miss' and elapsed>.6:
            assert not S['gather'].is_gathering() and fish_count()==S['before']+1;S['checks']['miss_cannot_grant_fish']=True
            approach_npc();return
        if S['phase']=='approach_npc' and elapsed>.8:
            S['checks']['npc_before_f']={'npc':str(S['npc'].get_actor_location()),'player':str(S['pawn'].get_actor_location()),'look':str(pc.get_control_rotation()),'hint':str(pc.get_editor_property('InteractionHint'))}
            pc.handle_input_command(u.ICInputCommand.INTERACT,0);phase('dialogue');return
        if S['phase']=='dialogue' and elapsed>2:
            assert str(pc.get_editor_property('menu'))=='Dialogue','F must open NPC dialogue'
            assert isinstance(pc.get_view_target(),u.CameraActor),'NPC face camera must be active'
            target=pc.get_view_target();mesh=S['npc'].get_component_by_class(u.SkeletalMeshComponent)
            S['checks']['face_camera_geometry']={'camera':str(target.get_actor_location()),'npc':str(S['npc'].get_actor_location()),'head':str(mesh.get_socket_location('J_Bip_C_Head')),'fov':target.get_component_by_class(u.CameraComponent).get_editor_property('field_of_view')}
            capture('NPC_FaceDialogue');S['checks']['npc_face_closeup']=True;pc.set_menu('');phase('dialogue_closed');return
        if S['phase']=='dialogue_closed' and elapsed>.7:
            assert pc.get_view_target()==S['pawn'];S['checks']['dialogue_close_restores_camera']=True
            boxes=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCContainerActor);assert len(boxes)==33
            ids=[str(a.get_editor_property('ContainerID')) for a in boxes];assert len(set(ids))==33
            state=pc.get_world_state();assert state.set_affinity('NPC_R01_001',199)
            selected=[a for a in boxes if str(a.get_editor_property('OwnerNPCID'))=='NPC_R01_001'];assert len(selected)==3
            for box in selected:assert not box.can_access_container(pc) and not box.try_interact(pc)
            assert state.set_affinity('NPC_R01_001',200);S['boxes']=selected;S['box_index']=0
            point=selected[0].get_actor_location();S['pawn'].set_actor_location(point+u.Vector(180,0,96),False,True);aim(point+u.Vector(600,-450,340),point+u.Vector(0,0,70));phase('containers');return
        if S['phase']=='containers' and elapsed>.6:
            if S['box_index']>=3:
                S['checks']['thirty_three_independent_npc_containers']=True;S['checks']['affinity_199_locked_200_unlocked']=True
                S['complete']=True;phase('complete');u.log('NPC_FISHING_LIVE_OK');finish();return
            box=S['boxes'][S['box_index']];assert box.can_access_container(pc);assert box.try_interact(pc)
            ui=pc.get_editor_property('storage_screen');assert ui and ui.is_open()
            assert len(S['bag'].get_snapshot().storage[box.get_editor_property('ContainerID')].slots)==40
            S['ui']=ui;phase('container_open');return
        if S['phase']=='container_open' and elapsed>.6:
            capture('NPC_Container_'+str(S['box_index']));S['ui'].close_storage();S['box_index']+=1;phase('containers');return
    except Exception:
        S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap');S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review();report()
