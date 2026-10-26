"""Author linked R01 life assets, recipes, markers and essence definitions."""
import csv
import io
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/LifePrototype';STAGING=OUT/'Data'
BASE='/Game/IsekaiClass/Data/DataTable/'
FARM='/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm'
LIFE='/Game/IsekaiClass/Life/'
lib=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
names={};descriptions={};registrations=[];report={'blueprints':[],'crops':[],'fish':[],'recipes':[],'materials':[]}

def load(path):
 asset=u.load_asset(path);assert asset,path;return asset
def save(asset):assert lib.save_loaded_asset(asset,only_if_is_dirty=False),asset
def read(relative):
 asset=load(BASE+relative)
 reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)))
 return asset,reader.fieldnames,list(reader)
def write(relative,asset,fields,rows):
 output=io.StringIO(newline='');writer=csv.DictWriter(output,fieldnames=fields,lineterminator='\n')
 writer.writeheader();writer.writerows(rows);text=output.getvalue()
 assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset,text),relative
 save(asset);dest=STAGING/(relative+'.csv');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text,encoding='utf-8',newline='')
def meta(fields,id,name,description=''):
 row={f:'' for f in fields};row.update({fields[0]:id,'ID':id,'NameKey':id+'.NAME','DescriptionKey':id+'.DESCRIPTION' if description else '',
 'Tags':'(GameplayTags=)','DataVersion':'1','bDeprecated':'False','bTestOnly':'False'})
 names[id+'.NAME']=name
 if description:descriptions[id+'.DESCRIPTION']=description
 return row
def update(relative,additions):
 asset,fields,rows=read(relative);ids={r['ID'] for r in additions}
 write(relative,asset,fields,[r for r in rows if r['ID'] not in ids]+additions)
 kind={'Items':'Item','Recipes':'Recipe','Combat':'Monster'}.get(relative.split('/')[0],relative.rsplit('_',1)[-1])
 if relative.endswith('DT_Ranching'):kind='Ranching'
 registrations.extend((r['ID'],kind,relative) for r in additions)
def item(id,name,category,mesh=None,food=False,description=''):
 row=dict(item_template);row.update(meta(item_fields,id,name,description))
 row.update({'ItemID':id,'DisplayName':name,'Kind':'Consumable' if food else 'Material','Category':'Food' if food else 'Fish' if category=='Fishing' else 'Gathering' if category=='Gathering' else 'Material',
 'Tab':'Normal','EquipSlot':'None','StackLimit':'99','CodexProductCategory':category,'bCanDiscard':'True','bCanSell':'True','bCanGift':'True',
 'BuyPrice':'0','SellPrice':'10','UseTarget':'LivingParty' if food else 'None','UseEffect':'Recovery' if food else 'None',
 'RestoreHP':'25' if food else '0','RestoreSP':'15' if food else '0','RestoreMP':'0','RestoreHPRatio':'0','RestoreMPRatio':'0','RestoreSPRatio':'0',
 'EquipmentID':'None','CropID':'None','Icon':'None','WorldMesh':mesh.get_path_name() if mesh else 'None','OptionPoolID':'None','StatusEffectIDs':'()'})
 items.append(row);return id
def amounts(entries):return '('+','.join('(ItemID="%s",Count=%d)'%(id,count) for id,count in entries)+')'
def production(id,name,activity,result,outputs=(),definition=None,materials=()):
 row=dict(production_template);row.update(meta(production_fields,id,name))
 row.update({'Activity':activity,'ResultKind':result,'Tools':'()','Materials':'('+','.join('(Item=(ItemID="%s",Count=%d),ConsumptionTiming=ConsumeOnSuccess)'%(id,count) for id,count in materials)+')',
 'WorkAnchorIDs':'("ANC_KITCHEN_001")' if activity=='Cooking' else '()','StartConditions':'()','DurationPlaySeconds':'5' if activity in ('Cooking','Gathering','Mining','Woodcutting') else '0',
 'SPCost':'5' if activity=='Cooking' else '0','bAllowRepeat':'True','Results':amounts(outputs),'ResultDefinition':'(Type=%s,ID="%s")'%definition if definition else '(Type=Item,ID="")'})
 productions.append(row)
