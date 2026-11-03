"""Native before/after screenshots for the NPC 001-only head revision."""
import json
import re
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive/NPC001SoftHead'
v=json.loads((OUT/'Final/Verification.json').read_text(encoding='utf-8'))
assert v['complete'] and len(v['samples'])==8
applied=json.loads((OUT/'Applied.json').read_text(encoding='utf-8'))
before=json.loads((OUT.parent/'Final/Verification.json').read_text(encoding='utf-8'))
labels=[('Night_Outside','밤 외부 · 20시'),('Night_Inside','밤 내부 · 20시'),('Day_Outside','낮 외부 · 10시'),('Day_Inside','낮 내부 · 10시')]
style='''*{box-sizing:border-box}body{background:#151920;color:#e7edf5;font:16px/1.7 "Segoe UI","Malgun Gothic",sans-serif;margin:0}main{max-width:1540px;padding:24px;margin:auto}h1{font-size:28px}h2{font-size:23px;margin-top:30px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0}img{width:100%;height:auto;display:block}figcaption{padding:6px 0}a{color:#a8cdff}.note{background:#263649;padding:16px}code{overflow-wrap:anywhere}@media(max-width:760px){.grid{grid-template-columns:1fr}}'''
def fig(path,label):
    image=OUT/path
    assert struct.unpack('>II',image.read_bytes()[16:24])==(1600,900)
    return f'<figure><a href="{path}"><img src="{path}" width="1600" height="900" alt="{label}"></a><figcaption>{label}</figcaption></figure>'
body='<h1>NPC 001 — 얼굴·눈 음영 완화 실험 기록</h1><p class="note">NPC_R01_001의 첫 번째 재질(얼굴·눈)에만 산란을 적용했던 기록이다. 후속 원본 아틀라스 검사 결과 실제 머리카락은 두 번째 몸 아틀라스에 포함된다. 이 실험에서 머리카락도 변경했다는 이전 설명을 바로잡는다. 현재 버전은 얼굴·눈 복원 후 머리카락 UV만 조정한 구성이다.</p>'
if (OUT.parent/'NPC001HairOnly/Comparison.html').is_file():body+='<p><a href="../NPC001HairOnly/Comparison.html">현재 버전: 눈 복원·머리카락만 보정</a></p>'
body+='<p><a href="FullBodyComparison.html">수정 전·후 전신샷 네 조건 비교</a></p>'
body+='<h2>햇빛 아래 눈 근접 · 같은 위치와 카메라</h2><div class="grid">'
body+=fig('Trials/Baseline_Day_Outside_Closeup.png','수정 전')
body+=fig('Final/NPC_R01_001_Day_Outside_Closeup.png','수정 후')+'</div>'
body+='<h2>최종 저장 후 · 네 조건</h2><div class="grid">'
for key,label in labels:body+=fig(f'Final/NPC_R01_001_{key}.png',label)
body+='</div><h2>눈·머리 근접 · 네 조건</h2><div class="grid">'
for key,label in labels:body+=fig(f'Final/NPC_R01_001_{key}_Closeup.png',label)
body+='</div><h2>적용 위치</h2><p><code>/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001/M_ICNPC001SoftHead</code>. 머리 인스턴스의 부모만 교체했다. '
body+=f'Subsurface 산란 {applied["scatter"]}, 밀도 {applied["density"]}. Masked·Two Sided·Roughness 1·Specular/Metallic/Emissive 0. 메시 기본 슬롯 경로와 몸 재질은 같다.</p>'
body+='<p><a href="Applied.json">적용 값</a> · <a href="Final/Verification.json">저장 후 새 실행 검증</a> · <a href="Trials/Verification.json">사전 비교 기록</a></p>'
document='<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>NPC 001 머리 음영 수정</title><style>'+style+'</style><main>'+body+'</main></html>'
(OUT/'Comparison.html').write_text(document,encoding='utf-8')
fullbody='<h1>NPC 001 — 머리·눈 음영 수정 전·후 전신 비교</h1><p class="note">왼쪽은 수정 전, 오른쪽은 수정 후다. 기존에 촬영한 원본 1600×900 전신샷을 사용했다. 위치·카메라(시야각 40°)·시간·조명 설정이 같은 네 조건이다.</p>'
fullbody+='<p><a href="Comparison.html">눈 근접과 적용 내역 보기</a></p>'
for key,label in labels:
    old=next(s for s in before['samples'] if s['npc']=='NPC_R01_001' and s['label']==key)
    new=next(s for s in v['samples'] if s['label']==key and not s['closeup'])
    assert old['minute']==new['minute'] and old['sun_ray_blocked']==new['sun_ray_blocked']
    assert old['lighting']==new['lighting']
    fullbody+='<h2>'+label+'</h2><div class="grid">'
    fullbody+=fig(f'../Final/NPC_R01_001_{key}.png','수정 전')
    fullbody+=fig(f'Final/NPC_R01_001_{key}.png','수정 후')+'</div>'
full_document='<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>NPC 001 전신 수정 전후</title><style>'+style+'</style><main>'+fullbody+'</main></html>'
(OUT/'FullBodyComparison.html').write_text(full_document,encoding='utf-8')
for link in re.findall(r'(?:src|href)="([^"]+)"',document):assert (OUT/link).is_file(),link
for link in re.findall(r'(?:src|href)="([^"]+)"',full_document):assert (OUT/link).is_file(),link
print('Built NPC 001 before/after and four-condition native screenshot comparison.')
