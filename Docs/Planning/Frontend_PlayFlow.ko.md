# 시작 UI와 첫 플레이 연결

게임을 실행하면 `L_Title`에서 시작한다. `시작하기 → 이름 → 성별 → 이야기 → 음악 → 행복했던 순간 → 답변 확인 → 로딩 → L_TestMap → 최초 자동 저장 → 플레이` 순서다.

메인 메뉴 버튼은 `시작하기 / 이어하기 / 설정 / 그만하기` 네 개다. 이번 단계에서 시작하기와 그만하기를 연결했고, 이어하기와 설정은 비활성 상태다. 플레이 중 P로 여는 기존 기본 설정은 별도로 작동한다.

## 에디터에서 수정하는 곳

| 위치 | 역할 |
| --- | --- |
| `/Game/IsekaiClass/UI/Widget/WBP_Mainmenu` | 제목과 메인 메뉴 버튼 4개 |
| `/Game/IsekaiClass/UI/Widget/WBP_CharacterCreate` | 질문 5개와 답변 확인 화면 |
| `/Game/IsekaiClass/UI/Widget/WBP_ConfirmPopup` | 시작 확인·자동 저장 교체 확인 |
| `/Game/IsekaiClass/UI/Widget/WBP_Loading` | 맵 진입 후 준비·최초 저장·실패 시 재시도 |
| `/Game/IsekaiClass/UI/Widget/WBP_GameHUD` | 이름·HP·MP·SP·상호작용·기본 메뉴 |
| `/Game/IsekaiClass/UI/Widget/WBP_UIRoot` | HUD·일반 화면·기존 연결 화면·팝업·로딩 레이어 |
| `/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI` | UI 한국어 문구 |
| `/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CreationOptions` | 질문 3~5의 선택지와 시작 보상 연결 |
| `/Game/IsekaiClass/UI/Fonts/FONT_IC_UI` | 패키징 가능한 나눔고딕 일반·굵은 글꼴 |

위젯의 디자이너 트리가 실제 에셋에 저장되어 있다. C++ 이벤트 연결에 쓰이는 `BTN_*`, `TXT_*`, `EDIT_Name`, `Layer_*` 이름은 유지하고 배치·색·글꼴을 편집한다. 런타임 C++ 기본 레이아웃은 트리가 없는 위젯에서만 사용한다.

## 질문과 보상

이름은 공백을 제외한 1~12자로 받는다. 성별은 남성·여성 외형에 연결된다. 선택지를 누르면 다음 질문으로 넘어가며, 돌아가기로 이전 답변을 수정할 수 있다. 다섯 답변은 맵 이동 후 첫 저장과 이후 저장에도 남는다.

| 음악 답변 | 현재 시작 무기 | 현재 테스트 기술 |
| --- | --- | --- |
| 내 청춘을 가득 채웠던 노래 | 검 | `SKL_START_SWORD_001` |
| 힘든 날마다 나를 위로했던 노래 | 활 | `SKL_START_BOW_001` |
| 사랑하는 사람과 함께 듣던 노래 | 스태프 | `SKL_START_STAFF_001` |
| 마지막까지 나답게, 신나는 노래 | 완드 | `SKL_START_WAND_001` |

음악 선택지에 시작 무기를 직접 안내한다. 이 연결은 임시 테스트 매핑이다. 직업을 고정하지 않으며 나중에 다른 계열 무기를 획득해 바꿀 수 있다. 현재는 선택한 계열의 무기 1개만 지급한다.

NPC·전용 레시피·인물 정보 연결은 비어 있다. 존재하지 않는 NPC나 보상을 만들어 지급하지 않는다. 행복했던 순간의 답변은 보상에 영향을 주지 않는다.

`DT_IC_CreationOptions`의 행 이름은 `Q3_1~4`, `Q4_1~4`, `Q5_1~4`다. `Question`과 `Option` 필드는 0부터 시작한다. 예를 들어 `Q4_1`은 `Question=3`, `Option=0`이다.

NPC 데이터가 준비되면 다음 필드를 연결한다.