def blueprint(folder,name,id,kind,mesh=None,anim=None,geometry=None,material=None):
 path=folder+'/'+name;bp=u.load_asset(path)
 if not bp:
  factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.ICLifeActor)
  bp=tools.create_asset(name,folder,u.Blueprint,factory)
 assert bp,path
 cdo=u.get_default_object(bp.generated_class());cdo.set_editor_property('definition_id',id);cdo.set_editor_property('life_type',u.ICDefinitionType.FISHING_POINT if kind=='FishingPoint' else getattr(u.ICDefinitionType,kind.upper()))
 cdo.set_editor_property('interaction_text',u.Text(names[id+'.NAME']))
 if kind=='FishingPoint':cdo.set_editor_property('show_life_label',True)
 visual=cdo.get_editor_property('visual_mesh');animal=cdo.get_editor_property('animal_mesh');ore=cdo.get_editor_property('ore_mesh')
 if mesh and isinstance(mesh,u.StaticMesh):visual.set_static_mesh(mesh)
 if mesh and isinstance(mesh,u.SkeletalMesh):
  animal.set_editor_property('skeletal_mesh_asset',mesh)
  if anim:
   if isinstance(anim,u.AnimBlueprint):animal.set_anim_instance_class(anim.generated_class())
   else:
    animal.set_editor_property('animation_mode',u.AnimationMode.ANIMATION_SINGLE_NODE)
    data=animal.get_editor_property('animation_data');data.set_editor_property('anim_to_play',anim);data.set_editor_property('saved_looping',True);animal.set_editor_property('animation_data',data)
 if geometry:
  ore.set_rest_collection(geometry)
  for index in range(ore.get_num_materials()):ore.set_material(index,material)
 u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
 report['blueprints'].append(path)
 return bp,path+'.'+name+'_C'

_,item_fields,old_items=read('Items/DT_IC_Item');item_template=next(r for r in old_items if r['Kind']=='Material')
_,production_fields,old_productions=read('Life/DT_IC_Production');production_template=old_productions[0]
_,resource_fields,_=read('Life/DT_IC_Resource');_,crop_fields,_=read('Life/DT_IC_Crop');_,fish_fields,_=read('Life/DT_IC_Fish')
items=[];productions=[];resources=[];crops=[];fishes=[]

plants=[('Foliage/Flowers/SM_Farm_Flower_Patch_Stem_01','들꽃 줄기'),('Foliage/Flowers/SM_Farm_Flower_Patch_Stem_03','산들꽃 줄기'),
 ('Foliage/Flowers/SM_Farm_Reed_Bush_01','갈대'),('Foliage/Flowers/SM_Farm_Yellow_Plant_Patch_01','노란 들풀'),
 ('Foliage/Grass/SM_Farm_GrassTall_01','키 큰 풀'),('Foliage/Berries/SM_Farm_Strawberry_Flowers','산딸기'),
 ('Props/LogsMushrooms/SM_Farm_Mushroom_01','숲버섯'),('Props/LogsMushrooms/SM_Farm_Mushroom_06','둥근버섯'),
 ('/Game/IsekaiClass/World/SampleLevel/Zenscape_Island/Model/Plants/SM_Flower_Pink_02','분홍꽃'),
 ('/Game/IsekaiClass/World/SampleLevel/Zenscape_Island/Model/Plants/SM_Flower_White_03','흰꽃')]
for number,(source,name) in enumerate(plants,1):
 id='RES_R01_GATHER_%02d'%number;mesh=load(source if source.startswith('/Game') else FARM+'/Meshes/'+source)
 row=meta(resource_fields,id,name);_,actor=blueprint(LIFE+'Collection','BP_R01_Collection_%02d'%number,id,'Resource',mesh)
 output=item('ITM_R01_PLANT_%02d'%number,name,'Gathering',mesh)
 prd='PRD_R01_GATHER_%02d'%number;production(prd,name+' 채집','Gathering','GatherResource',[(output,1)],('Resource',id))
 row.update({'Activity':'Gathering','RegionID':'R01','ProductionID':prd,'PlacementAnchorIDs':'()','Yields':amounts([(output,1)]),
 'RegenerationPlaySeconds':'300','ResourceActorClass':actor,'AvailableMesh':mesh.get_path_name(),'DepletedMesh':'None'});resources.append(row)

