# 캐릭터 생성창을 UMG 디자이너에서 편집하기

사용자가 올린 Source(1).zip 기준 패치입니다. 포함 파일 5개만 적용합니다.
기존 캐릭터 생성·이름 검증·자동 저장·성별 프리셋·직업 로직을 사용합니다.
생성창 외 HUD, 인벤토리, 저장/로드/종료 화면은 기존 C++ UI입니다.
실제 언리얼 컴파일 및 PIE 검증은 이 환경에서 수행하지 못했습니다.

## 1. 코드 적용
언리얼을 종료하고 기존 작업을 커밋하거나 백업합니다.
압축의 Source를 프로젝트 루트에 병합합니다. Source/Source로 넣지 마세요.
수정: PlayerPrototypeController.h, PlayerPrototypeWidget.h/.cpp
추가: ICCharacterCreateWidget.h/.cpp
Build.cs는 덮어쓰지 않습니다. 기존 UMG/Slate/SlateCore 의존성을 사용합니다.
업로드 이후 위 파일을 수정했다면 통째로 덮기 전에 차이를 비교하세요.
Rider에서 IsekaiClassEditor / Development Editor / Win64로 빌드합니다.

## 2. 생성 위젯
위젯 블루프린트 WBP_CharacterCreate 생성, 부모 ICCharacterCreateWidget 선택.
부모가 목록에 없다면 All Classes에서 검색합니다.
디자이너 기준 1920x1080. 배경/배치/사진은 자유롭게 편집합니다.
아래 이름/타입을 정확히 만들고 Is Variable을 켭니다.

| 이름 | 타입 | 연결 |
|---|---|---|
| IC_Create_Name | Editable Text Box | 필수, 이름 자동 연결 |
| IC_Create_Confirm | Button | 필수, 생성/초기 저장 자동 연결 |
| IC_Create_Notice | Text Block | 선택, 검증/저장 메시지 자동 표시 |
| IC_Create_Male | Button | OnClicked → Select Preset(0) |
| IC_Create_Female | Button | OnClicked → Select Preset(1) |
| IC_Create_Warrior | Button | OnClicked → Select Job(0) |
| IC_Create_Archer | Button | OnClicked → Select Job(1) |
| IC_Create_Mage | Button | OnClicked → Select Job(2) |
| IC_Create_Healer | Button | OnClicked → Select Job(3) |
| IC_Create_Load | Button | 선택, OnClicked → Open Load Screen |
| IC_Create_Quit | Button | 선택, OnClicked → Open Quit Screen |

Select Preset / Select Job 등의 Target은 Self입니다.
Confirm은 C++에서 자동 연결하므로 OnClicked를 추가하지 마세요.
Name의 OnTextChanged도 C++에서 연결합니다.
이름 입력칸은 Hint Text에 '이름을 입력하세요'를 지정합니다.
배경 이미지는 Not Hit-Testable(Self & All Children)로 설정합니다.
버튼과 입력칸은 Visible로 둡니다.

Graph에서 Event On Creation State Changed 추가:
Preset 0이면 남성, 1이면 여성 선택 스타일을 적용합니다.
Job 0/1/2/3은 전사/궁사/마법사/치료사입니다.
bCanEdit를 성별/직업 버튼의 Set Is Enabled에 연결합니다.
이 이벤트에서 입력칸 Set Text 또는 Select Preset/Select Job을 호출하지 마세요.
캐릭터 사진/3D 미리보기는 이번 패치에 추가하지 않았습니다.
모든 텍스트는 디자이너에서 한국어로 작성하고 추후 String Table에 연결 가능합니다.

## 3. 플레이어 컨트롤러 연결
APlayerPrototypeController 자식 BP_PlayerPC 생성 또는 기존 자식 사용.
Class Defaults → UI → Character Create Widget Class = WBP_CharacterCreate.
기존 플레이 레벨에 사용하는 GameMode의 Player Controller Class = BP_PlayerPC.
현재 GameMode가 순수 C++ PlayerPrototypeGameMode면 자식 BP_PlayerGM을 만들고
그 안의 Player Controller Class를 BP_PlayerPC로 지정합니다.
플레이 레벨 월드 세팅 → GameMode Override = BP_PlayerGM.
메뉴 레벨 L_MainMenu에는 기존 BP_MainMenuGM을 유지합니다.
캐릭터 생성 Widget Class를 비워두면 원래 C++ 생성창이 그대로 나타납니다.

## 4. 검증
생성 전 상태로 플레이 레벨 실행 → 새 디자이너 화면 표시.
이미 캐릭터를 만든 상태면 기존 코드대로 생성창을 건너뜁니다.
이름 입력 중 성별/직업 선택 후에도 입력 내용이 유지되는지 확인.
빈 이름/13자 이름 생성 시 기존 검증 메시지 확인.
남/여 선택이 실제 캐릭터에 반영되는지 확인.
생성 후 초기 자동 저장 성공 시 창이 닫히고 게임에 진입하는지 확인.
저장 실패 시 안내 표시 및 다시 시도 가능한지 확인.
선택적으로 로드/종료 화면 진입 후 돌아왔을 때 입력 내용 유지 확인.

실제 .uasset 위젯은 사용자의 언리얼 에디터에서 위 단계로 생성해야 합니다.
