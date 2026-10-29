"""Read current open-world clock/light wiring without saving the level."""
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/DayNightRepair';out.mkdir(parents=True,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name()!='L_OpenWorld':
    world=u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()


def value(obj,name):
    try:
        val=obj.get_editor_property(name)
        if isinstance(val,u.Object):return val.get_path_name()
        if val is None or isinstance(val,(int,float,bool,str)):return val
        return str(val)
    except Exception:return None


report={'world':world.get_path_name(),'actor_count':len(actors),'controllers':[],'lights':[],'sky_spheres':[],'postprocess':[]}
for actor in actors:
    if isinstance(actor,u.ICDayNightController):
        report['controllers'].append({'actor':actor.get_path_name(),'label':actor.get_actor_label(),
            'sun_lights':[value for value in [a.get_path_name() if a else None for a in actor.sun_lights]],
            'sky_lights':[a.get_path_name() if a else None for a in actor.sky_lights],
            'sky_spheres':[a.get_path_name() if a else None for a in actor.sky_spheres],
            'sunrise':value(actor,'sunrise_hour'),'sunset':value(actor,'sunset_hour'),
            'night_sky_scale':value(actor,'night_sky_intensity_scale'),'tick':str(actor.get_editor_property('primary_actor_tick'))})
    for component in actor.get_components_by_class(u.ActorComponent):
        if not isinstance(component,(u.DirectionalLightComponent,u.SkyLightComponent)):continue
        report['lights'].append({'actor':actor.get_path_name(),'label':actor.get_actor_label(),
            'component':component.get_name(),'class':component.get_class().get_name(),
            'properties':{key:value(component,key) for key in ['mobility','intensity','visible','atmosphere_sun_light','forward_shading_priority','real_time_capture','source_type','cubemap']}})
    if 'BP_Sky_Sphere' in actor.get_class().get_path_name():
        report['sky_spheres'].append({'actor':actor.get_path_name(),'light':value(actor,'Directional light actor'),
            'automatic_colors':value(actor,'Colors determined by sun position'),'height':value(actor,'Sun height')})
    if isinstance(actor,u.PostProcessVolume):
        settings=actor.get_editor_property('settings')
        report['postprocess'].append({'actor':actor.get_path_name(),'unbound':value(actor,'unbound'),
            'settings':{key:value(settings,key) for key in ['override_auto_exposure_method','auto_exposure_method','auto_exposure_min_brightness','auto_exposure_max_brightness','auto_exposure_bias','ambient_cubemap_intensity']}})
(out/'Before.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_DAY_NIGHT_REPAIR_AUDIT_COMPLETE')
