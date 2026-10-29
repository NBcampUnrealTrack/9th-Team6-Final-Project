# 메뉴 조작 / 이름 자판 / 프롤로그

- 메뉴·NPC 선택: 방향키 또는 WASD로 이동, **G / F로 확인**, ESC로 취소. 패드 방향키·스틱과 설정된 확인/취소 버튼도 사용 가능. 커서는 숨긴다.
- NPC 대화 중 플레이어 이동을 허용하는 기존 DT 설정을 켰다면 WASD는 캐릭터 이동에 쓰고, 대화 선택지는 방향키로 고른다.
- 이름: 화면 자판을 패드로 선택해 한글·영문·숫자·공백 입력. **LB 한/영**, **RB Shift**, 취소 버튼 또는 Backspace로 한 입력씩 지우기. `이름 결정`에서 확인하면 다음 질문으로 진행한다. 한글 받침·겹받침·복합모음을 조합하며 최대 12자다. 이름 칸에 키보드 초점을 두면 기존 직접 입력도 가능하다.
- 상점: G/F로 담기·선택, R로 결제. 패드의 기존 별도 결제 버튼도 유지한다.

선택을 모두 마치면 1.2초에 걸쳐 암전한 뒤 검은 화면 중앙에서 프롤로그 **55줄 전체**를 순서대로 표시한다. 글자는 0.045초마다 한 글자씩 나타난다. 문장이 완성된 뒤에만 **G 또는 패드 확인 버튼**으로 다음 대사에 진행한다. 길게 누르기, 타자 출력 중 입력은 진행에 사용하지 않는다. 스마트폰·상태창도 원문의 텍스트로만 표시한다.

마지막 문장에서 확인하면 글자가 서서히 사라지고 검은 화면을 유지한 채 게임 맵과 최초 저장을 준비한다. 맵이 준비되면 1.2초 동안 밝아진 다음 플레이어 조작과 게임 시간이 시작된다. 기존 저장 불러오기는 프롤로그를 다시 재생하지 않는다.

## BGM 편집

콘텐츠 브라우저의 `/Game/IsekaiClass/Data/DataTable/Audio/DT_IC_BGM`에서 다음 행을 편집한다.

| 행 | 재생 구간 | 기본 기존 음원 |
|---|---|---|
| `BGM_MainMenu` | 메인 메뉴·메인 메뉴의 설정/불러오기 | Ocean_Waves |
| `BGM_CharacterCreate` | 이름 입력·캐릭터 생성 질문 | Merchant |
| `BGM_Prologue` | 프롤로그·게임 진입 전환 | Inner_Dreams |

각 행의 `Music`, `Volume`, `FadeInSeconds`, `FadeOutSeconds`, `bLoop`로 음원·볼륨·전환·반복을 설정한다. 게임 진입 후에는 기존 지역/전투 BGM 규칙으로 돌아간다. CSV 원본은 `Data/Audio/DT_IC_BGM.csv`다.

게임 내 메뉴·설정·팝업은 MorpheusGUI의 평면 패널, 청록색 선택 표시, 상단 아이콘 분류, 하단 조작 안내를 따른다. 설정은 원본 `w_settings_sample`처럼 왼쪽 옵션 목록과 오른쪽 설명으로 배치한다. W/S로 항목을 이동하고, A/D로 값을 변경하며, F/G로 선택한다. 첫 항목에서 W로 상단 분류, 마지막 항목에서 S로 하단 적용/닫기 버튼으로 이동한다. 상단·하단에서는 A/D로 선택한다.

FirefightUI는 게임 플레이 HUD에만 사용한다. 메뉴 반영 스크립트 `Tools/UI/setup_morpheus_ui.py`는 HUD 에셋을 제외하고, 메뉴에 Firefight 브러시가 남아 있지 않은지 검사한다. `Tools/UI/setup_keyboard_prologue.py`는 BGM 행을 보존한 뒤 이 메뉴 스크립트를 호출한다. 원본 샘플 에셋과 아트는 수정하거나 새로 생성하지 않았다.

프롤로그 G 입력은 활성 프롤로그의 전용 입력 처리기가 받는다. 메뉴 전환이나 뷰포트 클릭으로 초점이 이동해도 문장이 완성되면 다음 줄로 진행한다. 검증에서도 매 대사마다 초점을 강제로 복구하지 않는다.

## 저장목록

저장·불러오기 화면은 슬롯별 한 줄 목록이다. 별도 상세창 없이 저장 위치, 이름, 지역, 플레이 시간, 저장 시각과 **플레이어 및 동료 3명의 얼굴**을 같은 행에 표시한다. W/S·위/아래 방향키 또는 패드 십자키·왼쪽 스틱 위아래로 슬롯을 선택하고 F/G·패드 확인 버튼으로 실행한다. 좌우키와 스틱 좌우는 선택을 바꾸지 않는다. Delete로 선택 슬롯을 삭제하고 ESC로 돌아간다. 수동 저장에서는 자동 저장 슬롯을 건너뛴다. 덮어쓰기·불러오기·삭제 확인 절차는 유지한다.

## 프롤로그 대사·사운드 DT

대사는 `/Game/IsekaiClass/Data/DataTable/UI/DT_IC_Prologue`에서 편집한다. 기본 55행은 `Line_001`부터 `Line_055`까지다. C++에 대사 배열을 두지 않는다.

