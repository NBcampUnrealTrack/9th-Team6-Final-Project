"""Review the native captures of the isolated hair outline exclusion trial."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/EdgeStudy_20261103'
ENV=[('Night_Outside','밤 외부 · 20시'),('Night_Inside','밤 내부 · 20시'),('Day_Outside','낮 외부 · 10시'),('Day_Inside','낮 내부 · 10시')]

def build():
    status=json.loads((OUT/'FinalVerification.json').read_text(encoding='utf-8'))
    assert status['complete'] and len(status['samples'])==16,status.get('error')
    assert status['existing_assets_unchanged']
    for sample in status['samples']:assert (OUT/'FinalCaptures'/sample['image']).exists()
    css='body{background:#13171e;color:#e4ecf7;padding:24px;font:16px/1.6 system-ui}main{max-width:1700px;margin:auto}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0}img{width:100%;display:block}figcaption{padding:8px 0}a{color:#9bcaff}'
    body='<h1>NPC_R01_003 · 머리 검은 선 원인 분리</h1><p>왼쪽: 7번 원본 RGB 단순 표면. 오른쪽: 머리와 인접 경계에서 후처리 윤곽선만 제외한 시험본. 네 조건에서 짙은 머리 경계선이 줄어든 것을 확인했다. 눈·피부의 원래 선, 조명·노출·월드 에셋은 유지했다. 현재 게임의 기본 슬롯에는 적용하지 않은 비교용이다.</p>'
    for env,label in ENV:
        body+='<h2>'+label+'</h2><div class="grid">'
        for view in ['Full','Closeup']:
            for method,title in [('07_raw_diffuse','7번 기준'),('15_hair_boundary_filter','머리 윤곽선만 제외')]:
                name=method+'_'+env+'_'+view+'.png'
                body+='<figure><a href="FinalCaptures/'+name+'"><img src="FinalCaptures/'+name+'"></a><figcaption>'+title+' · '+('전신' if view=='Full' else '근접')+'</figcaption></figure>'
        body+='</div>'
    body+='<p>모든 사진은 1600×900 원본 캡처. 이미지 후처리 없음.</p>'
    (OUT/'FinalComparison.html').write_text('<!doctype html><html lang="ko"><meta charset="utf-8"><title>NPC003 머리 검은 선 비교</title><style>'+css+'</style><main>'+body+'</main></html>',encoding='utf-8')
    report='''# NPC 003 머리 검은 선 — 원인 분리 및 머리 전용 보정 시험

기존 2번(추가 Normal 계산 우회)과 7번(원본 RGB의 단순 표면)에서도 남은 선을 조사했다.

## 원인 조사

- 원본 아틀라스, 투명 가장자리 색 보정, 알파 임계값, 더 선명한 mip, UV 마스크 확대를 비교했다.
- Lumen ShortRangeAO, ScreenTraces, SSAO, DistanceFieldAO, ContactShadows 각각 끄는 진단도 별도 캡처했다. 이는 원인 확인용이며 최종 월드 설정으로 저장하지 않았다.
- NPC에 별도 윤곽선 메시와 overlay material은 없었다.
- 월드 `PostProcessVolume_1 → Settings → Post Process Materials`에 `MI_postprocess_Inst`가 들어 있다.
- 그 부모 `M_postprocess`에는 WorldNormal의 주변 픽셀 차이로 선을 만들고 `edge_color`로 혼합하는 경로가 있다. `edge_mult=1`, `edge_power=1`이다. SceneDepth도 그래프에서 사용되지만, 검은 선의 활성 입력은 노멀 차이 계산부터 분리해 조사했다.
- 따라서 표면 재질의 노멀 계산을 바꿔도, 실제 메시 면 경계와 깊이 차이가 있으면 후처리에서 선을 다시 만들 수 있다.

## 머리만 제외하는 시험 구현

- 기존 월드 후처리 원본을 복사한 `M_HairOutlineFilter`와 MI를 만들었다. 레벨 저장 및 원본 수정 없음.
- 7번 재질의 머리 UV 픽셀만 Roughness 0.9375 표식을 사용했다. 머리의 Specular 0은 유지한다. 이는 시험용 픽셀 구분 수단이다.
- NPC 003의 현재 보이는 픽셀만 CustomDepth/Stencil 249 및 SceneDepth 검사로 제한했다.
- 프로젝트의 실제 `r.CustomDepth` 값은 1이었다. 이는 깊이만 활성화하고 스텐실은 쓰지 않는 상태다. 초기 필터 시험에서 이를 놓쳐 표식 조건이 항상 실패했다.
- 검증된 최종 시험에서는 `r.CustomDepth=3`을 해당 PIE 세션에서만 사용한다. 종료 시 원래 값 1로 복원하며 설정 파일은 수정하지 않는다.
- `Stencil == 249 && CustomDepth <= SceneDepth + 1 && abs(Roughness - 0.9375) < 0.004`인 픽셀에서만 윤곽선의 Alpha를 0으로 만든다.
- 15번은 머리 및 그 주변 1.5px의 3×3 이웃까지 표식을 확인한다. 머리와 얼굴이 접하는 선의 반대쪽 픽셀에도 적용하기 위해서다. 눈·얼굴·피부·옷과 월드의 나머지 윤곽선 및 후처리 계산은 유지한다. 머리 재질의 실제 조명 반응 및 바닥 그림자도 유지한다.
- 임시 PIE 인스턴스에서만 후처리와 표식을 할당했다. 종료 시 원래 후처리·CustomDepth 설정·재질 할당 복원, 기존 파일 해시 보존을 확인한다.
- Kawaii Toon Shading 비교에는 이 시험 보정을 섞지 않았다.

## 검증 자료

- `RenderingAudit.json`: 실제 NPC 컴포넌트, 오버레이 및 월드 후처리 확인.
- `PostProcessAudit.json`: 후처리 그래프 노드 및 파라미터.
- `Verification.json`: 10방향 20장 진단.
- `OutlineVerification.json`: 별도 후처리·AO 진단 12장.
- `FinalVerification.json`: 머리 전용 보정 전후, 네 환경 × 전신·근접 = 16장.
- `FinalComparison.html`: 전후 원본 사진.

## 결과와 한계

후처리의 `edge_mult`를 0으로 한 진단에서 검은 선이 사라져 후처리의 기여를 확인했다. 초기 머리 전용 시험은 스텐실 버퍼가 꺼진 상태여서 효과가 없었다. 스텐실을 활성화한 최종 15번에서는 밤 외부·밤 내부·낮 외부·낮 내부 모두 머리 경계의 짙은 선이 줄어들었다. 눈·코·입의 원래 선과 피부·옷의 나머지 후처리는 유지된다.

실제 메시 겹침으로 생긴 명암, 머리 텍스처의 원래 결, 남은 작은 점까지 모두 지우는 기능은 아니다. 이 결과는 새로운 시점이나 이동 장면까지 검증한 최종 양산 설정이 아니라, 원인을 확인하고 머리에만 적용 범위를 제한한 비교용 시험본이다. 기존 기본 슬롯에는 아직 저장하지 않았다. Kawaii 비교와도 별도이다.
'''
    (OUT/'Research.md').write_text(report,encoding='utf-8')
    print(str(OUT/'FinalComparison.html'))

if __name__=='__main__':build()
