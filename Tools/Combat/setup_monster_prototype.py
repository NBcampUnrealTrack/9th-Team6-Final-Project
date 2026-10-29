"""Author combat-ready dragon BPs, persistent test spawns, loot and HUD widgets."""
import csv
import io
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass'
LIB=u.EditorAssetLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()
ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem)
WORLD=u.EditorLoadingAndSavingUtils.load_map(GAME+'/World/L_TestMap')
OUT=ROOT/'Saved/MonsterPrototype'
OUT.mkdir(parents=True,exist_ok=True)
report={'monsters':[], 'widgets':[], 'tables':[]}

def load(path):
    asset=u.load_asset(path)
    if not asset: raise RuntimeError('Missing asset: '+path)
    return asset

def save(asset):
    assert LIB.save_loaded_asset(asset,only_if_is_dirty=False),asset.get_path_name()

def blueprint(path,parent,widget=False):
    if LIB.does_asset_exist(path):
        bp=load(path)
        assert isinstance(u.get_default_object(bp.generated_class()),parent),path
        return bp
    folder,name=path.rsplit('/',1)
    factory=u.WidgetBlueprintFactory() if widget else u.BlueprintFactory()
    factory.set_editor_property('parent_class',parent)
    bp=TOOLS.create_asset(name,folder,u.WidgetBlueprint if widget else u.Blueprint,factory)
    assert bp,path
    return bp

def read_csv(relative):
    with (ROOT/'Data'/relative).open(encoding='utf-8-sig',newline='') as file:
        reader=csv.DictReader(file)
        return list(reader.fieldnames),list(reader)

def update_table(relative,entries):
    fields,rows=read_csv(relative)
    by_id={row['---']:row for row in rows}
    for entry in entries:
        for field in entry:
            if field not in fields: fields.append(field)
        if entry['---'] in by_id: by_id[entry['---']].update(entry)
        else: rows.append(entry)
    stream=io.StringIO(newline='')
    writer=csv.DictWriter(stream,fieldnames=fields,lineterminator='\n')
    writer.writeheader();writer.writerows(rows)
    text=stream.getvalue()
    table=load(GAME+'/Data/DataTable/'+relative[:-4])
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,text),relative
    save(table)
    (ROOT/'Data'/relative).write_text(text,encoding='utf-8',newline='')
    report['tables'].append(relative)

def metadata(id,name=None):
    return {'---':id,'ID':id,'NameKey':id+'.NAME' if name else '', 'DescriptionKey':'',
            'Tags':'(GameplayTags=)','DataVersion':'1','bDeprecated':'False','bTestOnly':'False'}

specs=(('WBP_IC_PlayerDamage',u.ICDamageNumberWidget,'PlayerDamage'),
       ('WBP_IC_MonsterDamage',u.ICDamageNumberWidget,'MonsterDamage'),
       ('WBP_IC_MonsterHealthBar',u.ICMonsterHealthWidget,'MonsterHealthBar'),
       ('WBP_IC_RewardNotice',u.ICRewardNoticeWidget,'RewardNotice'),
       ('WBP_IC_CombatFeedback',u.ICCombatFeedbackWidget,'CombatFeedback'))
for name,parent,kind in specs:
    bp=blueprint(GAME+'/UI/Widget/'+name,parent,True)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    if kind in ('PlayerDamage','MonsterDamage'):
        u.get_default_object(bp.generated_class()).set_editor_property('player_damage',kind=='PlayerDamage')
    save(bp);report['widgets'].append(bp.get_path_name())
hud=load(GAME+'/UI/Widget/WBP_GameHUD')
assert u.ICBlueprintMaintenanceLibrary.add_combat_feedback_hud(hud)
u.BlueprintEditorLibrary.compile_blueprint(hud);save(hud)

