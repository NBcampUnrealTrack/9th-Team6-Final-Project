"""Capture the actual test level (no generated artwork), then verify map travel and upward chest lids."""
import unreal as u,time,json,pathlib,traceback
ROOT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));OUT=ROOT/'Saved/MapTravel/Live';OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='capture_start',started=time.monotonic(),checks={},trips=[],complete=False);LEVEL=u.get_editor_subsystem(u.LevelEditorSubsystem)
POINTER_REVIEW='-ICMapPointerReview' in u.SystemLibrary.get_command_line()
def phase(p):S.update(phase=p,t=time.monotonic())
def actors(cls):return u.GameplayStatics.get_all_actors_of_class(S['world'],cls)
def key(name):
    k=u.Key();k.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],k)
def shot(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def map_ui():return next(w for w in u.ObjectIterator(u.ICMapWidget) if w.get_owning_player()==S['pc'] and not w.get_editor_property('bMinimap') and (w.get_parent() or w.is_in_viewport()))
def finish():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ['phase','checks','trips','complete','error'] if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(S['handle']);LEVEL.editor_request_end_play();u.SystemLibrary.quit_editor()
def stop(a):
    c=a.get_controller()
    if c:
        brain=c.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Map travel review')
        c.stop_movement();c.set_actor_tick_enabled(False)
    movement=a.get_component_by_class(u.CharacterMovementComponent)
    if movement:movement.stop_movement_immediately()
def approach(a):
    p=a.get_actor_location()+a.get_actor_forward_vector()*190+u.Vector(0,0,96)
    S['player'].set_actor_location_and_rotation(p,u.MathLibrary.find_look_at_rotation(p,a.get_actor_location()),False,True)
def tick(dt):
    try:
        if time.monotonic()-S['started']>180:raise RuntimeError('Map travel review timeout: '+S['phase'])
        elapsed=time.monotonic()-S['t']
        if S['phase']=='capture_start':
            S['world']=u.EditorLevelLibrary.get_editor_world()
            a=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.SceneCapture2D,u.Vector(0,1000,25000),u.Rotator(pitch=-90,yaw=0),True)
            c=a.get_component_by_class(u.SceneCaptureComponent2D)
            rt=u.RenderingLibrary.create_render_target2d(S['world'],2048,1024,u.TextureRenderTargetFormat.RTF_RGBA8_SRGB,u.LinearColor(0,0,0,1))
            c.set_editor_property('texture_target',rt);c.set_editor_property('projection_type',u.CameraProjectionMode.ORTHOGRAPHIC);c.set_editor_property('ortho_width',42000)
            c.set_editor_property('capture_source',u.SceneCaptureSource.SCS_BASE_COLOR);c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False)
            S.update(capture_actor=a,capture=c,rt=rt);phase('capture_warm');return
        if S['phase']=='capture_warm' and elapsed>.5:S['capture'].capture_scene();phase('capture_save');return
        if S['phase']=='capture_save' and elapsed>.8:
            path='/Game/IsekaiClass/UI/Map/T_TestMap_Overview';u.EditorAssetLibrary.make_directory('/Game/IsekaiClass/UI/Map')
            tex=u.load_asset(path)
            if tex:u.RenderingLibrary.convert_render_target_to_texture2d_editor_only(S['world'],S['rt'],tex)
            else:tex=u.RenderingLibrary.render_target_create_static_texture2d_editor_only(S['rt'],path,u.TextureCompressionSettings.TC_DEFAULT,u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            assert tex;tex.set_editor_property('compression_no_alpha',True);tex.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI)
            assert u.EditorAssetLibrary.save_loaded_asset(tex,False)
            assert u.EditorAssetLibrary.save_loaded_asset(u.load_asset('/Game/IsekaiClass/Data/DataTable/World/DT_IC_Map'),False)
            u.RenderingLibrary.export_render_target(S['world'],S['rt'],str(OUT),'TestMap_Overview.png')
            # The transient capture is released with this isolated editor session, without touching the saved level.
            S['capture'].set_editor_property('texture_target',None);S['checks']['captured_real_test_map_without_AI']=tex.get_path_name()
            S['portals']=sorted(actors(u.ICWarpStone),key=lambda a:str(a.get_editor_property('WarpID')));assert len(S['portals'])==3
            S['trip']=0;phase('start');assert u.ICCharacterSetupLibrary.begin_monster_test_review();return
        if S['phase'].startswith('capture_'):return
        if S['phase']=='start':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('Menu'))!='Create':return
            S.update(world=worlds[0],pc=pc);assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);phase('created');return
        pc=S['pc'];elapsed=time.monotonic()-S['t']
        if S['phase']=='created':
            if not pc.get_editor_property('created'):return
            S.update(player=u.GameplayStatics.get_player_pawn(S['world'],0),state=pc.get_world_state())
            S['portals']=sorted(actors(u.ICWarpStone),key=lambda a:str(a.get_editor_property('WarpID')));assert len(S['portals'])==3
            u.SystemLibrary.execute_console_command(S['world'],'ICRecruitTestParty',pc);phase('party');return
        if S['phase']=='party' and elapsed>.8:
            for a in actors(u.ICNPCBase)+actors(u.ICEnemyBase):stop(a)
            S['chests']=[next(a for a in actors(u.ICPlayerStorage) if not a.get_editor_property('hidden')),next(a for a in actors(u.ICNPCContainerActor) if not a.get_editor_property('bPickupOnly'))]
            S['chest_index']=0;phase('chest_start');return
        if S['phase']=='chest_start':
            a=S['chests'][S['chest_index']];approach(a)
            if isinstance(a,u.ICNPCContainerActor):assert S['state'].set_affinity(a.get_editor_property('OwnerNPCID'),300)
            h=a.get_editor_property('ChestPresentation');lid=h.get_editor_property('Lid')
            S.update(chest=a,hinge=h,lid=lid,closed_z=u.SystemLibrary.get_component_bounds(lid)[0].z)
            phase('chest_ready');return
        if S['phase']=='chest_ready' and elapsed>.3:
            assert S['chest'].try_interact(pc);phase('chest_open');return
        if S['phase']=='chest_open' and elapsed>.85:
            assert S['hinge'].get_open_amount()>.99
            z=u.SystemLibrary.get_component_bounds(S['lid'])[0].z
            assert z>S['closed_z']+25,('Lid must rise',S['closed_z'],z)
            S['checks']['NPC_lid_opens_up' if S['chest_index'] else 'player_lid_opens_up']=dict(closed_z=S['closed_z'],opened_z=z)
            next(w for w in u.ObjectIterator(u.ICStorageWidget) if w.get_owning_player()==pc and w.is_open()).close_storage();phase('chest_close');return
        if S['phase']=='chest_close' and elapsed>.85:
            assert S['hinge'].get_open_amount()<.01
            if S['chest_index']==0:S['chest_index']=1;phase('chest_start');return
            shot('Chest_Closed');phase('hud');return
        if S['phase']=='hud' and elapsed>.8:
            shot('HUD_Map');key('Gamepad_Special_Left');phase('map_ready');return
        if S['phase']=='map_ready' and elapsed>.6:
            ui=map_ui();S['ui']=ui
            if S['trip']==0:
                uv=[ui.world_to_map_uv(a.get_actor_location()) for a in S['portals']]
                assert uv[0].x<.1 and .45<uv[1].x<.55 and uv[2].x>.9
                assert pc.set_gold(499);p=S['player'].get_actor_location()
                assert not ui.request_travel(S['portals'][0].get_editor_property('WarpID'));assert S['state'].export_state().gold==499
                S['checks']['insufficient_gold_does_not_move_or_charge']=True
                assert pc.set_gold(1600);shot('Map_Three_Portals')
                if POINTER_REVIEW:
                    (OUT/'PointerReady.json').write_text(json.dumps(dict(ready=True,started=time.time())),encoding='utf-8');phase('pointer_wait');return
                key('Enter')
            else:assert ui.request_travel(S['portals'][S['trip']].get_editor_property('WarpID'))
            assert ui.is_traveling();assert not ui.request_travel(S['portals'][0].get_editor_property('WarpID'))
            key('Escape');assert str(pc.get_editor_property('Menu'))=='Map'
            S['gold_before']=S['state'].export_state().gold;phase('travel');return
        if S['phase']=='pointer_wait':
            if elapsed>.35 and not S.get('pointer_shot'):shot('Map_Ready');S['pointer_shot']=True
            if not S['ui'].is_traveling():return
            S['checks']['actual_mouse_portal_click']=True
            assert not S['ui'].request_travel(S['portals'][0].get_editor_property('WarpID'))
            key('Escape');assert str(pc.get_editor_property('Menu'))=='Map'
            S['gold_before']=S['state'].export_state().gold;phase('travel');return
        if S['phase']=='travel':
            if elapsed>.15 and not S.get('loading_shot'):shot('Loading');S['loading_shot']=True
            if str(pc.get_editor_property('Menu'))=='Map':
                if elapsed>3:raise RuntimeError('Travel did not complete')
                return
            gold=S['state'].export_state().gold;assert gold==S['gold_before']-500
            target=S['portals'][S['trip']].get_actor_location();p=S['player'].get_actor_location();assert abs(p.y-target.y)<5 and abs(p.x+250)<5
            party=[a for a in actors(u.ICNPCBase) if abs(a.get_actor_location().x-p.x)<500 and abs(a.get_actor_location().y-p.y)<500]
            assert len(party)>=3,'All three recruited companions should arrive'
            S['trips'].append(dict(portal=str(S['portals'][S['trip']].get_editor_property('WarpID')),gold=gold,party_members_nearby=len(party)))
            S['trip']+=1;phase('arrived');return
        if S['phase']=='arrived' and elapsed>.4:
            shot('Arrived_'+str(S['trip']))
            if S['trip']<3:pc.set_menu('Map');phase('map_ready');return
            S['checks']['three_500G_trips_loading_and_companions']=True
            S['checks']['loading_blocks_duplicate_or_cancel_input']=True
            S['complete']=True;phase('complete');finish()
    except Exception:S['error']=traceback.format_exc();u.log_error(S['error']);phase('error');finish()
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['t']=time.monotonic();S['handle']=u.register_slate_post_tick_callback(tick)
