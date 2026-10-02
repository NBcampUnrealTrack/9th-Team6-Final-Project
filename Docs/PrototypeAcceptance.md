# 팀장 구현 결과와 확인 절차

상태: 1차 구현 및 승인된 상자·F6·저장/삭제·종료 수정 완료. 검증일 2026-10-03(KST). 이번 수정은 신규 2개, 수정 16개 파일이다. 전체 게임 루프와 패키징은 담당 기능을 연결한 뒤 확인한다.

## 후속 수정: 불러오기 뒤로가기

불러오기 화면 상단에 뒤로가기(F6)를 추가했다. 가방·메뉴에서 진입하면 이전 화면으로, 생성 전에는 생성 화면으로 돌아간다. 사망 후에는 일시정지 메뉴로 돌아가며 플레이와 시계 정지를 유지한다.

Editor 빌드 및 기존 자동 검사 12개 성공. 두 맵 PIE에서 실제 위젯의 뒤로가기 버튼 이벤트와 Slate F6 입력을 사용해 생성/가방/일시정지 복귀, 사망 후 정지를 확인했다. 이번 후속 검사는 게임 프로세스 재실행 검증을 다시 실행하지 않았다.

검사 결과: [20261003_020551_264/Automation/index.json](../Saved/TeamPrototype/20261003_020551_264/Automation/index.json).

## 이번 수정 결과

| 기능 | 실제 동작 |
| --- | --- |
| 저장 화면 | 가방 저장/F5 → 수동 1~4 표시 → 버튼/키보드 선택 → Enter/저장 실행 |
| 저장 안내 | 선택 강조, 이름·로컬 저장 시각·빈/손상 상태, 슬롯별 처리/완료 메시지 |
| 선택 분리 | 수동 저장 1~4와 불러오기 0~4 독립; 자동 로드 후에도 저장 선택 유지 |
| 덮어쓰기 | 기존 캐릭터와 슬롯을 확인한 뒤 저장; 신규 생성도 자동 슬롯 교체 확인 |
| 삭제 | 선택 슬롯의 원본·백업·임시·복구 파일 확인 삭제; 현재 플레이와 다른 슬롯 유지 |
| 기존 저장 | Version 1 원본 별도 가져오기/삭제; 가져온 수동 1과 원본 삭제 독립 |
| 게임 끝내기 | 저장 후 종료 / 저장하지 않고 종료 / 취소; 쓰기 완료 대기, 실패 시 종료 보류 |
| PIE 종료 | 현재 플레이만 중지; 일반 게임 창은 게임 종료 |
| 테스트 키 | Development/Editor F6 메뉴·대상 취소; Shipping ESC |
| 상자 | 중앙 조준 우선 + 250cm 전방 탐색 + 가림 검사; HUD 대상 및 실패 사유 |

사용자가 보고한 저장 오류는 저장 버튼이 선택 화면을 열지 않고, 자동/수동 선택을 공유하며, 숫자 키가 저장 선택에 연결되지 않았던 문제였다. 이번 수정은 이 흐름을 바꾼다.

## 최종 실행 결과

검증 환경은 UE 5.8 / Win64 Development Editor, D3D11/SM5, 1280×800이다.

- Editor 빌드와 일반 Game Development 빌드 성공.
- ID 등록부 39개 + 기존 예외 24개 검증, Python 자체 테스트 8개 성공.
- Unreal Automation 12개 모두 Success, 실패/미실행 0.
- L_PlayerPrototype과 L_TestMap 모두 실제 F 입력 → HOME A/B 공유 창고 확인.
- 가방 저장 버튼 → Slate UI 숫자 4/1 선택 → Enter → 비동기 수동 저장 확인.
- 자동 슬롯 로드 뒤 수동 선택이 1로 유지되는지 확인.
- 게임 프로세스에서 수동 2 저장 후 종료 → 새 프로세스 실행 → 수동 2 복원.
- 재실행 후 장비 GUID·가방 100칸·공유 창고 수량·GameClock/PlayClock 일치 확인.
- 슬롯 삭제 취소/실행, 종료 취소/실행 확인. 미래 버전 저장 거부 후 게임 유지 확인.
- 별도 PIE 검사에서 실제 Slate F6 입력으로 메뉴 열기/닫기와 플레이 세션 종료 확인.
- 두 맵의 스크린샷 각 10장 수집. 저장·삭제 확인·종료·가방·창고 배치 확인.
- 기존 사용자 저장 6개(신규 원본 4 + 자동 백업 1 + Version 1)의 SHA256가 실행 전후 모두 동일. 테스트 슬롯은 정리됐다.

