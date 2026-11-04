"""Lay out unmodified game captures as labeled comparison sheets for the TIL."""
import html,json,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Docs/Characters/TIL/VRoidLighting_2026-11-03_Final'
OUT.mkdir(parents=True,exist_ok=True);(OUT/'sheets').mkdir(exist_ok=True);(OUT/'images').mkdir(exist_ok=True)
SAVED=ROOT/'Saved/CharacterLighting';ALL=SAVED/'AllCharacterOutline_20261103';PLANS=[];MANIFEST=[]
ENV=[('Night_Outside','밤 외부 · 20:00'),('Night_Inside','밤 내부 · 20:00'),('Day_Outside','낮 외부 · 10:00'),('Day_Inside','낮 내부 · 10:00')]

def sheet(name,title,subtitle,items,columns=2):
    width=1800;cell=(width-48-18*(columns-1))/columns;height=130+((len(items)+columns-1)//columns)*(cell*9/16+48+18)
    body='<h1>'+html.escape(title)+'</h1><p>'+html.escape(subtitle)+'</p><div class="grid">'
    for path,label in items:
        assert path.exists(),path
        body+='<figure><img src="'+path.as_uri()+'"><figcaption>'+html.escape(label)+'</figcaption></figure>'
    body+='</div>'
    css='body{margin:0;padding:24px;box-sizing:border-box;background:#13171d;color:#edf2fa;font:18px/1.4 "Malgun Gothic",sans-serif}h1{font-size:28px;margin:0 0 10px}p{margin:0 0 24px;color:#c7d3e6}.grid{display:grid;grid-template-columns:repeat('+str(columns)+',1fr);gap:18px}figure{margin:0}img{display:block;width:100%;aspect-ratio:16/9;object-fit:contain;background:#080a0c}figcaption{padding-top:8px;min-height:35px}'
    page=OUT/'sheets'/(name+'.html');page.write_text('<!doctype html><html lang="ko"><meta charset="utf-8"><style>'+css+'</style>'+body+'</html>',encoding='utf-8')
    PLANS.append({'name':name,'html':str(page),'png':str(OUT/'images'/(name+'.png')),'width':width,'height':int(height+30)})
    MANIFEST.append({'image':name+'.png','sources':[{'path':str(p.relative_to(ROOT)),'caption':l} for p,l in items],'layout_only_no_colour_retouch':True})

def historic():
    first=[('00_current','기준 · 초기 부위 혼합'),('01_unlit_scale','1 · Unlit 고정 배율'),('02_tonemap','2 · 역톤매핑 해제'),('03_mtoon_lit','3 · MToon Lit'),('04_sss_skin','4 · 피부 SSS'),('05_character_fill','5 · 캐릭터 보조광'),('06_postprocess','6 · 화면 명도 보정'),('07_environment','7 · 조도 추정형 Unlit'),('08_single_sun','8 · 단일 태양광')]
    for time,label in [('04h','04:00'),('10h','10:00'),('14h','14:00'),('20h','20:00')]:sheet('initial-'+time,'초기 조명 8방식 + 기준 · '+label,'초기 실험 당시의 월드. 최종 설정과 구분해 비교한다.',[(SAVED/'MethodComparison'/method/(time+'.png'),caption) for method,caption in first],3)
    research=SAVED/'NPCWorldReactive/NPC003HairOnly/Research_20261103'
    for env,label in ENV:
        src=research/'ReportPreview'/('Overview_'+env+'.png');dest=OUT/'images'/('hair-eight-'+env+'.png');assert src.exists();shutil.copy2(src,dest)
        MANIFEST.append({'image':dest.name,'sources':[{'path':str(src.relative_to(ROOT))}],'layout_only_no_colour_retouch':True})
    edge=SAVED/'NPCWorldReactive/NPC003HairOnly/EdgeStudy_20261103'
    rows=json.loads((edge/'Methods.json').read_text(encoding='utf-8'))
    for env,label in [ENV[1],ENV[2]]:
        ids=['02_geometry_normal','07_raw_diffuse','09_mask_coverage','10_alpha_colour','11_alpha_clip','12_finer_mip','13_thin_normal','D1_unlit_view','D2_short_ao_off','D3_screen_trace_off']
        sheet('edge-extra-'+env,'머리 경계 추가 실험 · '+label,'기준 2·7번 / UV·알파·mip·얇은 면 / Unlit·간접광 진단. 진단용 화면은 최종 설정이 아니다.',[(edge/'Captures'/(i+'_'+env+'_Closeup.png'),next(m['title'] for m in rows if m['id']==i)) for i in ids],2)
        ids=['07_raw_diffuse','D4_post_material_off','D5_ssao_off','D6_dfao_off','D7_all_ao_off','D8_contact_off']
        sheet('ao-diagnostics-'+env,'후처리·AO 진단 · '+label,'후처리 전체 비활성 화면은 검게 나와 유효한 최종 후보가 아니다. 각 기능의 영향만 분리했다.',[(edge/'OutlineCaptures'/(i+'_'+env+'_Closeup.png'),next(m['title'] for m in rows if m['id']==i)) for i in ids],3)
    kawaii=SAVED/'KawaiiToon_20261103/Captures'
    for env,label in ENV:sheet('kawaii-'+env,'폐기한 Kawaii Toon 기본값 · '+label,'원본 재질 복사본에 플러그인 기본값 적용. 플레이어 여성 / NPC 003. 현재 플러그인은 제거했다.',[(kawaii/(subject+'_'+env+'_'+view+'.png'),title+' · '+vtitle) for subject,title in [('Player_F','플레이어 여성'),('NPC_R01_003','NPC 003')] for view,vtitle in [('Full','전신'),('Face','얼굴')]])
    preview=SAVED/'NPC003CharacterOutline_20261103/PreviewCaptures'
    for env,label in ENV:sheet('outline-before-after-'+env,'머리만 → 캐릭터 전체 선 제외 · '+label,'왼쪽: 머리만 보정 / 오른쪽: 피부·옷·얼굴까지 확대. 실제 조명과 그림자는 유지한다.',[(preview/(method+'_'+env+'_'+view+'.png'),title+' · '+vtitle) for view,vtitle in [('Full','전신'),('Closeup','근접')] for method,title in [('15_hair_boundary_filter','머리만'),('Character_Whole','전신')]])
    sheet('world-control','캐릭터를 숨긴 배경 대조','같은 카메라·시간. 정적 벽 영역의 평균 RGB 절대 차이 0.213/255, 95% 차이 1 이하.',[(preview/'World_Original.png','원본 후처리'),(preview/'World_Selected.png','캐릭터 전용 예외 추가')])
    npc=SAVED/'NPCWorldReactive'
    for env,label in ENV:
        revisions=[(npc/'Final','기준'),(npc/'NPC001SoftHead/Final','얼굴·눈 산란 실험'),(npc/'NPC001HairOnly/Final','눈 복원·머리카락만 산란')]
        items=[(folder/('NPC_R01_001_'+env+'.png'),title+' · 전신') for folder,title in revisions]
        if env=='Day_Outside':items += [(folder/('NPC_R01_001_'+env+'_Closeup.png'),title+' · 얼굴') for folder,title in revisions]
        sheet('npc001-revisions-'+env,'NPC 001의 앞선 수정 기록 · '+label,'얼굴·눈 산란은 철회했다. 아틀라스를 확인한 뒤 눈을 복원하고 실제 머리카락 UV만 조정했다. 현재 최종안 이전의 기록이다.',items,3)

def final():
    status=json.loads((ALL/'FinalVerification.json').read_text(encoding='utf-8'));assert status['complete'] and len(status['samples'])==71
    captures=ALL/'FinalCaptures'
    ids=['Player_M','Player_F']+[f'NPC_R01_{i:03}' for i in range(1,12)]
    for env,label in ENV:
        sheet('final-all-'+env,'최종 저장본 · 13개 외형 · '+label,'남·여 플레이어와 NPC 1~11. 임시 재질 덮어쓰기 없이 저장된 기본값으로 확인했다.',[(captures/(i+'_'+env+'_Full.png'),i) for i in ids],3)
        sheet('final-players-'+env,'최종 저장본 · 플레이어 남·여 · '+label,'왼쪽 남성 / 오른쪽 여성. 전신과 얼굴을 함께 비교한다.',[(captures/('Player_'+sex+'_'+env+'_'+view+'.png'),title+' · '+vtitle) for view,vtitle in [('Full','전신'),('Closeup','얼굴')] for sex,title in [('M','남성'),('F','여성')]])
        for view in ['Full','Closeup']:
            source=captures/('NPC_R01_003_'+env+'_'+view+'.png')
            if source.exists():shutil.copy2(source,OUT/'images'/('final-npc003-'+env+'-'+view+'.png'))
    sheet('final-npc-faces','최종 저장본 · NPC 1~11 얼굴 · 낮 외부 10:00','각자의 원본 텍스처와 머리카락 UV로 적용한 결과. 눈 재질은 기존 표현을 유지한다.',[(captures/(i+'_Day_Outside_Closeup.png'),i) for i in ids if i.startswith('NPC')],3)
    # Keep all final original files available to the user in the local artifact.
    gallery='<h1>최종 저장본 원본 71장</h1>'
    for sample in status['samples']:gallery+='<p>'+sample['subject']+' · '+sample['environment']+'</p><a href="'+(captures/sample['image']).as_uri()+'"><img style="max-width:100%" src="'+(captures/sample['image']).as_uri()+'"></a>'
    (OUT/'Originals.html').write_text('<!doctype html><meta charset="utf-8">'+gallery,encoding='utf-8')

if '--final' in sys.argv:final()
else:historic()
(OUT/('FinalRenderPlan.json' if '--final' in sys.argv else 'HistoricRenderPlan.json')).write_text(json.dumps(PLANS,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/('FinalImageSources.json' if '--final' in sys.argv else 'HistoricImageSources.json')).write_text(json.dumps(MANIFEST,ensure_ascii=False,indent=2),encoding='utf-8')
print('Prepared render sheets:',len(PLANS))
