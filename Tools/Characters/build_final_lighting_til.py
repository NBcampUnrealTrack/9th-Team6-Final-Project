"""Build a concise, portable Korean TIL from the accepted lighting and eye captures.

Documentation only: copies original screenshots and never opens or edits Unreal.
"""
import hashlib
import html
import json
import re
import shutil
import struct
import zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
LIGHTING=ROOT/'Saved/CharacterLighting'
FINAL=LIGHTING/'WorldReactive/EyeFix/Final'
OUT=ROOT/'Docs/Characters/TIL/VRoidLighting_2026-11-03'
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'images').mkdir(exist_ok=True)
TITLE='언리얼 VRoid 캐릭터의 실내외 밝기와 눈 재질 수정'
VERIFY=json.loads((FINAL/'Verification.json').read_text(encoding='utf-8'))
assert VERIFY['complete'] and len(VERIFY['samples'])==16
PROTECTED=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
    ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset',
    ROOT/'Content/IsekaiClass/Player/SamplePlayer/Female/SK_Player_F.uasset',
    ROOT/'Content/IsekaiClass/Player/SamplePlayer/Male/SK_Player_M.uasset']
PROTECTED+=list((ROOT/'Content/IsekaiClass/Player/Materials/WorldReactive').rglob('*.uasset'))
hash_file=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
before={str(path.relative_to(ROOT)):hash_file(path) for path in PROTECTED}

SOURCES={
    'before_inside.png':LIGHTING/'WorldReactive/Preview/Current_Day_Inside.png',
    'before_eyes.png':LIGHTING/'WorldReactive/EyeFix/F_Current.png',
    'after_eyes.png':FINAL/'F_Closeup_Day_Outside.png',
    'night_outside.png':FINAL/'F_Night_Outside.png',
    'night_inside.png':FINAL/'F_Night_Inside.png',
    'day_outside.png':FINAL/'F_Day_Outside.png',
    'day_inside.png':FINAL/'F_Day_Inside.png',
}
provenance=[]
for name,source in SOURCES.items():
    destination=OUT/'images'/name
    assert struct.unpack('>II',source.read_bytes()[16:24])==(1600,900)
    shutil.copy2(source,destination)
    assert hash_file(source)==hash_file(destination)
    provenance.append({'image':'images/'+name,'source':source.relative_to(ROOT).as_posix(),'sha256':hash_file(source)})