crop_sources=[('Wheat',5,'밀'),('Individual_Wheat',4,'외줄밀'),('Cabage',5,'양배추'),('Canola',2,'유채'),('Sunflower',2,'해바라기')]
for number,(source,days,name) in enumerate(crop_sources,1):
 id='CRP_R01_%02d'%number;row=meta(crop_fields,id,name)
 meshroot=FARM+'/Meshes/Foliage/Crops/GenericCrops/SM_Farm_'+source
 meshes=[load(meshroot+'_%02d'%stage) for stage in range(1,days+1)]
 _,actor=blueprint(LIFE+'Farming','BP_R01_Crop_%02d'%number,id,'Crop',meshes[-1])
 seed=item('ITM_R01_SEED_%02d'%number,name+' 씨앗','Automatic',description=name+'을 심는 데 사용하는 씨앗.')
 items[-1]['CropID']=id
 output=item('ITM_R01_CROP_%02d'%number,name,'Farming',meshes[-1])
 planting='PRD_R01_PLANT_%02d'%number;harvest='PRD_R01_HARVEST_%02d'%number
 production(planting,name+' 심기','Planting','PlantCrop',definition=('Crop',id),materials=[(seed,1)])
 production(harvest,name+' 수확','Harvesting','HarvestCrop',[(output,3)],('Crop',id))
 stages=['(StageID="SEED",DurationGameMinutes=1440,bHarvestable=False,Mesh=None)']
 stages+=['(StageID="STAGE_%02d",DurationGameMinutes=%d,bHarvestable=%s,Mesh="%s")'%(stage,0 if stage==days else 1440,'True' if stage==days else 'False',mesh.get_path_name()) for stage,mesh in enumerate(meshes,1)]
 row.update({'SeedItemID':seed,'CropActorClass':actor,'PlantingProductionID':planting,'HarvestProductionID':harvest,
 'GrowthStages':'('+','.join(stages)+')','HarvestYields':amounts([(output,3)]),'AllowedPlotAnchorIDs':'()','Icon':'None'});crops.append(row)
 report['crops'].append({'id':id,'name':name,'days':days,'stages':[None]+[m.get_path_name() for m in meshes]})

fish_root=LIFE+'Fishing/Underwater_life'
skeletal_assets=[]
registry=u.AssetRegistryHelpers.get_asset_registry()
for entry in registry.get_assets_by_path(fish_root,recursive=True):
 if str(entry.asset_class_path.asset_name)=='SkeletalMesh':skeletal_assets.append(load(str(entry.package_name)))
animations=[load(p) for p in sorted(lib.list_assets(fish_root+'/Blueprints',recursive=False,include_folder=False))]
animations=[a for a in animations if isinstance(a,u.AnimBlueprint)][:10];assert len(animations)==10
for number,animation in enumerate(animations,1):
 skeleton=animation.get_editor_property('target_skeleton')
 mesh=next(m for m in skeletal_assets if m.get_editor_property('skeleton')==skeleton)
 id='FSH_R01_%02d'%number;name='물고기%d'%number;row=meta(fish_fields,id,name)
 _,actor=blueprint(LIFE+'Fishing','BP_R01_Fish_%02d'%number,id,'Fish',mesh,animation)
 output=item('ITM_R01_FISH_%02d'%number,name,'Fishing')
 row.update({'ItemID':output,'RegionID':'R01','AllowedFishingPointIDs':'()','AcceptedBaitItemIDs':'()',
 'Icon':'None','Mesh':'None','FishActorClass':actor,'SkeletalMesh':mesh.get_path_name(),'AnimationClass':animation.generated_class().get_path_name()});fishes.append(row)
 report['fish'].append({'id':id,'source':animation.get_path_name(),'mesh':mesh.get_path_name()})

