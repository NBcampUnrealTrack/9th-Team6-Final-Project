"""Move obsolete demonstration rows out of runtime DTs and align NPC 1-11 labels.

Archived DTs are retained under Development/LegacyData and are not in the catalog.
Persistent NPC IDs are preserved so quests, affinity and existing saves keep links.
"""
import csv
import json
import re
import shutil
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass'
OUT=ROOT/'Saved/OpenWorldUpgrade'
BACKUP=OUT/'BeforeCleanup'
ARCHIVE=GAME+'/Development/LegacyData'
OLD_SPAWNER='SPN_R01_TEST_001'
NEW_SPAWNER='SPN_R01_001'
TABLES={}
REPORT={'archived':{},'npc_names':{},'updated_tables':[],'updated_maps':[]}
if (OUT/'Cleanup.json').exists():
    REPORT['archived']=json.loads((OUT/'Cleanup.json').read_text(encoding='utf-8')).get('archived',{})


def backup(relative):
    src,dest=ROOT/relative,BACKUP/relative
    if src.exists() and not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dest)


def asset_backup(path,extension='.uasset'):
    backup('Content/'+path.removeprefix('/Game/')+extension)


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()


for path in sorted((ROOT/'Data').rglob('*.csv')):
    relative=path.relative_to(ROOT/'Data')
    if relative.parts[0] in ('StringTables','Development'):continue
    asset_path=GAME+'/Data/DataTable/'+relative.with_suffix('').as_posix()
    asset=u.load_asset(asset_path)
    if isinstance(asset,u.DataTable):
        TABLES[relative.as_posix()]=(asset,json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(asset)))

removed=set()
for relative,(asset,rows) in TABLES.items():
    legacy=[r for r in rows if (re.search(r'(?:^|_)TEST(?:_|$)|DEBUG|PROTOTYPE',r.get('ID','')) and r.get('ID')!=OLD_SPAWNER) or r.get('ID')=='SKL_RECRUIT_001']
    if not legacy:continue
    removed.update(r['ID'] for r in legacy)
    REPORT['archived'][relative]=[r['ID'] for r in legacy]
    name='DT_Legacy_'+relative.replace('.csv','').replace('/','_').removeprefix('DT_IC_')
    archive_path=ARCHIVE+'/'+name
    archived=u.load_asset(archive_path)
    if not archived:
        factory=u.DataTableFactory();factory.set_editor_property('struct',asset.get_editor_property('row_struct'))
        archived=u.AssetToolsHelpers.get_asset_tools().create_asset(name,ARCHIVE,u.DataTable,factory)
    assert archived
    existing=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(archived))
    combined={r['Name']:r for r in existing+legacy}
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(archived,json.dumps(list(combined.values()),ensure_ascii=False))
    save(archived)
    archive_csv=ROOT/'Data/Development/LegacyData'/relative
    archive_csv.parent.mkdir(parents=True,exist_ok=True)
    archive_csv.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(archived),encoding='utf-8-sig')

for path in (ROOT/'Data/Development/LegacyData').rglob('*.csv'):
    if path.name!='Strings.csv':
        removed.update(r['ID'] for r in csv.DictReader(path.open(encoding='utf-8-sig')) if r.get('ID'))


def rewrite(value):
    if isinstance(value,dict):return {k:rewrite(v) for k,v in value.items()}
    if isinstance(value,list):return [rewrite(v) for v in value if not isinstance(v,str) or v not in removed]
    if isinstance(value,str):
        if value==OLD_SPAWNER:return NEW_SPAWNER
        if value.startswith(OLD_SPAWNER+'.'):return NEW_SPAWNER+value[len(OLD_SPAWNER):]
    return value