| 필드 | 용도 |
| --- | --- |
| `Order` | 출력 순서. 서로 다른 번호를 지정한다. |
| `Text` | 한 줄의 전체 대사. 스마트폰·시스템 메시지도 같은 필드에 쓴다. |
| `MusicAction` | `Keep`은 현재 BGM 유지, `Play`는 `MusicRow`로 전환, `Stop`은 정지. |
| `MusicRow` | 기존 `DT_IC_BGM`의 행 이름. 예: `BGM_Prologue`. 음원·볼륨·페이드는 해당 BGM 행에서 조절한다. |
| `Sound` | 이 대사에서 한 번 재생할 기존 효과음 에셋. 비워 두면 효과음이 없다. |
| `SoundVolume` / `SoundDelay` | 효과음 볼륨과 대사 시작 후 재생 지연(초). |
| `CharacterInterval` | 이 행의 글자 출력 간격(초). 0이면 공통 값을 따른다. |

`/Game/IsekaiClass/Data/DataTable/UI/DT_IC_ProloguePresentation`의 `Default` 행은 다음/건너뛰기 안내 문구, 공통 글자 간격, `FadeSeconds`, `SkipHoldSeconds`, `InitialMusicRow`를 관리한다. 기본 H 유지 시간은 1.5초다. 패드는 Y/△를 길게 누른다. 누르는 동안 반원 게이지가 차고, 떼면 초기화된다. 완료하면 남은 대사를 건너뛰고 동일한 암전·게임 초기화·밝아짐 경로를 거쳐 조작을 시작한다. 짧게 누르거나 일반 G 입력으로 출력 중인 문장을 넘길 수는 없다.

프롤로그 배치·글꼴·색·반원 게이지의 크기/두께/각도는 `WBP_IC_Prologue`에서 편집한다. 이 WBP의 Class Defaults에서 위 두 DT를 지정한다. `Tools/UI/setup_managed_ui_flow.py`는 프롤로그 DT가 이미 있으면 에디터에서 편집한 데이터를 보존한다. CSV는 `Data/UI/`에 내보낸다.

## WBP 관리 원칙

실행 중 C++이 화면 트리를 만들거나 누락된 WBP 대신 기본 화면을 만드는 경로를 제거했다. C++은 데이터 갱신, 입력 처리, 목록 인스턴스 추가, 월드 좌표 추적을 담당하고, 화면과 반복 항목의 구성·외형은 저장된 WBP를 사용한다. 기존 `BuildLayout` 계열 코드는 에디터 에셋 작성 도구에서만 호출하며, 게임 실행이나 화면 열기 시 자동으로 WBP를 다시 만들지 않는다.

대부분의 게임용 WBP는 `/Game/IsekaiClass/UI/Widget`에 있다. 제작대·요리대는 기존 `/Game/IsekaiClass/Life/Crafting/WBP_IC_CraftingStation`, `/Game/IsekaiClass/Life/Cooking/WBP_IC_CookingStation`을 유지한다.

| WBP | 관리 대상 |
| --- | --- |
| `WBP_IC_Root` | HUD·메뉴·팝업·로딩 레이어 및 공통 화면 배율 |
| `WBP_IC_Prologue`, `WBP_IC_LoadingMovie` | 프롤로그와 맵 전환 로딩 화면 |
| `WBP_IC_SaveSlot`, `WBP_IC_SaveLoad` | 얼굴 4개를 포함한 저장 행과 세로 목록 |
| `WBP_IC_RecipeChoice`, `WBP_IC_EnhancementChoice`, `WBP_IC_LifeChoice` | 제작·강화·상점·씨앗 선택의 반복 행 |
| `WBP_IC_KeyBindingRow`, `WBP_IC_TextLine` | 키 지정 행과 빈 목록 안내 |
| `WBP_IC_QuestReward`, `WBP_IC_StatusEffect` | 퀘스트 보상 행과 버프/디버프 아이콘·중첩 수 |
| `WBP_IC_CombatFeedback`, 각 캐릭터 Status WBP | 락온 표시, 초상화 스킬 재사용 시간, 상태 표시 |
| `WBP_IC_Crosshair` | 원거리 무기 조준점 |
| `WBP_IC_Map`, `WBP_IC_Minimap`, `WBP_IC_MapPortalMarker`, `WBP_IC_MapPlayerMarker` | 지도 이미지·안내·포탈과 플레이어 표시 |

반원/원형 게이지와 지도 가장자리 페더링은 WBP에 배치하는 재사용 UMG 컨트롤이다. 각도·두께·색·크기와 페더링 사용 여부를 WBP 속성에서 편집할 수 있다. 새 AI 이미지나 외부 아트는 사용하지 않는다. 에셋 작성 스크립트는 기본 레이아웃을 다시 작성하는 도구이므로, WBP를 수동 편집한 뒤에는 초기화할 의도가 있을 때만 실행한다.

시작 화면은 W/S·위/아래로 보이는 순서대로 선택하고 F/G로 실행한다. NPC 대화에서는 W/S가 선택지를 이동한다. 대화 중 플레이어 이동·공격·회피·스킬 입력과 진행 중인 행동을 멈추고, 월드 시간과 다른 NPC 동작은 계속된다. 대화를 닫으면 일반 조작으로 복귀한다.
