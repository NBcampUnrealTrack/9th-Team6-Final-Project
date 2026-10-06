# IsekaiClass

## 프로젝트 열기

- `IsekaiClass.uproject`를 Unreal Engine 5.8에서 엽니다.
- 플레이어 기능 확인용 맵은 `/Game/PlayerPrototype/L_PlayerPrototype`입니다.
- 첫 빌드에서는 프로젝트와 플러그인 바이너리가 다시 생성되므로 시간이 걸릴 수 있습니다.

## 플레이어 코드

- `Source/IsekaiClass/Player/PlayerCharacter.*`: 캐릭터와 외형 적용
- `PlayerPrototypeController.h`: 플레이어 입력과 시스템 연결 인터페이스
- `PlayerControllerInput.cpp`: 이동, 카메라, 키 입력, 상호작용
- `PlayerControllerPersistence.cpp`: 캐릭터 생성, 저장, 불러오기
- `PlayerControllerItems.cpp`: 아이템 사용, 사망, 보관함
- `PlayerControllerUI.cpp`: UI 명령 처리
- `PlayerGameMode.*`: 클래스 설정과 테스트 월드 구성
- `PlayerGameState.*`: 플레이 단계와 공유 창고 식별자

## 2026.10.06 작업 내용

- 플레이어 기능을 캐릭터, 입력, UI, 아이템, 저장, 게임 모드·스테이트 단위로 분리
- 아이템 정의와 인스턴스를 분리하고 `ItemID`와 `ItemInstanceID` 규칙 적용
- 아이템 데이터테이블 33종과 레시피 데이터테이블 생성·연결
- 인벤토리 20칸, 장비 4슬롯, 퀵슬롯 4개, 퀘스트 인벤토리 구현
- 아이템 획득, 스택, 이동, 분할, 정렬, 잠금, 버리기와 가방 초과 드롭 처리 구현
- 인벤토리 화면을 왼쪽 캐릭터, 오른쪽 아이템 목록 구조로 변경
- 아이템 선택 시 데이터테이블의 `ItemID`와 인스턴스 정보 표시
- 경험의 서 사용과 근력·민첩·지능·체력·매력·행운 성장 처리 구현
- 포션·음식 사용, HP·MP·SP, 과로, 수면과 게임 시간 진행 구현
- 기본 요리·제작 레시피, 재료·SP 검사와 결과 아이템 지급 구현
- 창고 생성, 공유·독립 보관함 구분, 접근 권한과 입출고 처리 구현
- 월드 드롭 아이템 생성과 상호작용 획득 처리 구현
- 물리 공격력과 마법 공격력을 공격력으로 통합
- 장비 등급 정보는 유지하고 현재 능력치 계산에서는 제외
- 장비 생성 시 종류별 랜덤 옵션 1~2개 부여 및 저장
- 대장간에서 미착용 장비 강화 기능 추가
- 강화 성공률, 비용, 최대 단계와 실패 시 골드 소모 규칙 구현
- 장비 기본 능력치, 강화 수치, 랜덤 옵션을 합산해 HUD에 표시
- 강화 비용과 성공률을 아이템 데이터테이블에 연결
- 캐릭터 생성 정보, 주 능력치, 생존 수치, 골드, 인벤토리, 장비, 퀵슬롯, 창고와 강화 결과 저장·불러오기 지원
- 게임 인스턴스 기반 플레이어 월드 상태 스냅샷과 파티 대상 조회 구조 추가
- 아이템 정의·인벤토리 상태 검증과 랜덤 옵션·강화 자동화 테스트 추가
- 카메라 감도를 1로 설정하고 상하 입력 반전 적용
- 로컬 플레이어 캐릭터 에셋 연결과 장비 소켓 연결 구조 추가
Unreal Engine 5.8 프로젝트. 플레이어 기능 확인 맵은 /Game/PlayerPrototype/L_PlayerPrototype이다.

## 팀장 1차 구현

공용 WorldState·GameClock/PlayClock·수동 4슬롯/자동 1슬롯 저장을 기존 생성·인벤토리·장비·창고와 연결했다. 전투·NPC AI·퀘스트·수면 계산의 실제 기능은 담당자 연결 대기다.

- [담당별 연결 계약](Docs/SystemContracts.md)
- [ID·DT·String Table 규칙](Docs/DT_Schema.md)
- [검증 결과와 확인 순서](Docs/PrototypeAcceptance.md)
- [프로토부터 최종 발표까지 팀장 로드맵](output/team_lead_roadmap.md)

## 빌드·검증·실행

PowerShell에서 프로젝트 루트 기준으로 실행한다.

~~~powershell
# Editor Development 모듈 빌드
.\Tools\BuildTeamPrototype.ps1

# ID 검사 + 빌드 + Unreal Automation + 실제 맵 실행 검증
.\Tools\RunTeamPrototypeChecks.ps1

# 팀장이 직접 조작할 창을 연다
.\Tools\OpenPlayerPrototype.ps1 -Visible
~~~

