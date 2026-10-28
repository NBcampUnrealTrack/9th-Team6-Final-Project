"""Rendered PIE review. Uses a disposable save namespace; never saves map assets."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
FEMALE='-ICLifeFemale' in u.SystemLibrary.get_command_line()
SLEEP_ONLY='-ICLifeSleepOnly' in u.SystemLibrary.get_command_line()
OUT=ROOT/('Saved/LifeExpansion/Live'+('Female' if FEMALE else '')+('Sleep' if SLEEP_ONLY else ''))
OUT.mkdir(parents=True,exist_ok=True)
S=dict(phase='starting',started=time.monotonic(),checks={},complete=False)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)

def report():
    (OUT/'Review.json').write_text(json.dumps({k:S[k] for k in ('phase','checks','complete','error') if k in S},ensure_ascii=False,indent=2),encoding='utf-8')
def phase(name):S.update(phase=name,phase_started=time.monotonic());report()
def capture(name):assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'],str(OUT/(name+'.png')))
def finish():
    report();u.unregister_slate_post_tick_callback(S['handle']);LEVELS.editor_request_end_play();u.SystemLibrary.quit_editor()
def widgets(cls):return u.WidgetLibrary.get_all_widgets_of_class(S['world'],cls,False)
def system(cls):return next(o for o in u.ObjectIterator(cls) if o.get_outer()==S['pc'].get_world_state().get_outer())
def press(name):
    key=u.Key();key.set_editor_property('key_name',name);assert u.ICCharacterSetupLibrary.press_ui_review_key(S['world'],key),name
def camera_at(point,distance=550):
    view=S['camera'];position=point+u.Vector(-distance,-distance*.65,distance*.55)
    view.set_world_location_and_rotation(position,u.MathLibrary.find_look_at_rotation(position,point),False,True)
def approach(actor):
    point=actor.get_actor_location();S['player'].set_actor_location_and_rotation(point+u.Vector(-170,0,96 if abs(point.z)<60 else 0),u.Rotator(),False,True)
    S['player'].get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();S['pc'].set_control_rotation(u.Rotator());camera_at(point+u.Vector(0,0,60))
def interact():S['pc'].handle_input_command(u.ICInputCommand.INTERACT,0)
def life_actor(point):return next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICLifeActor) if str(a.get_editor_property('point_id'))==point)
def check_montage(activity):
    code='Female' if FEMALE else 'Male';asset=u.load_asset('/Game/IsekaiClass/Animation/Player/Main'+code+'/Life/AM_Life_'+activity)
    assert S['player'].get_component_by_class(u.SkeletalMeshComponent).get_anim_instance().montage_is_playing(asset),activity
def begin_work(point,activity):
    actor=life_actor(point);approach(actor)
    if activity=='Fishing':
        origin,extent,_=u.SystemLibrary.get_component_bounds(actor.get_editor_property('visual_mesh'));landing=origin+u.Vector(0,0,extent.z)
        S['player'].set_actor_location(landing+u.Vector(-650,0,96),False,True);camera_at(landing,1000)
    S.update(target=actor,activity=activity,shot=False);phase('work_start')
def stop_ai(actor):
    controller=actor.get_controller()
    if controller:
        brain=controller.get_component_by_class(u.BrainComponent)
        if brain:brain.stop_logic('Life review')
        controller.set_actor_tick_enabled(False)

def tick(delta):
    try:
        now=time.monotonic()
        if now-S['started']>300:raise RuntimeError('Review timeout: '+S['phase'])
        if S['phase']=='starting':
            worlds=u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            pc=u.GameplayStatics.get_player_controller(worlds[0],0)
            if not pc or str(pc.get_editor_property('menu'))!='Create':return
            if FEMALE:pc.set_editor_property('preset',1)
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0]);S.update(world=worlds[0],pc=pc);phase('creating');return
        pc=S['pc'];elapsed=now-S['phase_started']
        if S['phase']=='creating':
            if not pc.get_editor_property('created'):return
            player=u.GameplayStatics.get_player_pawn(S['world'],0);camera=player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD)
            S.update(player=player,camera=camera,inventory=pc.get_inventory(),state=pc.get_world_state(),life=player.get_component_by_class(u.ICPlayerLifeComponent),gather=system(u.ICGatheringSubsystem),clock=pc.get_time())
            for actor in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Character):
                if actor!=player:stop_ai(actor)
            for i in range(1,6):assert S['inventory'].add_item(f'ITM_R01_SEED_{i:02d}',3,False)
            assert S['inventory'].add_item('ITM_TKT_001',1,False);assert pc.set_gold(10000)
            assert len(u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICFarmPlot))==8
            chests=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICPlayerStorage)
            assert len(chests)==3 and sum(not a.get_editor_property('hidden') for a in chests)==1
            S['checks']['eight_plots_and_one_of_three_chests']=True;S['chests']=chests
            pc.set_menu('')
            if SLEEP_ONLY:
                bed=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICPlayerBed)[0];approach(bed);S['bed']=bed;phase('bed_start');return
            begin_work('TEST_R01_GATHER_01','Gathering');return
        if S['phase']=='work_start' and elapsed>.8:
            S['count_before']=S['inventory'].count_item('ITM_R01_PLANT_01')
            if S['activity']=='Fishing':pc.request_fishing()
            else:interact()
            assert S['gather'].is_gathering(),S['activity'];phase('work');return
        if S['phase']=='work':
            gather=S['gather'];activity=S['activity']
            if gather.is_gathering():
                if elapsed>.8 and not S['shot']:
                    check_montage(activity)
                    tools=[a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.Actor) if a.get_owner()==S['player'] and ('Pickaxe' in a.get_class().get_name() or 'FishingPole' in a.get_class().get_name())]
                    assert len(tools)==(0 if activity=='Gathering' else 1),(activity,tools)
                    if tools:
                        mesh=tools[0].get_component_by_class(u.StaticMeshComponent);S['checks'][activity+'_tool_bounds']=str(u.SystemLibrary.get_component_bounds(mesh))
                    capture(activity);S['shot']=True
                return
            assert S['shot'],activity
            S['checks'][activity+'_animation_and_completion']=True
            if activity=='Gathering':
                assert S['inventory'].count_item('ITM_R01_PLANT_01')==S['count_before']+1
                begin_work('TEST_R01_ORE_01','Mining');return
            if activity=='Mining':begin_work('TEST_R01_FISH_RANDOM','Fishing');return
            fish_count=sum(S['inventory'].count_item(f'ITM_R01_FISH_{i:02d}') for i in range(1,11));assert fish_count==1
            pc.request_fishing();assert S['gather'].is_gathering(),'Fishing cooldown must be absent';S['gather'].cancel_gather();S['checks']['fishing_twenty_seconds_and_immediate_repeat']=elapsed>=19.8
            plot=life_actor('ANC_PLAYER_PLOT_01');approach(plot);S['plot']=plot;phase('plot_start');return
        if S['phase']=='plot_start' and elapsed>.8:
            interact();pickers=widgets(u.ICSeedPickerWidget);assert pickers
            S['picker']=pickers[0];capture('SeedPicker');S['picker'].pick_seed(0);phase('plant');return
        if S['phase']=='plant':
            if S['gather'].is_gathering():
                if elapsed>.7 and not S.get('plant_shot'):
                    check_montage('Planting');capture('Planting');S['plant_shot']=True
                return
            if elapsed<1:return
            assert str(S['plot'].get_editor_property('definition_id'))=='CRP_R01_01'
            assert S['inventory'].count_item('ITM_R01_SEED_01')==2
            assert S['clock'].jump_game_minutes(1440);assert S['plot'].get_growth_stage()==1
            assert S['clock'].jump_game_minutes(1440*5);assert S['plot'].is_harvestable()
            S['checks']['plant_seed_consume_and_daily_growth']=True;capture('RipePlot');interact();assert S['gather'].is_gathering();phase('harvest');return
        if S['phase']=='harvest':
            if S['gather'].is_gathering():
                if elapsed>.8 and not S.get('harvest_shot'):check_montage('Harvesting');capture('Harvesting');S['harvest_shot']=True
                return
            assert S['inventory'].count_item('ITM_R01_CROP_01')>0
            assert S['plot'].get_growth_stage()==-1;S['checks']['harvest_frees_plot']=True
            npc=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICNPCBase) if str(a.get_character_id())=='NPC_R01_006');approach(npc);S['npc']=npc;phase('shop_npc');return
        if S['phase']=='shop_npc' and elapsed>.8:
            interact();phase('shop_dialogue');return
        if S['phase']=='shop_dialogue':
            entries=widgets(u.ICDialogueEntryWidget)
            trade=next((v for v in entries if '상점' in str(v.get_entry_text())),None)
            if not trade:
                if elapsed>1:press('Enter');phase('shop_options')
                return
            trade.focus_entry();press('Enter');phase('shop_open');return
        if S['phase']=='shop_options' and elapsed>.3:
            trade=next(v for v in widgets(u.ICDialogueEntryWidget) if '상점' in str(v.get_entry_text()));trade.focus_entry();press('Enter');phase('shop_open');return
        if S['phase']=='shop_open' and elapsed>.6:
            shop=widgets(u.ICShopWidget)[0];S['shop']=shop;S['shop_gold']=pc.get_editor_property('gold');S['shop_seed']=S['inventory'].count_item('ITM_R01_SEED_01')
            shop.item_picked(0);shop.item_picked(0);shop.item_picked(1);assert len(shop.get_editor_property('cart'))==2;capture('ShopBuyCart');phase('shop_buy');return
        if S['phase']=='shop_buy' and elapsed>.5:
            shop=S['shop'];assert shop.submit_cart();assert S['inventory'].count_item('ITM_R01_SEED_01')==S['shop_seed']+2;assert pc.get_editor_property('gold')<S['shop_gold'];shop.select_tab(True);shop.item_picked(0);capture('ShopSellCart');phase('shop_sell');return
        if S['phase']=='shop_sell' and elapsed>.5:
            shop=S['shop'];assert shop.submit_cart();gold=pc.get_editor_property('gold');shop.item_picked(0);shop.close_shop();assert len(shop.get_editor_property('cart'))==0;assert pc.get_editor_property('gold')==gold
            S['checks']['npc06_shop_cart_buy_sell_cancel']=True
            pc.set_menu('Pause');menus=widgets(u.ICGameMenuWidget);assert menus[0].select_tab('Inventory');ui=widgets(u.ICInventoryMenuWidget)[0]
            ui.select_item_tab(u.PlayerItemTab.NORMAL);items=[v for v in S['inventory'].get_snapshot().bag.slots if str(v.item_id)!='None' and v.count>0];index=next(i for i,v in enumerate(items) if str(v.item_id)=='ITM_TKT_001')
            tickets=S['inventory'].count_item('ITM_TKT_001');capacity=len(S['inventory'].get_snapshot().bag.slots)
            assert ui.open_item(index);capture('ExpansionTicket');assert ui.begin_use_selected();assert not ui.get_editor_property('picking_use_target');assert S['inventory'].count_item('ITM_TKT_001')==tickets-1
            assert len(S['inventory'].get_snapshot().bag.slots)==capacity+10
            S['checks']['inventory_expansion_ticket_direct_use']=True
            menus[0].select_tab('Codex');codex=widgets(u.ICCodexMenuWidget)[0];codex.select_category(0);S['codex']=codex;capture('SkillCodex');phase('codex');return
        if S['phase']=='codex' and elapsed>.7:
            pets=system(u.ICCodexSubsystem);assert pets.add_pet_essence(0,100);assert pets.equip_pet(0,S['player'].get_character_id())
            S['codex'].select_category(1);phase('pet_codex');return
        if S['phase']=='pet_codex' and elapsed>.6:
            capture('PetCodex');S['checks']['actual_skill_and_pet_codex_visible']=True;pc.set_menu('')
            assert S['state'].set_main_stat(S['player'].get_character_id(),u.ICMainStat.CON,20)
            assert S['inventory'].add_item('ITM_R01_ORE_01',2,False);assert S['inventory'].add_item('ITM_R01_ORE_02',2,False)
            S['station']=next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICCraftingStation) if a.get_editor_property('recipe_type')==u.PlayerRecipeType.CRAFT)
            S['storage_step']=1;approach(S['station']);phase('storage_start');return
        if S['phase']=='storage_start' and elapsed>.8:
            assert S['station'].try_interact(pc);station_ui=next(v for v in widgets(u.ICCraftingStationWidget) if v.is_open());station_ui.select_recipe(f"RCP_PLAYER_STORAGE_{S['storage_step']:02d}")
            S['station_ui']=station_ui;phase('storage_detail');return
        if S['phase']=='storage_detail' and elapsed>.6:
            capture('StorageRecipe'+str(S['storage_step']));S['station_ui'].make_recipe();phase('storage_work');return
        if S['phase']=='storage_work':
            if S['station_ui'].is_open():return
            for chest in S['chests']:chest.refresh_availability()
            assert sum(not a.get_editor_property('hidden') for a in S['chests'])==S['storage_step']+1
            if S['storage_step']==1:S['storage_step']=2;phase('storage_start');return
            recipes=pc.get_crafting().get_recipes(u.PlayerRecipeType.CRAFT);assert all('RCP_PLAYER_STORAGE' not in str(r) for r in recipes)
            S['checks']['two_crafted_chests_visible_and_recipes_removed']=True
            bed=u.GameplayStatics.get_all_actors_of_class(S['world'],u.ICPlayerBed)[0];approach(bed);S['bed']=bed;phase('bed_start');return
        if S['phase']=='bed_start' and elapsed>.8:
            S['sleep_minute']=S['clock'].get_clock().game_minute;interact();assert S['life'].is_sleeping();phase('sleep');return
        if S['phase']=='sleep':
            if elapsed>.75 and S['life'].is_sleep_pose_active() and not S.get('sleep_pose'):
                mesh=S['player'].get_component_by_class(u.SkeletalMeshComponent);head=mesh.get_socket_location('J_Bip_C_Head');feet=(mesh.get_socket_location('J_Bip_L_Foot')+mesh.get_socket_location('J_Bip_R_Foot'))*.5
                direction=head-feet;right=S['bed'].get_actor_right_vector();alignment=abs((direction.x*right.x+direction.y*right.y)/max(.01,(direction.x**2+direction.y**2)**.5))
                S['checks']['sleep_alignment']=alignment;S['checks']['sleep_offset']=str(S['bed'].get_editor_property('sleep_pose_offset'))
                S['checks']['sleep_head_feet']=[[v.x,v.y,v.z] for v in (head,feet)]
                capture('SleepPose');S['sleep_pose']=True;report();assert alignment>.9 and abs(head.z-feet.z)<65
            if elapsed>1.5 and not S.get('sleep_black'):capture('SleepBlack');S['sleep_black']=True
            if S['life'].is_sleeping():return
            assert elapsed>1.8 and S['clock'].get_clock().game_minute>S['sleep_minute'];assert not widgets(u.ICSleepFadeWidget) or all(not v.is_in_viewport() for v in widgets(u.ICSleepFadeWidget))
            S['checks']['sleep_pose_fade_and_calendar_jump']=True;phase('wake_settle');return
        if S['phase']=='wake_settle' and elapsed>.7:
            capture('Wake');phase('complete');return
        if S['phase']=='complete':S['complete']=True;u.log('LIFE_EXPANSION_REVIEW_OK');finish()
    except Exception:
        S['error']=traceback.format_exc();phase('error');u.log_error(S['error']);finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle']=u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
