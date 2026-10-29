"""Close-up review of player weapon grips; transient actors, no map saves."""
import json, time, traceback
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/WeaponGripReview';OUT.mkdir(parents=True,exist_ok=True)
ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem)
WORLD=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
STATE={'phase':'init','case':0,'view':0,'started':time.monotonic(),'report':[]}
CASES=[(sex,family) for sex in ('M','F') for family in ('Sword','Bow','Staff','Wand')]

def vec(v):return [v.x,v.y,v.z]
def transform(t):
    q=t.rotation;return dict(location=vec(t.translation),rotation=[q.x,q.y,q.z,q.w],scale=vec(t.scale3d))
def status(**extra):
    (OUT/'Status.json').write_text(json.dumps(dict(phase=STATE['phase'],case=STATE['case'],**extra),indent=2),encoding='utf-8')
def begin_case(index):
    sex,family=CASES[index];asset=u.load_asset(f'/Game/IsekaiClass/Player/SamplePlayer/{"Male" if sex=="M" else "Female"}/SK_Player_{sex}')
    STATE['mesh'].set_skeletal_mesh_asset(asset)
    sockets=[]
    profile=u.load_asset(f'/Game/IsekaiClass/Animation/Player/Main{"Male" if sex=="M" else "Female"}/DA_ANIM_PLAYER_{sex}')
    family_enum=getattr(u.ICWeaponFamily,family.upper());animset=profile.get_editor_property('weapon_sets')[family_enum]
    sequence=animset.locomotion.get_editor_property('sample_data')[0].get_editor_property('animation')
    STATE['mesh'].play_animation(sequence,True);STATE['mesh'].set_play_rate(0)
    STATE['mesh'].set_position(sequence.get_play_length()*.35,False)
    if STATE.get('weapon'):ACTORS.destroy_actor(STATE['weapon'])
    name=dict(Sword='ArmingSword',Bow='Bow2',Staff='BoneSpear2',Wand='AncietStaff')[family]
    weapon=ACTORS.spawn_actor_from_class(u.load_class(None,f'/Game/IsekaiClass/Items/Weapons/Blueprints/BP_WPN_{name}.BP_WPN_{name}_C'),u.Vector(),u.Rotator(),True)
    socket='weapon_l' if family=='Bow' else 'weapon_r'
    if STATE['mesh'].does_socket_exist('weapon_grip_'+family.lower()):socket='weapon_grip_'+family.lower()
    assert weapon.attach_to_component(STATE['mesh'],socket,u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.KEEP_WORLD,False)
    STATE['capture'].clear_show_only_components()
    STATE['capture'].show_only_actor_components(STATE['actor'])
    STATE['capture'].show_only_actor_components(weapon)
    STATE.update(weapon=weapon,asset=asset,sex=sex,family=family,hand='J_Bip_L_Hand' if family=='Bow' else 'J_Bip_R_Hand',view=0)
    STATE['report'].append(dict(sex=sex,family=family,sockets=sockets,weapon_visual=transform(weapon.get_editor_property('visual').get_relative_transform()),length_axis=vec(weapon.get_editor_property('length_axis'))))
    STATE.update(phase='settle',since=time.monotonic());status()

def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        phase=STATE['phase']
        if phase=='init':
            actor=ACTORS.spawn_actor_from_class(u.SkeletalMeshActor,u.Vector(0,0,3000),u.Rotator(yaw=-90),True)
            mesh=actor.get_component_by_class(u.SkeletalMeshComponent);mesh.set_update_animation_in_editor(True)
            camera=ACTORS.spawn_actor_from_class(u.SceneCapture2D,u.Vector(),u.Rotator(),True)
            capture=camera.get_component_by_class(u.SceneCaptureComponent2D)
            target=u.RenderingLibrary.create_render_target2d(WORLD,768,768,u.TextureRenderTargetFormat.RTF_RGBA8_SRGB,u.LinearColor(.025,.03,.04,1))
            capture.set_editor_property('texture_target',target);capture.set_editor_property('capture_every_frame',False);capture.set_editor_property('capture_on_movement',False)
            capture.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
            capture.set_editor_property('projection_type',u.CameraProjectionMode.ORTHOGRAPHIC);capture.set_editor_property('ortho_width',35)
            capture.set_editor_property('primitive_render_mode',u.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
            for offset in (u.Vector(180,-140,100),u.Vector(-100,140,80)):
                light=ACTORS.spawn_actor_from_class(u.PointLight,actor.get_actor_location()+offset,u.Rotator(),True)
                comp=light.get_component_by_class(u.PointLightComponent);comp.set_intensity(12000);comp.set_attenuation_radius(1200)
            STATE.update(actor=actor,mesh=mesh,camera=camera,capture=capture,target=target)
            begin_case(0);return
        if phase=='settle' and time.monotonic()-STATE['since']>.7:
            mesh=STATE['mesh'];hand=mesh.get_socket_location(STATE['hand'])
            if STATE['view']==0:
                prefix='J_Bip_L_' if STATE['family']=='Bow' else 'J_Bip_R_'
                bones={name:transform(mesh.get_socket_transform(prefix+name,u.RelativeTransformSpace.RTS_COMPONENT)) for name in ('Hand','Thumb1','Thumb2','Thumb3','Index1','Index2','Index3','Middle1','Middle2','Middle3','Ring1','Ring2','Ring3','Little1','Little2','Little3')}
                STATE['report'][-1]['bones']=bones
            offset=u.Vector(130,-80,30) if STATE['view']==0 else u.Vector(-100,-100,35)
            STATE['capture'].set_world_location_and_rotation(hand+offset,u.MathLibrary.find_look_at_rotation(hand+offset,hand),False,True)
            STATE['capture'].capture_scene();STATE.update(phase='export',since=time.monotonic());return
        if phase=='export' and time.monotonic()-STATE['since']>.15:
            folder=OUT/'Before';folder.mkdir(exist_ok=True)
            u.RenderingLibrary.export_render_target(WORLD,STATE['target'],str(folder),f"{STATE['sex']}_{STATE['family']}_{STATE['view']}.png")
            if STATE['view']==0:STATE.update(view=1,phase='settle',since=time.monotonic())
            else:
                STATE['case']+=1
                if STATE['case']<len(CASES):begin_case(STATE['case'])
                else:
                    (OUT/'Before.json').write_text(json.dumps(STATE['report'],indent=2),encoding='utf-8')
                    STATE['phase']='commands';status(ready=True)
            return
        if phase=='commands':
            command=OUT/'Command.json'
            if not command.exists():return
            request=json.loads(command.read_text(encoding='utf-8'))
            if request.get('id')==STATE.get('last_command'):return
            STATE['last_command']=request.get('id')
            if request.get('quit'):
                u.unregister_slate_post_tick_callback(STATE['callback']);u.SystemLibrary.quit_editor();return
            script=(ROOT/request['script']).resolve();assert script.is_relative_to(ROOT)
            exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'),globals())
            status(command_done=STATE['last_command'])
    except Exception:
        error=traceback.format_exc();u.log_error(error);STATE['phase']='commands';status(error=error)
    finally:STATE['busy']=False

STATE['callback']=u.register_slate_post_tick_callback(tick)
