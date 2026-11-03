"""Create the eye-correction report from fresh, saved-material gameplay captures."""
import html
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/WorldReactive/EyeFix'
verification=json.loads((OUT/'Final/Verification.json').read_text(encoding='utf-8'))
assert verification['complete'] and len(verification['samples'])==16
assert any(path.endswith('M_ICAuthoredFeatureLit.M_ICAuthoredFeatureLit') for path in verification['graphs'])
LABELS=[('Night_Outside','밤 외부 · 20:00'),('Night_Inside','밤 내부 · 20:00'),('Day_Outside','낮 외부 · 10:00'),('Day_Inside','낮 내부 · 10:00')]
STYLE='''body{margin:0;background:#151920;color:#e7edf5;font:17px/1.7 system-ui,sans-serif}main{max-width:1500px;margin:auto;padding:28px}a{color:#a8cdff}h1{font-size:29px}h2{margin-top:34px;font-size:24px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0}img{display:block;width:100%;height:auto}figcaption{padding:7px 0}.note{padding:16px;background:#263649;border-left:4px solid #a8cdff}code,pre{background:#222e3c}code{padding:2px 5px}pre{padding:16px;overflow:auto}article{max-width:1000px;margin:auto}@media(max-width:760px){.grid{grid-template-columns:1fr}main{padding:14px}}'''
def document(title,body):
    return f'<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{title}</title><style>{STYLE}</style></head><body><main>{body}</main></body></html>'
def figure(path,label):
    safe=html.escape(path,quote=True)
    return f'<figure><a href="{safe}"><img src="{safe}" width="1600" height="900" loading="lazy" alt="{html.escape(label,quote=True)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'
body='''<h1>눈동자·아이라인 수정 — 실제 조명 반응 유지</h1><p>2026-11-03 · 저장 후 새 에디터에서 검증 · 실제 게임 창 1600×900 원본</p>
<p class="note">눈의 원본 색과 투명 마스크를 별도 Masked Default Lit 재질에 직접 연결했다. 자체 발광은 0이며 월드 조명·노출은 변경하지 않았다.
밤 외부 → 밤 내부 → 낮 외부 → 낮 내부 순서로 비교한다.</p><p><a href="TIL.html">진단과 수정 TIL</a> · <a href="Applied.json">적용 내역</a> · <a href="Final/Verification.json">새 실행 검증 기록</a></p>'''
for sex,name in [('F','여성'),('M','남성')]:
    body+=f'<h2>{name} — 눈 근접 확인</h2><div class="grid">'
    body+=''.join(figure(f'Final/{sex}_Closeup_{key}.png',label) for key,label in LABELS)+'</div>'
    body+=f'<h2>{name} — 전신과 환경</h2><div class="grid">'
    body+=''.join(figure(f'Final/{sex}_{key}.png',label) for key,label in LABELS)+'</div>'
