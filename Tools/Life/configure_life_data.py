"""Migrate the live life/item DT assets; export their editable CSV sources as well.

Run with the matching compiled editor. Existing rows are read from assets so
unrelated authored values are retained. This script does not author a scenario.
"""
import csv
import io
import json
import re
import time
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
BASE = '/Game/IsekaiClass/Data/DataTable/'
ST = BASE + 'StringTables/ST_IC_Definitions.ST_IC_Definitions'
OUT = ROOT / 'Saved/LifeDataReview'
OUT.mkdir(parents=True, exist_ok=True)
lib = u.EditorAssetLibrary
tables = {}
original_rows = {}
deleted = {
    'ITM_MAT_001': 'ITM_R01_PLANT_03', 'ITM_MAT_002': 'ITM_R01_CROP_01',
    'ITM_MAT_003': 'ITM_R01_ORE_01', 'ITEM_P0_01': 'ITM_R01_FOOD_15',
    'ITM_MAT_PORK': 'ITM_R01_RANCH_03', 'ITM_MAT_GREEN_HERB': 'ITM_R01_PLANT_05',
    'ITEM_HERO_GIFT': 'ITM_R01_FOOD_19',
    # Preserve old saves/data while keeping the fifteen usable elixirs.
    **{'ITM_BOOK_%03d' % i: 'ITM_ELIXIR_%03d' % i for i in list(range(1,13))+[16,17,18]},
    'ITM_BOOK_013': 'ITM_ELIXIR_001', 'ITM_BOOK_014': 'ITM_ELIXIR_002', 'ITM_BOOK_015': 'ITM_ELIXIR_003',
    'ITM_ELIXIR_013': 'ITM_ELIXIR_001', 'ITM_ELIXIR_014': 'ITM_ELIXIR_002', 'ITM_ELIXIR_015': 'ITM_ELIXIR_003',
    **{'ITEM_R01_%02d' % i: 'ITM_R01_PLANT_%02d' % i for i in range(1, 11)},
    'ITEM_R01_11': 'ITM_R01_CROP_01',
}
retired = {'RCP_HERO_GIFT', 'PRD_HERO_GIFT', 'RCP_GREEN_JUICE', 'PRD_GREEN_JUICE'}
reference_remap = dict(deleted, RCP_HERO_GIFT='RCP_R01_COOK_19', PRD_HERO_GIFT='PRD_R01_COOK_19',
                       RCP_GREEN_JUICE='RCP_R01_COOK_15', PRD_GREEN_JUICE='PRD_R01_COOK_15')


def write_file(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    for attempt in range(8):
        try:
            path.write_text(text, encoding='utf-8', newline='')
            return
        except OSError:
            if attempt == 7:
                raise
            time.sleep(.15)


def load(relative):
    if relative not in tables:
        asset = u.load_asset(BASE + relative)
        assert asset, relative
        reader = csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)))
        tables[relative] = (asset, reader.fieldnames, list(reader))
        original_rows[relative] = [dict(r) for r in tables[relative][2]]
    return tables[relative][2]


def row(relative, id):
    return next(r for r in load(relative) if r['ID'] == id)


def loc(key):
    return 'LOCTABLE("%s", "%s")' % (ST, key)


def amounts(values):
    return '(' + ','.join('(ItemID="%s",Count=%d)' % v for v in values) + ')'


def materials(values):
    return '(' + ','.join('(Item=(ItemID="%s",Count=%d),ConsumptionTiming=ConsumeOnSuccess)' % v for v in values) + ')'


string_file = ROOT / 'Data/StringTables/ST_IC_Definitions.csv'
strings = {r['Key']: r['SourceString'] for r in csv.DictReader(io.StringIO(string_file.read_text(encoding='utf-8-sig')))}
strings = {k: v for k, v in strings.items() if k.split('.')[0] not in set(deleted) | retired}

