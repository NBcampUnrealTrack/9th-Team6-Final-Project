"""Selected reference 8: MToon Lit on both presets and one world sun."""
import hashlib,json,shutil
from datetime import datetime
from pathlib import Path
import unreal as u
import setup_player_world_lighting as SOURCE
import setup_player_part_lighting as PART

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting/SingleSunLighting'
DEST='/Game/IsekaiClass/Player/Materials/SingleSunLighting'
MAP='/Game/IsekaiClass/World/Maps/L_OpenWorld'


def create_materials(post_scale=1.,color_scale=1.):
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    converted={}
    for sex,materials in SOURCE.source_materials().items():
        converted[sex]=[]
        for source in materials:
            path=f'{DEST}/{sex}/{source.get_name()}_Sun'
            target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
            assert target,path
            part=PART.category(sex,source.get_name());face=part in ['features','skin']
            parent=u.load_asset(source.get_editor_property('parent').get_path_name().split('.')[0].replace('Unlit','Lit'))
            assert parent
            u.MaterialEditingLibrary.set_material_instance_parent(target,parent)
            overrides=target.get_editor_property('base_property_overrides')
            overrides.set_editor_property('override_shading_model',False)
            target.set_editor_property('base_property_overrides',overrides)
            switches={str(n) for n in u.MaterialEditingLibrary.get_static_switch_parameter_names(target)}
            scalars={str(n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(target)}
            for name,value in {'bUseLight':True,'bUseMRSA':True,'bUseMatCap':False,'bUseRimLight':False,'bUseNormalmap':not face,'bUseCustomReflection':False}.items():
                if name in switches:u.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(target,name,value)
            for name,value in {'NormalToCameraMat':.95 if face else 0.,'NormalFresnelScale':2. if face else 1.,'NormalToLightMat':.10,'mtoon_ReceiveShadowRate':0. if face else .65,'mtoon_NormalScale':.15 if part=='hair' else .25}.items():
                if name in scalars:u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(target,name,value)
            assert 'PostLightScaleMat' in scalars
            assert 'ICCharacterBrightness' not in scalars
            u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(target,'PostLightScaleMat',post_scale)
            assert 'ColorScaleAllMat' in scalars
            u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(target,'ColorScaleAllMat',color_scale)
            u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(target,'MRSA',u.LinearColor(0,1. if face else .85,0. if face else .05,1.))
            u.MaterialEditingLibrary.update_material_instance(target)
            assert SOURCE.effective_shading_model(target)==u.MaterialShadingModel.MSM_DEFAULT_LIT
            assert u.EditorAssetLibrary.save_loaded_asset(target,False)
            converted[sex].append(target)
    return converted


def apply(color_scale=1.):
    assert '-ICApplySingleSunLighting' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    OUT.mkdir(parents=True,exist_ok=True)
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    map_file=ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap'
    shutil.copy2(map_file,backup/map_file.name)
    protected=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    protected += [p for p in (ROOT/'Content/IsekaiClass/Player/Materials').rglob('*.uasset') if '/SingleSunLighting/' not in p.as_posix()]
    protected += [p for p in (ROOT/'Content/IsekaiClass/Player/SamplePlayer').rglob('*.uasset') if p.name not in ['SK_Player_M.uasset','SK_Player_F.uasset']]
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    converted=create_materials(1.,color_scale)
    rows=[]
    for sex,folder in SOURCE.PLAYERS:
        path=SOURCE.mesh_path(sex,folder);file=ROOT/'Content'/Path(path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(path);slots=list(mesh.get_editor_property('materials'))
        assert len(slots)==len(converted[sex])
        for index,(slot,material) in enumerate(zip(slots,converted[sex])):
            rows.append({'sex':sex,'slot':index,'previous':slot.get_editor_property('material_interface').get_path_name(),'assigned':material.get_path_name(),'color_scale':color_scale,'post_light_scale':1.})
            slot.set_editor_property('material_interface',material)
        mesh.set_editor_property('materials',slots)
        assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    world=u.EditorLoadingAndSavingUtils.load_map(MAP);assert world
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    actor_paths={a.get_path_name() for a in actors}
    suns={a.get_name():a for a in actors if isinstance(a,u.DirectionalLight)}
    assert set(suns)=={'DirectionalLight_0','LightSource_2','LightSource_1'},str(list(suns))
    controllers=[a for a in actors if isinstance(a,u.ICDayNightController)];assert len(controllers)==1
    before={'sun_lights':[a.get_name() for a in controllers[0].sun_lights],
            'lights':[{ 'name':name,'intensity':a.get_component_by_class(u.DirectionalLightComponent).get_editor_property('intensity'),'visible':a.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')} for name,a in suns.items()]}
    for name,actor in suns.items():
        light=actor.get_component_by_class(u.DirectionalLightComponent);primary=name=='DirectionalLight_0'
        light.set_editor_property('visible',primary)
        light.set_editor_property('intensity',6. if primary else 0.)
    controllers[0].set_editor_property('sun_lights',[suns['DirectionalLight_0']])
    assert actor_paths=={a.get_path_name() for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    assert u.EditorLoadingAndSavingUtils.save_map(world,MAP)
    for path,digest in hashes.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,path
    result={'selected_method':'08_single_sun','color_scale':color_scale,'post_light_scale':1.,'slots':rows,'before':before,
            'active_sun':'DirectionalLight_0','daytime_sun_lux':6.,'disabled_suns':['LightSource_2','LightSource_1'],
            'backup':str(backup),'protected_hashes':hashes,'protected_files_unchanged':True}
    (OUT/'Applied.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_SINGLE_SUN_LIGHTING_APPLIED')
    return result
