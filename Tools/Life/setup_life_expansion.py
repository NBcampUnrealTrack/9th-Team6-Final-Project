"""Targeted life/shop/codex authoring. Preserves prior combat, items and recipes."""
import copy
import csv
import io
import json
import time
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass';BASE=GAME+'/Data/DataTable/'
OUT=ROOT/'Saved/LifeExpansion';OUT.mkdir(parents=True,exist_ok=True)
TOOLS=u.AssetToolsHelpers.get_asset_tools();TABLES={};REPORT={'tools':[],'animations':[],'map':[],'widgets':[],'tables':[]}

def save(asset):assert u.EditorAssetLibrary.save_loaded_asset(asset,False),str(asset)
def table(relative):
    if relative not in TABLES:
        asset=u.load_asset(BASE+relative);assert asset,relative
        TABLES[relative]=(asset,{row['Name']:row for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(asset))})
    return TABLES[relative][1]
def blueprint(path,parent,widget=False):
    asset=u.load_asset(path)
    if not asset:
        factory=u.WidgetBlueprintFactory() if widget else u.BlueprintFactory();factory.set_editor_property('parent_class',parent)
        asset=TOOLS.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.WidgetBlueprint if widget else u.Blueprint,factory)
    assert asset,path
    return asset
def localized(key):return f'LOCTABLE("{BASE}StringTables/ST_IC_Definitions.ST_IC_Definitions", "{key}")'
strings_path=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
STRINGS={row['Key']:row['SourceString'] for row in csv.DictReader(strings_path.open(encoding='utf-8-sig',newline=''))}
registry=table('Registry/DT_IC_IDRegistry');registry_template=copy.deepcopy(next(iter(registry.values())))
def register(identifier,kind,relative,label):
    row=copy.deepcopy(registry.get(identifier,registry_template));row.update(Name=identifier,ID=identifier,DefinitionType=kind,RegionID='R01',ReplacementID='None',DefinitionTable=BASE+relative+'.'+relative.rsplit('/',1)[-1],bDeprecated=False)
    registry[identifier]=row;STRINGS[identifier+'.NAME']=label
def metadata(row,identifier,label):
    row.update(Name=identifier,ID=identifier,NameKey=identifier+'.NAME',DescriptionKey=identifier+'.DESCRIPTION',bDeprecated=False,bTestOnly=False)
    STRINGS[identifier+'.NAME']=label;STRINGS[identifier+'.DESCRIPTION']=label
    return row

# Use the established retargeter without running its main setup routine.
helper_path=ROOT/'Tools/Characters/setup_assets.py';helper={}
exec(compile(helper_path.read_text(encoding='utf-8').split('\nif __name__ ==')[0],str(helper_path),'exec'),helper)
def life_source_rig(pack_name,mesh):
    path=GAME+'/Animation/Common/Retarget/IK_SRC_PlayerLife_'+mesh.get_name()
    rig=u.load_asset(path)
    if not rig:rig=TOOLS.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.IKRigDefinition,u.IKRigDefinitionFactory())
    control=u.IKRigController.get_controller(rig)
    assert control.set_skeletal_mesh(mesh)
    assert control.apply_auto_generated_retarget_definition()
    save(rig);return rig
helper['source_rig']=life_source_rig
pack='505_Survival_Mocap_Animations'
source_root=GAME+'/Animation/SampleAnimation/'+pack+'/Animations_UE4/IP/'
source={
    'Gathering':u.load_asset(source_root+'Harvest/IP_Crouch_Harvest_Down'),
    'Harvesting':u.load_asset(source_root+'Harvest/IP_Crouch_Harvest_Down'),
    'Planting':u.load_asset(source_root+'Harvest/IP_Crouch_Harvest_Mid'),
    'Mining':u.load_asset(source_root+'Pickaxe/IP_Pick_axe_hit_01'),
    'Fishing':u.load_asset(source_root+'Fishing/IP_fishing_cane_loop'),
}
assert all(source.values())
montages={}
for gender,code in [('Male','M'),('Female','F')]:
    source_folder=GAME+'/Player/SamplePlayer/'+gender
    mesh=u.load_asset(source_folder+'/SK_Player_'+code);rig=u.load_asset(source_folder+'/IK_Player_'+code+'_Mannequin');assert mesh and rig
    folder=GAME+'/Animation/Player/Main'+gender
    clips=helper['retarget'](pack,source,mesh,rig,folder+'/Life')
    actions={}
    for activity,clip in clips.items():
        path=folder+'/Life/AM_Life_'+activity
        montage=u.ICCharacterSetupLibrary.create_action_montage(path,clip,False);assert montage;save(montage)
        actions[getattr(u.ICLifeActivity,activity.upper())]=montage
        REPORT['animations'].append({'gender':gender,'activity':activity,'clip':clip.get_path_name(),'montage':montage.get_path_name()})
    profile=u.load_asset(folder+'/DA_ANIM_PLAYER_'+code);assert profile
    profile.set_editor_property('life_actions',actions);save(profile);montages[code]=actions

