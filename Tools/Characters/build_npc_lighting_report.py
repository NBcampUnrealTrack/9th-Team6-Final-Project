"""Build the eleven-NPC lighting gallery from completed native gameplay verification."""
import html
import json
import re
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive'
verification=json.loads((OUT/'Final/Verification.json').read_text(encoding='utf-8'))
applied=json.loads((OUT/'Applied.json').read_text(encoding='utf-8'))
assert verification['complete'] and len(verification['samples'])==55
IDS=applied['npcs'];assert len(IDS)==11 and applied['slot_count']==22
LABELS=[('Night_Outside','밤 외부 · 20:00'),('Night_Inside','밤 내부 · 20:00'),('Day_Outside','낮 외부 · 10:00'),('Day_Inside','낮 내부 · 10:00')]
for row in verification['samples']:
    assert struct.unpack('>II',(OUT/'Final'/row['image']).read_bytes()[16:24])==(1600,900)
STYLE='''*{box-sizing:border-box}body{margin:0;background:#151920;color:#e7edf5;font:16px/1.7 "Segoe UI","Malgun Gothic",sans-serif}main{max-width:1540px;margin:auto;padding:28px}h1{font-size:29px;line-height:1.4}h2{margin-top:28px;font-size:23px}a{color:#a8cdff}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0;min-width:0}img{display:block;width:100%;height:auto}figcaption{padding:7px 0}.note{padding:16px;background:#263649;border-left:4px solid #a8cdff}code{background:#26313e;padding:2px 4px;overflow-wrap:anywhere}.selector{display:flex;gap:8px;flex-wrap:wrap;margin:20px 0}button{font:inherit;color:inherit;background:#273342;border:1px solid #5b7088;border-radius:6px;padding:7px 13px;cursor:pointer}button[aria-pressed=true]{background:#245b8c;border-color:#a8cdff}.overview{display:grid;grid-template-columns:repeat(3,1fr);gap:16px}table{border-collapse:collapse;width:100%}th,td{border:1px solid #607188;padding:8px 12px;text-align:left}@media(max-width:760px){.grid,.overview{grid-template-columns:1fr}main{padding:15px}}'''
def document(title,body,script=''):
    return f'<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{title}</title><style>{STYLE}</style></head><body><main>{body}</main>{script}</body></html>'
def figure(path,label,identity='',lazy=True):
    attributes=f' id="{identity}"' if identity else ''
    return f'<figure><a{attributes} href="{path}"><img src="{path}" width="1600" height="900"'+(' loading="lazy"' if lazy else '')+f' alt="{html.escape(label,quote=True)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'

selected='NPC_R01_003'
body='''<h1>R01 NPC 11명 — 플레이어와 같은 조명 방식 적용</h1>
<p class="note">얼굴·눈은 원본 RGB/알파를 사용하는 Masked Default Lit, 두 번째 아틀라스는 플레이어와 같은 WorldReactive 출력이다.
표면색 배율 0.65, 자체 발광 0. 월드·플레이어·원본 재질과 메시의 스켈레톤·물리·후처리 애니메이션 연결은 보존했다.</p>
<p>저장 후 새 에디터에서 실제 NPC Blueprint로 실행했다. 임시 재질 덮어쓰기 없이 11명 × 네 조건과 눈 근접 화면, 총 55장을 1600×900으로 촬영했다.</p>
<p><a href="EyesOverview.html">11명 눈 한눈에 보기</a> · <a href="All.html">모든 조건 펼쳐 보기</a> · <a href="Applied.json">적용 내역</a> · <a href="Final/Verification.json">실행 검증 기록</a></p>
<div class="selector">'''
soft_revision=OUT/'NPC001SoftHead/Comparison.html'
hair_revision=OUT/'NPC001HairOnly/Comparison.html'
if hair_revision.is_file():
    body=body.replace('<div class="selector">','<p class="note">이 페이지는 최초 11명 일괄 적용 시점의 캡처다. NPC 001은 이후 눈·얼굴을 복원하고 실제 머리카락 UV만 추가 보정했다. <a href="NPC001HairOnly/Comparison.html">NPC 001 현재 버전 보기</a></p><div class="selector">')
elif soft_revision.is_file():
    body=body.replace('<div class="selector">','<p class="note">이 페이지는 11명 일괄 적용 시점의 캡처다. 이후 NPC 001의 머리·눈 음영을 추가로 완화했다. <a href="NPC001SoftHead/Comparison.html">NPC 001 최신 비교 보기</a></p><div class="selector">')
if (OUT/'NPC003HairOnly/Comparison.html').is_file():
    body=body.replace('<div class="selector">','<p class="note">NPC_R01_003도 이후 눈·얼굴을 유지하고 머리카락 UV만 추가 보정했다. <a href="NPC003HairOnly/Comparison.html">NPC 003 현재 버전 보기</a></p><div class="selector">')