for relative,(asset,original) in TABLES.items():
    rows=[rewrite(r) for r in original if r.get('ID') not in removed]
    if relative=='World/DT_IC_Region.csv':
        for row in rows:
            if row.get('ID')=='R01':
                row['World']=GAME+'/World/Maps/L_OpenWorld.L_OpenWorld'
                row['TownMapID']='MAP_OPENWORLD_001'
                row['RegionBossMonsterID']='MON_R01_BOSS_001'
                capture=json.loads((OUT/'Authored.json').read_text(encoding='utf-8'))['capture']
                x0,y0,x1,y1=capture['world_bounds']
                row['BoundsCm']=f'(Min=(X={x0},Y={y0},Z=-50000),Max=(X={x1},Y={y1},Z=100000),IsValid=True)'
                row['BoundaryPolygonCm']=[f'(X={x},Y={y})' for x,y in ((x0,y0),(x1,y0),(x1,y1),(x0,y1))]
    if relative=='Settings/DT_IC_CreationOptions.csv':
        for row in rows:row['bTestReward']=False
    if relative=='Registry/DT_IC_IDRegistry.csv':
        for row in rows:
            if row['ID']==NEW_SPAWNER:row['Notes']='R01 monster spawn group'
            elif re.search(r'test|prototype',row.get('Notes',''),re.IGNORECASE):row['Notes']='Runtime '+row.get('DefinitionType','')+' definition'
    if relative=='NPC/DT_IC_NPC.csv':
        by_id={r['ID']:r for r in rows}
        for number in range(1,12):
            identifier=f'NPC_R01_{number:03d}';row=by_id[identifier]
            assert row['ActorClass'].endswith(f'BP_NPC_R01_{number:03d}_C'),row['ActorClass']
            row['bTestOnly']=False
            REPORT['npc_names'][identifier]=f'NPC {number}'
    if rows==original:continue
    path=GAME+'/Data/DataTable/'+relative.removesuffix('.csv')
    asset_backup(path);backup('Data/'+relative)
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset,json.dumps(rows,ensure_ascii=False)),relative
    save(asset)
    (ROOT/'Data'/relative).write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset),encoding='utf-8-sig')
    REPORT['updated_tables'].append(relative)

strings_path=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
backup(str(strings_path.relative_to(ROOT)))
strings={r['Key']:r['SourceString'] for r in csv.DictReader(strings_path.open(encoding='utf-8-sig'))}
legacy_strings={k:v for k,v in strings.items() if k.split('.')[0] in removed}
archive_strings=ROOT/'Data/Development/LegacyData/Strings.csv'
if archive_strings.exists():
    legacy_strings={**{r['Key']:r['SourceString'] for r in csv.DictReader(archive_strings.open(encoding='utf-8-sig'))},**legacy_strings}
with archive_strings.open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f);writer.writerow(['Key','SourceString']);writer.writerows(legacy_strings.items())
strings={k:v for k,v in strings.items() if k not in legacy_strings}
for key,value in list(strings.items()):
    strings[key]=re.sub(r'R01_(0[1-9]|10|11)(?!\d)',lambda m:'NPC '+str(int(m.group(1))),value)
strings.update({identifier+'.NAME':name for identifier,name in REPORT['npc_names'].items()})
strings.update({
    'NPC_R01_101.NAME':'마을 상인','HOME_RECRUIT_001.NAME':'NPC 1의 집',
    'ANC_RECRUIT_BED.NAME':'NPC 1 침대','ANC_RECRUIT_HOME.NAME':'NPC 1 집','ANC_RECRUIT_WORK.NAME':'NPC 1 작업대',
    'SCH_RECRUIT_001.NAME':'NPC 1 시간표','RCB_RECRUIT_001.NAME':'NPC 1 전투 설정',
    'R01.NAME':'R01 지역','SHOP_MERCHANT_001.NAME':'마을 상점',
    'SFE_START_PHYSICAL_001.NAME':'초기 물리 공격 효과','SFE_START_MAGIC_001.NAME':'초기 마법 공격 효과','SFE_START_HEAL_001.NAME':'초기 치유 효과',
    NEW_SPAWNER+'.NAME':'R01 몬스터 출현 그룹'})
