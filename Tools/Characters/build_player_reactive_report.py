"""Build the local screenshot comparison and TIL from completed native verification."""
import html
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Saved/CharacterLighting/WorldReactive'
VERIFY = json.loads((OUT / 'Final/Verification.json').read_text(encoding='utf-8'))
assert VERIFY['complete'] and len(VERIFY['samples']) == 8
LABELS = [('Night_Outside', '밤 외부 · 20:00'), ('Night_Inside', '밤 내부 · 20:00'),
          ('Day_Outside', '낮 외부 · 10:00'), ('Day_Inside', '낮 내부 · 10:00')]
STYLE = '''body{margin:0;background:#151920;color:#e7edf5;font:17px/1.7 system-ui,sans-serif}
main{max-width:1500px;margin:auto;padding:28px}a{color:#a8cdff}h1{font-size:29px}h2{margin-top:36px;font-size:24px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0}img{display:block;width:100%;height:auto}
figcaption{padding:7px 0}.note{padding:16px;background:#263649;border-left:4px solid #a8cdff}
code,pre{background:#222e3c}code{padding:2px 5px}pre{padding:16px;overflow:auto}table{border-collapse:collapse}
td,th{border:1px solid #647084;padding:8px 15px}article{max-width:1000px;margin:auto}article .grid{max-width:none}
@media(max-width:760px){.grid{grid-template-columns:1fr}main{padding:14px}}'''


def document(title, content):
    return '<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>' + title + '</title><style>' + STYLE + '</style></head><body><main>' + content + '</main></body></html>'


def figure(path, label):
    safe = html.escape(path, quote=True)
    return f'<figure><a href="{safe}"><img src="{safe}" width="1600" height="900" loading="lazy" alt="{html.escape(label, quote=True)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'


content = '''<h1>플레이어가 실제 조명과 그림자를 받도록 수정</h1>
<p>2026-11-03 · 새 에디터에서 저장된 재질로 실행 · 남녀 프리셋 · 실제 게임 창 1600×900 원본</p>
<p class="note">밤 외부 → 밤 내부 → 낮 외부 → 낮 내부 순서다. 원래 툰 색을 Base Color로 연결하고 Emissive를 0으로 바꿨다.
이번 작업에서 현재 월드의 단일 태양광·SkyLight·노출·후처리 설정은 변경하지 않았다.</p>
<p><a href="TIL.html">문제 진단과 구현 TIL</a> · <a href="Applied.json">적용 내역</a> · <a href="Final/Verification.json">최종 실행 검증</a></p>'''
for sex, name in [('F', '여성'), ('M', '남성')]:
    content += f'<h2>{name} 프리셋 — 최종 저장 상태</h2><div class="grid">'
    content += ''.join(figure(f'Final/{sex}_{key}.png', label) for key, label in LABELS) + '</div>'
content += '''<h2>실내외 문제 — 수정 전과 저장 후 비교</h2><p>왼쪽은 직전 8번 MToon Lit, 오른쪽은 이번 재질이다.
동일 위치·시간·카메라로 촬영했다. 수정 전 화면은 Preview/Current 캡처이며, 오른쪽은 새 에디터에서 기본 재질로 실행한 최종 캡처다.</p>'''
for key, label in LABELS:
    content += f'<h3>{label}</h3><div class="grid">' + figure(f'Preview/Current_{key}.png', '수정 전 · ' + label) + figure(f'Final/F_{key}.png', '수정 후 · ' + label) + '</div>'
content += '''<h2>확인 범위와 남은 미술 조정</h2><p>이 집의 지붕 아래에서는 낮의 태양 방향 레이가 막히고, 바깥 위치에서는 막히지 않는다.
최종 재질은 자체 발광 없이 실제 빛을 받는다. 실내를 완전히 검게 만들지는 않는다. 기존 SkyLight와 간접광이 남아 있기 때문이다.
밤 외부에서도 기존 주변광에 의해 캐릭터가 보인다.</p>
<p>원래 텍스처와 얼굴용 노멀 보정은 유지했지만, 실제 그림자로 앞머리 아래 얼굴의 음영이 달라진다.
남성은 원래 두 아틀라스 슬롯을 유지한다. 피부·눈만 따로 조절하는 새 UV 마스크는 만들지 않았다.
다른 건물·실내 점광원·모든 표정과 카메라 각도는 이번 4조건 캡처로 검증한 범위에 포함하지 않는다.</p>'''
(OUT / 'Comparison.html').write_text(document('플레이어 실내외 조명 — 최종 4조건 비교', content), encoding='utf-8')

