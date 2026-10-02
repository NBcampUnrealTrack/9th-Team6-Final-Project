# 팀장 업무 및 구현 승인안

상태: 사용자가 "1차 구현안 승인"으로 승인했다. 승인 범위의 구현과 검증을 진행했으며 최종 결과는 Docs/PrototypeAcceptance.md에 기록한다. 아래는 승인 당시 범위와 후속 계획이다.

기준: 2026-10-02 현재 저장소 HEAD `e85c0f1`, `IsekaiClass_기획서_최종_1.pdf`, 현재 `Source/IsekaiClass/Player` 코드. 기획서의 1번 팀장 업무를 구현 대상으로 삼는다.

## 1. 승인 단위

이번 승인 대상은 **1차: 프로토타입 공용 기반 구현과 기존 플레이어 기능 연결**이다.

포함: 공용 상태, 두 시계, 수동 4슬롯·오토 1개 저장, 저장 안전장치, 치트, ID·데이터 계약 문서, CSV 검증 도구, 기존 플레이어 연결, 입력 불일치 수정, 빌드·자동 검증 도구.

2차 이후의 에디터 Validator, 실제 BGM·NPC 외형, 발표 자료는 전체 업무 목록에 기록하되 이번 구현 승인에 포함하지 않는다. 실제 외형·음원·발표 내용은 별도 선택과 원본 자료가 필요하다.

기존 플레이어의 캐릭터 생성·인벤토리·장비·창고 기능을 이어서 사용한다. 다른 담당의 전투·GAS·NPC AI·퀘스트·레벨 기능은 연결 계약과 저장 구조를 준비하는 범위다. 이 승인만으로 그 기능들 전체가 완성되는 것은 아니다.

## 2. 네가 해야 하는 업무 전체

| 번호 | 팀장 업무 | 이번 승인 | 어떻게 진행하는가 |
| --- | --- | --- | --- |
| T1 | 공용 데이터·함수 계약 | 포함 | 상태 구조, 조회·변경 함수, 이벤트, 데이터 소유권을 문서와 헤더로 고정 |
| T2 | WorldState | 포함 | 플레이어·NPC·파티·호감도·인벤토리 등의 상태를 GameInstance Subsystem에 관리 |
| T3 | Time | 포함 | GameClock·PlayClock, 일시정지, GameClock 점프 이벤트 |
| T4 | Save | 포함 | 4개 수동 슬롯·1개 오토 슬롯, 버전, 보류, 백업, 검증·복원 |
| T5 | 치트 콘솔 | 포함 | 시간 점프·아이템 지급·호감도 설정·순간이동 |
| T6 | ID_Registry·DT 규칙 | 포함 | CSV 등록부, 번호 대역, 스키마·참조 규칙, 파일 기반 검증 |
| T7 | 기존 플레이어 연결 | 포함 | 기존 저장·시험 스탯·인벤토리를 공용 상태와 저장으로 연결 |
| T8 | QA·빌드·통합 | 포함 | 엔진 빌드·자동 테스트 스크립트, 수동 검증 체크리스트 |
| T9 | 에디터 자동 Validator | 2차 | 에디터에서 DataTable·참조·ID 형식을 검사하는 전용 모듈 |
| T10 | 밸런스 시트·범위 결정 | 1차는 항목·현재 가값 정리 | 공용 수치 항목을 정의하고 전투·생활 수치는 담당 결과로 갱신 |
| T11 | 사운드 | 후속 | R01 2곡부터 총 BGM 9곡·효과음 목록, 실제 음원 선정·연결 |
| T12 | NPC 외형 | 후속 | 발표용 외형 우선, 확정된 제작 수량과 VRoid 파생·8번 임포트 연동 |
| T13 | 현지화 규칙 | 1차는 규칙 작성 | 신규 표시문자열의 String Table 규칙과 키 규격 정의; 전체 기존 UI 현지화는 7번과 후속 |
| T14 | 발표·영상·리허설 | 후속 | 담당별 결과 수집, 고정 빌드 시연·영상·슬라이드·백업 |

팀장이 직접 정할 것: 마감·기능 동결일, 작업 단위의 책임자, 최종 엔딩 NPC 10명, 부록 A의 미정 규칙, 콘텐츠 축소 여부. 이 문서의 코드를 승인해도 미정 기획을 자동으로 확정하지 않는다.

