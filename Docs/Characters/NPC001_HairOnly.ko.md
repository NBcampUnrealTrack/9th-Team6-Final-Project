# NPC 001 눈 복원·머리카락 UV만 음영 보정

원본 PNG 확인 결과 NPC 001은 첫 슬롯(T_texture0)에 얼굴·눈, 두 번째 슬롯(T_texture3)에 몸·옷·머리카락이 들어 있다.
이전 설명에서 머리카락이 얼굴·눈과 같은 슬롯이라고 했던 내용을 정정한다. 앞선 SoftHead 실험은 얼굴·눈에만 작용했다.

이번에는 눈·얼굴 MI를 산란 적용 전 백업과 바이트 단위로 동일하게 복원했다. 두 번째 슬롯은 원래 부모 계층을 NPC 001 전용으로
복제하고, 머리카락 UV 두 영역만 Subsurface로 선택한다. 다른 픽셀은 Default Lit이다.

- 부모: `/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001/M_ICNPC001HairOnly`
- 머리카락 UV: U 0.3745~0.6255 / V 0.2495~0.3755, U 0.255~0.37 / V 0.38~0.495.
- `ICHairScatter = 0.4`, `ICHairDensity = 0.6`. 강한 보정은 너무 밝아져 제외했다.
- 원본 텍스처·알파·표면색 배율 0.65·재질 파라미터를 유지했다. 자체 발광 0, 그림자 유지.
- 얼굴·눈 MI 복원 해시, 다른 파일 해시, 기존 몸 재질의 모든 상속 파라미터 일치를 확인했다.
- 다른 10명·플레이어·월드는 수정하지 않았다. 스켈레톤·물리·애니메이션·슬롯도 같다.
- 새 렌더링 에디터에서 저장된 기본 재질로 네 조건 전신·근접 8장을 1600×900으로 촬영했다.

캡처: `Saved/CharacterLighting/NPCWorldReactive/NPC001HairOnly/Comparison.html`.
직전 재질 백업: `C:\Users\20ori\Desktop\IsekaiClass\Saved\CharacterLighting\NPCWorldReactive\NPC001HairOnly\Backups\20261103-172520`.
설정 코드: `Tools/Characters/setup_npc001_hair_only.py`.
검증 코드: `review_npc001_soft_head.py -ICNPC001HairOnlyReview -ICNPC001HairOnlyFinal`.
원본 얼굴/몸 아틀라스 PNG도 같은 캡처 폴더에 보관했다.

참고: [Epic — From Material Expression](https://dev.epicgames.com/documentation/en-us/unreal-engine/from-material-expression-shading-model-in-unreal-engine).