기본 엔진 경로는 C:/Program Files/Epic Games/UE_5.8이다. 다른 설치는 각 스크립트의 -EngineRoot로 지정한다. 검증에는 Python 3가 필요하며 -PythonExe로 지정할 수 있다. 자동 검증은 숨겨진 프로세스에서 PIE와 두 맵의 게임 실행을 검사하고 종료한다. 결과는 Saved/TeamPrototype/<실행시각>에 모인다. -SkipBuild는 빌드 완료 후 재검증, -SkipSmoke는 Automation만 실행할 때 사용한다. -Maps 인자로 검사할 맵 배열을 지정할 수 있다. 기본값은 L_PlayerPrototype과 L_TestMap이다. Game Development 모듈만 빌드하려면 BuildTeamPrototype.ps1 -Target Game을 사용한다.

처음 빌드/실행은 플러그인 컴파일·셰이더 준비로 시간이 걸린다. 자동 검증의 -TimeoutSeconds 기본값은 300이다.

## 확인할 조작

WASD 이동, 마우스 시점, Space 점프, F 상호작용, I 가방, O 퀘스트, M 지도, P 동료. Development/Editor의 메뉴·대상 취소는 **F6**, Shipping은 ESC다. 에디터에서 ESC는 테스트를 중지하는 기본 키로 남는다.

소비품은 플레이 중 1~4로 선택 후 같은 숫자로 자신에게 사용, Z/X/C/V로 동료 대상 선택, F6로 취소한다. 현재 프로토에서는 시험 효과를 사용한다. 우클릭 방어 누름/해제, Shift 회피, Q/E/R/T 스킬, Z/X/C/V 동료 스킬은 전투 요청 이벤트를 전달한다.

**저장:** I 가방 → 저장 또는 F5 → 수동 1~4를 버튼/숫자로 선택 → Enter 또는 저장 실행. 기존 저장은 덮어쓰기 확인 후 처리하며 이름·저장 시각·선택 슬롯과 완료 메시지를 표시한다. 저장 선택과 불러오기 선택은 별개다. 불러오기 화면에서 자동/수동을 선택해 실행하며 F9의 현재 대상은 HUD에 표시된다. 불러오기 상단의 뒤로가기(F6)는 이전 화면으로 돌아간다. 사망 후에는 일시정지 메뉴로 돌아간다.

저장·불러오기 슬롯에는 `26.10.03 01:59 · 이세계3일차 오전6시20분`처럼 로컬 저장 날짜·시각과 해당 슬롯에 저장된 게임 날짜·시각을 함께 표시한다. 게임 시각은 오전/오후 12시간제로 표시하며 현재 플레이 중인 시간과 별개다.

**삭제:** 저장 또는 불러오기 화면에서 선택 슬롯 삭제 → 확인. 해당 슬롯의 원본·백업·임시·복구 파일을 삭제한다. 현재 캐릭터와 다른 슬롯은 유지된다. 자동 저장을 삭제해도 이후 자동 저장 조건을 만족하면 새로 만들어질 수 있다. 기존 Version 1 원본은 별도 가져오기/삭제 버튼을 사용한다.

**게임 끝내기:** 생성 화면 또는 메뉴의 게임 끝내기 → 저장 후 종료 / 저장하지 않고 종료 / 취소. 저장 후 종료는 수동 슬롯 선택과 덮어쓰기 확인을 거친다. 진행 중인 저장을 기다리며 실패하면 게임을 유지한다. PIE에서는 플레이만 중지하고 일반 게임 창에서는 게임을 종료한다.

**상자:** 정면 상자에 250cm 이내로 접근하면 HUD에 F 대상이 표시된다. 중앙 조준을 우선하며 가까운 전방 대상으로 보완한다. 벽 너머 상자는 열지 않는다. HOME A/B는 같은 창고다.

## Development 치트

콘솔에서 ICAddGameMinutes 180, ICGiveItem Food 3, ICSetAffinity NPC_R01_001 100, ICTeleport 0 0 130을 사용할 수 있다. Shipping에서는 실행 본문이 비활성화된다. 정식 아이템 ID는 ItemTable에 등록한 뒤 사용한다.

## 파일 위치

- Source/IsekaiClass/Core: 공용 타입·상태·시간·저장·참여자 인터페이스
- Source/IsekaiClass/Debug: 팀장 치트
- Source/IsekaiClass/Tests: 공용 자동 테스트
- Source/IsekaiClass/Player: 기존 플레이어·UI·인벤토리·프로토 실행 검증
- Data: ID 등록부·기존 데모 예외·밸런스 항목
- Tools: 빌드·검증·실행·ID 검사
- Saved/SaveGames/IC_Team_*.sav: 새 저장 슬롯
- Saved/SaveGames/PlayerPrototype_Auto.sav: 기존 Version 1 원본

기존 저장은 생성/저장/불러오기 화면의 'Version 1 저장 가져오기'로 빈 수동 슬롯 1에 변환한다. 원본은 보존한다. 자동 테스트는 별도 IC_Test_ 슬롯을 사용한다. 새 저장 파일을 기존 UPlayerPrototypeSave로 직접 읽지 않는다.
