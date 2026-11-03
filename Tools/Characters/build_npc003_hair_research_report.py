"""Eight-method, four-environment native research gallery; no photo modifications."""
import html,json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/Research_20261103'
methods=json.loads((OUT/'Methods.json').read_text(encoding='utf-8'))
v=json.loads((OUT/'Verification.json').read_text(encoding='utf-8'))
assert v['complete'] and len(v['samples'])==64 and v['permanent_selection_applied']==False
ENV=[('Night_Outside','밤 외부 · 20:00'),('Night_Inside','밤 내부 · 20:00'),('Day_Outside','낮 외부 · 10:00'),('Day_Inside','낮 내부 · 10:00')]
OBSERVATIONS={
    '01_current':'기준 화면. 밤·실내에서 머릿결을 따라 가는 어두운 선이 남는다.',
    '02_geometry_normal':'밤·실내에서 어두운 선이 크게 줄었다. 낮 외부에서는 머리의 밝은 면이 넓어지고 머릿결 입체감이 약해졌다.',
    '03_backface_normal':'현재 정면 사선 시점에서는 기준과 차이가 작다. 다른 시점의 뒷면 전체를 검증한 결과는 아니다.',
    '04_wide_scatter':'낮에는 빛 퍼짐이 늘지만 밤·실내의 가는 선은 크게 줄지 않는다.',
    '05_thin_surface':'얇은 면용 빛 전달로 바꿔도 현재 밤·실내의 가는 선은 대부분 남는다.',
    '06_preintegrated':'현재 조명과 수치에서는 기준과 차이가 작고, 밤·실내의 가는 선이 남는다.',
    '07_raw_diffuse':'밤·실내의 어두운 선이 줄었다. 낮 외부에는 넓은 입체 그림자가 남으며 표면 하이라이트 인상이 달라진다.',
    '08_no_cast_shadow':'낮에는 얼굴·목과 바닥의 캐릭터 메시 그림자가 달라지지만 머리카락의 가는 선이 남는다. 장비 등 별도 메시 그림자는 포함하지 않는다.',
}
for environment,_ in ENV:
    for close in [False,True]:
        samples=[s for s in v['samples'] if s['environment']==environment and s['closeup']==close]
        assert len(samples)==8
        for sample in samples:
            assert sample['camera']==samples[0]['camera'] and sample['field_of_view']==samples[0]['field_of_view']
            assert sample['position']==samples[0]['position'] and sample['lighting']==samples[0]['lighting']
            assert struct.unpack('>II',(OUT/'Captures'/sample['image']).read_bytes()[16:24])==(1600,900)
STYLE='''*{box-sizing:border-box}body{margin:0;background:#141920;color:#e8eef7;font:16px/1.65 "Segoe UI","Malgun Gothic",sans-serif}main{max-width:1700px;margin:auto;padding:24px}h1{font-size:28px;line-height:1.4;margin:6px 0 14px}h2{font-size:23px;margin:26px 0 12px}p{margin:10px 0}a{color:#a8ceff}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0;min-width:0}img{display:block;width:100%;height:auto}figcaption{padding:7px 0;line-height:1.5}.note{padding:15px;background:#273547;border-left:4px solid #a8ceff}button,select{font:inherit;color:inherit;background:#273342;border:1px solid #66798f;border-radius:6px;padding:7px 10px}button{cursor:pointer}button[aria-pressed=true]{background:#285d89;border-color:#b0d5ff}.controls{display:flex;gap:9px;flex-wrap:wrap;align-items:center;margin:14px 0}.methods{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin:20px 0}table{border-collapse:collapse;width:100%}th,td{border:1px solid #536278;padding:9px 12px;text-align:left}.overview h1{margin-bottom:10px}.overview .grid{gap:14px}.overview figcaption{font-size:19px;font-weight:600;padding:5px 0 8px}.overview main{padding:20px 24px}.tag{display:inline-block;color:#ffdb9a}code{overflow-wrap:anywhere}@media(max-width:760px){.grid{grid-template-columns:1fr}.methods{grid-template-columns:1fr 1fr}main{padding:14px}}'''
def document(title,body,script='',cls=''):
    return f'<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{title}</title><style>{STYLE}</style></head><body class="{cls}"><main>{body}</main>{script}</body></html>'