# Load every active table, replacing removed references even in nested structs.
for package in lib.list_assets(BASE, recursive=True, include_folder=False):
    asset = u.load_asset(package)
    if not isinstance(asset, u.DataTable):
        continue
    relative = asset.get_path_name().split(BASE, 1)[1].split('.')[0]
    rows = load(relative)
    rows[:] = [r for r in rows if r.get('ID') not in set(deleted) | retired]
    for r in rows:
        for field, value in r.items():
            for before, after in reference_remap.items():
                value = re.sub(r'(?<![A-Za-z0-9_])' + before + r'(?![A-Za-z0-9_])', after, value)
            r[field] = value

items = load('Items/DT_IC_Item')
for r in items:
    id = r['ID']
    r['NameKey'] = id + '.NAME'
    r['DisplayName'] = loc(r['NameKey'])
    if id.startswith(('ITM_R01_PLANT_', 'ITM_R01_CROP_', 'ITM_R01_FISH_', 'ITM_R01_ORE_')):
        r['bCanSell'] = 'True'
        # Keep authored prices; all four categories already have a usable price.
        if int(r['SellPrice']) <= 0:
            r['SellPrice'] = '10'
    if id.startswith('ITM_R01_SEED_'):
        number = int(id[-2:])
        r.update(BuyPrice=str(10 + (number - 1) * 2), SellPrice=str(4 + number - 1), bCanSell='True')
        strings[id + '.DESCRIPTION'] = strings[id + '.NAME'] + '. 작물을 심는 데 사용하는 씨앗.'
    if id.startswith('ITM_R01_FOOD_'):
        r.update(Kind='Consumable', UseEffect='Recovery', UseTarget='LivingParty', RestoreHP='25', RestoreSP='15',
                 RestoreMP='0', RestoreHPRatio='0', RestoreMPRatio='0', RestoreSPRatio='0', bCanSell='True')
        if int(r['SellPrice']) <= 0:
            r['SellPrice'] = '10'
        strings[id + '.DESCRIPTION'] = strings[id + '.NAME'] + '. 지역에서 구한 재료로 만든 음식.'
    if id.startswith('ITM_R01_RANCH_') or id == 'ITM_R01_CHICKEN_MEAT':
        hp, sp = {'ITM_R01_RANCH_01': (5, 5), 'ITM_R01_RANCH_02': (10, 10),
                  'ITM_R01_RANCH_03': (10, 5), 'ITM_R01_RANCH_04': (0, 0),
                  'ITM_R01_CHICKEN_MEAT': (10, 5)}[id]
        r.update(Kind='Consumable', UseEffect='Recovery', UseTarget='LivingParty', RestoreHP=str(hp),
                 RestoreSP=str(sp), RestoreMP='0', RestoreHPRatio='0', RestoreMPRatio='0', RestoreSPRatio='0', bCanSell='True')
        strings[id + '.DESCRIPTION'] = {
            'ITM_R01_RANCH_01': '닭에게서 얻은 달걀. 요리 재료로도 사용합니다.',
            'ITM_R01_RANCH_02': '신선한 우유. 마시면 기운을 회복합니다.',
            'ITM_R01_RANCH_03': '농장에서 얻은 돼지고기.',
            'ITM_R01_RANCH_04': '부드러운 양털. 각종 물품을 만드는 재료입니다.',
            'ITM_R01_CHICKEN_MEAT': '닭을 잡아 얻은 고기. 여러 닭고기 요리에 사용합니다.',
        }[id]
    if id + '.DESCRIPTION' not in strings:
        strings[id + '.DESCRIPTION'] = strings.get(id + '.NAME', id)
    r['DescriptionKey'] = id + '.DESCRIPTION'

strings['EQP_WPN_STAFF_001.NAME'] = '초보자 마법스태프'
strings['EQP_WPN_WAND_001.NAME'] = '초보자 치유완드'
strings['GIFT_R01_HERO.DESCRIPTION'] = '유채나물과 닭고기 구이를 선호합니다.'
strings['CDX_NPC_R01_008.DESCRIPTION'] = '지역 1의 용사. 선호 선물은 유채나물과 닭고기 구이입니다.'
# The quest ring remains a protected quest item.
row('Items/DT_IC_Item', 'ITM_QST_001').update(Tab='Quest', Kind='Quest', bCanDiscard='False', bCanSell='False', bCanGift='False')

