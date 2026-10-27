# 플레이어 Run_Fast 대시

> 현재 조작은 Shift 유지 달리기로 변경되었다. 아래 대시 기록은 이전 구현 이력이다.
> 현재 설정은 `Player_Sprint.ko.md`와 `setup_player_sprint.py`를 참조한다.

Shift의 기존 Dodge 행동에 빠른 달리기 애니메이션을 연결한다.
남녀 각각 맨손·검은 Sword_Animations, 활은 Archer, 스태프·완드는 Mage의
비 Root Motion Run_Fast_Loop을 대상 스켈레톤으로 리타겟팅한다.

기존 Dodge 슬롯을 사용하며 구르기나 별도 키는 추가하지 않는다.
이동 거리·무적 시간·쿨타임·SP 정책은 기존 전투 설정을 따른다.
대시 방향으로 캐릭터를 돌리고, 클립 재생 속도를 대시 시간에 맞춘다.
종료와 취소 시 몽타주를 정리하며, 애니메이션 종료가 무적 시간을 결정하지 않는다.
루트 모션은 끄고 기존 GAS 위치 이동을 유지한다.
쿨타임 중의 추가 대시 입력은 현재 행동을 취소하기 전에 거절한다.
GAS 기본 Tick 판단에 캐릭터 준비 상태를 포함하여 자체 행동·효과 타이머를 갱신한다.

## 에셋 생성

1. IsekaiClassEditor를 빌드한다.
2. Unreal Python에서 Tools/Characters/setup_player_dash.py를 실행한다.
3. Saved/PlayerDashSetup.json의 complete와 남녀 프로필 연결 결과를 확인한다.

스크립트는 플레이어 프로필의 Dodge만 변경하고 기존 공격·무기 전환 설정은 보존한다.
기존 setup_assets.py와 setup_weapon_switch.py도 생성된 Dodge 연결을 유지한다.

## 플레이 확인 항목

- 남녀, 맨손과 네 무기 상태에서 Shift로 빠른 달리기 동작 재생
- 이동 방향 및 정지 상태에서 방향 일치
- 종료·취소 후 지상 이동으로 복귀
- 벽 충돌, SP 미소모, 3초 쿨타임, 기존 무적 시간 유지
- 메뉴를 열면 이동과 애니메이션이 함께 정지

에셋 생성 성공과 실제 플레이의 시각적 품질 확인은 별개다.

## 2026-10-24 확인 결과

- IsekaiClassEditor 빌드 성공.
- 리타겟팅 클립 6개와 몽타주 6개 생성, 남녀 프로필 저장.
- 별도 Unreal 프로세스에서 10개 상태의 저장된 연결, 스켈레톤 일치,
  클립의 루트 모션 비활성화와 루트 고정 확인 성공.
- 생성 시 원본 Archer 데모의 ABP_Manny_PostProcess에서 Pose Asset 누락 오류 발생.
  Python 생성은 완료됐으며, 생성 결과만 읽는 별도 검증 프로세스는 종료 코드 0.
- 실제 플레이 확인 도구: `Tools/Characters/review_player_dash.py`.
  격리된 저장 슬롯과 PIE에서 실행하며 결과는 `Saved/PlayerDashLiveReview.json`,
  화면은 `Saved/LiveCharacterReview/dash_*.png`에 기록한다.
- 렌더링된 PIE에서 남녀 × 맨손·검·활·스태프·완드 10개 상태 모두 통과.
  지정 몽타주 시작·중간 재생·종료, 300cm 이동, SP 미소모, 연속 입력 차단 확인.
  대표 대시 중간 화면과 종료 후 대기 화면을 직접 확인했다.
- 확인 중 발견한 연속 입력 시 기존 대시 취소와 GAS Tick 비활성 문제 수정.
- 벽 충돌, 메뉴 일시정지, 지형별 시각 품질은 이번 렌더링 확인 범위에 포함하지 않았다.