def path(method,env,close):return 'Captures/'+method+'_'+env+('_Closeup' if close else '_Full')+'.png'
def figure(src,label,identity='',lazy=False):
    attributes=f' id="{identity}"' if identity else ''
    return f'<figure><a{attributes} href="{src}"><img src="{src}" width="1600" height="900"'+(' loading="lazy"' if lazy else '')+f' alt="{html.escape(label,quote=True)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'
def summary_table():
    text='<table><thead><tr><th>번호</th><th>비교 방식</th><th>바꾼 점</th><th>캡처 관찰</th></tr></thead><tbody>'
    for method in methods:text+=f'<tr><td>{method["id"][:2]}</td><td>{html.escape(method["title"])}</td><td>{html.escape(method["note"])}</td><td>{html.escape(OBSERVATIONS[method["id"]])}</td></tr>'
    return text+'</tbody></table>'

body='<h1>NPC_R01_003 — 머리카락 음영 8방식 비교</h1>'
body+='<p class="note">현재 상태 01번을 기준으로 8방식 × 밤·낮 실내외 × 전신·근접, 총 64장을 실제 게임 화면에서 촬영했다. 사진에 후처리를 하지 않았다. 기존 에셋은 보존했으며 새 방식은 게임 기본 재질에 적용하지 않았다. <strong>08번은 캐릭터 전체 그림자를 끄는 원인 분리용 비교다.</strong></p>'
body+='<p>각 방법의 번호는 버튼·표·파일명에 동일하게 표시한다. 눈·얼굴 재질은 모두 같다. 08번은 얼굴에 드리워지는 자기 그림자도 달라진다.</p>'
body+='<div class="methods">'
for method in methods:body+=f'<button data-method="{method["id"]}" aria-pressed="'+('true' if method['id']=='02_geometry_normal' else 'false')+'">'+method['id'][:2]+' · '+html.escape(method['title'])+'</button>'
body+='</div><div class="controls"><label>장면 <select id="environment">'
for key,label in ENV:body+=f'<option value="{key}"'+(' selected' if key=='Night_Inside' else '')+'>'+label+'</option>'
body+='</select></label><label>화면 <select id="view"><option value="Closeup">근접</option><option value="Full">전신</option></select></label></div>'
body+='<h2 id="selected-label">02 · 머리카락 노멀맵 제거</h2><p id="selected-note">머리카락만 기본 메시 노멀 사용. 겹친 메시 자체의 그림자는 유지.</p><div class="grid">'
body+=figure(path('01_current','Night_Inside',True),'01 · 현재 상태','baseline')
body+=figure(path('02_geometry_normal','Night_Inside',True),'02 · 머리카락 노멀맵 제거','candidate')+'</div>'
body+='<h2>선택한 방식 · 네 조건</h2><div class="grid">'
for key,label in ENV:body+=figure(path('02_geometry_normal',key,True),label,'env_'+key)
body+='</div><h2>8방식을 한 화면에서 비교</h2><p>'
for key,label in ENV:body+=f'<a href="Overview_{key}.html">{label} 근접</a> · <a href="Full_{key}.html">전신</a><br>'
body+='</p><p><a href="All.html">64장 모두 펼쳐 보기</a> · <a href="Research.md">연구 방법과 정리</a> · <a href="Verification.json">촬영·보존 검증 기록</a></p>'
body+='<h2>방법별 설정</h2>'+summary_table()
body+='<p>02번은 머리카락 Normal 출력을 (0,0,1)로 바꿔 기본 메시 노멀을 사용한다. 노멀맵뿐 아니라 기존 재질의 추가 Normal 계산도 건너뛴다. 07번은 색·노멀·표면 계산을 함께 단순화한 별도 방향이다. 각 요소를 하나씩 분리한 비교와 복합 대안을 함께 포함했다.</p>'
body+='<p>현재 게임에 적용된 0.25/0.65 머리카락 보정은 그대로 유지된다. 모든 후보는 별도 연구 폴더에 있고, 촬영 PIE 종료 시 임시 재질과 그림자 설정을 복원했다.</p>'
body+='<p class="note">이번 비교에서는 02번과 07번이 검은 선을 줄였다. 기존 Normal 출력의 영향이 크다는 근거지만, 02번이 노멀맵과 추가 계산을 함께 우회하므로 어느 세부 노드가 원인인지는 아직 분리하지 않았다. 낮의 머릿결·그림자 차이까지 보고 선택할 수 있다.</p>'
script='<script>const methods='+json.dumps(methods,ensure_ascii=False)+''';let selected='02_geometry_normal';const src=(id,env,view)=>'Captures/'+id+'_'+env+'_'+view+'.png';function refresh(){const env=document.getElementById('environment').value,view=document.getElementById('view').value,m=methods.find(x=>x.id===selected);document.getElementById('selected-label').textContent=m.id.slice(0,2)+' · '+m.title;document.getElementById('selected-note').textContent=m.note;for(const [key,id]of [['baseline','01_current'],['candidate',selected]]){const link=document.getElementById(key);link.href=src(id,env,view);link.querySelector('img').src=link.href;link.parentElement.querySelector('figcaption').textContent=methods.find(x=>x.id===id).id.slice(0,2)+' · '+methods.find(x=>x.id===id).title;}for(const envKey of ['Night_Outside','Night_Inside','Day_Outside','Day_Inside']){const link=document.getElementById('env_'+envKey);link.href=src(selected,envKey,view);link.querySelector('img').src=link.href;}for(const b of document.querySelectorAll('[data-method]'))b.setAttribute('aria-pressed',String(b.dataset.method===selected));}for(const b of document.querySelectorAll('[data-method]'))b.addEventListener('click',()=>{selected=b.dataset.method;refresh();});for(const id of ['environment','view'])document.getElementById(id).addEventListener('change',refresh);</script>'''
(OUT/'Comparison.html').write_text(document('NPC003 머리 음영 비교 01~08',body,script),encoding='utf-8')

