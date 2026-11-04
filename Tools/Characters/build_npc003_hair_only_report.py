"""NPC_R01_003 native full-body before/after lighting report."""
import json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly'
IDENTIFIER='NPC_R01_003'
v=json.loads((OUT/'Final/Verification.json').read_text(encoding='utf-8'))
a=json.loads((OUT/'Applied.json').read_text(encoding='utf-8'))
old=json.loads((OUT.parent/'Final/Verification.json').read_text(encoding='utf-8'))
assert v['complete'] and len(v['samples'])==8 and v['validation']['face_baseline_identical_bytes']
labels=[('Night_Outside','밤 외부 · 20시'),('Night_Inside','밤 내부 · 20시'),('Day_Outside','낮 외부 · 10시'),('Day_Inside','낮 내부 · 10시')]
style='''*{box-sizing:border-box}body{background:#151920;color:#e7edf5;font:16px/1.7 "Segoe UI","Malgun Gothic",sans-serif;margin:0}main{max-width:1540px;padding:24px;margin:auto}h1{font-size:28px}h2{font-size:23px;margin-top:30px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0}img{width:100%;height:auto;display:block}figcaption{padding:6px 0}a{color:#a8cdff}.note{background:#263649;padding:16px}code{overflow-wrap:anywhere}@media(max-width:760px){.grid{grid-template-columns:1fr}}'''
def fig(path,label):
    assert struct.unpack('>II',(OUT/path).read_bytes()[16:24])==(1600,900)
    return f'<figure><a href="{path}"><img src="{path}" width="1600" height="900" alt="{label}"></a><figcaption>{label}</figcaption></figure>'
body='<h1>NPC_R01_003 — 눈·얼굴 유지, 머리카락만 보정</h1><p class="note">분홍색 긴 머리 NPC 3에 적용했다. NPC 1과 다른 원본 아틀라스의 머리카락 두 영역을 확인해 선택했다. 얼굴·눈 재질 파일은 적용 전과 동일하다. 머리카락 UV만 Subsurface, 옷·피부는 기존 Default Lit이며, 자체 발광 0과 원본 색·알파·표면색 배율 0.65를 유지했다.</p>'
body+='<h2>햇빛 아래 머리카락 근접</h2><div class="grid">'+fig('Trials/Baseline_Day_Outside_Closeup.png','보정 전')+fig(f'Final/{IDENTIFIER}_Day_Outside_Closeup.png','현재')+'</div>'
body+='<h2>전신 비교 · 왼쪽 보정 전 / 오른쪽 현재</h2><p>같은 위치·카메라·시간의 1600×900 원본 캡처다.</p>'
for key,label in labels:
    before=next(row for row in old['samples'] if row['npc']==IDENTIFIER and row['label']==key)
    after=next(row for row in v['samples'] if row['label']==key and not row['closeup'])
    assert before['minute']==after['minute'] and before['lighting']==after['lighting']
    assert before['sun_ray_blocked']==after['sun_ray_blocked']
    body+='<h2>'+label+'</h2><div class="grid">'+fig(f'../Final/{IDENTIFIER}_{key}.png','보정 전')+fig(f'Final/{IDENTIFIER}_{key}.png','현재')+'</div>'
body+='<h2>현재 네 조건 근접</h2><div class="grid">'
for key,label in labels:body+=fig(f'Final/{IDENTIFIER}_{key}_Closeup.png',label)
body+='</div><h2>적용 값</h2>'
body+=f'<p><code>/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_003/M_ICNPC003HairOnly</code>. 산란 {a["scatter"]}, 밀도 {a["density"]}. NPC 1·다른 NPC·플레이어·월드 파일은 그대로다.</p>'
body+='<p><a href="Applied.json">적용 기록</a> · <a href="Final/Verification.json">새 실행 검증</a> · <a href="Atlas_Slot0.png">원본 얼굴·눈 아틀라스</a> · <a href="Atlas_Slot1.png">원본 몸·머리카락 아틀라스</a></p>'
document='<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>NPC_R01_003 머리카락 보정 비교</title><style>'+style+'</style><main>'+body+'</main></html>'
(OUT/'Comparison.html').write_text(document,encoding='utf-8')
for link in re.findall(r'(?:src|href)="([^"]+)"',document):assert (OUT/link).is_file(),link
scope=json.loads((OUT/'Scope.json').read_text(encoding='utf-8'))
doc=f'''# NPC_R01_003 머리카락만 음영 보정

분홍색 긴 머리 NPC_R01_003에 적용했다. 눈·얼굴의 현재 재질은 변경하지 않았다.
원본 아틀라스 슬롯 0은 얼굴·눈, 슬롯 1은 몸·옷·머리카락이다. 실제 PNG를 확인해 NPC 3의 머리카락 두 UV 영역을 선택했다.
NPC 1의 UV 좌표를 복사해서 적용하지 않았다.

- 머리카락 UV: `{scope['hair_uv_rectangles']}`.
- 전용 부모: `/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_003/M_ICNPC003HairOnly`.
- `ICHairScatter = {a['scatter']}`, `ICHairDensity = {a['density']}`.
- From Material Expression으로 머리카락 UV만 Subsurface, 그 밖의 옷·피부는 Default Lit이다.
- 원본 색·알파·표면색 배율 0.65·자체 발광 0·그림자를 유지했다.
- 기존 몸 재질의 상속 파라미터 동일성과 눈·얼굴 파일 해시 보존을 검사했다.
- NPC 1을 포함한 다른 NPC·플레이어·월드·메시·스킬·애니메이션은 변경하지 않았다.
- 저장 후 새 렌더링 에디터에서 기본 재질로 밤·낮 실내외의 전신·근접 8장을 촬영했다.

보고서: `Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/Comparison.html`.
백업: `{a['backup']}`.
설정: `Tools/Characters/setup_npc003_hair_only.py`.
검증: `review_npc001_soft_head.py -ICNPC003HairOnlyReview -ICNPC003HairOnlyFinal`.
'''
(ROOT/'Docs/Characters/NPC003_HairOnly.ko.md').write_text(doc,encoding='utf-8')
print('Built NPC_R01_003 native full-body and closeup comparison report.')