## 3. 현재 파일에서 확인한 점

| 현재 파일 | 확인한 동작 | 이번 변경 이유 |
| --- | --- | --- |
| `PlayerInventory.h/.cpp` | 가방·장비·퀵슬롯·창고가 ActorComponent에 있고 함수들이 직접 변경 | 원본 상태를 WorldState에 모으고 기존 조작 로직을 연결 |
| `PlayerPrototypeController.h` | TestHP·TestSP·TestStrength와 플레이어 생성 정보 보유 | 같은 상태를 UI·저장이 따로 보유하지 않도록 공용 조회·변경으로 이동 |
| `PlayerControllerPersistence.cpp` | 단일 슬롯에 동기 저장; 플레이어 위주 데이터 | 공용 SaveSubsystem과 다중 슬롯·오토 저장으로 전환 |
| `PlayerSaveGame.h/.cpp` | Version=1, 기존 프로토 슬롯 이름 | 기존 파일을 읽는 호환 경로로 활용 |
| `PlayerControllerInput.cpp` | Shift=방어, G=회피, 우클릭=대상 취소, T 스킬 입력 없음 | 최종 기획의 우클릭 방어·Shift 회피·ESC 취소·Q/E/R/T로 맞춤 |
| `PlayerControllerItems.cpp` | 시험 포션·음식·근력 서가 컨트롤러 값을 변경 | 아이템 처리 흐름을 유지하며 공용 상태로 기록 |
| `PlayerPrototypeWidget.cpp` | 컨트롤러·인벤토리 필드를 직접 읽고 단일 저장·로드 버튼 표시 | Snapshot 조회, 시계·저장 상태·슬롯 목록 표시 |
| `Tools/OpenPlayerPrototype.ps1` | 기본 엔진 경로가 현재 존재하지 않는 F 드라이브 경로 | 현재 설치 경로 탐지와 명시적 -EngineRoot 지원 |

현재 확인한 엔진 폴더: `C:/Program Files/Epic Games/UE_5.8`. 실제 컴파일·실행은 승인 후 진행한다.

## 4. 구현 방법과 경계

### T1·T2: 공용 상태

새 구조체에 플레이어 생성 정보, 주 스탯 6개, HP/MP/SP, NPC 호감도·모드·파티 슬롯, 장비, 인벤토리, 컨테이너, 퀘스트 진행 상태, 워프·처치 시각·엔딩 기록을 담을 수 있게 한다.

- 상태 원본은 `UICWorldStateSubsystem`이 가진다.
- UI는 복사된 Snapshot을 읽는다.
- 상태 변경은 서브시스템 함수로만 확정한다.
- 인벤토리의 슬롯·스택·장착 등 기존 규칙은 2번 영역의 `UPlayerInventory`가 처리한다. 임시 상태에 변경·검증한 결과를 WorldState에 확정한다.
- 기존 인벤토리 구조체 이름을 유지해 장비 인스턴스와 기존 저장 데이터의 의미를 보존한다. 정의 위치는 공용 헤더로 옮긴다.
- GAS 연결 전에는 기존 시험 효과가 공용 상태를 변경한다. GAS 구현은 3번 영역이며, 참여자 인터페이스를 통해 실시간 조회·저장 전 Flush·로드 후 복원을 연결할 수 있게 한다.
- NPC 액터의 생성·일과·동료 행동은 구현하지 않는다. ID별 참여자 등록에서 중복 등록을 감지하고 다른 파트에 연결 계약을 제공한다.

주요 API 계약:

| API | 역할 |
| --- | --- |
| `GetSnapshot(CharacterId)` | 살아 있는 참여자의 최신 전투 상태 또는 저장된 상태 반환 |
| `SetAffinity(NpcId, Value)` | 0~100 범위 검증, 변경 이벤트 |
| `SetMainStat(CharacterId, Stat, Value)` | 스탯 범위 검증과 변경 이벤트; 파생 계산은 전투 담당이 구독 |
| `SetCharacterMode/SetPartySlot` | 모드·파티 상태 변경, 최대 4명·중복 슬롯 검사 |
| `GetInventorySnapshot/CommitInventoryState` | 가방·장비·퀵슬롯·창고 조회와 검증된 변경 확정 |
| `TryAddItem(ItemId, Count)` | 수납되지 않은 수량 반환; 구매·보상 보류 등 정책은 호출 측 소유 |
| `RegisterStateParticipant/UnregisterStateParticipant` | 활성 액터/컴포넌트의 상태 조회·Flush·복원 등록 |
| `ExportState/ValidateState/RestoreState` | 저장 데이터 추출과 적용 전 검증 |

