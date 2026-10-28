"""Fit the existing player sleep pose to the medieval test-map bed."""
import unreal as u
path='/Game/IsekaiClass/Life/Home/Blueprints/BP_IC_PlayerBed'
bp=u.load_asset(path);assert bp
offset=u.Transform(location=u.Vector(36,0,-48),rotation=u.Rotator(yaw=-90))
cdo=u.get_default_object(bp.generated_class());cdo.set_editor_property('sleep_pose_offset',offset)
u.log('PLAYER_BED_BOUNDS '+str(cdo.get_editor_property('bed_visual').get_editor_property('static_mesh').get_bounding_box()))
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
world=u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,u.ICPlayerBed):actor.set_editor_property('sleep_pose_offset',offset)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/IsekaiClass/World/L_TestMap')
u.log('LIFE_BED_CALIBRATION_OK')