BLOCKS=[
('p','VRoid 캐릭터의 툰 느낌을 유지하면서 **밝은 곳에서는 밝게, 어두운 곳에서는 어둡게** 보이도록 수정했다. 최종안은 **단일 태양광 + 실제 조명을 받는 몸 재질 + 별도의 눈 재질**이다.'),
('h2','1 문제와 시도'),
('p','처음에는 밤이나 집 안에서도 캐릭터가 배경보다 밝았다. Unlit은 Emissive 중심으로 색을 출력하고, Default Lit은 직접광·간접광을 받는다. 밝기 배율만 낮추는 것과 환경에 반응하게 만드는 것은 달랐다. [Unreal 공식 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine)'),
('table',(['시도','확인한 점'],[
    ['초기 Simple PBR / 부위별 혼합','얼굴의 툰 느낌이 달라지거나, 보조 밝기 때문에 어둠 반응이 약했다.'],
    ['1 Unlit 고정 명도 / 2 역톤매핑 해제','전체가 어두워지거나 색이 변했지만, 실내외 반응의 해결책은 아니었다.'],
    ['3 MToon Lit / 4 피부 SSS','조명 반응은 생겼지만 낮의 과한 밝기·밤의 표정 가독성·피부 표현이 맞지 않았다.'],
    ['5 보조광 / 6 화면 명도 보정','가독성은 보완했으나 원래 색과 윤곽에 부작용이 있었다.'],
    ['7 조도 추정형 Unlit','광원 강도와 가림 검사로 밝기를 근사했다. 실제 표면 조명 방식은 아니었다.'],
    ['8 단일 태양광','중복 방향광을 정리해 낮밤은 나아졌지만, 집 안의 캐릭터는 여전히 밝았다.'],
])),
('p','같은 캐릭터에 **엔진 기본 Lit 재질**을 임시 적용하자 실내에서는 어둡고 밖에서는 밝았다. 지붕도 태양 방향을 막고 있었다. 이 장소에서는 월드 전체를 더 조정하기보다 **플레이어 재질의 출력 경로**를 고치는 것이 우선이었다.'),
('gallery',[
    ('before_inside.png','수정 전 · 8번 적용 상태의 낮 실내'),
    ('day_inside.png','최종 · 같은 집의 낮 실내'),
]),
('h2','2 에디터에서 바꾼 곳'),
('table',(['위치','최종 변경'],[
    ['`L_OpenWorld` → World Outliner','`DirectionalLight_0`만 사용(주간 기준 6.0 lux). `LightSource_1`, `LightSource_2`는 Visible 끔·Intensity 0. 액터 이름 기준이다.'],
    ['낮밤 컨트롤러 인스턴스 → Details → `IC / Day Night / Actors / Sun Lights`','배열에 `DirectionalLight_0` 하나만 연결. 컨트롤러가 시간에 따라 강도를 바꾸므로 게임 중 수동 강도 변경은 다시 덮어써질 수 있다.'],
    ['Content Browser → `IsekaiClass/Player/Materials/WorldReactive`','원본과 부모 계층을 플레이어 전용으로 복제. 몸은 `Default Lit`, 표면색은 `Base Color`, 자체 발광은 `0`으로 구성.'],
    ['몸 Material Instance → `Material Property Overrides`','`Shading Model` 오버라이드를 켜고 `Default Lit`으로 설정. 부모 재질만 바꾸고 Unlit 오버라이드가 남는 상황을 피한다.'],
    ['몸 Material Instance → Details에서 파라미터 검색','`ICAlbedoScale = 0.65`, `ColorScaleAllMat = 1`. `bUseLight`, `bUseLightForce`, `bUseFilmTonemapInverse`, `bDivAdaptation`, MatCap·Rim·CustomReflection 끔.'],
    ['몸 Material Instance → 부위별 보정','`bUseMRSA` 켬. 피부 MRSA `(0,1,0,1)`, 머리·옷 `(0,0.85,0.03,1)`. 여성 피부 `NormalToCameraMat = 0.95`, `NormalFresnelScale = 2`, 노멀맵 끔. 머리·옷은 노멀맵 켬, `mtoon_NormalScale`은 머리 0.15·그 외 0.25. `NormalToLightMat = 0`.'],
    ['`SamplePlayer/Female/SK_Player_F`, `Male/SK_Player_M` 열기 → Asset Details의 `Materials`','기본 슬롯을 `WorldReactive/F`, `WorldReactive/M`의 재질로 연결. 여성 17개·남성 2개.'],
])),
('p','몸의 부모 그래프는 `WorldReactive/Shared/M_VrmMToonBaseOpaque_6dd88d`에서 확인할 수 있다. 원본 속성을 Break/Make Material Attributes로 이어 받되, 색과 발광의 연결을 아래처럼 바꿨다.'),
('code','원본 툰 색 계산 × ICAlbedoScale(0.65) → Base Color\n상수 0 → Emissive Color\n원본 노멀·불투명도 등 → 대응 속성 유지'),
('p','**0.65는 표면색 배율이며 화면 밝기 65%가 아니다.** 태양을 하나로 줄인 8번 단계에서는 배경 조명도 바뀌었다. 이후 몸·눈 재질 수정에서는 그 월드 상태를 유지했고, SkyLight·노출·후처리를 추가로 바꾸지 않았다.'),
('h2','3 눈동자가 희게 변한 문제'),
('p','몸의 밝기는 좋아졌지만 눈동자와 아이라인이 희게 흐려졌다. 원본 텍스처는 정상이었고, 기존 MToon 부모 안에 색 연결 분기만 추가해도 해결되지 않았다. **원본 RGB와 알파만 사용하는 별도의 Masked Lit 부모**로 분리하자 눈이 정상적으로 보였다.'),
('gallery',[
    ('before_eyes.png','수정 전 · 밝기 변환 후 희게 흐려진 눈'),
    ('after_eyes.png','최종 · 원본 눈동자와 아이라인'),
]),
('p','Content Browser에서 `WorldReactive/Shared/M_ICAuthoredFeatureLit`을 열어 아래처럼 구성했다. 눈 관련 Material Instance의 **Details → Parent**도 이 재질로 바꿨다.'),
('code','원본 diffuse RGB × 원본 tint RGB × 0.65 → Base Color\n원본 diffuse Alpha × 원본 tint Alpha → Opacity Mask\nEmissive / Metallic / Specular = 0\nRoughness = 1'),
('table',(['Material Editor의 Details','값'],[
    ['Shading Model / Blend Mode / Two Sided','`Default Lit` / `Masked` / 켬'],
    ['Usage','`Used with Skeletal Mesh`, `Used with Morph Targets` 켬'],
    ['눈 Material Instance','`ICFeatureAlbedoTexture` = 원본 diffuse, `ICFeatureAlbedoTint` = 원본 색 인자'],
    ['Material Property Overrides → Opacity Mask Clip Value','약 `0.3333`'],
])),
('p','여성은 눈·흰자·하이라이트·눈썹·속눈썹·아이라인·입에 적용했다. 남성은 눈이 합쳐진 **머리 아틀라스 슬롯 전체**에 적용했다. 눈의 RGB/알파를 별도 표면으로 분리한 것이 최종 해결책이며, 기존 MToon 내부의 단일 원인 노드까지 특정한 것은 아니다.'),
('h2','4 최종 확인'),
('p','저장 후 에디터를 다시 실행해 남녀 프리셋의 **밤 외부·밤 내부·낮 외부·낮 내부**를 확인했다. 전신과 얼굴 근접을 합쳐 16장을 실제 게임 창 1600×900으로 촬영했다. 아래는 최종 여성 프리셋 전신 4장이다.'),
('gallery',[
    ('night_outside.png','밤 외부 · 20:00'),('night_inside.png','밤 내부 · 20:00'),
    ('day_outside.png','낮 외부 · 10:00'),('day_inside.png','낮 내부 · 10:00'),
]),
('p','프리셋 교체 코드 `ICPlayerAppearance::Apply`에서도 메시 변경 뒤 `EmptyOverrideMaterials()`를 호출해 이전 재질이 남지 않게 했다. 최종 검증은 임시 재질·시간별 강제 밝기 보정 없이 **저장된 메시의 기본 재질**로 진행했다. 실내와 밤에도 기존 간접광이 남으므로 완전히 검게 되지는 않는다.'),
('h2','배운 점'),
('p','**Lit이라는 이름만 확인하지 말고, 색이 Base Color와 Emissive 중 어디로 가는지와 알파 연결까지 봐야 한다.** 툰 캐릭터는 몸과 눈을 같은 방식으로 변환하면 표현이 깨질 수 있다. 낮밤뿐 아니라 실내외·전신·얼굴 근접·저장 후 재실행을 함께 확인해야 했다.'),
('p','설정 재현: `Tools/Characters/setup_player_reactive_lighting.py` · 검증: `review_player_reactive_lighting.py`의 `-ICEyeCorrectionReview`. 재질 종류를 비교할 때는 [VRM4U 공식 안내](https://ruyo.github.io/VRM4U/01_material/)를 참고했다.'),
]

