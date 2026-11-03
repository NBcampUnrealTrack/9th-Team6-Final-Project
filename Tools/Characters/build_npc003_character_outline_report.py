"""Show verified native before/after captures and the persisted implementation."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPC003CharacterOutline_20261103'
ENV=[('Night_Outside','밤 외부 · 20시'),('Night_Inside','밤 내부 · 20시'),('Day_Outside','낮 외부 · 10시'),('Day_Inside','낮 내부 · 10시')]

def build():
    preview=json.loads((OUT/'PreviewVerification.json').read_text(encoding='utf-8'));assert preview['complete']
    visual=json.loads((OUT/'WorldVisualCheck.json').read_text(encoding='utf-8'));assert visual['passes_static_world_check']
    applied=(OUT/'AppliedVerification.json').exists() and json.loads((OUT/'AppliedVerification.json').read_text(encoding='utf-8')).get('complete',False)
    folder='AppliedCaptures' if applied else 'PreviewCaptures'
    css='body{background:#13171d;color:#e7eef8;padding:24px;font:16px/1.6 system-ui}main{max-width:1700px;margin:auto}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0}img{width:100%;height:auto;display:block}figcaption{padding:6px 0}a{color:#a6ccff}'
    text='<h1>NPC_R01_003 · 7번 기반 전신 후처리 선 보정</h1>'
    text+='<p>'+('기본 재질·블루프린트·월드 연결을 저장한 뒤 새 에디터에서 확인한 적용본.' if applied else '현재 기본값을 유지한 격리 시험본. 저장 적용 전 검증.')+'</p>'
    if (OUT/'PendingApproval.json').exists() and not applied:text+='<p>현재 상태: 자동 승인 검토에서 NPC003 및 공용 월드 후처리 연결의 영구 저장을 거절했다. 명시적 저장 승인 대기 중이며 기본값 저장은 실행하지 않았다.</p>'
    text+='<p>머리·피부·옷·얼굴 전체에서 추가 후처리 선을 제외한다. 원래 텍스처의 속눈썹·눈동자·무늬와 실제 조명·그림자는 유지한다. 별도 머리 Roughness 표식은 제거했다. Kawaii Toon Shading 플러그인과 시험 재질 19개는 제거했다.</p>'
    def figure(path,label):return '<figure><a href="'+path+'"><img src="'+path+'"></a><figcaption>'+label+'</figcaption></figure>'
    for env,label in ENV:
        text+='<h2>'+label+'</h2><div class="grid">'
        for view,title in [('Full','전신'),('Closeup','근접')]:
            text+=figure('PreviewCaptures/15_hair_boundary_filter_'+env+'_'+view+'.png','기존 머리만 보정 · '+title)
            text+=figure(folder+'/Character_Whole_'+env+'_'+view+'.png','전신 선 보정 · '+title)
        text+='</div>'
    text+='<h2>배경만 대조</h2><p>캐릭터를 숨기고 같은 실내 좌표·시야·시간에서 원본 후처리와 새 후처리를 비교했다. 정적 벽 영역의 RGB 평균 절대 차이: '+str(round(visual['mean_absolute_rgb_difference_0_to_255'],6))+'/255.</p><div class="grid">'
    text+=figure('PreviewCaptures/World_Original.png','원본 월드 후처리')+figure('PreviewCaptures/World_Selected.png','캐릭터 전용 예외를 추가한 후처리')+'</div>'
    (OUT/'Comparison.html').write_text('<!doctype html><html lang="ko"><meta charset="utf-8"><title>NPC003 전신 선 보정</title><style>'+css+'</style><main>'+text+'</main></html>',encoding='utf-8')
    md='''# NPC 003 — 7번 기반 전신 윤곽선 보정

- 적용 대상: NPC_R01_003의 머리·얼굴·피부·옷 전체.
- 머리 기본 재질: 연구 07번의 원본 RGB × 0.65, Default Lit. 자체 발광 0.
- 피부·옷·눈 원본 텍스처와 실제 조명·그림자는 유지한다.
- 캐릭터 전체를 Stencil 249로 구분하므로, 이전 머리 전용 Roughness 0.9375 표식은 제거한다.
- 원본 월드 후처리는 복사해 사용하고, 선을 혼합하는 Alpha만 해당 캐릭터에서 0으로 만든다. 1.5px 주변 픽셀까지 검사한다.
- 다른 픽셀의 후처리 색감·노출·안개·윤곽선 계산은 유지한다.
- 원본 재질 및 공용 VRM4U 셰이더는 수정하지 않는다.
- Kawaii Toon Shading 원본·빌드 폴더 및 시험 재질 19개 제거. 과거 비교 사진만 보존한다.

## 에디터 연결

1. NPC_R01_003의 WorldReactive Body MI → SelectedRawRGB 부모 체인.
2. BP_NPC_R01_003 → Mesh → Rendering → Render CustomDepth Pass, CustomDepth Stencil Value 249.
3. Project Settings → Rendering → Postprocessing → Custom Depth-Stencil Pass: Enabled with Stencil (`r.CustomDepth=3`).
4. L_OpenWorld → PostProcessVolume_1 → Post Process Materials: 기존 MI 대신 M_ICCharacterWorldPostProcess의 전용 MI 연결.
5. `ICCharacterOutlineStrength=0`: 후처리 선 제외. 1로 올리면 원래 후처리 선을 유지한다.

## 검증

- 격리 PIE 전후 16장: 머리만 / 전신 × 네 환경 × 전신·근접.
- 캐릭터를 숨긴 배경 대조 원본 2장 및 정적 벽의 RGB 픽셀 비교.
- 저장 전 대상 파일 4개 백업, 다른 캐릭터·원본 에셋 해시 보존.
- 저장 시 조명·노출·후처리의 나머지 속성이 동일한지 비교.
- 새 에디터에서 저장된 기본값을 사용한 실제 적용 캡처를 별도로 확인.

'''
    md+='현재 저장 적용 및 새 에디터 검증 완료: '+str(applied)+'\n\n배경 정적 영역 평균 RGB 차이: '+str(visual['mean_absolute_rgb_difference_0_to_255'])+' / 255\n'
    (OUT/'Implementation.md').write_text(md,encoding='utf-8')
    print(str(OUT/'Comparison.html'))

if __name__=='__main__':build()
