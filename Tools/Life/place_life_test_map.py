"""Lay out life prototypes to the left, ending in the future random fishing box."""
import json
from pathlib import Path
import unreal as u

GAME='/Game/IsekaiClass';world=u.EditorLoadingAndSavingUtils.load_map(GAME+'/World/L_TestMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem);existing={str(a.get_editor_property('point_id')):a for a in actors.get_all_level_actors() if isinstance(a,u.ICLifeActor)}
report={'map':world.get_path_name(),'actors':[]}
def place(path,key,name,y,preview=False,z=0):
 bp=u.load_asset(path);assert bp,path
 actor=existing.get(key) or actors.spawn_actor_from_class(bp.generated_class(),u.Vector(2000,y,z))
 actor.set_actor_location_and_rotation(u.Vector(2000,y,z),u.Rotator(),False,True)
 actor.set_editor_property('point_id',key);actor.set_editor_property('preview_unplanted_crop',preview)
 if key=='TEST_R01_FISH_RANDOM':actor.set_editor_property('show_life_label',True)
 actor.set_actor_label(name);actor.set_folder_path(u.Name('IC_Life_Test/'+path.split('/')[-2]))
 report['actors'].append({'point':key,'name':name,'class':path,'position':[2000,y,z]})
 return actor
for i in range(1,11):place(GAME+'/Life/Collection/BP_R01_Collection_%02d'%i,'TEST_R01_GATHER_%02d'%i,'채집 식물 %02d'%i,-3000-(i-1)*420)
for i,name in enumerate(('밀','외줄밀','양배추','유채','해바라기'),1):place(GAME+'/Life/Farming/BP_R01_Crop_%02d'%i,'TEST_R01_CROP_%02d'%i,'농사 '+name,-7800-(i-1)*420,True)
for i in range(1,7):place(GAME+'/Life/Mining/BP_R01_Ore_%02d'%i,'TEST_R01_ORE_%02d'%i,'광석 %d'%i,-10500-(i-1)*420)
place(GAME+'/Life/Ranching/BP_R01_Chicken','TEST_R01_CHICKEN','목축 닭',-13600)
place(GAME+'/Life/Fishing/BP_R01_RandomFishingZone','TEST_R01_FISH_RANDOM','물고기 랜덤',-14800,z=40)
for actor in actors.get_all_level_actors():
 if actor.get_actor_label()=='IC_TestGround':
  actor.set_actor_scale3d(u.Vector(200,400,1));actor.set_actor_location(u.Vector(0,1000,-50),False,True)
 if isinstance(actor,u.NavMeshBoundsVolume) and actor.actor_has_tag('IC.NPCTestNavigation'):
  actor.set_actor_location(u.Vector(1500,1000,200),False,True)
  for bounds in actor.get_components_by_class(u.BoxComponent):
   if bounds.get_name()=='TestNavigationBounds':bounds.set_box_extent(u.Vector(6000,17000,500),True)
anchors=[a for a in actors.get_all_level_actors() if isinstance(a,u.ICWorldAnchor) and str(a.get_editor_property('anchor_id'))=='ANC_R01_FISH_RANDOM_001']
if not anchors:
 anchor=actors.spawn_actor_from_class(u.ICWorldAnchor,u.Vector(2000,-14800,40));anchor.set_editor_property('anchor_id','ANC_R01_FISH_RANDOM_001');anchor.set_actor_label('IC_Fishing_Random_Anchor')
assert u.EditorLoadingAndSavingUtils.save_map(world,GAME+'/World/L_TestMap')
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'LifePrototype';out.mkdir(parents=True,exist_ok=True)
(out/'TestMapLayout.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_TEST_MAP_OK gathering=10 crops=5 ores=6 chicken=1 fishing=1')
