# R01 NPC 11명 조명 적용 — 2026-11-03

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
- 직전 메시 백업: `C:\Users\20ori\Desktop\IsekaiClass\Saved\CharacterLighting\NPCWorldReactive\Backups\20261103-164746`.

복원 시 에디터를 종료하고 백업의 SK_R01_1~11.uasset을 각각 원래 NPC_R01_001~011 폴더에 복사한다.
머리 아틀라스는 눈 외 영역도 함께 조명을 받으며, 실내·밤에는 기존 주변광이 남아 완전히 검게 되지 않는다.

## NPC 001 현재 구성

위 55장은 최초 일괄 적용 기록이다. 후속 아틀라스 확인으로 NPC 001 얼굴·눈은 첫 슬롯, 실제 머리카락은
두 번째 몸 슬롯에 들어 있음을 확인했다. 얼굴·눈 산란 실험은 철회하고 이전 MI 파일을 그대로 복원했다.
현재는 두 번째 슬롯의 머리카락 UV 두 영역만 별도 산란을 사용하며 옷·피부는 Default Lit이다.
상세는 `NPC001_HairOnly.ko.md`, 최신 캡처는 `Saved/CharacterLighting/NPCWorldReactive/NPC001HairOnly/Comparison.html`을 참고한다.

## NPC 003 추가 수정

NPC_R01_003은 자체 아틀라스의 머리카락 UV 두 영역에만 약한 산란을 적용했다. 눈·얼굴 재질과 다른 캐릭터는 그대로다.
현재 구성은 `NPC003_HairOnly.ko.md`, 비교 캡처는 `Saved/CharacterLighting/NPCWorldReactive/NPC003HairOnly/Comparison.html`을 참고한다.