weapon_costs = {
    'SWORD': [('ITM_R01_ORE_01', 3), ('ITM_R01_ORE_03', 1), ('ITM_R01_PLANT_03', 2)],
    'BOW': [('ITM_R01_PLANT_03', 3), ('ITM_R01_PLANT_05', 2), ('ITM_R01_ORE_02', 1), ('ITM_R01_FISH_01', 1)],
    'STAFF': [('ITM_R01_ORE_04', 2), ('ITM_R01_ORE_05', 1), ('ITM_R01_PLANT_09', 2), ('ITM_R01_CROP_05', 1)],
    'WAND': [('ITM_R01_ORE_06', 1), ('ITM_R01_PLANT_10', 2), ('ITM_R01_CROP_04', 2), ('ITM_R01_FISH_04', 1)],
}
for family, values in weapon_costs.items():
    rid = 'RCP_CRAFT_001' if family == 'SWORD' else 'RCP_CRAFT_' + family + '_001'
    recipe = row('Recipes/DT_IC_Recipe', rid)
    recipe.update(Ingredients=amounts(values), WorkDurationPlaySeconds='5', bUnlockedByDefault='True')
    production = row('Life/DT_IC_Production', recipe['ProductionID'])
    production.update(Materials=materials(values), Results=recipe['Results'])
    strings[rid + '.NAME'] = strings['EQP_WPN_' + family + '_001.NAME']
    strings[rid + '.DESCRIPTION'] = '생활 재료를 조합해 ' + strings[rid + '.NAME'] + ' 제작.'
    strings[recipe['ProductionID'] + '.NAME'] = strings[rid + '.NAME'] + ' 제작'

# Basic bread no longer refers to the removed flour item.
row('Recipes/DT_IC_Recipe', 'RCP_COOK_001')['Ingredients'] = amounts([('ITM_R01_CROP_01', 2)])
row('Life/DT_IC_Production', 'PRD_COOK_001')['Materials'] = materials([('ITM_R01_CROP_01', 2)])
strings['RCP_COOK_001.DESCRIPTION'] = '밀을 익혀 빵 1개를 만듭니다.'
for r in load('Recipes/DT_IC_Recipe'):
    r['DisplayName'] = loc(r['ID'] + '.NAME')
    r['NameKey'] = r['ID'] + '.NAME'
    r['DescriptionKey'] = r['ID'] + '.DESCRIPTION'

for r in load('NPC/DT_IC_Shop'):
    if r['ID'] == 'SHOP_MERCHANT_001':
        r['Entries'] = '(' + ','.join('(ItemID="ITM_R01_SEED_%02d",DisplayOrder=%d)' % (i, i - 1) for i in range(1, 6)) + ')'

for r in load('Life/DT_IC_Resource'):
    if r['ID'].startswith('RES_R01_'):
        r['RegenerationGameMinutes'] = '120'
        bp = u.load_object(None, r['ResourceActorClass'])
        cdo = u.get_default_object(bp)
        transform = cdo.get_editor_property('visual_mesh').get_relative_transform()
        q, p, s = transform.rotation, transform.translation, transform.scale3d
        r['VisualTransform'] = '(Rotation=(X=%f,Y=%f,Z=%f,W=%f),Translation=(X=%f,Y=%f,Z=%f),Scale3D=(X=%f,Y=%f,Z=%f))' % (q.x, q.y, q.z, q.w, p.x, p.y, p.z, s.x, s.y, s.z)
        if r['ID'].startswith('RES_R01_ORE_'):
            n = int(r['ID'][-2:])
            gem = '/Game/IsekaiClass/Life/Mining/ShatterStone/StylizedGemstones'
            r['OreCollection'] = gem + '/GeometryCollections/GC_Gemstone%d_Cluster_C.GC_Gemstone%d_Cluster_C' % (n, n)
            r['MaterialOverride'] = gem + '/Materials/MI_Gemstone%d.MI_Gemstone%d' % (n, n)

