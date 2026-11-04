# 언리얼 VRoid 캐릭터의 실내외 밝기와 눈 재질 수정

2026-11-03 · Unreal Engine 5.8 · VRM4U

VRoid 캐릭터의 툰 느낌을 유지하면서 **밝은 곳에서는 밝게, 어두운 곳에서는 어둡게** 보이도록 수정했다. 최종안은 **단일 태양광 + 실제 조명을 받는 몸 재질 + 별도의 눈 재질**이다.

## 1 문제와 시도

처음에는 밤이나 집 안에서도 캐릭터가 배경보다 밝았다. Unlit은 Emissive 중심으로 색을 출력하고, Default Lit은 직접광·간접광을 받는다. 밝기 배율만 낮추는 것과 환경에 반응하게 만드는 것은 달랐다. [Unreal 공식 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine)

| 시도 | 확인한 점 |
| --- | --- |
| 초기 Simple PBR / 부위별 혼합 | 얼굴의 툰 느낌이 달라지거나, 보조 밝기 때문에 어둠 반응이 약했다. |
| 1 Unlit 고정 명도 / 2 역톤매핑 해제 | 전체가 어두워지거나 색이 변했지만, 실내외 반응의 해결책은 아니었다. |
| 3 MToon Lit / 4 피부 SSS | 조명 반응은 생겼지만 낮의 과한 밝기·밤의 표정 가독성·피부 표현이 맞지 않았다. |
| 5 보조광 / 6 화면 명도 보정 | 가독성은 보완했으나 원래 색과 윤곽에 부작용이 있었다. |
| 7 조도 추정형 Unlit | 광원 강도와 가림 검사로 밝기를 근사했다. 실제 표면 조명 방식은 아니었다. |
| 8 단일 태양광 | 중복 방향광을 정리해 낮밤은 나아졌지만, 집 안의 캐릭터는 여전히 밝았다. |

같은 캐릭터에 **엔진 기본 Lit 재질**을 임시 적용하자 실내에서는 어둡고 밖에서는 밝았다. 지붕도 태양 방향을 막고 있었다. 이 장소에서는 월드 전체를 더 조정하기보다 **플레이어 재질의 출력 경로**를 고치는 것이 우선이었다.

![단계별 실제 게임 캡처 · 수정 전 · 8번 적용 상태의 낮 실내](images/before_inside.png)

수정 전 · 8번 적용 상태의 낮 실내

![단계별 실제 게임 캡처 · 최종 · 같은 집의 낮 실내](images/day_inside.png)

최종 · 같은 집의 낮 실내

## 2 에디터에서 바꾼 곳

| 위치 | 최종 변경 |
| --- | --- |
| `L_OpenWorld` → World Outliner | `DirectionalLight_0`만 사용(주간 기준 6.0 lux). `LightSource_1`, `LightSource_2`는 Visible 끔·Intensity 0. 액터 이름 기준이다. |
| 낮밤 컨트롤러 인스턴스 → Details → `IC / Day Night / Actors / Sun Lights` | 배열에 `DirectionalLight_0` 하나만 연결. 컨트롤러가 시간에 따라 강도를 바꾸므로 게임 중 수동 강도 변경은 다시 덮어써질 수 있다. |
| Content Browser → `IsekaiClass/Player/Materials/WorldReactive` | 원본과 부모 계층을 플레이어 전용으로 복제. 몸은 `Default Lit`, 표면색은 `Base Color`, 자체 발광은 `0`으로 구성. |
| 몸 Material Instance → `Material Property Overrides` | `Shading Model` 오버라이드를 켜고 `Default Lit`으로 설정. 부모 재질만 바꾸고 Unlit 오버라이드가 남는 상황을 피한다. |
| 몸 Material Instance → Details에서 파라미터 검색 | `ICAlbedoScale = 0.65`, `ColorScaleAllMat = 1`. `bUseLight`, `bUseLightForce`, `bUseFilmTonemapInverse`, `bDivAdaptation`, MatCap·Rim·CustomReflection 끔. |
| 몸 Material Instance → 부위별 보정 | `bUseMRSA` 켬. 피부 MRSA `(0,1,0,1)`, 머리·옷 `(0,0.85,0.03,1)`. 여성 피부 `NormalToCameraMat = 0.95`, `NormalFresnelScale = 2`, 노멀맵 끔. 머리·옷은 노멀맵 켬, `mtoon_NormalScale`은 머리 0.15·그 외 0.25. `NormalToLightMat = 0`. |
| `SamplePlayer/Female/SK_Player_F`, `Male/SK_Player_M` 열기 → Asset Details의 `Materials` | 기본 슬롯을 `WorldReactive/F`, `WorldReactive/M`의 재질로 연결. 여성 17개·남성 2개. |

