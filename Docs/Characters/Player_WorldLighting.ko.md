# 플레이어의 실제 조명·그림자 반응 — 2026-11-03

최종 선택 기준의 간단한 TIL과 전후 사진: [전체 과정 TIL](TIL/VRoidLighting_2026-11-03/index.html)
· [블로그용 Markdown](TIL/VRoidLighting_2026-11-03/TIL.md)

남녀 플레이어의 19개 기본 재질 슬롯은 `/Game/IsekaiClass/Player/Materials/WorldReactive`를 사용한다.
원래 VRM 재질의 색·텍스처 계산을 복제한 뒤 최종 색을 Default Lit의 Base Color에 연결한다.
Emissive Color는 상수 0이다. 이제 캐릭터 표면이 실제 직접광·간접광·그림자를 받는다.

눈 수정: 눈동자·흰자·하이라이트·눈썹·속눈썹·아이라인·입은 별도 부모 재질
`WorldReactive/Shared/M_ICAuthoredFeatureLit`을 사용한다. 원본 diffuse RGB × 원본 색 인자 × ICAlbedoScale을
Base Color에, 원본 텍스처 알파 × 색 인자의 알파를 Opacity Mask에 직접 연결한다.
Default Lit · Masked · Roughness 1 · Specular 0 · Emissive 0이며, 스켈레탈 메시와 Morph Target 사용 플래그를 켠다.
기존 MToon 부모에서 최종 색을 재사용하거나 단순히 색 입력 분기만 추가했을 때는 눈이 희게 흐려지는 현상이 남았다.
별도 부모 재질에서는 원본 눈동자와 아이라인을 유지하면서 실제 조명을 받는다.
남성은 눈이 합쳐진 머리 아틀라스 슬롯 전체에 적용한다. 여성 피부·머리카락·옷의 기존 분기는 그대로다.

이전 8번 MToon Lit + 단일 태양광 설정은 낮밤 변화는 있었지만, 확인한 집 안에서 캐릭터가 지나치게 밝았다.
같은 메시를 엔진 기본 Lit 재질로 바꾸자 실내외 차이가 나타났다. 따라서 이번에는 월드를 추가 조정하지 않고
플레이어 재질의 최종 출력 경로를 바꿨다. Shading Model 이름만 Lit으로 바꾸는 것과는 다르다.

## 재질 설정

- `ICAlbedoScale = 0.65`: 조명을 받는 표면색의 배율. 화면 밝기나 야간 전용 보정 비율이 아니다.
- `bUseLight = false`, `bUseLightForce = false`: 원본 색 계산 안에서 MToon의 별도 조명 계산을 중복하지 않는다.
- `bUseFilmTonemapInverse = false`, `bDivAdaptation = false`: 색 계산의 역톤매핑·노출 보상 경로를 끈다.
- MatCap·Rim·CustomReflection은 끈다. 얼굴·눈 계열은 Roughness 1, Specular 0이다.
- 기존 MToon 계층을 사용하는 여성 피부는 NormalToCameraMat 0.95, NormalFresnelScale 2를 유지한다. 별도 눈 부모는 메시의 기본 노멀을 사용한다. 머리카락·옷의 원래 노멀은 약하게 사용한다.
- 원본 텍스처, 불투명도·마스크, 원본 VRM4U 공유 부모는 편집하지 않는다. 부모 계층도 플레이어 전용으로 복제했다.

툰 텍스처와 얼굴용 노멀 보정을 유지하지만, 최종 음영은 실제 조명에 따라 달라진다.
앞머리 아래 얼굴에는 그림자가 생길 수 있다. 기존 Unlit의 평평한 음영을 완전히 동일하게 유지하는 설정은 아니다.
남성 모델은 기존 두 아틀라스 슬롯을 유지한다. 피부·눈을 별도 재질로 분리하는 새 UV 마스크를 추가하지 않았다.

## 월드와 런타임

직전 8번 적용 상태인 `DirectionalLight_0` 한 개와 기존 SkyLight·노출·후처리를 그대로 사용한다.
`LightSource_2`, `LightSource_1`은 이전 작업에서 숨김·강도 0으로 설정한 상태다.
이번 재질 수정에서는 맵·엔진 설정·낮밤 Blueprint 파일의 SHA-256이 바뀌지 않았는지 확인한다.

`ICPlayerAppearance::Apply`는 프리셋을 바꾸면서 OverrideMaterials를 비우고 새 메시의 기본 재질을 사용한다.
시간별 밝기 컴포넌트, 태양 가림에 따른 강제 배율, 캐릭터 전용 보조광은 사용하지 않는다.
검증의 태양 방향 레이캐스트는 가림을 기록하는 진단용이며 재질 밝기에 연결하지 않는다.

## 적용·검증·복원 자료

- 생성·적용: `Tools/Characters/setup_player_reactive_lighting.py` (`-ICApplyReactiveLighting` 필요, PIE 종료 상태)
- 적용 내역: `Saved/CharacterLighting/WorldReactive/Applied.json`
- 직전 8번 메시 백업: `Saved/CharacterLighting/WorldReactive/Backups/20261103-151034`
- 새 에디터에서 검증: `Tools/Characters/review_player_reactive_lighting.py`
- 실제 게임 캡처: `Saved/CharacterLighting/WorldReactive/Final`
- 전체 비교와 TIL: `Saved/CharacterLighting/WorldReactive/Comparison.html`, `TIL.html`
- 눈 수정 후 최신 캡처: `Saved/CharacterLighting/WorldReactive/EyeFix/Comparison.html`
- 눈 수정 내역과 재질 백업: `EyeFix/Applied.json`, `EyeFix/Before/Materials`

최종 캡처는 밤 외부·밤 내부·낮 외부·낮 내부를 남녀 각각 촬영한다. 밤 20시, 낮 10시다.
1600×900 실제 게임 창을 촬영하며 런타임 재질이나 밝기 값을 시험용으로 덮어쓰지 않는다.
`Final/Verification.json`의 complete와 보호 파일 해시를 함께 확인한다.
눈 수정 검증은 `-ICEyeCorrectionReview`를 추가해 실행한다. `EyeFix/Final/Verification.json`과
밤·낮 실내외 전신 및 얼굴 근접 화면을 남녀 각각 확인한다.

직전 상태로 복원할 때는 에디터를 닫고 백업의 SK_Player_F/M.uasset을 각각 원래 SamplePlayer 경로로 복사한다.
새 재질은 플레이어 전용 폴더에 있으므로 원본 VRM 재질을 수정할 필요가 없다.
이전 SingleSunLighting 및 MethodComparison 보고서는 당시 실험 기록으로 보존한다.