growth = []
for r in load('Life/DT_IC_Crop'):
    if not r['ID'].startswith('CRP_R01_'):
        continue
    stages = re.findall(r'\(StageID=.*?Mesh=[^)]*\)', r['GrowthStages'])
    stages = [s for s in stages if 'Mesh=None' not in s]
    assert stages, r['ID']
    r.update(bGrowsNaturally='True', RegrowthGameMinutes='120', GrowthStages='(' + ','.join(stages) + ')')
    growth.append({'id': r['ID'], 'stages': len(stages), 'day_minutes': 1440, 'regrowth_minutes': 120})

# A native combat character replaces the old decorative chicken BP.
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
old_chickens = []
for a in actors.get_all_level_actors():
    if a.get_class().get_name().startswith('BP_R01_Chicken'):
        old_chickens.append((a.get_actor_transform(), a.get_actor_label(), str(a.get_editor_property('point_id'))))
        actors.destroy_actor(a)
chicken = u.load_asset('/Game/IsekaiClass/Life/Ranching/BP_R01_Chicken')
if not isinstance(u.get_default_object(chicken.generated_class()), u.ICRanchAnimal):
    u.BlueprintEditorLibrary.reparent_blueprint(chicken, u.ICRanchAnimal)
    u.BlueprintEditorLibrary.compile_blueprint(chicken)
cdo = u.get_default_object(chicken.generated_class())
cdo.set_editor_property('definition_id', 'RAN_R01_01')
cdo.set_editor_property('point_id', 'RAN_R01_01')
u.BlueprintEditorLibrary.compile_blueprint(chicken)
assert lib.save_loaded_asset(chicken, only_if_is_dirty=False)
if not old_chickens:
    old_chickens = [(u.Transform(location=u.Vector(2000, -13600, 40)), 'IC_R01_Chicken', 'RAN_R01_01')]
for transform, label, point in old_chickens:
    a = actors.spawn_actor_from_class(chicken.generated_class(), transform.translation, transform.rotation.rotator())
    a.set_actor_scale3d(u.Vector(1, 1, 1))
    a.set_actor_label(label)
    a.set_folder_path('IC_Life_Test/Ranching')
    a.set_editor_property('point_id', point or 'RAN_R01_01')

ranch = row('Life/DT_Ranching', 'RAN_R01_01')
farm = '/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm'
ranch.update(DropTableID='DRP_R01_CHICKEN', MaxHP='30', RespawnGameMinutes='0',
             IdleAnimation=farm + '/Animations/Chicken/AS_Chicken_Idle.AS_Chicken_Idle',
             MeshTransform='(Rotation=(X=0,Y=0,Z=0,W=1),Translation=(X=0,Y=0,Z=-40),Scale3D=(X=1,Y=1,Z=1))')
for r in load('Life/DT_Ranching'):
    r['DisplayName'] = loc(r['ID'] + '.NAME')
for r in load('Life/DT_IC_Fish'):
    if 'DisplayName' in r:
        r['DisplayName'] = loc(r['ID'] + '.NAME')

drops = load('Rewards/DT_IC_DropTable')
drop = next((r for r in drops if r['ID'] == 'DRP_R01_CHICKEN'), None)
if drop is None:
    drop = dict(drops[0]); drops.append(drop)
drop.update({tables['Rewards/DT_IC_DropTable'][1][0]: 'DRP_R01_CHICKEN', 'ID': 'DRP_R01_CHICKEN',
             'Selection': 'IndependentProbability', 'Rolls': '1', 'MinimumGold': '0', 'MaximumGold': '0',
             'EquipmentGrades': '()', 'bAutoCollectItems': 'True', 'bApplyPlayerLUKToGold': 'False',
             'NameKey': 'DRP_R01_CHICKEN.NAME', 'DescriptionKey': '',
             'Entries': '((ItemID="ITM_R01_CHICKEN_MEAT",MinimumCount=1,MaximumCount=1,Probability=1,Weight=1,bApplyPlayerLUKToProbability=False,bEquipmentGradeRoll=False),(ItemID="ITM_R01_RANCH_01",MinimumCount=1,MaximumCount=4,Probability=0.5,Weight=1,bApplyPlayerLUKToProbability=False,bEquipmentGradeRoll=False))'})