def inline(value):
    value=html.escape(value)
    value=re.sub(r'`([^`]+)`',r'<code>\1</code>',value)
    value=re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',value)
    return re.sub(r'\[([^\]]+)\]\((https?://[^)]+)\)',r'<a href="\2">\1</a>',value)

md=['# '+TITLE,'','2026-11-03 · Unreal Engine 5.8 · VRM4U','']
parts=['<header><p class="meta">TIL · 2026-11-03 · Unreal Engine 5.8 · VRM4U</p><h1>'+TITLE+'</h1><p class="download"><a href="TIL.md">블로그용 Markdown</a></p></header>']
for kind,value in BLOCKS:
    if kind=='h2':
        anchor='editor' if value.startswith('2 ') else 'eyes' if value.startswith('3 ') else 'final' if value.startswith('4 ') else ''
        parts.append(f'<h2 id="{anchor}">{html.escape(value)}</h2>');md+=['## '+value,'']
    elif kind=='p':parts.append('<p>'+inline(value)+'</p>');md+=[value,'']
    elif kind=='code':parts.append('<pre><code>'+html.escape(value)+'</code></pre>');md+=['```text',value,'```','']
    elif kind=='table':
        heads,rows=value
        parts.append('<div class="table"><table><thead><tr>'+''.join('<th>'+inline(cell)+'</th>' for cell in heads)+'</tr></thead><tbody>'+''.join('<tr>'+''.join('<td>'+inline(cell)+'</td>' for cell in row)+'</tr>' for row in rows)+'</tbody></table></div>')
        md+=['| '+' | '.join(heads)+' |','| '+' | '.join('---' for _ in heads)+' |']
        md+=['| '+' | '.join(cell.replace('|',r'\|') for cell in row)+' |' for row in rows]+['']
    elif kind=='gallery':
        parts.append('<div class="gallery">')
        for name,label in value:
            parts.append(f'<figure><a href="images/{name}"><img src="images/{name}" width="1600" height="900" alt="{html.escape(label)}"></a><figcaption>{html.escape(label)}</figcaption></figure>')
            md+=['![단계별 실제 게임 캡처 · '+label+'](images/'+name+')','',label,'']
        parts.append('</div>')