numbers=('07','03','164','153','150','114','107','10','38','08')
monster_rows=[]
names={}
sound_root=GAME+'/Audio/SampleAudio/cplomedia_FantasyMonsterandCreatureSFXSet/Screams/WAV/'
vfx=load(GAME+'/VFX/SampleVFX/MagicProjectilesVol5/Particles/NiagaraSystems/Muzzles/NS_Muzzle_Bullet01').get_path_name()
for index,number in enumerate(numbers+('Boss',)):
    boss=number=='Boss'
    bp_name='BP_R01_Boss_DragonTheUsurper' if boss else 'BP_DragonSD_'+number
    path=GAME+'/Enemy/Blueprints/Monsters/'+bp_name
    bp=blueprint(path,u.ICEnemyBase)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo=u.get_default_object(bp.generated_class())
    mesh=cdo.get_component_by_class(u.SkeletalMeshComponent)
    capsule=cdo.get_component_by_class(u.CapsuleComponent)
    if boss:
        source_mesh=load(GAME+'/Enemy/SampleEnemies/FourEvilDragonsHP/Meshes/DragonTheUsurper/DragonTheUsurperSK')
        materials=[load(GAME+'/Enemy/SampleEnemies/FourEvilDragonsHP/Materials/DragonTheUsurper/GreenMat')]
        animation_root=GAME+'/Enemy/SampleEnemies/FourEvilDragonsHP/Animations/DragonTheUsurper/'
        animations=('Idle01Anim','RunAnim','AttackHandAnim','GetHitAnim','DieAnim')
        half_height,radius,scale=180,85,.4
    else:
        source_bp=load(GAME+'/Enemy/SampleEnemies/MEGA_Tiny_Dragon/Blueprints/BP_DragonSD_'+number)
        sample=ACTORS.spawn_actor_from_class(source_bp.generated_class(),u.Vector(0,0,5000))
        source_component=sample.get_component_by_class(u.SkeletalMeshComponent)
        source_mesh=source_component.get_editor_property('skeletal_mesh_asset')
        materials=source_component.get_materials()
        ACTORS.destroy_actor(sample)
        animation_root=GAME+'/Enemy/SampleEnemies/MEGA_Tiny_Dragon/Animations/'
        animations=('Anim_Dra_Idle','Anim_Dra_Run','Anim_Dra_Fire','Anim_Dra_Sick','Anim_Dra_Die')
        half_height,radius,scale=80,40,1
    mesh.set_editor_property('skeletal_mesh_asset',source_mesh)
    mesh.set_editor_property('override_materials',materials)
    mesh.set_relative_location(u.Vector(0,0,-half_height),False,True)
    # Both dragon skeletons face +Y; Character/AI forward is +X.
    mesh.set_relative_rotation(u.Rotator(pitch=0,yaw=-90,roll=0),False,True)
    mesh.set_relative_scale3d(u.Vector(scale,scale,scale))
    capsule.set_capsule_size(radius,half_height)
    save(bp)
    monster_id='MON_R01_BOSS_001' if boss else 'MON_R01_%03d'%(index+1)
    cdo.set_editor_property('monster_id',monster_id)
    cdo.set_editor_property('initialize_when_placed',True)
    cdo.set_editor_property('placed_leash_radius_cm',1600. if boss else 1000.)
    save(bp)
    name='R01 보스' if boss else 'DragonSD_'+number
    row=metadata(monster_id,name)
    row.update({'Kind':'RegionalBoss' if boss else 'Normal','RegionID':'R01',
        'CombatStats':'(MaxHP=%d,MaxMP=0,PhysicalAttack=%d,MagicAttack=0,Defense=%d,MoveSpeedCmPerSecond=%d,AttackSpeedMultiplier=1,CriticalChance=0,CriticalMultiplier=1.5)'%(1500 if boss else 120,20 if boss else 8,8 if boss else 2,300 if boss else 260),
        'DetectionRangeCm':'1200' if boss else '700','SightAngleDegrees':'360','bRequiresLineOfSight':'True',
        'AttackRangeCm':'260' if boss else '180','AttackIntervalPlaySeconds':'2' if boss else '1.2',
        'PathFailurePlaySeconds':'3','ReturnAcceptanceRadiusCm':'80','Skills':'()',
        'BehaviorTree':GAME+'/Enemy/AI/BehaviorTrees/BT_ICMonsterBase.BT_ICMonsterBase',
        'Blackboard':GAME+'/Enemy/AI/Blackboards/BB_ICMonsterBase.BB_ICMonsterBase','RespawnPlaySeconds':'90',
        'DropTableID':'DRP_R01_BOSS_001' if boss else 'DRP_R01_NORMAL_001',
        'FirstKillRewardID':'None','RepeatKillRewardID':'None',
        'bImmuneToOrdinaryHitStagger':'True' if boss else 'False','BossEntryConditions':'()',
        'BossExitConditions':'()','BossPhases':'()','PawnClass':path+'.'+bp_name+'_C',
        'Mesh':source_mesh.get_path_name(),'AnimationClass':'None','Portrait':'None','BattleBGM':'None',
        'HitSound':load(sound_root+('WAV_DeepScream086' if boss else 'WAV_SmallCreatureWimper%03d'%(index+1))).get_path_name(),
        'HitVFX':vfx})
    for field,animation in zip(('IdleAnimation','MoveAnimation','AttackAnimation','HitAnimation','DeathAnimation'),animations):
        row[field]=load(animation_root+animation).get_path_name()
    monster_rows.append(row);names[monster_id+'.NAME']=name
    report['monsters'].append({'id':monster_id,'bp':path,'mesh':row['Mesh'],'materials':[m.get_path_name() for m in materials],
                               'sound':row['HitSound'],'animations':animations})
