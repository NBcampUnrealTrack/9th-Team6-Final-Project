"""Author player-only grip sockets, then capture actual equipped/attacking poses."""
import json, math, shutil, time, traceback
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/WeaponGripReview';(OUT/'After').mkdir(parents=True,exist_ok=True)
LEVEL=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'start','started':time.monotonic(),'case':0,'view':0,'samples':[]}
FAMILIES=(u.ICWeaponFamily.SWORD,u.ICWeaponFamily.BOW,u.ICWeaponFamily.STAFF,u.ICWeaponFamily.WAND)
NAMES=('Sword','Bow','Staff','Wand')
BASELINE=json.loads((OUT/'Before.json').read_text(encoding='utf-8'))
def vec(v):return [v.x,v.y,v.z]
exec(compile((ROOT/'Tools/Characters/fine_tune_player_grips.py').read_text(encoding='utf-8'),'<staff-grip-tuning>','exec'))
def phase(name):STATE.update(phase=name,since=time.monotonic());u.log('IC_GRIP_REVIEW '+name)
def finish(error=None):
    report=dict(complete=not bool(error),samples=STATE['samples'],error=error)
    (OUT/'Review.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(STATE['callback']);LEVEL.editor_request_end_play();u.SystemLibrary.quit_editor()
def setup_capture(world):
    camera=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.SceneCapture2D) if a.get_name()==STATE['capture_actor_name'])
    capture=camera.get_component_by_class(u.SceneCaptureComponent2D)
    target=u.RenderingLibrary.create_render_target2d(world,768,768,u.TextureRenderTargetFormat.RTF_RGBA8_SRGB,u.LinearColor(.03,.035,.045,1))
    capture.set_editor_property('texture_target',target);capture.set_editor_property('capture_every_frame',False);capture.set_editor_property('capture_on_movement',False)
    capture.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    capture.set_editor_property('projection_type',u.CameraProjectionMode.ORTHOGRAPHIC);capture.set_editor_property('ortho_width',30)
    capture.set_editor_property('primitive_render_mode',u.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
    settings=capture.get_editor_property('post_process_settings')
    settings.set_editor_property('override_auto_exposure_method',True);settings.set_editor_property('auto_exposure_method',u.AutoExposureMethod.AEM_MANUAL)
    settings.set_editor_property('override_auto_exposure_bias',True);settings.set_editor_property('auto_exposure_bias',-1.)
    settings.set_editor_property('override_bloom_intensity',True);settings.set_editor_property('bloom_intensity',0.)
    capture.set_editor_property('post_process_settings',settings)
    STATE.update(capture=capture,target=target)
def pose_metrics():
    mesh=STATE['player'].get_editor_property('mesh');prefix='J_Bip_L_' if STATE['case']%4==1 else 'J_Bip_R_'
    positions={name:mesh.get_socket_location(prefix+name) for name in ('Hand','Index1','Index2','Index3','Middle1','Middle2','Middle3','Little1','Little2','Little3','Thumb1','Thumb2','Thumb3')}
    assert all(math.isfinite(c) for value in positions.values() for c in vec(value))
    reference=BASELINE[STATE['case']]['bones'];length_errors=[]
    for finger in ('Index','Middle','Little','Thumb'):
        for joint in (1,2):
            a=finger+str(joint);b=finger+str(joint+1)
            expected=u.Vector(*reference[a]['location']).distance(u.Vector(*reference[b]['location']))
            length_errors.append(abs(positions[a].distance(positions[b])-expected))
    assert max(length_errors)<.03,length_errors
    hand=positions['Hand'];tips={finger:positions[finger+'3'].distance(hand) for finger in ('Index','Middle','Little')}
    return dict(max_bone_length_error=max(length_errors),tip_distances=tips)
def capture_view():
    mesh=STATE['player'].get_editor_property('mesh');bow=STATE['case']%4==1;prefix='J_Bip_L_' if bow else 'J_Bip_R_'
    hand=mesh.get_socket_location(prefix+'Hand');index=mesh.get_socket_location(prefix+'Index1');little=mesh.get_socket_location(prefix+'Little1');middle=mesh.get_socket_location(prefix+'Middle1');thumb=mesh.get_socket_location(prefix+'Thumb2')
    width=(index-little).normal();forward=(middle-hand).normal();normal=width.cross(forward).normal()
    if (thumb-index).dot(normal)<0:normal*=-1
    socket='weapon_grip_'+NAMES[STATE['case']%4].lower();grip=mesh.get_socket_location(socket)
    focus=middle+normal*1.5
    offset=normal*95+width*35+forward*15 if STATE['view']==0 else forward*90+normal*55+width*25
    capture=STATE['capture'];capture.set_world_location_and_rotation(focus+offset,u.MathLibrary.find_look_at_rotation(focus+offset,focus),False,True)
    capture.capture_scene()
def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        if time.monotonic()-STATE['started']>240:raise RuntimeError('Grip review timeout: '+STATE['phase'])
        elapsed=time.monotonic()-STATE.get('since',STATE['started'])
        if STATE['phase']=='start':
            for sex,label in (('M','Male'),('F','Female')):
                relative=f'Content/IsekaiClass/Player/SamplePlayer/{label}/SK_Player_{sex}.uasset'
                backup=OUT/'Backup'/relative;backup.parent.mkdir(parents=True,exist_ok=True)
                if not backup.exists():shutil.copy2(ROOT/relative,backup)
                mesh=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{label}/SK_Player_{sex}')
                assert u.ICCharacterSetupLibrary.prepare_character_mesh(mesh);assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
                tune_staff_socket(mesh)
            camera=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.SceneCapture2D,u.Vector(),u.Rotator(),False)
            camera.get_component_by_class(u.SceneCaptureComponent2D).set_editor_property('capture_every_frame',False)
            STATE['capture_actor_name']=camera.get_name()
            assert u.ICCharacterSetupLibrary.begin_monster_test_review();phase('create');return
        if STATE['phase']=='create':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or not pc.get_editor_property('Screen') or str(pc.get_editor_property('Menu'))!='Create':return
            STATE.update(world=worlds[0],pc=pc);assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        if STATE['phase']=='created':
            pc=STATE['pc']
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(STATE['world'],0);inventory=pc.get_inventory()
            for index,item in enumerate(('EQP_WPN_SWORD_001','EQP_WPN_BOW_001','EQP_WPN_STAFF_001','EQP_WPN_WAND_001')):
                if inventory.get_weapon_loadout()[index].count>0:continue
                assert inventory.add_item(item,1,False)
                slot=next(i for i,v in enumerate(inventory.get_snapshot().bag.slots) if str(v.item_id)==item);assert inventory.equip(slot)
            for enemy in u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICEnemyBase):
                controller=enemy.get_controller()
                if controller:
                    brain=controller.get_component_by_class(u.BrainComponent)
                    if brain:brain.stop_logic('Weapon grip review')
                    controller.stop_movement();controller.set_actor_tick_enabled(False)
            STATE.update(player=player,inventory=inventory,asc=player.get_component_by_class(u.ICAbilitySystemComponent));setup_capture(STATE['world']);phase('next');return
        if STATE['phase']=='next':
            if STATE['case']>=8:finish();return
            STATE['asc'].cancel_action();STATE['player'].apply_preset(STATE['case']//4)
            for _ in range(6):
                if STATE['inventory'].get_equipped_weapon_family()==FAMILIES[STATE['case']%4]:break
                assert STATE['inventory'].cycle_weapon()
            assert STATE['inventory'].get_equipped_weapon_family()==FAMILIES[STATE['case']%4]
            STATE['player'].refresh_weapon_appearance()
            weapon=next(a for a in u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICWeaponActor) if a.get_owner()==STATE['player'])
            assert str(weapon.get_attach_parent_socket_name())=='weapon_grip_'+NAMES[STATE['case']%4].lower()
            STATE['capture'].clear_show_only_components();STATE['capture'].show_only_actor_components(STATE['player']);STATE['capture'].show_only_actor_components(weapon)
            STATE['view']=0;phase('settle');return
        if STATE['phase']=='settle' and elapsed>.75:
            capture_view();phase('export');return
        if STATE['phase']=='attack' and elapsed>.2:
            capture_view();phase('export');return
        if STATE['phase']=='export' and elapsed>.15:
            name=f"{'M' if STATE['case']<4 else 'F'}_{NAMES[STATE['case']%4]}_{STATE['view']}"
            u.RenderingLibrary.export_render_target(STATE['world'],STATE['target'],str(OUT/'After'),name+'.png')
            STATE['samples'].append(dict(name=name,**pose_metrics()))
            if STATE['view']==0:STATE['view']=1;phase('settle')
            elif STATE['view']==1:
                result=STATE['asc'].request_action('Attack','None',None);assert result.success,str(result.reason)
                STATE['view']=2;phase('attack')
            else:STATE['case']+=1;phase('next')
    except Exception:
        error=traceback.format_exc();u.log_error(error);finish(error)
    finally:STATE['busy']=False
STATE['callback']=u.register_slate_post_tick_callback(tick)
