"""Refresh the UI map capture from this branch's L_OpenWorld without saving the level.

Run in UnrealEditor-Cmd with -run=pythonscript -AllowCommandletRendering.
Then run setup_open_world_map.py to reimport the texture and projection.
"""
from pathlib import Path
import json
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
world = u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
assert world
u.SystemLibrary.execute_console_command(world, 'Editor.AsyncAssetCompilationFinishAll')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
rotation = u.Rotator(pitch=-90, yaw=0, roll=0)
capture = actors.spawn_actor_from_class(u.SceneCapture2D, u.Vector(-56000,610000,65000), rotation)
assert capture
try:
    component = capture.get_editor_property('capture_component2d')
    target = u.RenderingLibrary.create_render_target2d(world,3072,3072,u.TextureRenderTargetFormat.RTF_RGBA8,u.LinearColor(.015,.02,.025,1),False,False)
    component.set_editor_property('projection_type', u.CameraProjectionMode.ORTHOGRAPHIC)
    component.set_editor_property('ortho_width', 65000)
    component.set_editor_property('texture_target', target)
    component.set_editor_property('capture_source', u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    component.set_editor_property('capture_every_frame', False)
    component.set_editor_property('capture_on_movement', False)
    component.set_editor_property('post_process_blend_weight', 0)
    component.capture_scene()
    destination = root/'Content/IsekaiClass/UI/Map/Source'
    destination.mkdir(parents=True,exist_ok=True)
    u.RenderingLibrary.export_render_target(world,target,str(destination),'L_OpenWorld_DevelopedArea_TopDown.png')
    (destination/'Capture.json').write_text(json.dumps(dict(
        level='/Game/IsekaiClass/World/Maps/L_OpenWorld',centre_cm=[-56000,610000],
        ortho_width_cm=65000,resolution=[3072,3072],right_axis=[0,1],down_axis=[-1,0],
        level_saved=False),indent=2),encoding='utf-8')
    u.log('IC_MAP_CAPTURE_OK')
finally:
    actors.destroy_actor(capture)