- 이야기 행: `RecipeID`에 해당 인물 선물 레시피, `CodexID`에 인물 정보 ID를 지정한다.
- 음악 행: `NPCID`, 해당 NPC의 고정 기술인 `SkillID`, 같은 계열 초보 무기의 `WeaponItemID`를 지정한다. `bTestReward=False`로 바꾸고 문구도 갱신한다.
- `NPCID`를 지정하면 새 게임 초기 상태를 만들 때 호감도 +300을 적용한다. 기술·NPC·레시피·정보 ID가 실제 카탈로그에 있어야 한다.

원본 CSV는 `Data/Settings/DT_IC_CreationOptions.csv`, 문구 원본은 `Data/StringTables/ST_IC_UI.csv`다. CSV만 고치면 실행 중인 게임 에셋에는 반영되지 않으므로 해당 DT·String Table을 다시 가져와 저장한다. 에디터에서 직접 DT를 고쳤다면 CSV도 함께 갱신한다.

## 소스의 책임

- `Core/Flow/ICGameFlowSubsystem`: 질문 답변 보관, 시작·로딩·플레이 상태, 실패 상태, 맵 이동.
- `Core/Flow/ICLoadingScreenSubsystem`: 실제 맵 로딩 중 MoviePlayer의 Slate 화면. 일반 UMG가 멈추는 구간을 담당한다.
- `UI/Management/ICUIManagerSubsystem`: 레이어 표시, 입력 모드·포커스·게임 시간 정지의 공통 관리.
- `UI/Framework/ICUIRootWidget`: CommonUI Activatable Widget 스택과 HUD 컨테이너.
- `UI/CharacterCreate/ICCharacterCreateWidget`: 질문 진행·이전 답변 수정·최종 확인.
- `Core/Controller/ICPlayerControllerPersistence`: 맵에 준비된 플레이어에 초기 장비·기술·답변을 적용하고 최초 저장.

CommonUI는 화면 스택과 활성화·포커스에 사용한다. 입력 모드 변경은 UIManager가 담당해 기존 메뉴와 중복 적용되지 않게 했다. CommonGame 의존성은 추가하지 않았다.

최초 저장 전에는 플레이 입력과 시간 진행을 막는다. 저장 성공 후 로딩을 닫고 HUD와 플레이 입력을 활성화한다. 저장 실패는 메시지와 재시도·메인 메뉴 복귀 버튼을 제공한다. 맵 로딩 진행률을 임의의 숫자로 표시하지 않는다.

## 맵과 외형

`L_TestMap`이 빈 맵이어서 테스트 바닥·PlayerStart·조명·작은 단차 3개를 `PlayFoundation` 폴더에 추가했다. 남녀 외형은 기존 `SK_Player_M`, `SK_Player_F`를 연결했다. 보행·점프·공격 애니메이션은 아직 선택·리타기팅·AnimBP 연결이 필요하다.

최종 맵으로 바꿀 때는 Project Settings의 `IsekaiClass Game Flow → Play Map`을 `L_OpenWorld`로 바꾸고 패키징 맵 목록에도 추가한다. 설정 원본은 `Config/DefaultGame.ini`의 `ICGameFlowSettings`다. Game Default Map은 계속 `L_Title`로 둔다.

## 현재 범위

시작부터 실제 이동까지의 기본 흐름과 최소 HUD를 구현한 단계다. NPC 배치·질문 보상 NPC 연결·플레이어 집 지급·완성형 인벤토리/파티/도감 UI·캐릭터 애니메이션·지역과 보스 제작은 후속 작업이다. 기존 퀘스트·대화·저장 위젯의 시스템 연결은 유지했다.

기존 시작 메뉴의 이미지 전환·안개 애니메이션과 직업 버튼 생성 화면은 교체했다. 주석·툴팁은 새 코드에도 추가하지 않았다.

원본 백업, 실행 로그, 테스트 보고서, 화면비별 미리보기는 `Saved/CodexFrontend`에 있다. `create_assets.py`는 초기 구성용이며 재실행하면 위젯 디자이너 트리를 재생성하므로 편집 후 반복 실행하지 않는다.

에디터·게임 Development 빌드와 시작/종료·입력·GAS·월드 저장 자동 테스트 22개를 통과했다. 16:9·16:10·21:9, 1280×720에서 질문/이름/답변 확인을 렌더링해 확인했다. 일부 기존 NPC 데이터가 미배치 앵커를 기다리는 경고는 남아 있으며 이번 단계에서는 NPC를 배치하지 않았다. 배포용 전체 패키징과 실제 게임패드 조작은 아직 검증하지 않았다.
