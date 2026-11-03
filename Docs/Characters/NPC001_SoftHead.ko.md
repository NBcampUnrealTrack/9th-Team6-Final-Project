# NPC 001 머리·눈 음영 완화

적용 범위는 `NPC_R01_001`의 머리 재질 하나다. 몸·다른 10명·플레이어·월드는 유지했다.

이 문서는 이전 얼굴·눈 산란 실험 기록이다. 후속 원본 아틀라스 검사로 NPC 001의 얼굴·눈은
첫 번째 슬롯(T_texture0), 실제 머리카락은 두 번째 슬롯(T_texture3)에 포함되는 것을 확인했다.
따라서 이 실험에서 머리카락도 함께 변경했다는 이전 설명은 잘못되었다.
현재는 얼굴·눈을 이전 상태로 복원하고 머리카락 UV만 조정했다. `NPC001_HairOnly.ko.md`를 참고한다.

- 부모: `/Game/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_001/M_ICNPC001SoftHead`
- 기존 머리 인스턴스의 부모만 변경했다. 메시의 기본 슬롯 경로는 같다.
- `Subsurface`, `ICHeadScatter = 0.8`, `ICHeadDensity = 0.25`.
- 원본 RGB·tint·알파, `ICAlbedoScale = 0.65`, Masked·Two Sided를 유지했다.
- 자체 발광·Specular·Metallic은 0, Roughness는 1이다. 조명·노출·시간별 배율은 바꾸지 않았다.
- 캐릭터 그림자는 유지한다. 그림자를 끄는 진단 시도는 채택하지 않았다.

사전 비교는 기존 재질, 그림자 끄기, 산란 3단계로 진행했다. 햇빛 아래 눈 주변 음영이 완화되고
밤 실내에서는 기존의 어두운 반응을 유지하는 설정을 선택했다.

저장 후 새 에디터에서 기본 재질을 사용해 네 조건의 전신·근접 화면 8장을 검증했다.
결과: `Saved/CharacterLighting/NPCWorldReactive/NPC001SoftHead/Comparison.html`.
원본 캡처는 1600×900이며 이미지를 후처리하지 않는다.

적용/검증 코드는 `setup_npc001_soft_head.py`, `review_npc001_soft_head.py`다.
`Scope.json`에는 변경 대상에서 제외한 파일의 해시와 기존 머리 인스턴스 백업 경로를 기록했다.

재질 설정 참고: [Epic — Using Subsurface Scattering](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-subsurface-scattering-in-unreal-engine-materials).