body+='<h2>수정 전후 눈 근접</h2><p>왼쪽은 밝기 변경 직후 눈이 희게 흐려진 상태, 오른쪽은 별도 눈 재질을 저장한 뒤 다시 실행한 상태다.</p><div class="grid">'
body+=figure('F_Current.png','수정 전 · 눈동자와 아이라인이 희게 흐려짐')+figure('Final/F_Closeup_Day_Outside.png','수정 후 · 원본 눈동자와 아이라인')+'</div>'
body+='''<h2>설정과 범위</h2><p>눈·눈썹·속눈썹·아이라인·입은 M_ICAuthoredFeatureLit 부모를 사용한다.
원본 RGB × 원본 색 인자 × ICAlbedoScale(0.65)을 Base Color에, 원본 알파 × 색 인자 알파를 Opacity Mask에 연결했다.
Roughness 1, Specular 0, Emissive 0이다. 스켈레탈 메시와 Morph Target 플래그를 켰다.</p>
<p>여성의 피부·머리카락·옷은 기존 재질 경로를 유지한다. 남성은 눈이 포함된 머리 아틀라스 슬롯 전체가 별도 부모를 사용한다.
실제 조명을 받으므로 원래 Unlit처럼 모든 상황에서 동일한 눈 밝기를 유지하지 않는다.</p>'''
(OUT/'Comparison.html').write_text(document('플레이어 눈 수정 — 밤·낮 실내외',body),encoding='utf-8')
til='''<article><h1>밝기는 좋아졌는데 눈이 희게 변한 이유를 분리해서 확인하기</h1>
<p>2026-11-03 · Unreal Engine 5.8 · VRM4U</p>
<p>플레이어가 실제 빛과 그림자를 받게 바꾼 뒤, 전신의 실내외 밝기 차이는 생겼지만 눈동자와 아이라인이 희게 흐려졌다.
이전 확인은 전신 화면 위주여서 눈의 품질 문제를 놓쳤다. 이번에는 같은 장소에서 얼굴 근접 화면을 기준으로 비교했다.</p>
<h2>진단</h2><p>원본 재질로 눈 부분만 바꾸면 눈동자와 아이라인이 다시 보였다. 원본 diffuse 텍스처를 PNG로 내보내 확인했을 때도 색과 알파가 정상적으로 남아 있었다.
텍스처 파일을 다시 만드는 문제가 아니라 재질 변환 경로를 확인해야 했다.</p>
<p>기존 MToon 부모 안에 원본 RGB를 직접 사용하는 분기를 추가하고 파라미터 이름도 분리해 시험했지만, 흰색으로 흐려지는 현상은 남았다.
따라서 그 시도를 최종안으로 채택하지 않았다. 이 시험의 새 실행 캡처는 RejectedDirectBranch/Final에 보존했다.</p>
<p>다음으로 원본 RGB와 알파만 사용하는 별도 Default Lit/Unlit 대조 재질을 만들었다.
처음의 체크무늬 화면은 스켈레탈 메시·Morph Target 사용 플래그가 준비되지 않은 대체 재질 표시였다.
두 플래그를 명시적으로 켜고 다시 컴파일한 뒤 Default Lit에서 정상적인 눈동자와 아이라인이 보였다.</p>
<p>이 비교는 기존 복잡한 MToon 부모를 제외한 별도 표면에서 문제가 해결된다는 근거다.
MToon 내부의 어떤 단일 노드가 원인이었는지까지 완전히 분리했다고 주장하지 않는다.</p>
<h2>최종 구성</h2><pre>원본 diffuse RGB × 원본 tint RGB × 0.65 → Base Color
원본 diffuse Alpha × 원본 tint Alpha → Opacity Mask
0 → Emissive / Metallic / Specular
1 → Roughness
Shading Model = Default Lit
Blend Mode = Masked
Used With Skeletal Mesh = true
Used With Morph Targets = true</pre>
<p>별도 부모 M_ICAuthoredFeatureLit을 만들고 기존 플레이어 전용 눈·눈썹·속눈썹·아이라인·입 인스턴스의 부모로 연결했다.
여성의 피부·머리카락·옷에 사용하던 출력은 유지했다. 시험 중 기존 부모에 추가했던 색 분기는 제거했다.
남성은 눈과 피부·머리카락이 합쳐진 머리 아틀라스 슬롯 전체에 적용한다. 눈만 분리하는 새 UV 마스크를 추가하지 않았다.</p>
<p>눈만 빛나게 하는 Unlit 복원 대신 자체 발광 0의 실제 조명 표면을 사용한다.
실내나 밤에는 기존 주변광과 간접광을 받으면서 어두워진다. 월드·SkyLight·노출·낮밤 Blueprint는 변경하지 않았다.</p>
<h2>저장 후 새 실행 검증</h2><p>검증용 에디터를 종료하고 새로 실행한 게임에서 남녀 프리셋을 바꿨다.
눈이 별도 부모를 사용하는지, 원본 diffuse 텍스처가 연결되는지, 최종 Emissive가 상수 0인지 검사했다.
게임 중 재질 덮어쓰기나 밝기 컴포넌트 없이 저장된 기본 재질을 사용했다.</p>
<p>밤 20시 외부·내부, 낮 10시 외부·내부를 전신과 얼굴 근접으로 남녀 각각 촬영했다.
실제 창 크기는 1600×900이며 사진을 밝게 보정하거나 합성하지 않았다.
최종 실행 검증은 16장이고, 맵·DefaultEngine.ini·낮밤 Blueprint의 파일 해시도 보존됐다.</p>
<p><a href="Comparison.html">네 조건 전신·얼굴 비교</a> · <a href="Final/Verification.json">검증 기록</a> · <a href="../TIL.html">앞선 실내외 조명 진단 TIL</a></p>
<h2>복원과 한계</h2><p>눈 수정 전 플레이어 전용 재질은 Before/Materials에 백업했다.
원래 VRM 텍스처와 공유 VRM4U 부모 재질은 편집하지 않았다. 남성 머리 슬롯은 하나의 아틀라스이므로 눈 외 영역의 음영도 함께 달라질 수 있다.
모든 표정과 카메라 각도를 이 네 조건만으로 검증했다고 할 수는 없다. 성능 수치는 측정하지 않았다.</p>
<p>재현 코드는 Tools/Characters/setup_player_reactive_lighting.py,
새 실행 검증은 review_player_reactive_lighting.py에 -ICEyeCorrectionReview를 추가해 수행한다.</p></article>'''
(OUT/'TIL.html').write_text(document('TIL — VRoid 눈 재질 변환 문제와 수정',til),encoding='utf-8')
for name in ['Comparison.html','TIL.html']:
    path=OUT.parent/name;text=path.read_text(encoding='utf-8')
    if 'id="eye-correction-latest"' not in text:
        text=text.replace('<main>','<main><p class="note" id="eye-correction-latest">눈 수정 후 최신 결과: <a href="EyeFix/Comparison.html">밤·낮 실내외 전신·얼굴 캡처</a> · <a href="EyeFix/TIL.html">눈 수정 TIL</a>. 아래는 눈 수정 전의 조명 작업 기록입니다.</p>',1)
        path.write_text(text,encoding='utf-8')
print('Eye comparison and TIL built from 16 verified saved-material screenshots.')
