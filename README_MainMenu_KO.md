# IsekaiClass 메인메뉴 연결 안내 (UE 5.8 / Rider)

## 들어 있는 것
수정된 Source 전체와 C++ 클래스 3종입니다. .uasset이나 기존 UI 이미지는 포함하지 않습니다.
- ICMainMenuWidget: Designer 버튼을 이름으로 연결. 시작 맵 이동 / 종료 / 불러오기·설정 이벤트 제공.
- ICMainMenuPlayerController: 메뉴 생성, 마우스 표시, UI 입력 모드, 시작 버튼 포커스.
- ICMainMenuGameMode: 메뉴 전용 게임모드. 플레이어 캐릭터를 자동 생성하지 않음.
- Build.cs: Public UMG, Private Slate/SlateCore 추가.

## 1. 소스 적용과 빌드
1. 언리얼 에디터와 Rider를 종료합니다.
2. 프로젝트의 Source 폴더를 백업합니다.
3. 압축의 Source 폴더를 IsekaiClass.uproject가 있는 프로젝트 폴더에 병합합니다.
   프로젝트/Source/Source가 되지 않도록 주의하세요.
   이번 변경 파일은 Build.cs 및 위 3개 클래스의 .h/.cpp, 총 7개입니다.
   업로드 이후 다른 코드가 추가됐다면 Source 전체를 덮지 말고 새 파일 6개만 복사하고 Build.cs의 의존성을 병합합니다.
4. Rider에서 IsekaiClassEditor / Development Editor / Win64 대상으로 빌드합니다.
   새 UCLASS를 추가했으므로 첫 빌드는 에디터를 닫은 상태에서 합니다.
5. 성공 후 언리얼 에디터를 엽니다.

## 2. 배치할 위젯 만들기
Content/UI/MainMenu에 WBP_MainMenu를 만듭니다.
Widget Blueprint 생성 시 부모 클래스를 ICMainMenuWidget으로 선택합니다.
이미 만들었다면 Class Settings > Parent Class를 ICMainMenuWidget으로 변경합니다.
Designer 화면 기준은 1920x1080입니다.

권장 계층:
Canvas Panel
  IMG_Background (전체 화면, 앵커 전체 늘이기, 네 오프셋 0)
  IMG_Title (별도 제목 이미지가 있을 때)
  VB_Menu (버튼 네 개의 세로 배치)
    SizeBox (Width Override 480 / Height Override 80)
      BTN_Start
    SizeBox (480 / 80)
      BTN_Load
    SizeBox (480 / 80)
      BTN_Settings
    SizeBox (480 / 80)
      BTN_Quit

VB_Menu 시작 배치는 왼쪽 위 앵커, Position X=160 Y=480, Size X=480 Y=400.
각 SizeBox의 Vertical Box Slot Padding Bottom=20, Size=Auto.
이미지 구성에 맞게 숫자는 조정하세요. Is Variable 켜기, 네 Button 이름과 타입은 정확히 일치해야 합니다.
WBP_MainMenu의 Class Defaults에서 Is Focusable을 켭니다.
전체 화면 배경과 제목은 Visibility를 Not Hit-Testable (Self & All Children)로 설정합니다.
버튼은 Visible 상태를 유지합니다.

버튼 Style의 Normal / Hovered / Pressed에 기존 상태별 텍스처를 지정합니다.
이미지 안에 메뉴 글자가 들어 있으면 Button 내부 Text를 중복 추가하지 않습니다.
텍스처가 있는 버튼의 Background Color는 흰색으로 설정합니다.
글자가 들어 있는 전체 이미지 위에 투명 버튼을 놓는 경우:
Normal / Hovered / Pressed를 Draw As=None으로 설정해 클릭 영역만 만듭니다.
이 방식에는 호버 이미지 변화가 없으므로 상태 이미지가 있으면 별도 버튼 텍스처 방식으로 연결하세요.
이미지 자체가 없으면 각 버튼 안에 Text를 넣어 게임 시작 / 불러오기 / 설정 / 종료로 설정해 테스트할 수 있습니다.

## 3. 버튼 동작
Class Defaults의 Main Menu > Start Map에서 실제 플레이할 맵을 선택합니다.
메뉴 맵을 선택하면 다시 메뉴로 돌아오니 게임플레이 맵을 선택하세요.
게임 시작과 종료는 C++에 연결되어 있어 OnClicked를 중복 연결하지 않습니다.
불러오기: Graph에서 Event On Load Requested를 추가해 이후 슬롯 UI를 연결합니다.
설정: Graph에서 Event On Settings Requested를 추가해 이후 설정 UI를 연결합니다.
지금은 각 이벤트에 Print String을 붙여 클릭 신호부터 확인할 수 있습니다.
이 두 이벤트는 실제 저장 불러오기 / 설정 변경 구현이 아닙니다.
Start Map 미지정 때 Event On Start Map Not Configured로 안내 UI를 연결할 수 있고 Output Log에도 경고가 남습니다.

## 4. 메뉴 전용 컨트롤러와 맵
1. ICMainMenuPlayerController 자식 Blueprint BP_MainMenuPC를 만듭니다.
2. BP_MainMenuPC Class Defaults > Main Menu Widget Class에 WBP_MainMenu를 지정합니다.
3. ICMainMenuGameMode 자식 Blueprint BP_MainMenuGM을 만듭니다.
4. BP_MainMenuGM > Player Controller Class를 BP_MainMenuPC로 지정합니다.
5. 메뉴 레벨 L_MainMenu를 만들고 저장합니다.
6. L_MainMenu의 World Settings > GameMode Override를 BP_MainMenuGM으로 지정합니다.
   전역 기본 게임모드를 바꾸지 않고 메뉴 레벨에만 적용하세요.
7. Project Settings > Maps & Modes > Game Default Map과 Editor Startup Map을 L_MainMenu로 지정합니다.
8. 플레이 대상 맵은 별도의 게임플레이 GameMode를 사용해야 합니다. BP_MainMenuGM을 적용하지 마세요.

위젯을 Level Blueprint에서 추가 생성하지 마세요. BP_MainMenuPC가 자동으로 띄웁니다.
패키징에서는 메뉴와 Start Map이 모두 쿠킹되도록 Project Settings > Packaging의 맵 목록에 포함합니다.

## 5. 검증
- L_MainMenu에서 Play: 메뉴 한 개만 보이고 커서가 보이는지 확인.
- 각 버튼의 호버 / 눌림 이미지와 클릭 영역 확인.
- 시작 클릭: 지정한 게임플레이 맵으로 전환되고 메뉴가 사라지는지 확인.
- 불러오기 / 설정: 연결한 이벤트의 Print String 확인.
- 종료: PIE 플레이가 종료되는지 확인. 패키지에서는 프로세스 종료 확인.
- 1280x720, 1920x1080, 2560x1440에서 잘림 확인.
- 게임패드/키보드 포커스의 시각 강조는 UMG에서 별도 구성해야 합니다. 마우스 Hovered 상태와 동일하지 않습니다.

이 환경에는 Unreal Engine/Rider 빌드 도구가 없어 실제 UHT·컴파일·PIE 검증은 수행하지 못했습니다.
