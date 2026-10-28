"""Export the actual item/recipe CSVs as a Korean text reference, without changing game data."""
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(name):
    with (ROOT / 'Data' / name).open(encoding='utf-8-sig', newline='') as stream:
        return list(csv.DictReader(stream))


strings = {r['Key']: r['SourceString'] for r in read('StringTables/ST_IC_Definitions.csv')}
items = {r['ID']: r for r in read('Items/DT_IC_Item.csv')}
recipes = read('Recipes/DT_IC_Recipe.csv')
names = {id: strings.get(id + '.NAME', id) for id in items}
sections = [
    ('채집물', lambda id: id.startswith('ITM_R01_PLANT_')),
    ('씨앗', lambda id: id.startswith('ITM_R01_SEED_')),
    ('작물', lambda id: id.startswith('ITM_R01_CROP_')),
    ('물고기', lambda id: id.startswith('ITM_R01_FISH_') and id[-2:].isdigit()),
    ('광석', lambda id: id.startswith('ITM_R01_ORE_')),
    ('축산물', lambda id: id.startswith('ITM_R01_RANCH_') or id == 'ITM_R01_CHICKEN_MEAT'),
    ('낚시 용품', lambda id: id in ('ITM_R01_FISH_ROD', 'ITM_R01_FISH_BAIT')),
    ('R01 요리', lambda id: id.startswith('ITM_R01_FOOD_')),
    ('기존 회복·음식', lambda id: id.startswith(('ITM_CONS_', 'ITM_FOOD_'))),
    ('성장서', lambda id: id.startswith('ITM_BOOK_')),
    ('무기', lambda id: id.startswith('EQP_WPN_')),
    ('확장권·퀘스트 아이템', lambda id: id in ('ITM_TKT_001', 'ITM_QST_001')),
]
lines = ['퀘스트 제작용 DT 아이템 정리', '기준일: 2026-10-27',
         '출처: Data/Items/DT_IC_Item.csv, Data/StringTables/ST_IC_Definitions.csv, Data/Recipes/DT_IC_Recipe.csv',
         '수록 아이템: 총 %d종' % len(items), '이 파일은 참고 문서이며 게임 데이터를 변경하지 않습니다.', '']
seen = set()
for number, (title, accept) in enumerate(sections, 1):
    ids = sorted(id for id in items if accept(id))
    lines.append('%d. %s (%d종)' % (number, title, len(ids)))
    for id in ids:
        seen.add(id); r = items[id]
        extra = []
        if id.startswith('ITM_R01_SEED_'):
            extra.append('구매가 %s' % r['BuyPrice'])
        if r['bCanSell'] == 'True':
            extra.append('판매가 %s' % r['SellPrice'])
        if r['UseEffect'] == 'Recovery':
            extra.append('회복 HP %g / MP %g / SP %g' % tuple(float(r[k]) for k in ('RestoreHP', 'RestoreMP', 'RestoreSP')))
        if r['UseEffect'] == 'GrowthBook':
            extra.append('해당 능력치 +%s' % r['GrowthAmount'])
        if id == 'ITM_QST_001':
            extra.append('퀘스트 탭, 버리기·판매·선물 불가')
        if id.startswith('ITM_R01_FOOD_') or id.startswith('EQP_WPN_'):
            recipe = next((q for q in recipes if '(ItemID="%s",Count=1)' % id in q['Results']), None)
            if recipe:
                costs = re.findall(r'ItemID="?([^",)]+)"?,Count=(\d+)', recipe['Ingredients'])
                extra.append('재료: ' + ' + '.join(names[k] + ' ×' + count for k, count in costs))
                extra.append('레시피 ' + recipe['ID'])
        lines.append('- ' + id + ' | ' + names[id] + (' | ' + ' | '.join(extra) if extra else ''))
    lines.append('')
assert seen == set(items), set(items) - seen
lines += ['퀘스트 연결 시 참고',
          '- 기존 빵 ITM_FOOD_002: 밀 2개. 밀빵 ITM_R01_FOOD_12: 밀 3개.',
          '- 닭 처치 시 닭고기 1개, 50% 확률로 달걀 1~4개. 드롭 DT에서 변경 가능.',
          '- 채집물은 게임시간 2시간 후 재생. 작물은 하루마다 다음 단계, 마지막 단계에서 수확.',
          '- 수확 후 작물은 게임시간 2시간 뒤 첫 단계로 재생.',
          '- 현재 낚시 포인트는 물고기 10종을 연결한 데이터와 표시용 박스까지이며 실제 낚시 조작은 미연결.',
          '- 아이템 이름·설명 번역 키: ST_IC_Definitions의 <ID>.NAME / <ID>.DESCRIPTION.',
          '- 삭제된 재료·선물·임시 이름 아이템은 신규 퀘스트에 사용하지 않는다.', '']
(ROOT / 'Docs/퀘스트_아이템_정리.txt').write_text('\n'.join(lines), encoding='utf-8')
print('Exported %d items.' % len(items))