CSS='''*{box-sizing:border-box}body{margin:0;background:#f3f5f8;color:#1f2937;font:16px/1.75 "Segoe UI","Malgun Gothic",sans-serif}main{max-width:1100px;margin:32px auto;background:white;border:1px solid #dde3eb;border-radius:14px;padding:42px 48px}h1{font-size:30px;line-height:1.4;color:#111827;margin:8px 0 12px;letter-spacing:-.7px}h2{font-size:22px;line-height:1.5;margin:36px 0 16px;color:#172e54;scroll-margin-top:20px}p{margin:14px 0}header{border-bottom:1px solid #e0e6ef;margin-bottom:24px;padding-bottom:12px}.meta,.download{font-size:13px;color:#637084}a{color:#1d58a6;text-decoration:underline;text-underline-offset:3px}code{font-family:Consolas,"Malgun Gothic",monospace;font-size:.88em;background:#edf2f8;border-radius:4px;padding:2px 4px;overflow-wrap:anywhere}pre{background:#172337;color:#eef4ff;padding:18px 20px;border-radius:8px;white-space:pre-wrap;line-height:1.7}pre code{background:none;padding:0;color:inherit}.table{overflow-x:auto;margin:18px 0}table{width:100%;border-collapse:collapse;table-layout:fixed;font-size:14px;line-height:1.65}th,td{padding:11px 14px;border:1px solid #dbe2ec;vertical-align:top;overflow-wrap:anywhere;text-align:left}th{background:#edf3fb;color:#18375f}th:first-child,td:first-child{width:37%}td code{background:#f3f5f8}.gallery{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin:22px 0}figure{margin:0;min-width:0}img{display:block;width:100%;height:auto;border:1px solid #d6deea;border-radius:7px}figcaption{font-size:13px;color:#516078;line-height:1.5;margin-top:7px}strong{font-weight:700;color:#122f56}@media(max-width:700px){main{margin:0;border:0;border-radius:0;padding:22px 18px}.gallery{grid-template-columns:1fr}h1{font-size:25px}table{font-size:12px}th,td{padding:8px}}@media print{body{background:white}main{margin:0;max-width:none;border:0;padding:0}.download{display:none}figure,pre,tr{break-inside:avoid}h2{break-after:avoid}}'''
(OUT/'TIL.md').write_text('\n'.join(md),encoding='utf-8')
(OUT/'index.html').write_text('<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>'+TITLE+'</title><style>'+CSS+'</style></head><body><main>'+''.join(parts)+'</main></body></html>',encoding='utf-8')
(OUT/'이미지_출처.json').write_text(json.dumps({'screenshots_unchanged':True,'sources':provenance,'final_verification':str((FINAL/'Verification.json').relative_to(ROOT))},ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'사용안내.txt').write_text('index.html을 열면 이미지가 포함된 TIL을 읽을 수 있습니다.\n블로그에는 TIL.md 내용을 옮기고 images 폴더의 PNG를 해당 위치에 업로드하세요.\n캡처는 기존 게임 원본을 그대로 복사했으며 게임 에셋은 변경하지 않았습니다.\n',encoding='utf-8')
for relative,digest in before.items():assert hash_file(ROOT/relative)==digest,relative
archive=OUT.with_suffix('.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as package:
    for file in sorted(OUT.rglob('*')):
        if file.is_file() and 'Preview' not in file.relative_to(OUT).parts:
            package.write(file,arcname=file.relative_to(OUT).as_posix())
print(json.dumps({'article':str(OUT/'index.html'),'markdown':str(OUT/'TIL.md'),'zip':str(archive),'images':len(SOURCES),'zip_mb':round(archive.stat().st_size/1024**2,1),'game_files_unchanged':True},ensure_ascii=False))