# Temporary held tools are separate BPs; they do not occupy weapon/inventory slots.
def held_tool(path,mesh_path,length,grip_fraction):
    bp=blueprint(path,u.ICWeaponActor);cdo=u.get_default_object(bp.generated_class());mesh=u.load_asset(mesh_path);assert mesh
    visual=cdo.get_editor_property('visual');visual.set_editor_property('static_mesh',mesh)
    bounds=mesh.get_bounding_box();size=bounds.max-bounds.min;dimensions=[size.x,size.y,size.z];major=max(range(3),key=dimensions.__getitem__)
    axis=u.Vector(*[1 if i==major else 0 for i in range(3)]);rotation=u.MathLibrary.quat_find_between_normals(axis,u.Vector(0,0,1));scale=length/max(1,dimensions[major])
    grip=(bounds.min+bounds.max)*.5+axis*(dimensions[major]*(grip_fraction-.5));offset=u.MathLibrary.quat_rotate_vector(rotation,grip*scale)*-1
    visual.set_editor_property('relative_rotation',u.MathLibrary.quat_rotator(rotation));visual.set_editor_property('relative_location',offset);visual.set_editor_property('relative_scale3d',u.Vector(scale,scale,scale))
    cdo.set_editor_property('length_axis',axis);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    REPORT['tools'].append({'bp':path,'mesh':mesh_path,'source_dimensions':dimensions,'length':length})
    return bp.generated_class().get_path_name()
pickaxe=held_tool(GAME+'/Life/Mining/Blueprints/BP_IC_Pickaxe',GAME+'/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Meshes/Props/FarmTools/SM_Farm_Pickaxe_01',100,.15)
rod=held_tool(GAME+'/Life/Fishing/Blueprints/FishingPole/BP_IC_FishingPole',GAME+'/Life/Fishing/Fishing_Pole/fishingpole_ang3dmodels/StaticMeshes/fishingpole_ang3dmodels',230,.12)

# Organize the authored fishing BPs; redirects preserve existing references.
fish_rows=table('Life/DT_IC_Fish')
for number in range(1,11):
    name=f'BP_R01_Fish_{number:02d}';old=GAME+'/Life/Fishing/'+name;new=GAME+'/Life/Fishing/Blueprints/Fish/'+name
    if not u.EditorAssetLibrary.does_asset_exist(new):assert u.EditorAssetLibrary.rename_asset(old,new)
    bp=u.load_asset(new);assert bp;save(bp)
    row=next(r for r in fish_rows.values() if r['ID']==f'FSH_R01_{number:02d}');row['FishActorClass']=bp.generated_class().get_path_name()
zone_old=GAME+'/Life/Fishing/BP_R01_RandomFishingZone';zone_new=GAME+'/Life/Fishing/Blueprints/BP_R01_RandomFishingZone'
if not u.EditorAssetLibrary.does_asset_exist(zone_new):assert u.EditorAssetLibrary.rename_asset(zone_old,zone_new)
zone=u.load_asset(zone_new);assert zone

productions=table('Life/DT_IC_Production');points=table('Life/DT_IC_FishingPoint');crops=table('Life/DT_IC_Crop')
for row in productions.values():
    activity=row['Activity']
    if activity in source:
        row['WorkMontage']=montages['M'][getattr(u.ICLifeActivity,activity.upper())].get_path_name()
        row['WorkToolClass']=pickaxe if activity=='Mining' else rod if activity=='Fishing' else 'None'
        row['WorkToolSocket']='weapon_r'
        if activity=='Fishing':row.update(DurationPlaySeconds=20,bAllowRepeat=True,Tools=[],Materials=[])
        elif activity=='Planting':row['DurationPlaySeconds']=2.5