til = '''<article><h1>VRoid 캐릭터가 집 안에서도 밝았던 문제를 분리해서 확인하기</h1>
<p>2026-11-03 · Unreal Engine 5.8 · VRM4U · 플레이어 전용 재질 수정</p>
<p>목표는 밝은 곳에서는 밝고, 지붕 아래와 밤에는 주변 환경에 맞게 어두워지는 캐릭터였다.
원래 툰 텍스처와 얼굴을 최대한 유지하면서 배경 조명은 더 바꾸지 않는 조건이었다.</p>
<h2>8번 적용만으로 해결되지 않았던 부분</h2>
<p>앞선 8번 실험은 MToon Lit에 태양광 한 개만 남긴 설정이었다. 낮밤 차이는 보였지만 실제 집 안과 밖에서 캐릭터 밝기가 충분히 달라지지 않았다.
ColorScaleAllMat를 1에서 0.25로 낮추면 전체가 어두워졌으나, 그것만으로 지붕에 가려지는 조명 반응이 생기는 것은 아니었다.
그 보정은 이후 철회했고, 이번 진단은 원본 8번 색 배율 1에서 시작했다.</p>
<h2>월드 문제인지 재질 문제인지 분리</h2>
<p>같은 캐릭터·같은 카메라로 집 안과 밖을 촬영했다. 태양 방향 레이캐스트로 실내 지붕이 실제로 광원 방향을 막는지 확인했다.
이 집의 지붕은 그림자 설정이 켜져 있었고, 낮 실내 위치는 막힘, 외부 위치는 열림으로 나왔다.</p>
<p>다음으로 같은 스켈레탈 메시에 엔진 기본 Lit 재질을 임시 적용했다. 기본 Lit은 실내에서 어두워지고 외부에서 밝아졌다.
따라서 이 장소의 문제를 월드 광원 전체의 오류로 단정할 근거는 없었다. 플레이어 재질 출력 경로를 먼저 고치기로 했다.</p>
<p><a href="../IndoorOutdoorCurrent/Comparison.html">당시 실내외 및 기본 Lit 대조 실험</a></p>
<h2>원본 툰 색을 실제 표면색으로 사용</h2>
<p>원본 VRM 재질과 부모 계층을 플레이어 전용 폴더에 복제했다. 원본의 색·텍스처 계산 결과를 최종 Base Color에 연결하고,
Emissive Color에는 상수 0을 연결했다. 출력의 Shading Model은 Default Lit이다.
기존 셰이더가 Unlit용으로 만든 색을 표면색으로 재사용하되, 최종 조명과 그림자는 엔진이 처리하도록 구성했다.</p>
<pre>원본 재질 속성\n  Emissive의 색 계산 → × ICAlbedoScale(0.65) → 새 Base Color\n  불투명도·마스크·노멀 등 → 새 대응 속성\n  상수 0 → 새 Emissive Color\n최종 Shading Model = Default Lit</pre>
<p>원본 색 계산 안에서 조명을 다시 계산하지 않도록 bUseLight와 bUseLightForce를 껐다.
bUseFilmTonemapInverse와 bDivAdaptation도 꺼서 역톤매핑·노출 보상 계산을 색에 넣지 않았다.
MatCap·Rim·CustomReflection은 끄고, 얼굴의 카메라 방향 노멀 보정과 낮은 반짝임 설정을 유지했다.</p>
<p>ICAlbedoScale 0.65는 조명을 받는 표면색 배율이다. 화면 밝기 65%나 야간 전용 배율이 아니다.
시간이나 레이캐스트 결과에 따라 배율을 바꾸는 코드와 캐릭터 보조광은 사용하지 않는다.</p>
<h2>저장한 뒤 새 실행에서 확인</h2>
<p>남녀 메시의 기본 재질 슬롯 19개를 새 복제 재질로 저장했다. 검증용 에디터를 종료하고 새 에디터에서 다시 게임을 실행했다.
프리셋을 바꾼 뒤 실제 메시에 적용된 재질이 저장된 기본 슬롯과 같은지 확인했다.
런타임 재질 덮어쓰기와 밝기 컴포넌트가 없는지도 검사했다.</p>
<p>재질 그래프는 최종 Emissive 입력이 값 0의 Constant인지, Base Color가 색 배율 Multiply에 연결됐는지 검사했다.
밤 20시 외부·내부, 낮 10시 외부·내부를 남녀 각각 1600×900으로 촬영했다.
맵·DefaultEngine.ini·낮밤 Blueprint의 SHA-256이 적용 전후와 같은지 확인했다.</p>
<p><a href="Comparison.html">남녀 최종 캡처 8장 및 수정 전후 비교</a> · <a href="Final/Verification.json">실행 검증 기록</a></p>
<h2>관찰과 한계</h2>
<p>확인한 집 안에서는 낮의 직접광이 사라지고 캐릭터가 방의 간접광 색과 어두움에 맞게 변한다.
외부에서는 태양광에 노출된 옷과 다리가 밝아지고 그림자도 나타난다.
밤에는 현재 월드의 SkyLight와 간접광이 남아 있어 캐릭터가 완전히 검게 되지는 않는다.</p>
<p>툰 텍스처가 같더라도 실제 그림자를 받으면 얼굴 음영은 달라진다. 앞머리 그림자와 얼굴 가독성은 별도의 미술 조정 대상이다.
남성은 원래 두 아틀라스 재질을 유지하므로 피부·눈의 완전한 개별 조절은 별도 마스크가 필요하다.
다른 건물, 실내의 추가 점광원, 모든 표정·카메라 회전은 이 캡처만으로 검증했다고 할 수 없다. 성능 수치는 측정하지 않았다.</p>
<h2>재현과 복원</h2>
<p>생성·적용 코드는 Tools/Characters/setup_player_reactive_lighting.py,
새 실행 검증 코드는 Tools/Characters/review_player_reactive_lighting.py다.
프로젝트 고유의 캐릭터 준비 함수와 경로를 사용한다. 원본 에셋과 공유 VRM4U 재질은 편집하지 않았다.</p>
<p>적용 직전 남녀 메시 백업은 Backups/20261103-151034에 있다. 복원이 필요하면 에디터를 종료하고 원래 SamplePlayer 경로에 복사한다.
현재 재질 설정과 파일 해시는 Applied.json, 자세한 작업 설명은 Docs/Characters/Player_WorldLighting.ko.md에 남겼다.</p>
<h2>참고한 공식 자료</h2><ul>
<li><a href="https://ruyo.github.io/VRM4U/01_material/">VRM4U 재질</a></li>
<li><a href="https://ruyo.github.io/VRM4U/02_envlight/">VRM4U 환경광</a></li>
<li><a href="https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine">Unreal Engine Shading Models</a></li>
<li><a href="https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine">Unreal Engine Auto Exposure</a></li>
</ul></article>'''
(OUT / 'TIL.html').write_text(document('TIL — VRoid 플레이어 실내외 조명 진단과 수정', til), encoding='utf-8')

# Earlier screenshots remain evidence of their own experiments, with the current result linked clearly.
for folder in ['MethodComparison', 'EnvironmentLighting', 'SingleSunLighting', 'SingleSunLighting/ExactReference8']:
    for path in (OUT.parent / folder).glob('*.html'):
        text = path.read_text(encoding='utf-8')
        relative = '../' * (len(Path(folder).parts)) + 'WorldReactive/Comparison.html'
        banner = f'<p style="padding:16px;background:#263649;border-left:4px solid #9bc5ff">현재 적용: <strong>실제 조명·그림자를 받는 WorldReactive 재질</strong>. 아래는 이전 실험 기록입니다. <a href="{relative}">밤·낮 실내외 최종 캡처</a></p>'
        pattern = r'<p[^>]*>현재 (?:최종 적용|적용):.*?</p>'
        if re.search(pattern, text, re.S):
            text = re.sub(pattern, lambda _: banner, text, count=1, flags=re.S)
        else:
            text = text.replace('<main>', '<main>' + banner, 1)
        path.write_text(text, encoding='utf-8')
print('Created Comparison.html and TIL.html; eight final screenshots verified.')