for identifier in IDS:
    body+=f'<button type="button" data-npc="{identifier}" aria-pressed="'+('true' if identifier==selected else 'false')+'">'+identifier[-3:]+'</button>'
body+='</div><h2 id="selected-name">'+selected+'</h2><div class="grid">'
for key,label in LABELS:body+=figure(f'Final/{selected}_{key}.png',label,key,False)
body+='</div><h2>눈·얼굴 비교 · 낮 외부</h2><div class="grid">'
body+=figure(f'Before/{selected}_Day_Outside_Closeup.png','수정 전','before',False)
body+=figure(f'Final/{selected}_Day_Outside_Closeup.png','최종 저장 후','after',False)
body+='''</div><h2>실제 적용 위치</h2><p>NPC별 <code>SK_R01_1~11</code>의 기본 재질 두 슬롯을
<code>/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001~011</code>로 연결했다.
공통 부모 <code>Shared/M_ICNPCFeatureLit</code>와 <code>Shared/M_ICNPCBodyLit</code>은 플레이어 최종 부모의 NPC 전용 복제본이다.</p>
<p>이름만으로 머리카락의 재질 슬롯을 판단하면 안 된다. NPC 001의 원본 아틀라스 확인 결과 얼굴·눈은 첫 슬롯, 머리카락은 몸과 함께 두 번째 슬롯에 들어 있다.
밤과 실내에도 기존 SkyLight·간접광이 남으므로 완전히 검게 되지는 않는다.</p>
<p>검증 중 AI 이동과 위치만 PIE에서 고정했다. 실제 스케줄·애니메이션·스킬 에셋은 수정하지 않았다.
검증 에디터의 GPU 장치 리셋 이후, 재질 저장은 NullRHI 실행에서 마쳤고 새 렌더링 에디터에서 위 캡처를 검증했다.</p>'''
script='''<script>const labels=['Night_Outside','Night_Inside','Day_Outside','Day_Inside'];for(const button of document.querySelectorAll('[data-npc]')){button.addEventListener('click',()=>{const id=button.dataset.npc;document.getElementById('selected-name').textContent=id;for(const key of labels){const link=document.getElementById(key);link.href='Final/'+id+'_'+key+'.png';link.querySelector('img').src=link.href;}for(const [key,folder]of [['before','Before'],['after','Final']]){const link=document.getElementById(key);link.href=folder+'/'+id+'_Day_Outside_Closeup.png';link.querySelector('img').src=link.href;}for(const other of document.querySelectorAll('[data-npc]'))other.setAttribute('aria-pressed',String(other===button));});}</script>'''
(OUT/'Comparison.html').write_text(document('NPC 11명 조명 적용 — 밤·낮 실내외',body,script),encoding='utf-8')

all_body='<h1>NPC 11명 전체 캡처</h1><p><a href="Comparison.html">NPC 선택 비교</a></p>'
for identifier in IDS:
    all_body+='<h2>'+identifier+'</h2><div class="grid">'
    all_body+=''.join(figure(f'Final/{identifier}_{key}.png',label) for key,label in LABELS)
    all_body+=figure(f'Final/{identifier}_Day_Outside_Closeup.png','눈 근접 · 낮 외부')+'</div>'
(OUT/'All.html').write_text(document('NPC 전체 55장',all_body),encoding='utf-8')
for label,title in [('Day_Outside_Closeup','EyesOverview'),('Day_Outside','DayOverview'),('Night_Inside','NightOverview')]:
    overview=f'<h1>{title} · NPC 001~011</h1><div class="overview">'
    overview+=''.join(figure(f'Final/{identifier}_{label}.png',identifier,lazy=False) for identifier in IDS)+'</div>'
    (OUT/(title+'.html')).write_text(document(title,overview),encoding='utf-8')