for point in points.values():
    point.update(FishingRodItemID='None',AcceptedBaitItemIDs=[],SuccessChance=1,PointActorClass=zone.generated_class().get_path_name())
    for fish in point['FishSelections']:fish['AcceptedBaitItemIDs']=[]

# NPC R01_06 gets an always-available seed service, including while accompanying.
services=table('NPC/DT_IC_Service');shops=table('NPC/DT_IC_Shop');npcs=table('NPC/DT_IC_NPC')
service_id='SRV_R01_006_SHOP';shop_id='SHOP_R01_006_SEEDS'
service=metadata(copy.deepcopy(next(iter(services.values()))),service_id,'R01_06 씨앗 상점')
service.update(Type='Shop',NPCID='NPC_R01_006',WorkAnchorID='None',OpeningHours=[],ShopID=shop_id,bRequireWorkSchedule=False)
services[service_id]=service
shop=metadata(copy.deepcopy(next(iter(shops.values()))),shop_id,'씨앗 상점');shop.update(ServiceID=service_id,Entries=[dict(ItemID=f'ITM_R01_SEED_{i:02d}',DisplayOrder=i-1) for i in range(1,6)]);shops[shop_id]=shop
npc=npcs['NPC_R01_006'];npc['ServiceIDs']=list(dict.fromkeys(npc['ServiceIDs']+[service_id]))
register(service_id,'Service','NPC/DT_IC_Service','R01_06 씨앗 상점');register(shop_id,'Shop','NPC/DT_IC_Shop','씨앗 상점')

# Calendar respawns and definition-backed pets, including the actual test creature.
monsters=table('Combat/DT_IC_Monster');taken={r['PetCodexIndex'] for r in monsters.values() if r['PetCodexIndex']>=0}
next_index=max(taken,default=-1)+1
for monster in monsters.values():
    monster['RespawnGameMinutes']=60
    monster['VisualScaleMultiplier']=3 if monster['Kind']!='Normal' else 1
    if monster['PetCodexIndex']<0:monster['PetCodexIndex']=next_index;next_index+=1

# Player home, eight plot anchors, and sequential additional chests.
anchors=table('World/DT_IC_Anchor');anchor_template=copy.deepcopy(next(iter(anchors.values())))
def anchor(identifier,label,kind,position):
    row=metadata(copy.deepcopy(anchor_template),identifier,label);row.update(Type=kind,RegionID='R01',HomeID='HOME_PLAYER_001',OwnerNPCID='None',InteriorFloorID='None',SmartObjectDefinition='None',WorldTransform=dict(Rotation=dict(X=0,Y=0,Z=0,W=1),Translation=dict(zip(('X','Y','Z'),position)),Scale3D=dict(X=1,Y=1,Z=1)))
    anchors[identifier]=row;register(identifier,'Anchor','World/DT_IC_Anchor',label)
plot_ids=[f'ANC_PLAYER_PLOT_{i:02d}' for i in range(1,9)]
plot_positions=[]
for index,identifier in enumerate(plot_ids):
    group=index//4;cell=index%4;position=(-1250+(cell%2)*260,-850-group*750+(cell//2)*260,50)
    plot_positions.append(position);anchor(identifier,f'플레이어 밭 {index+1}','FarmPlot',position)
