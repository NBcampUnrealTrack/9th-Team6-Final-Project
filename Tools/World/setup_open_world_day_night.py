"""Connect L_OpenWorld's existing sun/sky actors to the shared game clock."""
import json,shutil
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/DayNightRepair';out.mkdir(parents=True,exist_ok=True)
map_path='/Game/IsekaiClass/World/Maps/L_OpenWorld'
map_file=root/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap'
backup=out/'L_OpenWorld.before_day_night.umap'
if not backup.exists():shutil.copy2(map_file,backup)

world=u.EditorLoadingAndSavingUtils.load_map(map_path);assert world
assert u.get_editor_subsystem(u.LevelEditorSubsystem).set_current_level_by_name('L_OpenWorld')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
existing=actors.get_all_level_actors()
before_paths={a.get_path_name() for a in existing}
protected=[root/'Content/IsekaiClass/UI/Widget/WBP_IC_Minimap.uasset',root/'Content/IsekaiClass/UI/Widget/WBP_GameHUD.uasset',root/'Content/IsekaiClass/UI/Widget/WBP_IC_Map.uasset']
import hashlib
protected_hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
suns=sorted([a for a in existing if isinstance(a,u.DirectionalLight)],key=lambda a:a.get_name())
skies=sorted([a for a in existing if isinstance(a,u.SkyLight)],key=lambda a:a.get_name())
spheres=[a for a in existing if a.get_class().get_path_name()=='/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere_C']
assert suns and skies,'Existing sun and sky lights are required'

path='/Game/IsekaiClass/World/Blueprints/BP_ICDayNightController'
bp=u.load_asset(path)
created_blueprint=not bp
if not bp:
    factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.ICDayNightController)
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.Blueprint,factory)
assert bp and isinstance(u.get_default_object(bp.generated_class()),u.ICDayNightController)
if created_blueprint:
    defaults=u.get_default_object(bp.generated_class())
    for key,val in {'sunrise_hour':6.,'sunset_hour':18.,'noon_elevation_degrees':60.,'night_sky_intensity_scale':.08,'real_time_sky_capture':True,'is_spatially_loaded':False}.items():
        defaults.set_editor_property(key,val)
    defaults.set_editor_property('sunrise_azimuth_degrees',(suns[0].get_actor_rotation().yaw-270.)%360.)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
controllers=[a for a in existing if isinstance(a,u.ICDayNightController)]
assert len(controllers)<=1,'Duplicate day/night controllers'
controller=controllers[0] if controllers else actors.spawn_actor_from_class(bp.generated_class(),u.Vector(0,0,0))
assert controller
controller.set_actor_label('IC_OpenWorld_DayNight')
controller.set_editor_property('sun_lights',suns)
controller.set_editor_property('sky_lights',skies)
controller.set_editor_property('sky_spheres',spheres)
controller.set_editor_property('is_spatially_loaded',False)
controller.set_editor_property('is_editor_only_actor',False)
controller.set_editor_property('sunrise_hour',6.)
controller.set_editor_property('sunset_hour',18.)
controller.set_editor_property('night_sky_intensity_scale',.08)
controller.set_editor_property('real_time_sky_capture',True)
for index,sun in enumerate(suns):
    component=sun.get_component_by_class(u.DirectionalLightComponent)
    component.set_mobility(u.ComponentMobility.MOVABLE)
    component.set_editor_property('forward_shading_priority',1 if index==0 else 0)
    component.set_editor_property('atmosphere_sun_light',index==0)
    if index==0:component.set_editor_property('atmosphere_sun_light_index',0)
for sky in skies:
    component=sky.get_component_by_class(u.SkyLightComponent)
    component.set_mobility(u.ComponentMobility.MOVABLE)
    if component.get_editor_property('source_type')==u.SkyLightSourceType.SLS_CAPTURED_SCENE:
        component.set_editor_property('real_time_capture',True)
for sphere in spheres:
    sphere.set_editor_property('is_spatially_loaded',False)
    sphere.set_editor_property('Directional light actor',suns[0])
    sphere.set_editor_property('Colors determined by sun position',True)
    sphere.call_method('UpdateSunDirection')
assert before_paths<={a.get_path_name() for a in actors.get_all_level_actors()},'Existing map actors were removed'
assert u.EditorLoadingAndSavingUtils.save_map(world,map_path)
for p in protected:assert hashlib.sha256(p.read_bytes()).hexdigest()==protected_hashes[str(p)],str(p)
report={'controller':controller.get_path_name(),'blueprint':path,'sunrise':6,'sunset':18,'suns':[a.get_path_name() for a in suns],'skylights':[a.get_path_name() for a in skies],'sky_spheres':[a.get_path_name() for a in spheres]}
report.update(existing_actors_preserved=True,hud_and_map_ui_preserved=True,previous_controller_count=len(controllers))
(out/'Setup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_OPEN_WORLD_DAY_NIGHT_READY')
if '-TestExit=' in u.SystemLibrary.get_command_line():u.EditorPythonScripting.set_keep_python_script_alive(True)
