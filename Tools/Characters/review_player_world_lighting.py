"""Compare player materials in the saved open world; never saves a level or lighting setting.

Run in a dedicated -ICCharacterLightingReview editor. Creates an isolated review
save namespace and exposes a small file command queue for material-only trials.
"""
import hashlib
import json
import sys
import time
import traceback
from pathlib import Path
import unreal as u

assert '-ICCharacterLightingReview' in u.SystemLibrary.get_command_line()
assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
import setup_player_world_lighting as LIGHTING
SOFT_TOON_PREVIEW='-ICSoftToonLightingPreview' in u.SystemLibrary.get_command_line()
TOON_OUTPUT_PREVIEW='-ICToonOutputPreview' in u.SystemLibrary.get_command_line()
NATIVE_TOON_REVIEW='-ICNativeToonReview' in u.SystemLibrary.get_command_line()
OUT=ROOT/('Saved/CharacterLighting/ToonCorrection' if SOFT_TOON_PREVIEW else 'Saved/CharacterLighting');OUT.mkdir(parents=True,exist_ok=True)
MAP='/Game/IsekaiClass/World/Maps/L_OpenWorld'
PROTECTED=[ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',ROOT/'Config/DefaultEngine.ini',
           ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
HASHES={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in PROTECTED if p.exists()}
STATE={'phase':'starting','started':time.monotonic(),'samples':[],'complete':False,'commands':[]}
STATE['automatic_variants']=['NativeToon'] if NATIVE_TOON_REVIEW else ['ToonOutput'] if TOON_OUTPUT_PREVIEW else ['ToonSoft','ToonDim'] if SOFT_TOON_PREVIEW else ['WorldLit']
if (OUT/'Command.json').exists():
    STATE['commands'].append(json.loads((OUT/'Command.json').read_text(encoding='utf-8'))['id'])
TIMES=[('Noon',720),('Night',1320),('Morning',480)]
if SOFT_TOON_PREVIEW:TIMES=TIMES[:2]
CASES=[]

def report(**extra):
    result={k:STATE[k] for k in ['phase','samples','complete','error','command_result'] if k in STATE}
    result.update(extra)
    (OUT/'Review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')

def light_state():
    world=STATE['world']
    return {'suns':[{'name':a.get_path_name(),'intensity':a.get_component_by_class(u.DirectionalLightComponent).get_editor_property('intensity'),
                     'rotation':[a.get_actor_rotation().pitch,a.get_actor_rotation().yaw,a.get_actor_rotation().roll]} for a in u.GameplayStatics.get_all_actors_of_class(world,u.DirectionalLight)],
            'skies':[{'name':a.get_path_name(),'intensity':a.get_component_by_class(u.SkyLightComponent).get_editor_property('intensity')}
                     for a in u.GameplayStatics.get_all_actors_of_class(world,u.SkyLight)]}


def native_brightness():
    component=STATE['player'].get_component_by_class(u.ICPlayerToonBrightnessComponent)
    return component.get_brightness() if component else None

def finish():
    for path,digest in HASHES.items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,'Protected world file changed: '+path
    STATE['complete']=True;STATE['phase']='complete';report(protected_files_unchanged=True)
    u.unregister_slate_post_tick_callback(STATE['handle'])
    u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.log('IC_CHARACTER_LIGHTING_REVIEW_FINISHED')
    u.SystemLibrary.quit_editor()

def queue(variant):
    CASES[:]=[(gender,name,minute,variant) for gender in ['M','F'] for name,minute in TIMES]
    STATE.update(index=0,phase='next')

def material_properties(material):
    info={'path':material.get_path_name(),'parents':[],'base_overrides':[]}
    current=material
    while isinstance(current,u.MaterialInstance):
        settings=current.get_editor_property('base_property_overrides')
        values={}
        for key in ['override_shading_model','shading_model','override_blend_mode','blend_mode','override_two_sided','two_sided','override_opacity_mask_clip_value','opacity_mask_clip_value']:
            try:values[key]=str(settings.get_editor_property(key))
            except Exception:pass
        info['base_overrides'].append({'path':current.get_path_name(),**values})
        current=current.get_editor_property('parent');info['parents'].append(current.get_path_name())
    info['switches']={str(n):u.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(material,n)
                      for n in u.MaterialEditingLibrary.get_static_switch_parameter_names(material)
                      if str(n) in ['bUseLight','bEmissiveAsBaseColor','bUseFilmTonemapInverse']}
    return info

def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        phase=STATE['phase'];now=time.monotonic()
        if phase!='commands':assert now-STATE['started']<1200,'Review timeout in '+phase
        if phase=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            world=worlds[0];pc=u.GameplayStatics.get_player_controller(world,0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(world)
            STATE.update(world=world,pc=pc,phase='creating');report();return
        if phase=='creating':
            pc=STATE['pc']
            created=pc.get_editor_property('created');saving=pc.get_saves().get_editor_property('saving')
            menu=str(pc.get_editor_property('menu'))
            report(waiting={'created':created,'saving':saving,'menu':menu})
            if not created or saving:return
            if menu:
                pc.set_menu('')
                return
            player=u.GameplayStatics.get_player_pawn(STATE['world'],0)
            player.get_component_by_class(u.CharacterMovementComponent).disable_movement()
            player.set_actor_rotation(u.Rotator(),False)
            for _ in range(6):
                if pc.get_inventory().get_equipped_weapon_family()==u.ICWeaponFamily.NONE:break
                pc.get_inventory().cycle_weapon()
            pc.get_time().set_running(False)
            camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            focus=player.get_actor_location()+u.Vector(0,0,0)
            location=focus+u.Vector(350,380,75)
            rotation=u.MathLibrary.find_look_at_rotation(location,focus)
            camera.set_world_location_and_rotation(location,rotation,False,True);camera.set_editor_property('field_of_view',40)
            original=LIGHTING.source_materials()
            STATE.update(player=player,camera=camera,original=original,location=location,rotation=rotation)
            (OUT/'CurrentMaterials.json').write_text(json.dumps({sex:[material_properties(m) for m in materials] for sex,materials in original.items()},indent=2),encoding='utf-8')
            STATE.update(phase='freeze',ready=now+8);report();return
        if phase=='freeze':
            if now<STATE['ready']:return
            queue('Original');return
        if phase=='next':
            if STATE['index']>=len(CASES):
                if STATE['automatic_variants']:queue(STATE['automatic_variants'].pop(0));return
                if '-ICCharacterLightingAutoExit' in u.SystemLibrary.get_command_line():finish();return
                STATE['phase']='commands';report();return
            sex,name,minute,variant=CASES[STATE['index']]
            STATE['player'].apply_preset(0 if sex=='M' else 1)
            body=STATE['player'].get_editor_property('mesh')
            materials=STATE['original'][sex] if variant in ['Original','NativeToon','NativeLit'] else STATE['variants'][variant][sex]
            assert body.get_num_materials()==len(materials)
            for index,material in enumerate(materials):
                if variant in ['NativeToon','NativeLit']:continue
                body.set_material(index,material)
                if variant in ['ToonSoft','ToonDim']:
                    scale=({'ToonSoft':.60,'ToonDim':.45} if name=='Night' else {'ToonSoft':.90,'ToonDim':.85})[variant]
                    dynamic=body.create_dynamic_material_instance(index,material)
                    dynamic.set_scalar_parameter_value('ColorScaleAllMat',scale)
                elif variant=='ToonOutput':
                    dynamic=body.create_dynamic_material_instance(index,material)
                    dynamic.set_scalar_parameter_value('ICCharacterBrightness',.60 if name=='Night' else 1.)
            if variant=='NativeToon':
                body.set_editor_property('override_materials',[])
                component=STATE['player'].get_component_by_class(u.ICPlayerToonBrightnessComponent)
                if component:component.refresh_materials()
            clock=STATE['pc'].get_time();current=clock.get_clock().game_minute
            assert clock.jump_game_minutes((minute-current%1440+1440)%1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(STATE['world'],u.ICDayNightController):cycle.refresh_from_clock()
            STATE.update(phase='settle',ready=now+6,case=(sex,name,minute,variant),lighting=light_state());report();return
        if phase=='settle':
            if now<STATE['ready']:return
            sex,name,minute,variant=STATE['case']
            label=f'{sex}_{name}_{variant}'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(label+'.png')))
            sample={'name':label,'sex':sex,'minute':minute,'variant':variant,'lighting':light_state(),
                    'lighting_before_settle':STATE['lighting'],'clock_minute':STATE['pc'].get_time().get_clock().game_minute,
                    'player_location':str(STATE['player'].get_actor_location()),'camera':str(STATE['camera'].get_world_location()),
                    'native_brightness':native_brightness()}
            STATE['samples'].append(sample);STATE.update(index=STATE['index']+1,phase='next');report();return
        if phase=='commands':
            command=OUT/'Command.json'
            if not command.exists():return
            request=json.loads(command.read_text(encoding='utf-8'))
            if request.get('id') in STATE['commands']:return
            STATE['commands'].append(request['id'])
            if request.get('finish'):finish();return
            script=(ROOT/request['script']).resolve();assert script.is_relative_to(ROOT)
            exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'),globals())
            STATE['command_result']=request['id'];report();return
    except Exception:
        STATE.update(phase='commands',error=traceback.format_exc());u.log_error(STATE['error']);report()
    finally:STATE['busy']=False

STATE['original']=LIGHTING.source_materials()
STATE['variants']={name:STATE['original'] for name in ['ToonSoft','ToonDim']} if SOFT_TOON_PREVIEW else {'WorldLit':LIGHTING.create_lit_materials(STATE['original'])}
if TOON_OUTPUT_PREVIEW:
    import setup_player_toon_lighting as TOON
    STATE['variants']['ToonOutput'],STATE['toon_assets']=TOON.create_materials(STATE['original'])
STATE['phase']='starting'
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]!=MAP:
    u.EditorLoadingAndSavingUtils.load_map(MAP)
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
