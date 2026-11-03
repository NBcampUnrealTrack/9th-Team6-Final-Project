# NPC_R01_003 머리카락만 음영 보정

분홍색 긴 머리 NPC_R01_003에 적용했다. 눈·얼굴의 현재 재질은 변경하지 않았다.
원본 아틀라스 슬롯 0은 얼굴·눈, 슬롯 1은 몸·옷·머리카락이다. 실제 PNG를 확인해 NPC 3의 머리카락 두 UV 영역을 선택했다.
NPC 1의 UV 좌표를 복사해서 적용하지 않았다.

- 머리카락 UV: `[[0.5001, 0.5001, 0.7499, 0.9999], [0.765, 0.52, 0.995, 0.74]]`.
- 전용 부모: `/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_003/M_ICNPC003HairOnly`.
- `ICHairScatter = 0.25`, `ICHairDensity = 0.65`.
- From Material Expression으로 머리카락 UV만 Subsurface, 그 밖의 옷·피부는 Default Lit이다.
- 원본 색·알파·표면색 배율 0.65·자체 발광 0·그림자를 유지했다.
- 기존 몸 재질의 상속 파라미터 동일성과 눈·얼굴 파일 해시 보존을 검사했다.
- NPC 1을 포함한 다른 NPC·플레이어·월드·메시·스킬·애니메이션은 변경하지 않았다.
- 저장 후 새 렌더링 에디터에서 기본 재질로 밤·낮 실내외의 전신·근접 8장을 촬영했다.

보고서: `Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/Comparison.html`.
백업: `C:\Users\20ori\Desktop\IsekaiClass\Saved\CharacterLighting\NPCWorldReactive\NPC003HairOnly\Backups\20261103-173844`.
설정: `Tools/Characters/setup_npc003_hair_only.py`.
검증: `review_npc001_soft_head.py -ICNPC003HairOnlyReview -ICNPC003HairOnlyFinal`.