for crop in crops.values():crop['AllowedPlotAnchorIDs']=list(dict.fromkeys(crop['AllowedPlotAnchorIDs']+plot_ids))
anchor('ANC_PLAYER_BED','플레이어 침대','Bed',(0,-500,96))
anchor('ANC_PLAYER_CHEST_02','추가 창고 1','StorageInstallation',(450,-500,0));anchor('ANC_PLAYER_CHEST_03','추가 창고 2','StorageInstallation',(650,-500,0))
homes=table('World/DT_IC_Home')
home=metadata(copy.deepcopy(next(iter(homes.values()))),'HOME_PLAYER_001','플레이어 생활 구역')
home.update(Type='Player',OwnerNPCID='None',EntranceAnchorID='ANC_PLAYER_BED',BedAnchorIDs=['ANC_PLAYER_BED'],StorageInstallAnchorIDs=['ANC_PLAYER_CHEST_02','ANC_PLAYER_CHEST_03'],FarmPlotAnchorIDs=plot_ids,MaximumStorageInstallations=3)
homes[home['Name']]=home;register(home['ID'],'Home','World/DT_IC_Home','플레이어 생활 구역')
recipes=table('Recipes/DT_IC_Recipe');recipe_template=copy.deepcopy(recipes['RCP_CRAFT_001']);production_template=copy.deepcopy(productions['PRD_CRAFT_001'])
storage_production=metadata(copy.deepcopy(production_template),'PRD_PLAYER_STORAGE','창고 추가 설치');storage_production.update(Activity='Crafting',ResultKind='InstallStorage',Results=[],Tools=[],Materials=[],StartConditions=[],DurationPlaySeconds=5)
productions[storage_production['Name']]=storage_production;register(storage_production['ID'],'Production','Life/DT_IC_Production','창고 추가 설치')
for step in (1,2):
    identifier=f'RCP_PLAYER_STORAGE_{step:02d}';label=f'개인 창고 추가 설치 {step}/2'
    row=metadata(copy.deepcopy(recipe_template),identifier,label)
    row.update(RecipeID=identifier,DisplayName=localized(identifier+'.NAME'),Type='Craft',ProductionID=storage_production['ID'],ResultKind='InstallStorage',Results=[],Tools=[],IngredientGroups=[],Ingredients=[dict(ItemID='ITM_R01_ORE_01',Count=1),dict(ItemID='ITM_R01_ORE_02',Count=1)],WorkDurationPlaySeconds=5,SPCost=5,RequiredMainStat='CON',RequiredMainStatValue=step*10,RequiredExtraStorage=step-1,InstalledContainerID=f'PlayerChest{step+1}',HomeID=home['ID'],InstallationAnchorIDs=[f'ANC_PLAYER_CHEST_{step+1:02d}'],StorageCapacityAdded=40,UnlockConditions=[],bUnlockedByDefault=True)
    recipes[identifier]=row;STRINGS[identifier+'.DESCRIPTION']=f'체력 {step*10} 이상. 광석1 1개와 광석2 1개로 개인 창고 1개(40칸)를 설치합니다.'
    register(identifier,'Recipe','Recipes/DT_IC_Recipe',label)

# Localized row names before importing the FText-backed recipe data.
stream=io.StringIO(newline='');writer=csv.writer(stream,lineterminator='\n');writer.writerow(['Key','SourceString']);writer.writerows(STRINGS.items())
strings_path.write_text(stream.getvalue(),encoding='utf-8',newline='')
string_asset=u.load_asset(BASE+'StringTables/ST_IC_Definitions');assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(string_asset,str(strings_path));save(string_asset)
for relative,(asset,rows) in TABLES.items():
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset,json.dumps(list(rows.values()),ensure_ascii=False)),relative
    save(asset);reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)));output=io.StringIO(newline='');writer=csv.DictWriter(output,fieldnames=reader.fieldnames,lineterminator='\n');writer.writeheader();writer.writerows(reader)
    path=ROOT/'Data'/(relative+'.csv');path.parent.mkdir(parents=True,exist_ok=True)
    for attempt in range(8):
        try:path.write_text(output.getvalue(),encoding='utf-8',newline='');break
        except OSError:
            if attempt==7:raise
            time.sleep(.15)
    REPORT['tables'].append(relative)

# Editable WBPs backed by the C++ layouts.
for name,parent,kind in [('WBP_IC_Shop',u.ICShopWidget,'Shop'),('WBP_IC_SeedPicker',u.ICSeedPickerWidget,'SeedPicker'),('WBP_IC_SleepFade',u.ICSleepFadeWidget,'SleepFade'),('WBP_IC_CodexMenu',u.ICCodexMenuWidget,'CodexMenu'),('WBP_IC_InventoryItemPopup',u.ICInventoryItemPopupWidget,'InventoryItemPopup')]:
    bp=blueprint(GAME+'/UI/Widget/'+name,parent,True);assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp);REPORT['widgets'].append(name)
player_bp=u.load_asset(GAME+'/Player/Blueprints/BP_ICPlayerCharacter');assert player_bp;u.BlueprintEditorLibrary.compile_blueprint(player_bp);save(player_bp)

# Work area actors, preserving unrelated map content and the existing fishing spot.
world=u.EditorLoadingAndSavingUtils.load_map(GAME+'/World/L_TestMap');actors=u.get_editor_subsystem(u.EditorActorSubsystem)
farm_bp=blueprint(GAME+'/Life/Farming/Blueprints/BP_IC_FarmPlot',u.ICFarmPlot);save(farm_bp)
def solid_material(path,color):
    material=u.load_asset(path)
    if not material:
        material=TOOLS.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.Material,u.MaterialFactoryNew())
        node=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant3Vector,-220,0);node.set_editor_property('constant',u.LinearColor(*color,1))
        assert u.MaterialEditingLibrary.connect_material_property(node,'',u.MaterialProperty.MP_BASE_COLOR)
        u.MaterialEditingLibrary.recompile_material(material);save(material)
    return material
