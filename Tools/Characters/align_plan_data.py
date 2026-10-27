"""Apply the approved R01 creation rewards and prototype recipe/settings data."""
import csv
import io
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / 'Data'
TABLE = '/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions'

def read(relative):
    with (DATA / relative).open(encoding='utf-8-sig', newline='') as stream:
        reader = csv.DictReader(stream)
        return reader.fieldnames, list(reader)

def write(relative, fields, rows):
    stream = io.StringIO(newline='')
    writer = csv.DictWriter(stream, fields, lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
    path = DATA / relative
    value = stream.getvalue().replace('\r\n', '\n').replace('\r', '')
    if path.read_bytes() == value.encode('utf-8'):
        return
    temporary = path.with_suffix('.csv.tmp')
    temporary.write_text(value, encoding='utf-8', newline='')
    temporary.replace(path)

def update(relative, changes):
    fields, rows = read(relative)
    for row in rows:
        key = row.get('ID') or row.get('Name') or row.get('---')
        row.update(changes.get(key, {}))
    write(relative, fields, rows)

def upsert(rows, row, key='ID'):
    for index, old in enumerate(rows):
        if old[key] == row[key]:
            rows[index] = row
            return
    rows.append(row)

def named(template, key):
    row = dict(template)
    for field in ('---', 'ID', 'ItemID', 'RecipeID'):
        if field in row:
            row[field] = key
    row.update(NameKey=key + '.NAME', DescriptionKey=key + '.DESCRIPTION', bTestOnly='False')
    if 'DisplayName' in row:
        row['DisplayName'] = f'LOCTABLE("{TABLE}", "{key}.NAME")'
    return row

update('Settings/DT_IC_TimeSettings.csv', {'CFG_TIME_001': {'MinimumSleepGameMinutes': '120', 'SleepRecoveryPerGameMinute': '1'}})
update('Settings/DT_IC_InventorySettings.csv', {'CFG_INVENTORY_001': {'MaximumStorageInstallations': '3', 'DropLifetimePlaySeconds': '600'}})
update('Settings/DT_IC_StatSettings.csv', {'CFG_STAT_001': {'CHABonus': '0', 'CompanionCHAAffectedStats': '()'}})

fields, rows = read('World/DT_IC_Home.csv')
for row in rows:
    row['MaximumStorageInstallations'] = '3'
write('World/DT_IC_Home.csv', fields, rows)

choices = {}
for index, npc in enumerate((2, 4, 1, 3), 1):
    choices[f'Q4_{index}'] = {'NPCID': f'NPC_R01_{npc:03}', 'SkillID': f'SKL_NPC_R01_{npc:03}', 'bTestReward': 'False'}
for index in range(1, 5):
    choices[f'Q3_{index}'] = {'NPCID': 'NPC_R01_008' if index == 1 else 'None', 'RecipeID': 'RCP_HERO_GIFT' if index == 1 else 'RCP_GREEN_JUICE', 'CodexID': 'CDX_NPC_R01_008' if index == 1 else 'None', 'bTestReward': 'True'}
update('Settings/DT_IC_CreationOptions.csv', choices)

fields, items = read('Items/DT_IC_Item.csv')
gift = next(row for row in items if row['ID'] == 'ITEM_P0_01')
gift.update(DisplayName=f'LOCTABLE("{TABLE}", "ITEM_P0_01.NAME")', DescriptionKey='ITEM_P0_01.DESCRIPTION', bTestOnly='False')
for key in ('ITM_MAT_PORK', 'ITM_MAT_GREEN_HERB', 'ITEM_HERO_GIFT'):
    row = named(gift, key)
    row['bCanGift'] = 'True' if key == 'ITEM_HERO_GIFT' else 'False'
    upsert(items, row)
# Old books stay resolvable for saves; the native item-use service rejects CHA growth.
for row in items:
    if row['GrowthStat'] == 'Charisma':
        row.update(bDeprecated='False', bTestOnly='False')
write('Items/DT_IC_Item.csv', fields, items)

fields, recipes = read('Recipes/DT_IC_Recipe.csv')
template = recipes[0]
fields_p, productions = read('Life/DT_IC_Production.csv')
production_template = productions[0]
for key, material, count, result in (
    ('HERO_GIFT', 'ITM_MAT_PORK', 2, 'ITEM_HERO_GIFT'),
    ('GREEN_JUICE', 'ITM_MAT_GREEN_HERB', 1, 'ITEM_P0_01'),
):
    recipe = named(template, 'RCP_' + key)
    recipe.update(ProductionID='PRD_' + key, Ingredients=f'((ItemID="{material}",Count={count}))', Results=f'((ItemID="{result}",Count=1))', bUnlockedByDefault='False')
    upsert(recipes, recipe)
    production = named(production_template, 'PRD_' + key)
    production.update(Materials=f'((Item=(ItemID="{material}",Count={count}),ConsumptionTiming=ConsumeOnSuccess))', Results=recipe['Results'])
    upsert(productions, production)
write('Recipes/DT_IC_Recipe.csv', fields, recipes)
write('Life/DT_IC_Production.csv', fields_p, productions)

fields, prefs = read('NPC/DT_IC_GiftPreference.csv')
hero = named(prefs[0], 'GIFT_R01_HERO')
# Keep the original common +20 gift valid for every R01 NPC, including the hero.
hero['Items'] = '((ItemID="ITEM_P0_01",Preference=Preferred),(ItemID="ITEM_HERO_GIFT",Preference=Preferred))'
upsert(prefs, hero)
write('NPC/DT_IC_GiftPreference.csv', fields, prefs)
fields, npcs = read('NPC/DT_IC_NPC.csv')
hero_npc = next(row for row in npcs if row['ID'] == 'NPC_R01_008')
hero_npc['GiftPreferenceID'] = 'GIFT_R01_HERO'
write('NPC/DT_IC_NPC.csv', fields, npcs)

strings = {
    'ITEM_P0_01': ('Item_P0_01 · 녹즙 (테스트)', '초록약초 1개로 만드는 공통 선호 선물. 호감도 +20. 임시 네모 외형.'),
    'ITEM_HERO_GIFT': ('용사 선물 (테스트)', '돼지고기 2개로 만드는 R01_08의 선호 선물. 호감도 +20. 임시 네모 외형.'),
    'ITM_MAT_PORK': ('돼지고기', '용사 선물 테스트 재료.'),
    'ITM_MAT_GREEN_HERB': ('초록약초', '녹즙 테스트 재료.'),
    'RCP_HERO_GIFT': ('용사 레시피 (테스트)', '돼지고기 2개 → 용사 선물 1개.'),
    'RCP_GREEN_JUICE': ('녹즙 레시피 (테스트)', '초록약초 1개 → 녹즙 1개.'),
    'PRD_HERO_GIFT': ('용사 선물 요리', '용사 선물 테스트 생산.'),
    'PRD_GREEN_JUICE': ('녹즙 요리', '녹즙 테스트 생산.'),
    'GIFT_R01_HERO': ('R01_08 선호 선물', '용사 선물과 공통 녹즙을 선호한다.'),
    'CDX_NPC_R01_008': ('R01_08 · 용사', '지역 1 용사 배경의 인물. 마법사. 선호 선물: 용사 선물(돼지고기 2개).'),
}
fields, codex = read('Presentation/DT_IC_Codex.csv')
entry = dict.fromkeys(fields, '')
entry.update({'---': 'CDX_NPC_R01_008', 'ID': 'CDX_NPC_R01_008', 'Category': 'NPC', 'Subject': '(Type=NPC,ID="NPC_R01_008")', 'Registration': 'FirstNPCConversation', 'DisplayOrder': '8', 'Image': 'None', 'NameKey': 'CDX_NPC_R01_008.NAME', 'DescriptionKey': 'CDX_NPC_R01_008.DESCRIPTION', 'Tags': '(GameplayTags=)', 'DataVersion': '1', 'bDeprecated': 'False', 'bTestOnly': 'False'})
upsert(codex, entry)
write('Presentation/DT_IC_Codex.csv', fields, codex)
fields, rows = read('StringTables/ST_IC_Definitions.csv')
for key, (name, description) in strings.items():
    for suffix, value in (('NAME', name), ('DESCRIPTION', description)):
        upsert(rows, {'Key': key + '.' + suffix, 'SourceString': value}, 'Key')
write('StringTables/ST_IC_Definitions.csv', fields, rows)

fields, registry = read('Registry/DT_IC_IDRegistry.csv')
template = registry[0]
for key in strings:
    kind, folder, table = ('Item', 'Items', 'Item') if key.startswith(('ITEM_', 'ITM_')) else ('Recipe', 'Recipes', 'Recipe') if key.startswith('RCP_') else ('Production', 'Life', 'Production') if key.startswith('PRD_') else ('Codex', 'Presentation', 'Codex') if key.startswith('CDX_') else ('GiftPreference', 'NPC', 'GiftPreference')
    row = named(template, key)
    row.update(DefinitionType=kind, RegionID='R01', ReplacementID='None', DefinitionTable=f'/Game/IsekaiClass/Data/DataTable/{folder}/DT_IC_{table}.DT_IC_{table}', Notes='Approved R01 prototype recipe/gift definition.')
    upsert(registry, row)
write('Registry/DT_IC_IDRegistry.csv', fields, registry)

fields, ui = read('StringTables/ST_IC_UI.csv')
updates = {
    'Create_ReadyProgress': '5 / 5 · 시작 준비',
    'Menu_Dead': '쓰러졌습니다', 'Menu_Confirm': '확인', 'Menu_Exit': '게임 종료', 'Menu_Pause': '일시 정지',
    'Save_GameClock': '이세계 {0}일차 {1} {2}:{3}', 'AM': '오전', 'PM': '오후',
    'Q3_1_Result': 'R01_08(용사)의 정보와 용사 레시피를 받습니다. 테스트 재료: 돼지고기 2개.',
    'Q3_2_Result': '해당 지역 NPC는 준비 중입니다. 테스트용 녹즙 레시피를 받습니다. 초록약초 1개가 필요합니다.',
    'Q3_3_Result': '해당 지역 NPC는 준비 중입니다. 테스트용 녹즙 레시피를 받습니다. 초록약초 1개가 필요합니다.',
    'Q3_4_Result': '해당 지역 NPC는 준비 중입니다. 테스트용 녹즙 레시피를 받습니다. 초록약초 1개가 필요합니다.',
}
for index, npc in enumerate((2, 4, 1, 3), 1):
    updates[f'Q4_{index}_Result'] = f'R01_{npc:02}의 호감도 +300, 고유 기술과 해당 계열의 초보 무기를 받습니다.'
for key, value in updates.items():
    upsert(ui, {'Key': key, 'SourceString': value}, 'Key')
write('StringTables/ST_IC_UI.csv', fields, ui)
print('PLAN_DATA_ALIGNED')
