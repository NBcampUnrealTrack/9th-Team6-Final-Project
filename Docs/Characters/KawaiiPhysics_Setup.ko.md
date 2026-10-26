# Kawaii Physics 적용 — 2026-10-23

남녀 플레이어와 R01 NPC 11명의 머리·옷 보조 물리를 **Kawaii Physics
1.21.0**으로 전환했다. 13개 `ABP_Post_*`의 VRM SpringBone 노드를
제거하고 Kawaii Physics 노드 43개로 교체했다. 총 445개 기존 스프링
루트를 옮겼으며, VRM 표정 블렌드와 Constraint 노드 및 입력/출력은 유지했다.
이전 VRM 물리 수정 문서는 변경 이력이며 현재 보조 물리 실행 방식과 다르다.

## 설정

Kawaii Physics는 이미 프로젝트의 `Plugins/KawaiiPhysics`에 설치되어 있다.
프로젝트 플러그인 목록에 활성화를 명시하고, 각 캐릭터가 원래 사용하던
post-process 그래프에 직접 연결했다. 설정은 해당 ABP 노드에서 편집할 수 있다.

| 분류 | 강성 | 감쇠 | 중력 Z (cm/s²) | 회전 제한 |
|---|---:|---:|---:|---:|
| 긴 머리 | 0.025 | 0.18 | -200 | 55° |
| 짧은 머리 | 0.08 | 0.30 | -120 | 25° |
| 치마·코트 자락 | 0.08 | 0.25 | -220 | 45° |
| 소매 | 0.14 | 0.30 | -160 | 35° |
| 가슴 | 0.15 | 0.30 | -40 | 15° |
| 기타 장식 | 0.14 | 0.30 | -160 | 35° |

뿌리에서 끝으로 강성 배율을 1.5→0.5로 낮춰 끝이 더 부드럽게 따라오게 했다.
기존 VRM 메타데이터의 몸 충돌 그룹을 Kawaii 구·캡슐로 변환했다. 좌표와
미터→센티미터 단위를 변환하고 캡슐의 중점·축·길이를 보존한다. 앞서
보완한 치마·소매 연결과 허벅지 충돌체도 사용한다. VRM 중력 값을 조정하던
중간 실험은 되돌렸으며 현재 움직임은 Kawaii 노드 설정으로 결정된다.

고정 60Hz 계산과 프레임당 최대 8개 substep을 사용한다. 실제 스킨 본의
길이를 유지하도록 가상 본 분할과 끝 dummy는 사용하지 않는다. 레벨 충돌은
사용하지 않으며, 요청대로 침대나 수면 모션 에셋은 변경하지 않았다.

## 고정·늘어남 원인 수정

`AICCharacterBase::UpdatePresentationTransform`이 매 프레임
`TeleportPhysics`로 메시 변환을 설정하고 있었다. UE의 SkeletalMesh는
변환이 같아도 이 경로에서 애니메이션 물리를 초기화할 수 있다. 평소 갱신은
`None`으로 바꾸고, 기존 취침/기상 전환과 큰 위치 이동의 명시적 초기화는
유지해 머리 물리가 프레임 사이에 이어지게 했다.

Kawaii의 고정 계산 사이에 물리 step이 없는 렌더 프레임에서는 현재 뿌리
위치와 이전 물리 자식 위치가 섞여 본 길이가 달라졌다. 해당 프레임의 출력만
현재 뿌리에서 길이를 보존하도록 투영한다. 물리 시간과 위치 이력은 바꾸지 않는다.
이 수정으로 120/144Hz에서도 머리·옷이 늘어나지 않는 것을 수치로 확인했다.

## 검증과 재현

- Editor 빌드 성공.
- `verify_assets.py`: 에셋·데이터 검사 373개 통과.
- `ReviewSecondaryMotion`: 13개 메시 × 30/60/120/144Hz × 6상황,
  총 312개 측정 통과. 최대 본 길이 오차 0.000001cm, 대기 중 보조 본의
  최대 프레임 간 이동 약 3.39cm. 초기화 후 현재 입력 자세 일치도 검사한다.
- `ReviewHairDrape`: 여자 NPC 8명 × 8/30/60/120Hz의 32개 측정 통과.
  같은 입력 자세에서 중력 유무를 비교했다. 각 경우의 최대 머리 끝 하강은
  약 1.12~16.71cm이며 본 길이는 유지된다. 매 프레임 실제 캐릭터의
  presentation 갱신도 호출하여 반복 초기화 회귀를 검사한다.
- `IC.Characters` 자동 회귀 6개 모두 통과. 최종 보고서는
  `Saved/KawaiiPhysicsReview/AutomationFinal`에 있다.
- 실제 `L_TestMap` PIE에서 남녀 플레이어와 NPC 11명의 걷기·달리기
  26개 상황을 실행하고, 앞·뒤 시점의 208개 프레임을 남겼다. 플레이어는
  135/450cm/s, NPC는 180/600cm/s로 실제 이동·애니메이션 경로를 사용했다.
  확인한 화면에서 비정상적인 머리·옷 늘어남이나 폭발적인 튐은 관찰하지
  못했다. 긴 머리는 이동을 따라 흔들리며, 짧은 머리와 옷자락도 움직인다.
  접촉 시트는 `Saved/KawaiiPhysicsReview/ReviewSheets`, 원본과 이동 속도
  기록은 `Saved/LiveCharacterReview`의 `KAWAII_` 캡처와 `samples.json`에 있다.

수치 결과는 `Saved/SecondaryMotionReview/KawaiiFinal/Measurements.csv`,
`Saved/HairDrapeReview/KawaiiFinal/HairGravity.csv`에 있다.
그래프 적용 목록은 `Saved/KawaiiPhysicsReview/SetupApplied.csv`다.
`VRMNodes`는 13종 모두 0이다.

Unreal Python에서 `Tools/Characters/setup_kawaii_physics.py`를 실행하면
현재 그래프를 검사한다. `IC_APPLY_KAWAII=1`일 때만 저장한다. 최초 그래프
변경 전 패키지는 `Saved/KawaiiPhysicsReview/OriginalAssets`에 백업했다.
기존 `IC Kawaii <분류>` 노드는 도구를 다시 적용하면 위 설정으로 갱신되므로,
수동 튜닝을 보존하려면 기본 검사 모드를 사용한다.

자동 회귀 이름은 `IC.Characters.HairGravityAndLowFrameRate`와
`IC.Characters.SecondaryMotionResetAndLengths`다. 실제 렌더링한 PIE는
`live_character_review.py`로 재현하며 `KAWAII_` 접두사의 캡처를 사용한다.

[Kawaii Physics 공식 문서](https://github.com/pafuhana1213/KawaiiPhysics)는
그래프 노드, 충돌체 및 파라미터의 사용법을 설명한다.