현재 자동 삭제되는 인벤토리 초과분은 새 반환값/초과 이벤트로 확인할 수 있게 한다. 실제 드롭 액터 생성·습득·삭제는 2번 소유이며 이번에는 그 연결 지점까지 제공한다. 기존 버리기 기능의 월드 드롭 전환 역시 2번 후속 작업으로 문서에 남긴다.

### T3: 시간

- GameClock 단위: 게임 분. 현실 2초마다 게임 1분, 하루 48분.
- PlayClock 단위: 플레이 초.
- 새 게임·생성 화면·메뉴·사망·일시정지에서의 진행 여부를 명시적으로 관리한다. 생성·메뉴·사망 중에는 진행하지 않는다.
- `JumpGameMinutes`는 GameClock만 변경하고 `OnTimeJumped(from, to)`를 보낸다.
- NPC 일과·농사·SP 구독용 이벤트와 PlayClock 타이머용 조회 함수를 공개한다.
- 실제 부족 SP로 수면 시간을 계산하고 회복시키는 처리는 2번 소유다. 1차는 그쪽이 호출할 시간 점프와 QA 치트를 제공한다.
- 저장과 로드에 두 시계 모두 포함한다.

### T4: 저장·로드

- 수동 슬롯 1~4, 오토 슬롯 1개. 목록에는 오토 1개만 보이고 직전 오토 백업을 내부 보관한다.
- 캐릭터 생성 직후 초기 오토세이브, PlayClock 300초 기본 간격.
- 수면·워프·지역 이동·퀘스트·보스 이벤트는 다른 담당이 `RequestAutoSave(Reason)`를 호출할 수 있게 한다. 해당 시스템이 아직 없으면 실제 이벤트가 자동 발생하는 것으로 표시하지 않는다.
- 전투·대화·이벤트·사망·아이템 효과 처리 중 저장을 금지/보류한다. 오토세이브는 낮은 HP·활성 디버프에서도 보류한다. HP 임계값은 프로토 설정값으로 공개한다.
- 스냅샷 생성과 UObject 접근은 게임 스레드, 파일 쓰기는 비동기로 처리한다. 저장 완료·실패 이벤트를 UI에 전달한다.
- 임시 파일 작성과 읽기 검증이 성공한 뒤 정상 파일을 교체한다. 교체 실패 시 정상본/백업으로 복구 가능하도록 한다.
- 로드 데이터는 전부 검증한 뒤 적용한다. 잘못된 파일을 불러와 현재 게임 상태가 부분 변경되지 않게 한다.
- 저장 파일에 SaveVersion을 넣고, 향후 버전을 알 수 없으면 안내 후 거부한다.
- 기존 `PlayerPrototype_Auto` Version 1은 읽기 호환 경로를 유지한다. 최초 가져오기는 수동 슬롯 1이 비어 있을 때만 새 형식으로 저장하며 원본은 보존한다. 자동 테스트는 별도 슬롯 이름을 사용한다.

### T5: 치트

`UICDebugCheatManager`를 연결해 Development/Editor에서 다음 명령을 제공한다. Shipping에서는 실행하지 않는다.

| 명령 | 예시 목적 |
| --- | --- |
| `ICAddGameMinutes <Minutes>` | 수면·일과 시간 점프 이벤트 확인 |
| `ICGiveItem <ItemId> <Count>` | 보상·가방 공간·소비품 테스트 |
| `ICSetAffinity <NpcId> <0..100>` | 상태 저장·영입 조건 연결 테스트 |
| `ICTeleport <X> <Y> <Z>` | 필드·앵커·지역 이동 검증 |

NPC AI·영입이 아직 구현되지 않았으면 호감도 값만 변경된다. 실제 시스템의 연결 상태를 문서에 명시한다.

### T6·T10·T13: 데이터·수치·문자열 규칙