update_table('Combat/DT_IC_Monster.csv',monster_rows)

_,existing_items=read_csv('Items/DT_IC_Item.csv')
template=next((row for row in existing_items if row['Kind']=='Material'),existing_items[0])
items=[]
for index in range(1,12):
    # Definition IDs use the project's uppercase convention; the displayed name is R01_01..11.
    id='ITEM_R01_%02d'%index
    row=dict(template);row.update(metadata(id,id))
    row.update({'ItemID':id,'DisplayName':'R01_%02d'%index,'Tab':'Normal','Category':'Material','Kind':'Material',
                'EquipSlot':'None','StackLimit':'99','bCanDiscard':'True','bCanSell':'True','bCanGift':'False',
                'UseTarget':'None','UseEffect':'None','EquipmentID':'None','CropID':'None','Icon':'None',
                'WorldMesh':'None','StatusEffectIDs':'()','OptionPoolID':'None','BuyPrice':'0','SellPrice':'0'})
    items.append(row);names[id+'.NAME']='R01_%02d'%index
update_table('Items/DT_IC_Item.csv',items)
drops=[]
for boss in (False,True):
    row=metadata('DRP_R01_BOSS_001' if boss else 'DRP_R01_NORMAL_001')
    row.update({'Selection':'WeightedChoice','Rolls':'1','Entries':'('+','.join(
        '(ItemID=ITEM_R01_%02d,MinimumCount=1,MaximumCount=5,Probability=1,Weight=1,bApplyPlayerLUKToProbability=False,bEquipmentGradeRoll=False)'%index
        for index in range(1,12 if boss else 11))+')','EquipmentGrades':'()',
        'MinimumGold':'100','MaximumGold':'1000','bApplyPlayerLUKToGold':'False','bAutoCollectItems':'True'})
    drops.append(row)
update_table('Rewards/DT_IC_DropTable.csv',drops)

anchors=[];slots=[]
_,old_anchors=read_csv('World/DT_IC_Anchor.csv')
for index,monster in enumerate(monster_rows):
    id='ANC_R01_MON_%03d'%(index+1)
    y=3000+700*index+(600 if index>=10 else 0)
    z=182 if index>=10 else 82
    row=dict(old_anchors[0]);row.update(metadata(id,id));row.update({'Type':'MonsterOrigin','RegionID':'R01',
        'HomeID':'None','OwnerNPCID':'None','WorldTransform':'(Rotation=(X=0,Y=0,Z=1,W=0),Translation=(X=2000,Y=%d,Z=%d),Scale3D=(X=1,Y=1,Z=1))'%(y,z)})
    anchors.append(row);names[id+'.NAME']='몬스터 테스트 %02d'%(index+1)
    slots.append('(SpawnSlotIndex=%d,MonsterID=%s,OriginAnchorID=%s,LeashShape=Radius,LeashRadiusCm=%d,bOverrideRespawnTime=False)'%(index,monster['ID'],id,1600 if index>=10 else 1000))