결과 폴더: [Saved/TeamPrototype/20261003_014149_206](../Saved/TeamPrototype/20261003_014149_206).

| 증거 | 파일 |
| --- | --- |
| 자동 검사 12개 | [Automation/index.json](../Saved/TeamPrototype/20261003_014149_206/Automation/index.json) |
| PIE 포함 검사 로그 | [Automation.log](../Saved/TeamPrototype/20261003_014149_206/Automation.log) |
| 프로토 맵 저장/종료 | [L_PlayerPrototype/Save.log](../Saved/TeamPrototype/20261003_014149_206/L_PlayerPrototype/Save.log) |
| 프로토 맵 재실행/삭제 | [L_PlayerPrototype/Reload.log](../Saved/TeamPrototype/20261003_014149_206/L_PlayerPrototype/Reload.log) |
| 기존 맵 저장/종료 | [L_TestMap/Save.log](../Saved/TeamPrototype/20261003_014149_206/L_TestMap/Save.log) |
| 기존 맵 재실행/삭제 | [L_TestMap/Reload.log](../Saved/TeamPrototype/20261003_014149_206/L_TestMap/Reload.log) |
| 저장 화면 | [06_Save.png](../Saved/TeamPrototype/20261003_014149_206/L_PlayerPrototype/06_Save.png) |
| 삭제 확인 화면 | [09_DeleteConfirm.png](../Saved/TeamPrototype/20261003_014149_206/L_PlayerPrototype/09_DeleteConfirm.png) |
| 종료 선택 화면 | [10_Quit.png](../Saved/TeamPrototype/20261003_014149_206/L_PlayerPrototype/10_Quit.png) |

## 자동 검사가 확인한 범위

| 테스트 | 내용 |
| --- | --- |
| WorldStateRules | ID·종류·대역·호감도·주 스탯·파티·쓰러짐·Snapshot·무효 복원 거부 |
| ParticipantRegistration | 재등록·중복/잘못된 해제 거부·교체 |
| TimeRules | 시간 비율·정지·게임 시간 점프·두 시계 복원 |
| SaveSlotsAndRoundTrip | 네 수동 슬롯 격리, 스탯·장비 GUID·파티·퀘스트·워프·엔딩·시계 |
| SaveRecoveryAndVersions | CRC·백업 복구·미래 버전·잘못된 시계/슬롯·누락/잘린 파일 거부 |
| LegacySaveConversion | Version 1 실제 파일 가져오기·원본 보존·원본 삭제와 수동 저장 독립 |
| AutoSaveSafetyAndDeferral | 안전 상태·저HP·디버프·비동기·쓰기 중 새 요청·삭제 거부·300초 |
| SaveDeletion | 모든 관련 파일 삭제·백업 유령 방지·다른 저장/현재 상태 유지·자동 타이머·손상 임시 파일 |
| InventoryRules | 스택·초과분·퀘스트·장비·창고·확장·잠금·분할·퀵슬롯·제작 원자성 |
| InteractionTarget | 가까운 정면·거리 초과·뒤쪽·벽 가림 |
| PIESession.PrototypeMap | 실제 프로토 PIE·초기 저장·Slate F6·플레이 종료와 에디터 유지 |
| PIESession.ExistingTestMap | 기존 맵 PIE에서 같은 확인 |

이전 1차 실행은 창고를 OpenContainer로 직접 열었고 같은 프로세스에서 사망/로드까지만 확인했다. 이번 결과는 F 입력과 두 프로세스 사이 저장/복원을 추가한 결과다. 이전 기록은 [20261003_000747_396](../Saved/TeamPrototype/20261003_000747_396)에 있다.

디스크 부족·쓰기 도중 강제 종료·삭제 권한 실패·실제 GAS/NPC 복원·Shipping 패키지 실행은 이번 실험에 포함하지 않았다. 종료 실패 검증은 미래 버전 파일의 저장 요청 거부를 사용했다. 비동기 디스크 실패 후 종료 보류 분기는 구현되어 있으나 강제 디스크 오류 주입은 하지 않았다.

기존 VRM4U deprecation·MovieRenderPipeline 의존성 경고, CustomDepth 경고, 여성 VRM AssetList의 없는 Male 경로 참조 및 독립 테스트 월드의 context 경고는 남아 있다. 기존 외형과 맵 에셋은 이번 수정 범위에서 변경하지 않았다.