- `ID_Registry.csv`에 ID 종류, 담당, 번호 대역, 상태, 비고를 기록한다.
- 신규 ID는 기획서 규격을 따른다. 기존 `Sword`, `Food`, `PlayerHome` 등은 임시 허용 목록에 명시하고 이번에는 일괄 이름 변경하지 않는다.
- 파일 기반 검증은 ID 형식·중복·폐기 ID 재사용·등록부/CSV 참조 누락을 검사한다.
- NPC·아이템·퀘스트·스킬 스키마는 필수 컬럼과 소유권을 문서화한다. 각 담당이 기능별 DT를 만들고 데이터를 입력한다.
- 밸런스 템플릿은 현재 가값과 미정값을 구분한다. 전투 공식·선물 효율·최소 수면 등 미정 기획을 임의 확정하지 않는다.
- String Table 이름·키 규칙을 문서로 배포한다. 전체 기존 UI 텍스트를 String Table 에셋으로 옮기는 것은 이번 범위에 포함하지 않는다.

## 5. 이번 승인으로 생성할 파일

아래 경로는 모두 프로젝트 루트 `C:/Users/20ori/Desktop/IsekaiClass` 기준이다.

| 생성할 파일 | 들어갈 내용 |
| --- | --- |
| `Source/IsekaiClass/Core/ICInventoryTypes.h` | 기존 아이템 정의·인스턴스·컨테이너 구조체를 공용 위치로 이동; 기존 타입 이름 유지 |
| `Source/IsekaiClass/Core/ICWorldStateTypes.h` | 캐릭터·NPC·파티·인벤토리·월드·시계·저장 상태 구조체와 enum |
| `Source/IsekaiClass/Core/ICStateParticipant.h` | 최신 Snapshot, 저장 전 Flush, 로드 후 복원 인터페이스 |
| `Source/IsekaiClass/Core/ICWorldStateSubsystem.h` | 상태 조회·변경·이벤트·참여자 등록 API |
| `Source/IsekaiClass/Core/ICWorldStateSubsystem.cpp` | 공용 상태 관리·검증 구현 |
| `Source/IsekaiClass/Core/ICTimeSubsystem.h` | 두 시계·게임 시작/정지·점프·이벤트 API |
| `Source/IsekaiClass/Core/ICTimeSubsystem.cpp` | 시계 진행·정지·점프·저장 복원 구현 |
| `Source/IsekaiClass/Core/ICSaveGame.h` | 새 공용 USaveGame 클래스·버전·슬롯 메타데이터 |
| `Source/IsekaiClass/Core/ICSaveGame.cpp` | 새 저장 모델의 초기값과 버전 정보 |
| `Source/IsekaiClass/Core/ICSaveSubsystem.h` | 저장·로드·슬롯 목록·안전 상태·오토 요청 API |
| `Source/IsekaiClass/Core/ICSaveSubsystem.cpp` | 직렬화·비동기 파일 저장·백업·보류·검증·기존 저장 가져오기 |
| `Source/IsekaiClass/Debug/ICDebugCheatManager.h` | 팀장용 Exec 명령 선언 |
| `Source/IsekaiClass/Debug/ICDebugCheatManager.cpp` | 시간·아이템·호감도·순간이동 명령 구현 |
| `Source/IsekaiClass/Tests/ICWorldStateTests.cpp` | 상태 제한·파티·Snapshot·참여자 중복·이벤트 검증 |
| `Source/IsekaiClass/Tests/ICTimeTests.cpp` | 48분 하루·정지·점프·저장 복원 검증 |
| `Source/IsekaiClass/Tests/ICSaveTests.cpp` | 슬롯 격리·원형 복원·버전·손상·백업·보류·기존 파일 가져오기 검증 |
| `Data/ID_Registry.csv` | 공용 ID 등록부와 번호 대역 |
| `Data/Legacy_ID_Allowlist.csv` | 기존 데모 ID의 명시적 임시 예외 |
| `Data/BalancePrototype.csv` | 현재 가값·확정 수치·미정 수치를 구분한 밸런스 템플릿 |
| `Docs/SystemContracts.md` | 담당별 호출 API, 상태 원본, 이벤트·인계 규칙 |
| `Docs/DT_Schema.md` | NPC·아이템·퀘스트·스킬 DT 필수 컬럼·ID·String Table 규칙 |
| `Docs/PrototypeAcceptance.md` | 팀장 수동 검증 절차, 구현/연결 대기 항목 |
| `Tools/ValidateIds.py` | 등록부·내보낸 CSV의 ID·참조 검증 |
| `Tools/BuildTeamPrototype.ps1` | UE 경로 인자를 받아 프로젝트 모듈 빌드 |
| `Tools/RunTeamPrototypeChecks.ps1` | ID 검증·Automation·프로토 실행 검증과 결과 수집 |

