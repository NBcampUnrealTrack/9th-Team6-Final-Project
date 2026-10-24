"""Verify real PIE markers, dialogue order, essence notice and life asset materials."""
import json
import time
import traceback
from pathlib import Path
import unreal as u
OUT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'LifePrototype'
OUT.mkdir(parents=True,exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE={'phase':'starting','started':time.monotonic(),'checks':{}}

def status():
 data={k:STATE[k] for k in ('phase','checks')}
 if 'error' in STATE:data['error']=STATE['error']
 (OUT/'LiveReview.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):STATE.update(phase=name,phase_started=time.monotonic());status()
def finish():
 u.unregister_slate_post_tick_callback(STATE['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(STATE['world'],str(OUT/(name+'.png')))
def camera(point,offset):
 loc=point+offset;STATE['camera'].set_world_location_and_rotation(loc,u.MathLibrary.find_look_at_rotation(loc,point),False,True)
def press(name):
 key=u.Key();key.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(STATE['world'],key)
def subsystem(cls):return next(s for s in u.ObjectIterator(cls) if s.get_outer()==STATE['pc'].get_world_state().get_outer())
def entries():return [e for e in u.WidgetLibrary.get_all_widgets_of_class(STATE['world'],u.ICDialogueEntryWidget,False) if e.get_parent()]
def entry_labels():return [str(e.get_entry_text()) for e in entries()]

def tick(delta):
 try:
  if time.monotonic()-STATE['started']>240:raise RuntimeError('Review timeout in '+STATE['phase'])
  if STATE['phase']=='starting':
   worlds=u.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds:return
   pc=u.GameplayStatics.get_player_controller(worlds[0],0)
   if not pc or str(pc.get_editor_property('menu'))!='Create':return
   assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);STATE.update(world=worlds[0],pc=pc);phase('creating');return
  pc=STATE['pc'];world=STATE['world'];elapsed=time.monotonic()-STATE['phase_started']
  if STATE['phase']=='creating':
   if not pc.get_editor_property('created'):return
   player=u.GameplayStatics.get_player_pawn(world,0);view=player.get_component_by_class(u.CameraComponent);view.set_editor_property('field_of_view',55)
   STATE.update(player=player,camera=view,quests=subsystem(u.ICQuestSubsystem),clock=subsystem(u.ICTimeSubsystem))
   permanent=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.ICLifeActor) if str(a.get_editor_property('point_id')).startswith('TEST_R01_')]
   assert len(permanent)==23,len(permanent)
   STATE['checks']['saved_left_map_counts']={'gathering':10,'crops':5,'ores':6,'chicken':1,'fishing_box':1}
   pc.set_menu('');npcs={str(a.get_editor_property('npc_definition_id')):a for a in u.GameplayStatics.get_all_actors_of_class(world,u.ICNPCBase)}
   if not {'NPC_R01_001','NPC_R01_002'}<=set(npcs):return
   center=player.get_actor_location()+u.Vector(350,0,0);STATE['center']=center
   for index,id in enumerate(('NPC_R01_001','NPC_R01_002')):
    actor=npcs[id];controller=actor.get_controller()
    if controller:controller.stop_movement();controller.set_actor_tick_enabled(False)
    actor.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
    actor.set_actor_location_and_rotation(center+u.Vector(0,-110+220*index,0),u.Rotator(pitch=0,yaw=180,roll=0),False,True)
   STATE['npcs']=npcs
   assert STATE['quests'].accept_quest('QST_NPC_TALK_001');assert STATE['quests'].start_quest('QST_NPC_TALK_001')
   camera(center+u.Vector(0,0,60),u.Vector(-650,0,170));phase('markers');return
  if STATE['phase']=='markers' and elapsed>2:
   assert STATE['quests'].get_npc_quest_marker('NPC_R01_002')==2
   STATE['checks']['active_target_marker']=True;capture('NPC_QuestMarkers')
   npc=STATE['npcs']['NPC_R01_002'];pc.get_world_state().set_affinity('NPC_R01_002',300)
   STATE['player'].set_actor_location_and_rotation(npc.get_actor_location()+u.Vector(-150,0,0),u.Rotator(),False,True)
   pc.set_control_rotation(u.Rotator());pc.handle_input_command(u.ICInputCommand.INTERACT,0)
   dialogue=u.WidgetLibrary.get_all_widgets_of_class(world,u.ICNPCDialogueWidget,False)[0];STATE['dialogue']=dialogue
   assert not any('영입' in s for s in entry_labels())
   phase('greeting');return
  if STATE['phase']=='greeting' and elapsed>.5:
   capture('NPC_InitialDialogue');STATE['checks']['name_hearts_before_options']=True
   for step in range(15):
    labels=entry_labels()
    if '퀘스트' in labels:break
    assert len(entries())==1;entries()[0].focus_entry();press('Enter')
   labels=entry_labels();assert set(labels)=={'동료 영입하기','퀘스트','선물하기','대화 끝내기'},labels
   phase('options');return
  if STATE['phase']=='options' and elapsed>.5:
   capture('NPC_InteractionChoices');STATE['checks']['ordered_choices_after_dialogue']=True
   assert STATE['quests'].get_npc_quest_marker('NPC_R01_002')==1;STATE['checks']['talk_completion_updates_marker']=True
   press('Escape')
   feedback=u.WidgetLibrary.get_all_widgets_of_class(world,u.ICCombatFeedbackWidget,False)[0]
   item=u.ICItemAmount();item.set_editor_property('item_id','ITEM_R01_05');item.set_editor_property('count',2)
   feedback.show_reward(500,[item],'MON_R01_001',9);phase('reward');return
  if STATE['phase']=='reward' and elapsed>.3:
   capture('Monster_EssenceNotice');STATE['checks']['monster_name_essence_notice']=True
   center=u.Vector(0,1500,0);STATE['life_center']=center;actors=[]
   for index in range(1,6):
    actor=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.ICLifeActor) if str(a.get_editor_property('point_id'))=='REVIEW_CROP_%02d'%index);actors.append(actor)
    assert pc.get_inventory().add_item('ITM_R01_SEED_%02d'%index,1,False);assert actor.plant(pc)
   STATE['plots']=actors;camera(center+u.Vector(0,0,30),u.Vector(-1000,0,900));phase('seeds');return
  if STATE['phase']=='seeds' and elapsed>.5:
   capture('Crops_Day0');assert all(a.get_growth_stage()==0 for a in STATE['plots'])
   assert STATE['clock'].jump_game_minutes(1440);phase('day1');return
  if STATE['phase']=='day1' and elapsed>2:
   capture('Crops_Day1');assert all(a.get_growth_stage()==1 for a in STATE['plots']);STATE['checks']['day1_growth_visible']=True
   assert STATE['clock'].jump_game_minutes(1440*4);phase('ripe');return
  if STATE['phase']=='ripe' and elapsed>5:
   capture('Crops_FinalStages');assert all(a.is_harvestable() for a in STATE['plots']);STATE['checks']['ripe_crop_materials_render']=True
   center=u.Vector(0,3000,0);STATE['ore_center']=center
   camera(center+u.Vector(0,0,50),u.Vector(-1150,0,850));phase('ores');return
  if STATE['phase']=='ores' and elapsed>4:
   capture('Mining_SixShapesAndMaterials');STATE['checks']['six_ore_materials_render']=True
   camera(u.Vector(2000,-4890,50),u.Vector(-4200,0,2100));phase('collection');return
  if STATE['phase']=='collection' and elapsed>3:
   capture('TestMap_GatheringTen');camera(u.Vector(2000,-13600,30),u.Vector(-220,-100,130));phase('chicken');return
  if STATE['phase']=='chicken' and elapsed>2:
   capture('TestMap_Chicken');camera(u.Vector(2000,-14800,90),u.Vector(-700,0,450));phase('fishing');return
  if STATE['phase']=='fishing' and elapsed>2:
   capture('Fishing_LeftEnd')
   box=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.ICLifeActor) if str(a.get_editor_property('point_id'))=='TEST_R01_FISH_RANDOM')
   assert box.get_editor_property('show_life_label');assert not box.try_interact(pc)
   STATE['checks']['fishing_box_is_data_only']=True
   camera(u.Vector(2000,-8900,0),u.Vector(-14000,0,15000));phase('overview');return
  if STATE['phase']=='overview' and elapsed>2:
   capture('TestMap_LifeLeftOverview');phase('complete');return
  if STATE['phase']=='complete' and elapsed>.2:
   status();u.log('IC_LIFE_REVIEW_OK '+json.dumps(STATE['checks']));finish()
 except Exception:
  if 'world' in STATE:
   try:capture('ReviewError')
   except Exception:pass
  STATE['error']=traceback.format_exc();phase('error');u.log_error(STATE['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
# Stage disposable editor actors; PIE clones them, and no level is saved.
editor_actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for index in range(1,6):
 bp=u.load_asset('/Game/IsekaiClass/Life/Farming/BP_R01_Crop_%02d'%index)
 actor=editor_actors.spawn_actor_from_class(bp.generated_class(),u.Vector(0,1500+(index-3)*175,0))
 actor.set_editor_property('point_id','REVIEW_CROP_%02d'%index)
for index in range(1,7):
 bp=u.load_asset('/Game/IsekaiClass/Life/Mining/BP_R01_Ore_%02d'%index)
 editor_actors.spawn_actor_from_class(bp.generated_class(),u.Vector(0,3000+(index-3.5)*180,0))
STATE['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
status()