strings.pop(OLD_SPAWNER+'.NAME',None)
for number in range(1,12):strings[f'ANC_R01_MON_{number:03d}.NAME']=f'몬스터 출현 지점 {number:02d}'
for key,value in list(strings.items()):
    if key.startswith('SKL_START_'):strings[key]=value.replace(' (테스트)','')
for point in json.loads((OUT/'Authored.json').read_text(encoding='utf-8'))['destinations']:
    code=point['id'].removeprefix('WRP_OW_')
    strings['ANC_WRP_OW_VISIT_'+code+'.NAME']=point['name']+' 워프돌'
    strings['ANC_WRP_OW_ARRIVAL_'+code+'.NAME']=point['name']+' 워프 도착 지점'
st_path=GAME+'/Data/DataTable/StringTables/ST_IC_Definitions';asset_backup(st_path)
with strings_path.open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f);writer.writerow(['Key','SourceString']);writer.writerows(strings.items())
st=u.load_asset(st_path);assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(st,str(strings_path));save(st)

# Creation results and dialogue captions live in other shared String Tables.
for source_path in (ROOT/'Data/StringTables').glob('*.csv'):
    if source_path==strings_path:continue
    entries=list(csv.DictReader(source_path.open(encoding='utf-8-sig')))
    changed=False
    for entry in entries:
        if 'SourceString' not in entry:continue
        before=entry['SourceString']
        entry['SourceString']=re.sub(r'R01_(0[1-9]|10|11)(?!\d)',lambda m:'NPC '+str(int(m.group(1))),before)
        changed|=entry['SourceString']!=before
    if not changed:continue
    path=GAME+'/Data/DataTable/StringTables/'+source_path.stem
    string_asset=u.load_asset(path)
    if not string_asset:continue
    backup(str(source_path.relative_to(ROOT)));asset_backup(path)
    with source_path.open('w',encoding='utf-8-sig',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(entries[0]));writer.writeheader();writer.writerows(entries)
    assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(string_asset,str(source_path));save(string_asset)

for folder in (GAME+'/World/Blueprints',GAME+'/Enemy/Blueprints'):
    for path in u.EditorAssetLibrary.list_assets(folder,True,False):
        if not any(word in path.rsplit('/',1)[-1].lower() for word in ('test','layout')):continue
        bp=u.load_asset(path)
        if not isinstance(bp,u.Blueprint):continue
        default=u.get_default_object(bp.generated_class())
        try:
            if str(default.get_editor_property('SpawnerID'))==OLD_SPAWNER:
                asset_backup(path.split('.')[0]);default.set_editor_property('SpawnerID',NEW_SPAWNER);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
        except Exception:pass

# Update authored actor references, including the preserved development scene.
for map_path in (GAME+'/World/Maps/L_OpenWorld',GAME+'/World/L_TestMap'):
    if not u.EditorAssetLibrary.does_asset_exist(map_path):continue
    u.EditorLoadingAndSavingUtils.load_map(map_path)
    world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();changed=False
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        try:
            if str(actor.get_editor_property('SpawnerID'))==OLD_SPAWNER:
                actor.set_editor_property('SpawnerID',NEW_SPAWNER);changed=True
        except Exception:pass
        try:
            identifier=str(actor.get_editor_property('CharacterID'))
            if identifier in REPORT['npc_names']:
                actor.set_actor_label(REPORT['npc_names'][identifier]);changed=True
        except Exception:pass
    if changed:
        asset_backup(map_path,'.umap');assert u.EditorLoadingAndSavingUtils.save_map(world,map_path);REPORT['updated_maps'].append(map_path)

REPORT['removed_runtime_ids']=sorted(removed)
REPORT['persistent_npc_ids_preserved']=True
(OUT/'Cleanup.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_RUNTIME_DEFINITIONS_CLEANED '+str(len(removed)))