몸의 부모 그래프는 `WorldReactive/Shared/M_VrmMToonBaseOpaque_6dd88d`에서 확인할 수 있다. 원본 속성을 Break/Make Material Attributes로 이어 받되, 색과 발광의 연결을 아래처럼 바꿨다.

```text
원본 툰 색 계산 × ICAlbedoScale(0.65) → Base Color
상수 0 → Emissive Color
원본 노멀·불투명도 등 → 대응 속성 유지
```

**0.65는 표면색 배율이며 화면 밝기 65%가 아니다.** 태양을 하나로 줄인 8번 단계에서는 배경 조명도 바뀌었다. 이후 몸·눈 재질 수정에서는 그 월드 상태를 유지했고, SkyLight·노출·후처리를 추가로 바꾸지 않았다.

## 3 눈동자가 희게 변한 문제

몸의 밝기는 좋아졌지만 눈동자와 아이라인이 희게 흐려졌다. 원본 텍스처는 정상이었고, 기존 MToon 부모 안에 색 연결 분기만 추가해도 해결되지 않았다. **원본 RGB와 알파만 사용하는 별도의 Masked Lit 부모**로 분리하자 눈이 정상적으로 보였다.

![단계별 실제 게임 캡처 · 수정 전 · 밝기 변환 후 희게 흐려진 눈](images/before_eyes.png)

수정 전 · 밝기 변환 후 희게 흐려진 눈

![단계별 실제 게임 캡처 · 최종 · 원본 눈동자와 아이라인](images/after_eyes.png)

최종 · 원본 눈동자와 아이라인

Content Browser에서 `WorldReactive/Shared/M_ICAuthoredFeatureLit`을 열어 아래처럼 구성했다. 눈 관련 Material Instance의 **Details → Parent**도 이 재질로 바꿨다.

```text
원본 diffuse RGB × 원본 tint RGB × 0.65 → Base Color
원본 diffuse Alpha × 원본 tint Alpha → Opacity Mask
Emissive / Metallic / Specular = 0
Roughness = 1
```

| Material Editor의 Details | 값 |
| --- | --- |
| Shading Model / Blend Mode / Two Sided | `Default Lit` / `Masked` / 켬 |
| Usage | `Used with Skeletal Mesh`, `Used with Morph Targets` 켬 |
| 눈 Material Instance | `ICFeatureAlbedoTexture` = 원본 diffuse, `ICFeatureAlbedoTint` = 원본 색 인자 |
| Material Property Overrides → Opacity Mask Clip Value | 약 `0.3333` |

여성은 눈·흰자·하이라이트·눈썹·속눈썹·아이라인·입에 적용했다. 남성은 눈이 합쳐진 **머리 아틀라스 슬롯 전체**에 적용했다. 눈의 RGB/알파를 별도 표면으로 분리한 것이 최종 해결책이며, 기존 MToon 내부의 단일 원인 노드까지 특정한 것은 아니다.

## 4 최종 확인

저장 후 에디터를 다시 실행해 남녀 프리셋의 **밤 외부·밤 내부·낮 외부·낮 내부**를 확인했다. 전신과 얼굴 근접을 합쳐 16장을 실제 게임 창 1600×900으로 촬영했다. 아래는 최종 여성 프리셋 전신 4장이다.

![단계별 실제 게임 캡처 · 밤 외부 · 20:00](images/night_outside.png)

밤 외부 · 20:00

![단계별 실제 게임 캡처 · 밤 내부 · 20:00](images/night_inside.png)

밤 내부 · 20:00

![단계별 실제 게임 캡처 · 낮 외부 · 10:00](images/day_outside.png)

낮 외부 · 10:00

![단계별 실제 게임 캡처 · 낮 내부 · 10:00](images/day_inside.png)

낮 내부 · 10:00

프리셋 교체 코드 `ICPlayerAppearance::Apply`에서도 메시 변경 뒤 `EmptyOverrideMaterials()`를 호출해 이전 재질이 남지 않게 했다. 최종 검증은 임시 재질·시간별 강제 밝기 보정 없이 **저장된 메시의 기본 재질**로 진행했다. 실내와 밤에도 기존 간접광이 남으므로 완전히 검게 되지는 않는다.

## 배운 점

**Lit이라는 이름만 확인하지 말고, 색이 Base Color와 Emissive 중 어디로 가는지와 알파 연결까지 봐야 한다.** 툰 캐릭터는 몸과 눈을 같은 방식으로 변환하면 표현이 깨질 수 있다. 낮밤뿐 아니라 실내외·전신·얼굴 근접·저장 후 재실행을 함께 확인해야 했다.

설정 재현: `Tools/Characters/setup_player_reactive_lighting.py` · 검증: `review_player_reactive_lighting.py`의 `-ICEyeCorrectionReview`. 재질 종류를 비교할 때는 [VRM4U 공식 안내](https://ruyo.github.io/VRM4U/01_material/)를 참고했다.