strings['DRP_R01_CHICKEN.NAME'] = '닭 사냥 보상'
registry = load('Registry/DT_IC_IDRegistry')
if not any(r['ID'] == 'DRP_R01_CHICKEN' for r in registry):
    entry = dict(next(r for r in registry if r['DefinitionType'] == 'DropTable'))
    entry.update({tables['Registry/DT_IC_IDRegistry'][1][0]: 'DRP_R01_CHICKEN', 'ID': 'DRP_R01_CHICKEN', 'NameKey': 'DRP_R01_CHICKEN.NAME', 'DescriptionKey': ''})
    registry.append(entry)

# Remove stale descriptions without changing any quest sequence or dialogue.
for key in list(strings):
    for before, after in deleted.items():
        strings[key] = re.sub(r'(?<![A-Za-z0-9_])' + before + r'(?![A-Za-z0-9_])', after, strings[key])
for relative in ('StringTables/ST_IC_Definitions', 'StringTables/ST_IC_Quest'):
    path = ROOT / 'Data' / (relative + '.csv')
    source = strings if relative.endswith('Definitions') else {r['Key']: r['SourceString'] for r in csv.DictReader(io.StringIO(path.read_text(encoding='utf-8-sig')))}
    if relative.endswith('Quest'):
        for key in list(source):
            for before, after in deleted.items():
                source[key] = source[key].replace(before, after)
            if '밀가루' in source[key]:
                source[key] = source[key].replace('밀가루', '밀')
    output = io.StringIO(newline=''); writer = csv.writer(output, lineterminator='\n')
    writer.writerow(['Key', 'SourceString']); writer.writerows(source.items()); write_file(path, output.getvalue())
    asset = u.load_asset(BASE + relative)
    assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(asset, str(path))
    assert lib.save_loaded_asset(asset, only_if_is_dirty=False)

ui_path = ROOT / 'Data/StringTables/ST_IC_UI.csv'
ui_strings = {r['Key']: r['SourceString'] for r in csv.DictReader(io.StringIO(ui_path.read_text(encoding='utf-8-sig')))}
ui_strings.update(Life_HarvestReady='{0} · 수확 가능 [F]', Life_Growing='{0} · 성장 {1}/{2}',
                  Life_Regrowing='{0} · 재생까지 게임시간 {1}분', Life_BagFull='가방이 가득 찼습니다.')
ui_strings['Q3_1_Result'] = 'R01_08의 정보와 닭고기 구이 레시피를 받습니다. 재료: 닭고기 2개.'
for i in (2, 3, 4):
    ui_strings['Q3_%d_Result' % i] = '유채나물 레시피를 받습니다. 재료: 유채 2개.'
output = io.StringIO(newline=''); writer = csv.writer(output, lineterminator='\n', quoting=csv.QUOTE_ALL)
writer.writerow(['Key', 'SourceString']); writer.writerows(ui_strings.items()); write_file(ui_path, output.getvalue())
ui_asset = u.load_asset(BASE + 'StringTables/ST_IC_UI')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(ui_asset, str(ui_path))
assert lib.save_loaded_asset(ui_asset, only_if_is_dirty=False)

changed = []
for relative, (asset, fields, rows) in tables.items():
    output = io.StringIO(newline=''); writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
    writer.writeheader(); writer.writerows(rows)
    text = output.getvalue()
    assert not any(re.search(r'(?<![A-Za-z0-9_])' + id + r'(?![A-Za-z0-9_])', text) for id in deleted), relative
    if rows != original_rows[relative]:
        assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset, text), relative
        assert lib.save_loaded_asset(asset, only_if_is_dirty=False)
        changed.append(relative)
        write_file(ROOT / 'Data' / (relative + '.csv'), text)
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
write_file(OUT / 'setup.json', json.dumps({'deleted_items': list(deleted), 'weapon_costs': weapon_costs,
           'growth': growth, 'changed_tables': changed, 'chicken_egg_probability': .5}, ensure_ascii=False, indent=2))
u.log('LIFE_DATA_SETUP_OK ' + str(OUT / 'setup.json'))