gemroot=LIFE+'Mining/ShatterStone/StylizedGemstones'
for number in range(1,7):
 id='RES_R01_ORE_%02d'%number;name='광석%d'%number;row=meta(resource_fields,id,name)
 geometry=load(gemroot+'/GeometryCollections/GC_Gemstone%d_Cluster_C'%number);material=load(gemroot+'/Materials/MI_Gemstone%d'%number)
 _,actor=blueprint(LIFE+'Mining','BP_R01_Ore_%02d'%number,id,'Resource',geometry=geometry,material=material)
 output=item('ITM_R01_ORE_%02d'%number,name,'Mining');prd='PRD_R01_MINE_%02d'%number
 production(prd,name+' 채광','Mining','GatherResource',[(output,1)],('Resource',id))
 row.update({'Activity':'Mining','RegionID':'R01','ProductionID':prd,'PlacementAnchorIDs':'()','Yields':amounts([(output,1)]),
 'RegenerationPlaySeconds':'300','ResourceActorClass':actor,'AvailableMesh':'None','DepletedMesh':'None'});resources.append(row)

ranch_path=BASE+'Life/DT_Ranching';ranch=u.load_asset(ranch_path)
if not ranch:
 factory=u.DataTableFactory();factory.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.ICRanchingRow'));ranch=tools.create_asset('DT_Ranching',BASE+'Life',u.DataTable,factory)
reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(ranch)));ranch_fields=reader.fieldnames
ranch_rows=[]
for number,(source,name,product) in enumerate([('Chicken','닭','달걀'),('Cow','소','우유'),('Pig','돼지','돼지고기'),('Sheep','양','양털')],1):
 id='RAN_R01_%02d'%number;row=meta(ranch_fields,id,name);mesh=load(FARM+'/Meshes/Farm_Animals/'+source+'/SK_'+source)
 idle_path=FARM+'/Animations/'+source+'/AS_'+source+'_Idle';idle=u.load_asset(idle_path)
 _,actor=blueprint(LIFE+'Ranching','BP_R01_'+source,id,'Ranching',mesh,idle)
 output=item('ITM_R01_RANCH_%02d'%number,product,'Ranching')
 row.update({'DisplayName':name,'AnimalActorClass':actor,'Mesh':mesh.get_path_name(),'AnimationClass':'None','Products':amounts([(output,1),('ITM_R01_CHICKEN_MEAT',1)] if source=='Chicken' else [(output,1)]),'ProductionIntervalGameMinutes':'1440'});ranch_rows.append(row)
item('ITM_R01_CHICKEN_MEAT','닭고기','Ranching')

# A future fishing pool: data and a labeled target box only, with no catch action.
pool_id='FPT_R01_RANDOM_001';anchor_id='ANC_R01_FISH_RANDOM_001';rod='ITM_R01_FISH_ROD';bait='ITM_R01_FISH_BAIT'
item(rod,'낚싯대','Automatic');items[-1]['bCanGift']='False'
item(bait,'미끼','Automatic');items[-1]['bCanGift']='False'
_,point_fields,_=read('Life/DT_IC_FishingPoint');point=meta(point_fields,pool_id,'물고기 랜덤')
bp,actor=blueprint(LIFE+'Fishing','BP_R01_RandomFishingZone',pool_id,'FishingPoint',load('/Engine/BasicShapes/Cube'))
cdo=u.get_default_object(bp.generated_class());cdo.set_editor_property('show_life_label',True)
cdo.get_editor_property('visual_mesh').set_relative_scale3d(u.Vector(3,3,.8));save(bp)
fishing_production='PRD_R01_FISH_RANDOM_001'
production(fishing_production,'물고기 랜덤 낚시','Fishing','FishingCatch',definition=('FishingPoint',pool_id),materials=[(bait,1)])
productions[-1].update({'Tools':'((ItemID="%s",Count=1))'%rod,'Materials':'((Item=(ItemID="%s",Count=1),ConsumptionTiming=ConsumeAtStart))'%bait,'WorkAnchorIDs':'("%s")'%anchor_id})
point.update({'RegionID':'R01','AnchorID':anchor_id,'ProductionID':fishing_production,'FishingRodItemID':rod,
 'AcceptedBaitItemIDs':'("%s")'%bait,'SuccessChance':'1','FishSelections':'('+','.join('(FishID="FSH_R01_%02d",Weight=1,CatchCount=1,AcceptedBaitItemIDs=("%s"))'%(i,bait) for i in range(1,11))+')','PointActorClass':actor})
