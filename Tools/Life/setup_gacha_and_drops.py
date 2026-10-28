"""Create the editable gacha DT/BP and switch current world drops to basic cubes.

One-time setup: later balance edits should use the DT directly.
"""
import csv
import io
import time
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
BASE='/Game/IsekaiClass/Data/DataTable/'
lib=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()


def save(asset):
    assert lib.save_loaded_asset(asset,only_if_is_dirty=False),asset


def write(path,text):
    path.parent.mkdir(parents=True,exist_ok=True)
    for attempt in range(8):
        try:
            path.write_text(text,encoding='utf-8',newline='');return
        except OSError:
            if attempt==7: raise
            time.sleep(.15)


def read_table(relative):
    asset=u.load_asset(BASE+relative);assert asset,relative
    reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)))
    return asset,reader.fieldnames,list(reader)


def write_table(relative,asset,fields,rows):
    output=io.StringIO(newline='');writer=csv.DictWriter(output,fieldnames=fields,lineterminator='\n')
    writer.writeheader();writer.writerows(rows);text=output.getvalue()
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset,text),relative
    save(asset);write(ROOT/'Data'/(relative+'.csv'),text)


item_asset,item_fields,items=read_table('Items/DT_IC_Item')
for r in items:
    r['WorldMesh']='/Engine/BasicShapes/Cube.Cube'
    r['WorldMeshScale']='(X=0.25,Y=0.25,Z=0.25)'
write_table('Items/DT_IC_Item',item_asset,item_fields,items)

relative='Life/DT_IC_GachaMachine'
table=u.load_asset(BASE+relative)
struct=u.load_object(None,'/Script/IsekaiClass.ICGachaMachineRow')
if not table:
    factory=u.DataTableFactory();factory.set_editor_property('struct',struct)
    table=tools.create_asset('DT_IC_GachaMachine',BASE+'Life',u.DataTable,factory)
assert table
reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
fields=reader.fieldnames;rows=list(reader)
id='GAC_R01_BASIC_001'
row={key:'' for key in fields}
row.update({fields[0]:id,'ID':id,'CostGold':'1000','NameKey':id+'.NAME','DescriptionKey':id+'.DESCRIPTION',
            'Tags':'(GameplayTags=)','DataVersion':'1','bDeprecated':'False','bTestOnly':'False'})
plants=['ITM_R01_PLANT_%02d'%i for i in range(1,11)]
books=['ITM_BOOK_%03d'%i for i in list(range(1,13))+[16,17,18]]
assert all(any(r['ID']==item for r in items) for item in plants+books)


def pool(name,ids):
    entries=','.join('(ItemID="%s",Weight=1,MinimumCount=1,MaximumCount=1)'%item for item in ids)
    return '(PoolID="%s",NameKey="%s.%s",Weight=50,Items=(%s))'%(name,id,name,entries)


row['Pools']='('+pool('GATHERING',plants)+','+pool('GROWTH_BOOK',books)+')'
write_table(relative,table,fields,[r for r in rows if r['ID']!=id]+[row])
catalog=u.load_asset('/Game/IsekaiClass/Data/DataAsset/DA_IC_DefinitionCatalog')
bindings=list(catalog.get_editor_property('tables'))
if not any(b.get_editor_property('type')==u.ICDefinitionType.GACHA_MACHINE for b in bindings):
    binding=u.ICDefinitionBinding();binding.set_editor_property('type',u.ICDefinitionType.GACHA_MACHINE)
    binding.set_editor_property('row_struct',struct);binding.set_editor_property('table',table);bindings.append(binding)
catalog.set_editor_property('tables',bindings);save(catalog)

registry,gf,gr=read_table('Registry/DT_IC_IDRegistry')
entry={key:'' for key in gf}
entry.update({gf[0]:id,'ID':id,'DefinitionType':'GachaMachine','RegionID':'R01','ReplacementID':'None',
              'DefinitionTable':BASE+relative+'.DT_IC_GachaMachine','Notes':'Paid two-stage weighted item draw.',
              'NameKey':id+'.NAME','DescriptionKey':id+'.DESCRIPTION','Tags':'(GameplayTags=)',
              'DataVersion':'1','bDeprecated':'False','bTestOnly':'False'})
write_table('Registry/DT_IC_IDRegistry',registry,gf,[r for r in gr if r['ID']!=id]+[entry])

strings_path=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
strings={r['Key']:r['SourceString'] for r in csv.DictReader(io.StringIO(strings_path.read_text(encoding='utf-8-sig')))}
strings.update({id+'.NAME':'뽑기 기계',id+'.DESCRIPTION':'골드를 지불하고 무작위 아이템을 받습니다.',
                id+'.GATHERING':'채집물',id+'.GROWTH_BOOK':'성장서'})
output=io.StringIO(newline='');writer=csv.writer(output,lineterminator='\n');writer.writerow(['Key','SourceString']);writer.writerows(strings.items())
write(strings_path,output.getvalue())
strings_asset=u.load_asset(BASE+'StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings_asset,str(strings_path));save(strings_asset)

folder='/Game/IsekaiClass/Life/Gacha';name='BP_IC_GachaMachine'
bp=u.load_asset(folder+'/'+name)
if not bp:
    factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.ICGachaMachine)
    bp=tools.create_asset(name,folder,u.Blueprint,factory)
cdo=u.get_default_object(bp.generated_class());cdo.set_editor_property('gacha_machine_id',id)
u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
world=u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
existing=[a for a in actors.get_all_level_actors() if isinstance(a,u.ICGachaMachine)]
assert len(existing)<=1,'Duplicate gacha machines'
location=u.Vector(1400,-3550,75)
machine=existing[0] if existing else actors.spawn_actor_from_class(bp.generated_class(),location)
machine.set_actor_location_and_rotation(location,u.Rotator(),False,True)
machine.set_actor_label('IC_GachaMachine_Test');machine.set_folder_path('IC_Life_Test/Gacha')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
u.log('GACHA_AND_CUBE_DROPS_SETUP_OK: 1000G, Gathering 50 / GrowthBook 50, 97 cube item appearances')