soil=solid_material(GAME+'/Life/Farming/Materials/M_IC_PlotSoil',(.14,.055,.018))
u.get_default_object(farm_bp.generated_class()).get_editor_property('soil').set_material(0,soil);save(farm_bp)
water=solid_material(GAME+'/Life/Fishing/Materials/M_IC_FishingWater',(.015,.16,.26))
zone_default=u.get_default_object(zone.generated_class());zone_visual=zone_default.get_editor_property('visual_mesh')
zone_visual.set_editor_property('static_mesh',u.load_asset('/Engine/BasicShapes/Plane'));zone_visual.set_relative_scale3d(u.Vector(5,5,1));zone_visual.set_relative_location(u.Vector(0,0,-60),False,False);zone_visual.set_material(0,water);save(zone)
bed_bp=blueprint(GAME+'/Life/Home/Blueprints/BP_IC_PlayerBed',u.ICPlayerBed)
u.get_default_object(bed_bp.generated_class()).set_editor_property('sleep_pose_offset',u.Transform(location=u.Vector(36,0,-48),rotation=u.Rotator(yaw=-90)))
u.get_default_object(bed_bp.generated_class()).get_editor_property('bed_visual').set_editor_property('static_mesh',u.load_asset(GAME+'/World/SampleLevel/Medieval_House/meshes/props/SM_bed_01'));save(bed_bp)
chest_bp=blueprint(GAME+'/Life/Home/Blueprints/BP_IC_PlayerStorage',u.ICPlayerStorage)
chest_default=u.get_default_object(chest_bp.generated_class());chest_mesh=u.load_asset(GAME+'/World/SampleLevel/Medieval_House/meshes/props/SM_chest_01');assert chest_mesh
chest_default.get_editor_property('shape').set_editor_property('static_mesh',chest_mesh);chest_default.get_editor_property('label').set_visibility(False);save(chest_bp)
all_actors=actors.get_all_level_actors()
for actor in all_actors:
    if isinstance(actor,u.ICLifeActor) and str(actor.get_editor_property('point_id')).startswith('TEST_R01_CROP_'):actors.destroy_actor(actor)
def placed(bp,key,position):
    actor=next((a for a in actors.get_all_level_actors() if a.actor_has_tag(key)),None)
    if actor is None:actor=actors.spawn_actor_from_class(bp.generated_class(),u.Vector(*position));actor.set_editor_property('tags',list(actor.get_editor_property('tags'))+[u.Name(key)])
    actor.set_actor_location(u.Vector(*position),False,True);actor.set_actor_label(key);actor.set_folder_path('IC_Player_Life');REPORT['map'].append({'actor':key,'position':position});return actor
for index,position in enumerate(plot_positions):
    actor=placed(farm_bp,f'IC_Player_FarmPlot_{index+1:02d}',position);actor.set_editor_property('point_id',plot_ids[index])
placed(bed_bp,'IC_Player_Bed',(0,-500,96))
for index in range(3):
    actor=placed(chest_bp,f'IC_Player_Storage_{index+1}',(250+index*200,-500,-chest_mesh.get_bounding_box().min.z))
    actor.set_editor_property('required_extra_storage',index);actor.set_editor_property('container_id','PlayerHome' if index==0 else f'PlayerChest{index+1}');actor.set_editor_property('home',False)
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.ICLifeActor) and str(actor.get_editor_property('point_id'))=='TEST_R01_FISH_RANDOM':
        actor.set_editor_property('interaction_text',u.Text('낚시 구역 · 20초 낚시'))
        visual=actor.get_editor_property('visual_mesh');visual.set_editor_property('static_mesh',u.load_asset('/Engine/BasicShapes/Plane'));visual.set_material(0,water)
        point=actor.get_actor_location();visual.set_world_location(u.Vector(point.x,point.y,1),False,False);visual.set_relative_scale3d(u.Vector(5,5,1))
assert u.EditorLoadingAndSavingUtils.save_map(world,GAME+'/World/L_TestMap')
(OUT/'Setup.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('LIFE_EXPANSION_SETUP_OK')
