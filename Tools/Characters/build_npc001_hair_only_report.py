"""Document the actual atlas split and native captures for the NPC001 hair-only revision."""
import json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive/NPC001HairOnly'
v=json.loads((OUT/'Final/Verification.json').read_text(encoding='utf-8'))
a=json.loads((OUT/'Applied.json').read_text(encoding='utf-8'))
assert v['complete'] and len(v['samples'])==8 and v['validation']['face_restored_exact_bytes']
labels=[('Night_Outside','밤 외부 · 20시'),('Night_Inside','밤 내부 · 20시'),('Day_Outside','낮 외부 · 10시'),('Day_Inside','낮 내부 · 10시')]
style='''*{box-sizing:border-box}body{background:#151920;color:#e7edf5;font:16px/1.7 "Segoe UI","Malgun Gothic",sans-serif;margin:0}main{max-width:1540px;padding:24px;margin:auto}h1{font-size:28px}h2{font-size:23px;margin-top:30px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0}img{width:100%;height:auto;display:block}figcaption{padding:6px 0}a{color:#a8cdff}.note{background:#263649;padding:16px}code{overflow-wrap:anywhere}@media(max-width:760px){.grid{grid-template-columns:1fr}}'''
def fig(path,label):
    assert struct.unpack('>II',(OUT/path).read_bytes()[16:24])==(1600,900)
    return f'<figure><a href="{path}"><img src="{path}" width="1600" height="900" alt="{label}"></a><figcaption>{label}</figcaption></figure>'
body='<h1>NPC 001 — 눈 복원·머리카락만 보정</h1><p class="note">눈·얼굴 재질은 산란 수정 전 백업과 동일한 파일로 복원했다. 실제 머리카락은 몸과 같은 두 번째 아틀라스에 있어, 두 UV 영역만 선택해 약하게 음영을 완화했다. 얼굴·눈은 Default Lit, 옷·피부도 기존 Default Lit, 머리카락 UV만 Subsurface다. 자체 발광 0, 원본 색/알파와 배율 0.65를 유지했다.</p>'
body+='<h2>머리카락 보정 전·후 · 눈은 양쪽 모두 복원 상태</h2><div class="grid">'
body+=fig('Trials/Baseline_Day_Outside_Closeup.png','눈 복원·머리카락 보정 전')
body+=fig('Final/NPC_R01_001_Day_Outside_Closeup.png','현재: 눈 복원·머리카락만 보정')+'</div>'
body+='<h2>직전 버전과 현재 전신 비교</h2><p>왼쪽은 직전 얼굴·눈 산란 버전, 오른쪽은 현재 눈 복원·머리카락 보정 버전이다. 동일 위치·카메라·시간의 원본 캡처다.</p>'
for key,label in labels:
    body+='<h2>'+label+'</h2><div class="grid">'
    body+=fig(f'../NPC001SoftHead/Final/NPC_R01_001_{key}.png','직전 버전')
    body+=fig(f'Final/NPC_R01_001_{key}.png','현재')+'</div>'
body+='<h2>현재 네 조건 근접</h2><div class="grid">'
for key,label in labels:body+=fig(f'Final/NPC_R01_001_{key}_Closeup.png',label)
body+='</div><h2>적용 위치와 정정</h2><p>NPC 001 원본 T_texture0은 얼굴·눈, T_texture3은 몸·의상·머리카락이다. 모든 머리 영역이 첫 슬롯이라는 이전 설명은 잘못되었다. 실제 PNG로 확인한 두 머리카락 UV 영역을 선택했다. 메시 슬롯·스켈레톤·애니메이션은 변경하지 않았다.</p>'
body+=f'<p>전용 부모 <code>/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001/M_ICNPC001HairOnly</code>. 산란 {a["scatter"]}, 밀도 {a["density"]}. 머리카락 이외 픽셀은 기존 Default Lit을 사용한다.</p>'
body+='<p><a href="Applied.json">적용 기록</a> · <a href="Final/Verification.json">저장 후 새 실행 검증</a> · <a href="TextureAtlas.png">원본 얼굴 아틀라스</a> · <a href="T_texture3.png">원본 몸·머리카락 아틀라스</a></p>'
document='<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>NPC 001 눈 복원·머리카락 보정</title><style>'+style+'</style><main>'+body+'</main></html>'
(OUT/'Comparison.html').write_text(document,encoding='utf-8')
for link in re.findall(r'(?:src|href)="([^"]+)"',document):assert (OUT/link).is_file(),link
doc=f'''# NPC 001 눈 복원·머리카락 UV만 음영 보정

원본 PNG 확인 결과 NPC 001은 첫 슬롯(T_texture0)에 얼굴·눈, 두 번째 슬롯(T_texture3)에 몸·옷·머리카락이 들어 있다.
이전 설명에서 머리카락이 얼굴·눈과 같은 슬롯이라고 했던 내용을 정정한다. 앞선 SoftHead 실험은 얼굴·눈에만 작용했다.

이번에는 눈·얼굴 MI를 산란 적용 전 백업과 바이트 단위로 동일하게 복원했다. 두 번째 슬롯은 원래 부모 계층을 NPC 001 전용으로
복제하고, 머리카락 UV 두 영역만 Subsurface로 선택한다. 다른 픽셀은 Default Lit이다.

- 부모: `/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001/M_ICNPC001HairOnly`
- 머리카락 UV: U 0.3745~0.6255 / V 0.2495~0.3755, U 0.255~0.37 / V 0.38~0.495.
- `ICHairScatter = {a['scatter']}`, `ICHairDensity = {a['density']}`. 강한 보정은 너무 밝아져 제외했다.
- 원본 텍스처·알파·표면색 배율 0.65·재질 파라미터를 유지했다. 자체 발광 0, 그림자 유지.
- 얼굴·눈 MI 복원 해시, 다른 파일 해시, 기존 몸 재질의 모든 상속 파라미터 일치를 확인했다.
- 다른 10명·플레이어·월드는 수정하지 않았다. 스켈레톤·물리·애니메이션·슬롯도 같다.
- 새 렌더링 에디터에서 저장된 기본 재질로 네 조건 전신·근접 8장을 1600×900으로 촬영했다.

캡처: `Saved/CharacterLighting/NPCWorldReactive/NPC001HairOnly/Comparison.html`.
직전 재질 백업: `{a['backup']}`.
설정 코드: `Tools/Characters/setup_npc001_hair_only.py`.
검증 코드: `review_npc001_soft_head.py -ICNPC001HairOnlyReview -ICNPC001HairOnlyFinal`.
원본 얼굴/몸 아틀라스 PNG도 같은 캡처 폴더에 보관했다.

참고: [Epic — From Material Expression](https://dev.epicgames.com/documentation/en-us/unreal-engine/from-material-expression-shading-model-in-unreal-engine).
'''
(ROOT/'Docs/Characters/NPC001_HairOnly.ko.md').write_text(doc,encoding='utf-8')
print('Built hair-only revision report, native before/after galleries, and corrected atlas documentation.')