doc=f'''# R01 NPC 11명 조명 적용 — 2026-11-03

플레이어의 최종 WorldReactive 방식으로 NPC_R01_001~011을 적용했다. 총 11명·22개 기본 재질 슬롯이다.
모든 NPC가 얼굴 아틀라스와 두 번째 아틀라스 두 슬롯을 사용한다. NPC 001의 실제 머리카락은 두 번째 아틀라스에 포함된다.

## 재질

- 경로: `/Game/IsekaiClass/NPC/Materials/WorldReactive`.
- 얼굴·눈 아틀라스: `Shared/M_ICNPCFeatureLit`. 원본 diffuse RGB × 원본 tint × 0.65를 Base Color에,
  원본 알파 × tint 알파를 Opacity Mask에 연결한다. Masked, Two Sided, Roughness 1, Specular/Metallic/Emissive 0이다.
- 몸 아틀라스: `Shared/M_ICNPCBodyLit`. 플레이어 최종 몸 부모를 복제한 출력이며 Emissive 0, ICAlbedoScale 0.65다.
  MToon 추가 조명·역톤매핑·노출 보상·MatCap·Rim·CustomReflection은 끄고 MRSA `(0,0.85,0.03,1)`을 사용한다.
- 두 공통 부모는 플레이어의 최종 부모를 NPC 전용으로 복제했다. 원본 NPC 재질과 공유 VRM4U 재질은 보존했다.
- 각 `SK_R01_1~11`의 두 기본 슬롯을 NPC별 새 인스턴스에 연결했다. Blueprint 기본 재질 덮어쓰기는 없었으며,
  새 실행에서도 런타임 덮어쓰기 없이 기본 슬롯이 적용되는지 확인했다.

## 보존·검증

월드·엔진 설정·낮밤 Blueprint·플레이어·원본 재질/부모 파일은 SHA-256으로 보존을 확인했다.
NPC 메시의 스켈레톤·물리·post process 애니메이션 연결과 슬롯 이름/순서를 유지했다.
NPC 스킬·스케줄·애니메이션 에셋은 변경하지 않았다.

저장 후 새 에디터에서 실제 NPC Blueprint 11명을 사용했다. 각 NPC의 밤 외부·밤 내부·낮 외부·낮 내부와 낮 외부 눈 근접을
합쳐 55장(1600×900)을 촬영했다. AI 이동과 위치 고정은 검증 PIE에만 적용했다.
검증 에디터에서 GPU 장치 리셋이 발생하여 에셋 저장을 NullRHI로 분리했고, 이후 새 렌더링 에디터에서 최종 확인했다.

- 적용 코드: `Tools/Characters/setup_npc_reactive_lighting.py` (`-ICApplyNPCReactiveLighting` 필요).
- 검증 코드: `Tools/Characters/review_npc_reactive_lighting.py`.
- 적용·검증: `Saved/CharacterLighting/NPCWorldReactive/Applied.json`, `Final/Verification.json`.
- 캡처 보고서: `Saved/CharacterLighting/NPCWorldReactive/Comparison.html`.
- 직전 메시 백업: `{applied['backup']}`.

복원 시 에디터를 종료하고 백업의 SK_R01_1~11.uasset을 각각 원래 NPC_R01_001~011 폴더에 복사한다.
머리 아틀라스는 눈 외 영역도 함께 조명을 받으며, 실내·밤에는 기존 주변광이 남아 완전히 검게 되지 않는다.
'''
if hair_revision.is_file():
    doc+='''\n## NPC 001 현재 구성\n\n위 55장은 최초 일괄 적용 기록이다. 후속 아틀라스 확인으로 NPC 001 얼굴·눈은 첫 슬롯, 실제 머리카락은\n두 번째 몸 슬롯에 들어 있음을 확인했다. 얼굴·눈 산란 실험은 철회하고 이전 MI 파일을 그대로 복원했다.\n현재는 두 번째 슬롯의 머리카락 UV 두 영역만 별도 산란을 사용하며 옷·피부는 Default Lit이다.\n상세는 `NPC001_HairOnly.ko.md`, 최신 캡처는 `Saved/CharacterLighting/NPCWorldReactive/NPC001HairOnly/Comparison.html`을 참고한다.\n'''
elif soft_revision.is_file():
    doc+='''\n## NPC 001 추가 수정\n\n위 내용과 55장은 최초 11명 일괄 적용 시점의 기록이다. 이후 NPC 001 머리만 전용 Subsurface 부모로 변경해\n햇빛 아래 눈 주변 음영을 완화했다. 다른 10명은 위 설정을 유지한다. 현재 구성은 `NPC001_SoftHead.ko.md`,\n최신 비교는 `Saved/CharacterLighting/NPCWorldReactive/NPC001SoftHead/Comparison.html`을 참고한다.\n'''
if (OUT/'NPC003HairOnly/Comparison.html').is_file():
    doc+='''\n## NPC 003 추가 수정\n\nNPC_R01_003은 자체 아틀라스의 머리카락 UV 두 영역에만 약한 산란을 적용했다. 눈·얼굴 재질과 다른 캐릭터는 그대로다.\n현재 구성은 `NPC003_HairOnly.ko.md`, 비교 캡처는 `Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/Comparison.html`을 참고한다.\n'''
(ROOT/'Docs/Characters/NPC_WorldLighting.ko.md').write_text(doc,encoding='utf-8')
print('Built selectable NPC comparison, 55-photo gallery, three visual-review overviews, and implementation guide.')