update_table('World/DT_IC_Anchor.csv',anchors)
spawner=metadata('SPN_R01_TEST_001','테스트 몬스터')
names['SPN_R01_TEST_001.NAME']='테스트 몬스터'
spawner.update({'RegionID':'R01','Slots':'('+','.join(slots)+')'})
update_table('World/DT_IC_Spawner.csv',[spawner])
registry=[]
for kind,relative,rows in (('Monster','Combat/DT_IC_Monster',monster_rows),('Item','Items/DT_IC_Item',items),
                          ('DropTable','Rewards/DT_IC_DropTable',drops),('Anchor','World/DT_IC_Anchor',anchors),
                          ('Spawner','World/DT_IC_Spawner',[spawner])):
    for source in rows:
        row=metadata(source['ID']);row.update({'DefinitionType':kind,'RegionID':'R01','ReplacementID':'None',
            'DefinitionTable':GAME+'/Data/DataTable/'+relative+'.'+relative.rsplit('/',1)[1], 'Notes':'R01 monster combat prototype.'})
        registry.append(row)
update_table('Registry/DT_IC_IDRegistry.csv',registry)
fields,strings=read_csv('StringTables/ST_IC_Definitions.csv')
known={row['Key']:row for row in strings}
for key,value in names.items():
    if key in known:known[key]['SourceString']=value
    else:strings.append({'Key':key,'SourceString':value})
with (ROOT/'Data/StringTables/ST_IC_Definitions.csv').open('w',encoding='utf-8',newline='') as file:
    writer=csv.DictWriter(file,fieldnames=fields,lineterminator='\n');writer.writeheader();writer.writerows(strings)
strings_asset=load(GAME+'/Data/DataTable/StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings_asset,str(ROOT/'Data/StringTables/ST_IC_Definitions.csv'))
save(strings_asset)

layout_bp=blueprint(GAME+'/World/Blueprints/BP_R01MonsterTestLayout',u.ICMonsterTestLayout)
u.BlueprintEditorLibrary.compile_blueprint(layout_bp);save(layout_bp)
existing=[a for a in ACTORS.get_all_level_actors() if isinstance(a,u.ICMonsterTestLayout)]
assert len(existing)<=1,'Duplicate monster test layouts'
layout=existing[0] if existing else ACTORS.spawn_actor_from_class(layout_bp.generated_class(),u.Vector(2000,3000,0))
layout.set_actor_label('IC_R01_Monster_TestLayout')
layout.set_actor_location_and_rotation(u.Vector(2000,3000,0),u.Rotator(pitch=0,yaw=180,roll=0),False,True)
layout.set_editor_property('spacing_cm',700)
for actor in ACTORS.get_all_level_actors():
    if actor.get_actor_label()=='IC_TestGround':
        actor.set_actor_scale3d(u.Vector(200,300,1))
        actor.set_actor_location(u.Vector(0,4500,-50),False,True)
assert u.ICCharacterSetupLibrary.prepare_test_navigation(WORLD)
for actor in ACTORS.get_all_level_actors():
    if isinstance(actor,u.RecastNavMesh):actor.set_editor_property('runtime_generation',u.RuntimeGenerationType.DYNAMIC)
assert u.EditorLoadingAndSavingUtils.save_map(WORLD,GAME+'/World/L_TestMap')
report['map']=WORLD.get_path_name();report['layout']=layout.get_path_name()
(OUT/'Setup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_MONSTER_PROTOTYPE_SETUP_OK monsters=11 widgets=5')