for key,label in ENV:
    for closeup in [True,False]:
        title=label+' · '+('근접' if closeup else '전신')+' · 방법 01~08'
        overview='<h1>'+title+'</h1><p>왼쪽 위부터 01→08 순서. 08번은 캐릭터 전체 그림자를 끄는 진단용이다.</p><div class="grid">'
        for method in methods:overview+=figure(path(method['id'],key,closeup),method['id'][:2]+' · '+method['title'])
        overview+='</div>'
        (OUT/(('Overview_' if closeup else 'Full_')+key+'.html')).write_text(document(title,overview,cls='overview'),encoding='utf-8')
all_body='<h1>8방식 · 원본 64장</h1><p><a href="Comparison.html">비교 도구로 돌아가기</a></p>'
for method in methods:
    all_body+='<h2>'+method['id'][:2]+' · '+method['title']+'</h2><p>'+method['note']+'</p><div class="grid">'
    for closeup in [False,True]:
        for key,label in ENV:all_body+=figure(path(method['id'],key,closeup),label+(' · 근접' if closeup else ' · 전신'),lazy=True)
    all_body+='</div>'
(OUT/'All.html').write_text(document('NPC003 8방식 원본 64장',all_body),encoding='utf-8')

md='''# NPC_R01_003 머리 음영 비교 연구 — 2026-11-03

목표는 눈·얼굴과 월드를 유지하면서, 그림자와 어두운 장면에서 머리카락에 생기는 검은 선·음영을 비교하는 것이다.
원본 아틀라스를 직접 확인한 NPC 003의 머리카락 UV 두 영역에만 재질 출력을 선택했다.

## 비교 조건

- 현재 상태 포함 8방식. 각 방식에 밤 외부·밤 내부(20시), 낮 외부·낮 내부(10시).
- 각 조건 전신·근접 1장씩: 총 64장, 1600×900 원본. 이미지 후처리 없음.
- 같은 장면 안에서 8방식을 연속 촬영해 좌표·카메라·시야각·게임 시간·조명 설정 일치를 검증했다.
- 얼굴·눈 재질은 동일. 08번은 원인 분리용으로 캐릭터 전체 그림자를 꺼 얼굴의 자기 그림자와 바닥 그림자도 달라진다.
- 모든 방식 자체 발광 0. 원본 텍스처/알파/표면색 배율 0.65 유지. 07번은 원본 RGB를 별도 단순 표면에 사용한다.
- 연구 전후 기존 NPC·플레이어·월드·설정 파일 해시 보존. 연구 에셋은 기존 기본 슬롯에 할당하지 않았다.

## 8방식

| 번호 | 방법 | 변경점 | 캡처 관찰 |
|---|---|---|---|
'''
for m in methods:md+='| '+m['id'][:2]+' | '+m['title']+' | '+m['note']+' | '+OBSERVATIONS[m['id']]+' |\n'
md+='''
02번은 기존 Normal 출력을 기본 탄젠트 노멀 (0,0,1)로 바꾼다. 노멀맵과 추가 Normal 계산을 함께 우회한다.
03번은 TwoSidedSign을 곱해 뒷면의 노멀 반전을 보상한다.
07번은 색·노멀·거칠기·반사·AO 입력을 머리카락에만 단순화한다. 그러므로 02번과 비교해 추가로 달라지는 부분도 함께 봐야 한다.
08번은 최종 추천안이 아닌 진단용이며 머리카락만 그림자를 끄는 기능은 아니다.

## 관찰과 추론

02와 07에서 밤·실내의 머리카락 검은 선이 크게 줄었다. 02는 기존 Normal 입력을 대체한 비교이므로 기존 Normal 출력이
선을 강조한다는 근거다. 노멀맵과 추가 Normal 계산 중 어느 세부 노드의 영향인지는 이 비교만으로 확정하지 않는다.
02는 낮에 밝은 면이 넓어지고, 07은 낮에 넓은 입체 그림자를 유지한다. 취향과 의도에 맞는 선택이 필요하다.
08은 캐릭터 메시의 그림자만 끈다. 별도 무기·장비 메시의 그림자는 남을 수 있다.

## 참고 자료

- [Epic: Shading Models](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine): Default Lit, Subsurface, Two Sided Foliage, Preintegrated Skin의 빛 반응 차이.
- [Epic: Using Subsurface Scattering](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-subsurface-scattering-in-unreal-engine-materials): 산란 색과 Opacity로 빛 퍼짐 제어.
- [Epic: From Material Expression](https://dev.epicgames.com/documentation/en-us/unreal-engine/from-material-expression-shading-model-in-unreal-engine): UV 마스크로 픽셀별 조명 모델 선택.
- [Epic: Material Inputs](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-inputs-in-unreal-engine): Normal과 표면색·거칠기·반사의 역할.
- 설치된 UE 5.8의 `Engine/Shaders/Private/ShadingModels.ush`와 현재 VRM4U 부모 재질·상속 파라미터도 확인했다.

## 재현·파일

- 생성: `Tools/Characters/research_npc003_hair_setup.py -ICNPC003HairResearch` (PIE 밖, NullRHI).
- 촬영: `Tools/Characters/review_npc003_hair_research.py` (별도 리뷰 에디터).
- 연구 에셋: `/Game/IsekaiClass/NPC/Materials/LightingResearch/NPC_R01_003`.
- 사진·비교 도구: 이 문서와 같은 폴더의 `Comparison.html`, `All.html`, `Captures`.
- 검증: `Verification.json`, 보호 파일 해시: `Scope.json`.

사용자 선택 전까지 현재 게임의 NPC 003 기본 재질을 유지한다.
'''
(OUT/'Research.md').write_text(md,encoding='utf-8')
for report in OUT.glob('*.html'):
    for link in re.findall(r'(?:src|href)="([^"]+)"',report.read_text(encoding='utf-8')):assert (report.parent/link).is_file(),str(report)+' '+link
print('Built interactive eight-method comparison, 64-photo gallery, eight overviews, and research Markdown.')