for fish in fishes:fish.update({'AllowedFishingPointIDs':'("%s")'%pool_id,'AcceptedBaitItemIDs':'("%s")'%bait})
_,anchor_fields,old_anchors=read('World/DT_IC_Anchor');anchor=dict(old_anchors[0]);anchor.update(meta(anchor_fields,anchor_id,'물고기 랜덤 구역'))
anchor.update({'Type':'FishingPoint','RegionID':'R01','WorldTransform':'(Rotation=(X=0,Y=0,Z=0,W=1),Translation=(X=2000,Y=-14800,Z=40),Scale3D=(X=1,Y=1,Z=1))'})
update('World/DT_IC_Anchor',[anchor]);update('Life/DT_IC_FishingPoint',[point])

_,recipe_fields,old_recipes=read('Recipes/DT_IC_Recipe');recipe_template=old_recipes[0];recipes=[]
P=lambda i:'ITM_R01_PLANT_%02d'%i
C=lambda i:'ITM_R01_CROP_%02d'%i
F=lambda i:'ITM_R01_FISH_%02d'%i
EGG='ITM_R01_RANCH_01';CHICKEN='ITM_R01_CHICKEN_MEAT'
dishes=[('숲버섯 구이',[(P(7),2)]),('둥근버섯 구이',[(P(8),2)]),('모둠 버섯볶음',[(P(7),1),(P(8),1)]),
 ('산딸기 절임',[(P(6),3)]),('들꽃 차',[(P(1),2)]),('산들꽃 차',[(P(2),2)]),('갈대 차',[(P(3),2)]),
 ('노란 들풀 차',[(P(4),2)]),('풀잎 차',[(P(5),2)]),('분홍꽃 차',[(P(9),2)]),('흰꽃 차',[(P(10),2)]),
 ('밀빵',[(C(1),3)]),('외줄밀 납작빵',[(C(2),3)]),('양배추 구이',[(C(3),2)]),('유채나물',[(C(4),2)]),
 ('해바라기씨 볶음',[(C(5),2)]),('달걀찜',[(EGG,2)]),('달걀빵',[(C(1),2),(EGG,1)]),
 ('닭고기 구이',[(CHICKEN,2)]),('버섯 닭고기볶음',[(CHICKEN,1),(P(7),2)]),
 ('양배추 닭고기찜',[(CHICKEN,1),(C(3),2)]),('산딸기 닭고기구이',[(CHICKEN,1),(P(6),2)]),
 ('생선 구이',[(F(1),2)]),('양배추 생선찜',[(F(2),1),(C(3),2)]),('버섯 생선탕',[(F(3),1),(P(8),2)]),
 ('생선 달걀부침',[(F(4),1),(EGG,1)]),('유채 생선찜',[(F(5),1),(C(4),2)]),
 ('해바라기씨 생선구이',[(F(6),1),(C(5),2)]),('모둠 생선탕',[(F(7),1),(F(8),1)]),
 ('생선 밀전병',[(F(9),1),(F(10),1),(C(2),2)])]
assert len(dishes)==30
for number,(name,ingredients) in enumerate(dishes,1):
 id='RCP_R01_COOK_%02d'%number;prd='PRD_R01_COOK_%02d'%number
 ingredient_text=', '.join(names[item_id+'.NAME']+' '+str(count)+'개' for item_id,count in ingredients)
 result=item('ITM_R01_FOOD_%02d'%number,name,'Automatic',food=True,description=name+'. HP 25, SP 15 회복.')
 row=dict(recipe_template);row.update(meta(recipe_fields,id,name,'재료: '+ingredient_text+'\n조리 시간: 5초 / SP: 5\n결과: '+name+' 1개'))
 row.update({'RecipeID':id,'DisplayName':name,'Type':'Cook','ProductionID':prd,'Ingredients':amounts(ingredients),'Tools':'()',
 'WorkDurationPlaySeconds':'5','SPCost':'5','bUnlockedByDefault':'True','ResultKind':'BagItems','Results':amounts([(result,1)]),'UnlockConditions':'()'})
 recipes.append(row);production(prd,name+' 조리','Cooking','BagItems',[(result,1)],materials=ingredients)
 report['recipes'].append({'id':id,'name':name,'ingredients':ingredients,'result':result})