## 6. 이번 승인으로 수정할 파일

| 수정할 파일 | 변경 내용 |
| --- | --- |
| `Source/IsekaiClass/Player/PlayerInventory.h` | 타입 헤더 분리, Snapshot 조회·TryAddItem·공용 상태 연결 선언 |
| `Source/IsekaiClass/Player/PlayerInventory.cpp` | 기존 조작 규칙을 WorldState 조회·확정 구조에 연결; 직접 상태 변경 지점 정리 |
| `Source/IsekaiClass/Player/PlayerPrototypeController.h` | 공용 상태·저장 접근, 참여자 인터페이스, 선택 슬롯·저장 안전 상태 선언 |
| `Source/IsekaiClass/Player/PlayerController.cpp` | 초기화·상태 이벤트·메뉴 시간 정지·치트 매니저 연결 |
| `Source/IsekaiClass/Player/PlayerControllerPersistence.cpp` | 단일 슬롯 직접 저장을 SaveSubsystem 호출로 교체; 새 게임·복원 연결 |
| `Source/IsekaiClass/Player/PlayerControllerItems.cpp` | 시험 아이템 효과를 공용 상태에 기록; 취소 안내를 ESC로 맞춤 |
| `Source/IsekaiClass/Player/PlayerControllerInput.cpp` | 우클릭 방어·해제, Shift 회피, Q/E/R/T 슬롯, ESC 대상 취소; F5/F9 선택 슬롯 처리 |
| `Source/IsekaiClass/Player/PlayerControllerUI.cpp` | UI 명령을 Snapshot 기반 조작·슬롯 선택·저장 요청으로 연결 |
| `Source/IsekaiClass/Player/PlayerPrototypeWidget.cpp` | Snapshot 조회, 시계, 수동 4슬롯·오토 목록, 저장 중/실패 표시; 기존 화면 배치 유지 |
| `Source/IsekaiClass/Player/PlayerControllerSmoke.cpp` | 기존 생성·창고·장비·저장 검증을 새 구조로 갱신; 테스트 전용 슬롯 사용 |
| `Source/IsekaiClass/Player/PlayerInventoryTests.cpp` | 공용 상태 테스트 환경, 기존 인벤토리 규칙·기존 저장 직렬화 검증 갱신 |
| `Tools/OpenPlayerPrototype.ps1` | 실제 엔진 경로 탐지, -EngineRoot 지원, 실행 경로 안내 |
| `README.md` | 팀장용 빌드·검증·치트·저장·연결 방법 추가 |

기존 `PlayerSaveGame.h/.cpp`는 Version 1 읽기용으로 그대로 남긴다. 공용 시스템 파일에서 그 클래스를 참조한다. 이번에는 `.uproject`, 렌더링 설정, 기본 맵, VRM 플러그인, 기존 `.uasset/.umap`을 변경하지 않는다.

플레이어 담당 파일을 수정하는 이유는 상태·저장·입력 계약을 연결하기 위해서다. 인벤토리 기능 설계, 요리·제작·강화, 전투 AI 등의 작업을 새로 인수하는 범위가 아니다.

## 7. 구현 순서

1. 기존 데이터 타입을 공용 헤더로 이동하고 현재 코드의 컴파일 상태 확인.
2. WorldState·Snapshot·변경 이벤트·참여자 API 구현.
3. 기존 인벤토리·시험 스탯·UI를 공용 상태에 연결.
4. Time 구현과 메뉴·새 게임·로드 연결.
5. Save 모델·4+1슬롯·기존 저장 호환·오토세이브·백업 구현.
6. 입력 불일치 수정과 치트 연결.
7. 등록부·스키마·수치 템플릿·검증·빌드 도구 작성.
8. 자동 테스트·프로토 실행 확인, 결과와 연결 대기 항목 기록.

## 8. 네가 확인할 완료 기준