## 팀장이 직접 확인할 순서

프로젝트 루트에서 실행한다.

~~~powershell
.\Tools\OpenPlayerPrototype.ps1 -Visible
~~~

1. 기존 저장을 이어가려면 생성 화면에서 자동/수동 슬롯 선택 → 불러오기. 새 캐릭터 생성은 기존 자동 저장 교체 확인을 거친다.
2. I 가방 → 저장 → 숫자 1~4 또는 버튼으로 선택 → Enter → 덮어쓰기 확인 → 슬롯별 저장 완료를 확인한다.
3. F6로 메뉴를 열고 불러오기 화면에서 슬롯을 선택한다. F9 대상은 HUD에 표시된다.
4. 정면 상자에 접근해 F 대상 표시를 확인하고 F로 연다. HOME A에 넣은 아이템이 HOME B에도 보이는지 확인한다.
5. 테스트할 저장을 선택해 삭제 → 취소 또는 확인. 자동 파일 삭제는 이후 자동 저장이 새로 생성될 수 있다.
6. 게임 끝내기 → 저장 후 종료로 수동 슬롯에 저장한다. 다시 실행해 해당 슬롯을 불러온다.
7. 에디터 PIE에서도 F6 메뉴와 게임 끝내기를 확인한다. ESC는 에디터의 테스트 중지 키다.
8. 실제 Version 1 원본 삭제는 가져온 수동 저장과 별개다. 삭제할 대상을 확인한다.

## 이번 생성·수정 파일

| 구분 | 파일 |
| --- | --- |
| 신규 2개 | Player/PlayerControllerSession.cpp, Player/PlayerInteractionTests.cpp |
| 저장 및 검사 3개 | Core/ICSaveSubsystem.h/.cpp, Tests/ICSaveTests.cpp |
| 컨트롤러·UI 8개 | Player/PlayerPrototypeController.h, PlayerController.cpp, PlayerControllerInput.cpp, PlayerControllerItems.cpp, PlayerControllerPersistence.cpp, PlayerControllerUI.cpp, PlayerPrototypeWidget.cpp, PlayerControllerSmoke.cpp |
| 빌드 설정 1개 | IsekaiClass.Build.cs |
| 검증 도구 1개 | Tools/RunTeamPrototypeChecks.ps1 |
| 문서 3개 | README.md, Docs/SystemContracts.md, Docs/PrototypeAcceptance.md |

C++ 경로는 Source/IsekaiClass 기준이다. 승인안의 완료 상태도 갱신했다. 1차 공용 구현 파일과 팀장 로드맵은 유지한다.

## 연결 대기와 팀장의 다음 할 일

| 담당 | 전달할 계약 / 남은 작업 |
| --- | --- |
| 1번 팀장 | 실제 이름·마감·범위 배정, ID 추가 검토, 밸런스 미정값 결정, 통합 QA |
| 2번 플레이어 | 실제 DT, 시작 지급품, 소비품 GAS 연동 정책, 수면 시간/회복, 초과분·버리기의 실제 드롭 액터 |
| 3번 전투 | 참여자에 GAS 실시간 조회·Flush·복원 연결, 공격·회피·방어·4스킬, 파생 스탯, 디버프·전투 저장 플래그 |
| 4번 생활 NPC | 일과·시간 점프 구독, ID별 액터 등록/해제, 로드 재배치·중복 스폰 방지 |
| 5번 관계·퀘스트 | 대화/이벤트 플래그, 선물·영입, 퀘스트 기록과 자동 저장 요청, 엔딩 지정 NPC 10명 확정 |
| 6번 레벨 | 워프·지역 이동·보스 등 이벤트 연결, 스트리밍, 프로토 통합·패키징 공동 확인 |
| 7번 UI | 정식 화면, String Table 이관, Snapshot/이벤트 조회 |
| 8번 애니메이션 | 무기 계열/소켓·리타겟·NPC 외형 임포트 연결 |

에디터 에셋 Validator, 실제 BGM·NPC 외형 제작, 발표 자료는 후속 승인 범위다. 수직 슬라이스의 전체 전투→보상→선물→영입→퀘스트→수면 루프와 패키징은 이 공용 기반에 각 담당 기능을 연결한 뒤 완료 기준으로 확인한다.

다른 담당에게 전달할 문서: [SystemContracts.md](SystemContracts.md), [DT_Schema.md](DT_Schema.md). 일정 전체는 [team_lead_roadmap.md](../output/team_lead_roadmap.md)를 따른다.