update('Items/DT_IC_Item',items);update('Life/DT_IC_Production',productions);update('Life/DT_IC_Resource',resources)
update('Life/DT_IC_Crop',crops);update('Life/DT_IC_Fish',fishes);write('Life/DT_Ranching',ranch,ranch_fields,ranch_rows)
registrations.extend((r['ID'],'Ranching','Life/DT_Ranching') for r in ranch_rows)
update('Recipes/DT_IC_Recipe',recipes)

asset,fields,monsters=read('Combat/DT_IC_Monster')
prototype=[r for r in monsters if r['ID'].startswith('MON_R01_')]
prototype.sort(key=lambda r:(r['Kind']!='Normal',r['ID']))
for index,row in enumerate(prototype):
 row['PetCodexIndex']=str(index);row['PetName']='R01 보스' if row['Kind']!='Normal' else 'DragonSD_'+('07','03','164','153','150','114','107','10','38','08')[index]
 row['PetBuffDescription']='';row['PetBuffs']='()'
write('Combat/DT_IC_Monster',asset,fields,monsters)

catalog=load('/Game/IsekaiClass/Data/DataAsset/DA_IC_DefinitionCatalog');bindings=list(catalog.get_editor_property('tables'))
if not any(b.get_editor_property('type')==u.ICDefinitionType.RANCHING for b in bindings):
 binding=u.ICDefinitionBinding();binding.set_editor_property('type',u.ICDefinitionType.RANCHING);binding.set_editor_property('row_struct',u.load_object(None,'/Script/IsekaiClass.ICRanchingRow'));binding.set_editor_property('table',ranch);bindings.append(binding)
catalog.set_editor_property('tables',bindings);save(catalog)

asset,fields,registry_rows=read('Registry/DT_IC_IDRegistry')
new_ids={r[0] for r in registrations};registry_rows=[r for r in registry_rows if r['ID'] not in new_ids]
for id,kind,relative in registrations:
 row=meta(fields,id,names.get(id+'.NAME',id));row.update({'DefinitionType':kind,'RegionID':'R01','ReplacementID':'None','DefinitionTable':BASE+relative+'.'+relative.rsplit('/',1)[-1],'Notes':'R01 life prototype.'});registry_rows.append(row)
write('Registry/DT_IC_IDRegistry',asset,fields,registry_rows)

path=ROOT/'Data/StringTables/ST_IC_Definitions.csv';reader=csv.DictReader(io.StringIO(path.read_text(encoding='utf-8-sig')));fields=reader.fieldnames;strings=list(reader)
entries={**names,**descriptions};strings=[r for r in strings if r['Key'] not in entries]+[{'Key':k,'SourceString':v} for k,v in entries.items()]
dest=STAGING/'StringTables/ST_IC_Definitions.csv';dest.parent.mkdir(parents=True,exist_ok=True)
with dest.open('w',encoding='utf-8',newline='') as file:
 writer=csv.DictWriter(file,fieldnames=fields,lineterminator='\n');writer.writeheader();writer.writerows(strings)
strings_asset=load(BASE+'StringTables/ST_IC_Definitions');assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings_asset,str(dest));save(strings_asset)

for name,parent,kind in [('WBP_IC_NPCQuestMarker',u.ICNPCQuestMarkerWidget,'NPCQuestMarker'),('WBP_IC_RewardNotice',u.ICRewardNoticeWidget,'RewardNotice'),('WBP_IC_LifeLabel',u.UserWidget,'LifeLabel'),('WBP_IC_GatherProgress',u.ICGatherProgressWidget,'GatherProgress')]:
 folder='/Game/IsekaiClass/UI/Widget';bp=u.load_asset(folder+'/'+name)
 if not bp:
  factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent);bp=tools.create_asset(name,folder,u.WidgetBlueprint,factory)
 assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
save(load('/Game/IsekaiClass/UI/Widget/WBP_IC_NPCDialogue'))
(OUT/'Setup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_SETUP_OK '+json.dumps({'plants':10,'crops':5,'fish':10,'ores':6,'ranching':4,'recipes':30}))
import runpy
runpy.run_path(str(ROOT/'Tools/Life/place_life_test_map.py'))
