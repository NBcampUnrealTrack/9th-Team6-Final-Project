"""Refresh inherited fishing-zone mesh overrides in the authored test map."""
import unreal as u
world=u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
water=u.load_asset('/Game/IsekaiClass/Life/Fishing/Materials/M_IC_FishingWater');assert water
plane=u.load_asset('/Engine/BasicShapes/Plane');assert plane
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.ICLifeActor) and str(actor.get_editor_property('point_id'))=='TEST_R01_FISH_RANDOM':
        visual=actor.get_editor_property('visual_mesh');visual.set_editor_property('static_mesh',plane);visual.set_material(0,water)
        point=actor.get_actor_location();visual.set_world_location(u.Vector(point.x,point.y,1),False,False);visual.set_relative_scale3d(u.Vector(5,5,1))
        actor.set_editor_property('interaction_text',u.Text('낚시 구역 · 20초 낚시'))
        u.log('FISHING_PLANE_POLISHED '+str(point))
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/IsekaiClass/World/L_TestMap')
u.log('LIFE_EXPANSION_POLISH_OK')
