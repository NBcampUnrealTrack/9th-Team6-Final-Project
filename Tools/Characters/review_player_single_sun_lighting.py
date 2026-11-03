"""Fresh native-gameplay verification of saved single-sun materials and world settings."""
import sys,json,hashlib,struct
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(ROOT/'Tools/Characters'))
import setup_player_single_sun_lighting as single
import review_player_world_lighting as review
OUT=single.OUT/'TimeOfDay';OUT.mkdir(parents=True,exist_ok=True)
APPLIED=json.loads((single.OUT/'Applied.json').read_text(encoding='utf-8'))
review.OUT=OUT;review.TIMES[:]=[('04h',240),('10h',600),('14h',840),('20h',1200)]
review.STATE['automatic_variants']=[]
CHECK={'started':False,'cases':{},'complete':False}


def tick(delta):
    if review.STATE.get('error'):return
    phase=review.STATE['phase']
    if phase=='freeze' and not CHECK['started']:
        player=review.STATE['player'];camera=review.STATE['camera'];point=u.Vector(-60465,561595,-6440)
        assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'],1600,900)
        player.set_actor_location_and_rotation(point,u.Rotator(),False,True)
        location=point+u.Vector(350,380,75)
        camera.set_world_location_and_rotation(location,u.MathLibrary.find_look_at_rotation(location,point),False,True)
        CHECK['started']=True;review.queue('NativeLit');return
    if phase=='settle':
        sex,hour,minute,variant=review.STATE['case'];key=sex+'_'+hour
        if key in CHECK['cases']:return
        player=review.STATE['player'];body=player.get_editor_property('mesh')
        assert not player.get_component_by_class(u.ICPlayerToonBrightnessComponent)
        slots=body.get_skeletal_mesh_asset().get_editor_property('materials');rows=[]
        for index,slot in enumerate(slots):
            source=slot.get_editor_property('material_interface');material=body.get_material(index)
            assert source==material,'Unexpected runtime material override'
            assert source.get_path_name().startswith(single.DEST+'/')
            assert single.SOURCE.effective_shading_model(source)==u.MaterialShadingModel.MSM_DEFAULT_LIT
            post=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(source,'PostLightScaleMat')
            assert abs(post-APPLIED['post_light_scale'])<.00001
            color=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(source,'ColorScaleAllMat')
            assert abs(color-APPLIED['color_scale'])<.00001
            rows.append({'slot':index,'material':source.get_path_name(),'post_light_scale':post,'color_scale':color,'shader':'MToon Lit'})
        suns=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.DirectionalLight)
        visible=[s.get_name() for s in suns if s.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
        assert visible==['DirectionalLight_0'],str(visible)
        cycles=u.GameplayStatics.get_all_actors_of_class(review.STATE['world'],u.ICDayNightController)
        assert len(cycles)==1 and [s.get_name() for s in cycles[0].sun_lights]==['DirectionalLight_0']
        CHECK['cases'][key]={'sex':sex,'hour':hour,'visible_suns':visible,'slots':rows}
        return
    if phase=='commands' and not CHECK['complete']:
        assert len(CHECK['cases'])==8 and len(review.STATE['samples'])==8
        for sample in review.STATE['samples']:
            assert struct.unpack('>II',(OUT/(sample['name']+'.png')).read_bytes()[16:24])==(1600,900)
        for rel,digest in APPLIED['protected_hashes'].items():assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==digest,rel
        CHECK.update(complete=True,post_light_scale=APPLIED['post_light_scale'],color_scale=APPLIED['color_scale'],no_unlit_brightness_controller=True,
                     actual_preset_materials_without_review_overrides=True,protected_files_unchanged=True,samples=review.STATE['samples'])
        (OUT/'Verification.json').write_text(json.dumps({k:v for k,v in CHECK.items() if k!='handle'},ensure_ascii=False,indent=2),encoding='utf-8')
        u.unregister_slate_post_tick_callback(CHECK['handle']);review.finish()
CHECK['handle']=u.register_slate_post_tick_callback(tick)