- [ ] 생성·인벤토리·장비·창고가 기존 프로토 맵에서 동작한다.
- [ ] WorldState 변경이 UI Snapshot과 저장에 동일하게 반영된다.
- [ ] 메뉴에서 두 시계가 멈추고, 시간 점프는 GameClock만 바뀐다.
- [ ] 수동 4개 슬롯이 서로 섞이지 않고 오토는 목록에 1개만 보인다.
- [ ] 저장→상태 변경→로드 후 상태와 두 시계가 돌아온다.
- [ ] 전투·대화·이벤트 중 저장이 거부/보류되고, 안전 상태 후 오토 저장이 실행된다.
- [ ] 잘못된 저장은 현재 상태를 바꾸지 않는다. 백업으로 복구 가능한 경우 확인한다.
- [ ] 기존 Version 1 저장은 원본을 보존하며 가져올 수 있다.
- [ ] Development 치트 4종이 작동하고 잘못된 ID·인자는 안내한다.
- [ ] 우클릭 방어, Shift 회피, Q/E/R/T 요청, ESC 취소가 기획서와 일치한다. 실제 공격 판정·스킬은 3번 연결 대기 상태를 표시한다.
- [ ] ID 검증과 관련 자동 테스트, 프로젝트 모듈 컴파일 결과를 기록한다.
- [ ] 실제 NPC·전투·퀘스트·수면·드롭·패키징 중 다른 파트가 연결해야 할 항목을 분리해 인계한다.

완료 후 직접 확인할 순서: 기존 프로토 맵 실행 → 캐릭터 생성 → 시계 정지/재개 → 아이템·창고 조작 → 치트로 상태 변경 → 수동 슬롯별 저장·로드 → 오토 보류·실행 → 기존 저장 가져오기. 실제 전체 게임 루프와 패키징은 다른 담당 기능이 통합된 뒤 6번과 공동 확인한다.

## 9. 승인 이후의 팀장 업무와 예상 파일

아래는 다음 단계의 계획이며 이번 1차 승인 대상이 아니다.

| 후속 업무 | 생성/수정할 파일 또는 에셋 | 진행 방법 |
| --- | --- | --- |
| 에디터 Validator | `Source/IsekaiClassEditor/IsekaiClassEditor.Build.cs`, `IsekaiClassEditor.h/.cpp`, `Validation/ICDataTableValidator.h/.cpp`; `.uproject`, `Source/IsekaiClassEditor.Target.cs` | 에디터 모듈을 추가해 실제 DT 행·참조·ID를 에셋 저장/검증 시 검사 |
| 전체 저장 범위 확장 | `Core/ICWorldStateTypes.h`, `Core/ICSaveGame.*`, `Core/ICSaveSubsystem.*`, `Tests/ICSaveTests.cpp` | 퀘스트·드롭·자원·보스·엔딩 실제 기능과 참여자 연결, 버전 마이그레이션 추가 |
| 사운드 | `Data/Audio_Registry.csv`, `Content/IsekaiClass/Audio/BGM/`, `Content/IsekaiClass/Audio/SFX/`, 필요 시 `Source/IsekaiClass/Audio/ICAudioSubsystem.h/.cpp` | 보유 음원과 출처 확인 후 지역·전투·보스 전환, 볼륨·효과음 연결 |
| NPC 외형 | `Data/NPC_Appearance_Registry.csv`, `Docs/VRoidAppearancePipeline.md`, `Content/IsekaiClass/Characters/NPC/` | 지정 NPC·발표 등장 NPC 우선 제작, 8번 임포트·소켓·공용 모션 규격 적용 |
| 밸런스·범위 확정 | `Data/BalancePrototype.csv`, `Docs/ScopeDecisions.md` | 플레이테스트 수치를 반영하고 부록 A 결정·축소 항목 기록 |
| 패키징·발표 | `Docs/PresentationChecklist.md`, `Tools/PackageTeamPrototype.ps1`, `output/presentations/`, `output/videos/` | 6번 패키징 계약·발표 맵을 확정하고 실제 화면·클립으로 자료 제작 |

## 10. 승인 문구

`1차 구현안 승인`이라고 답하면 이 문서 5·6장의 파일 범위와 4·7·8장의 동작·검증 기준대로 구현한다.

수정하고 싶은 경우 예: `저장은 다음으로 미루고 WorldState·Time만 승인`, `입력 수정은 빼줘`, `기존 플레이어 파일 수정 범위를 줄여줘`.

소스 구현은 승인 이후 시작한다.
