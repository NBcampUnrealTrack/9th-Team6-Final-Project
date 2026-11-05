# 대화 표정과 입 모양 설정

2026-11-05 기준. 플레이어 남녀와 NPC 1~11의 기존 얼굴 모프를 사용한다. 다음 대사에 표정을 넣을 때는 `DT_IC_Dialogue`의 대사 노드에서 화자, 표정, 강도를 설정하면 된다.

## 에디터에서 대사 한 줄에 표정 넣기

1. PIE를 종료하고 Content Browser에서 **IsekaiClass → Data → DataTable → NPC → DT_IC_Dialogue**를 연다.
2. 대화 행을 선택하고 `Nodes`에서 해당 줄의 `NodeID`를 찾는다. 대화는 `EntryNodeID`에서 시작해 `NextNodeID`를 따라간다. 실제 대사 문구는 `TextKey`를 [ST_IC_Dialogue](../../Content/IsekaiClass/Data/DataTable/StringTables/ST_IC_Dialogue.uasset)에서 찾으면 된다.
3. 노드의 `SpeakerNPCID`로 화자를 지정한다. NPC 대사는 대화 상대의 ID를 사용한다. 플레이어가 직접 말하고 카메라에 잡힐 줄에는 **`CHR_PLAYER`**를 넣는다. `None`은 화자 미지정으로 처리하므로 내레이션이나 기존 연출을 유지할 줄에 사용한다. 행 상단의 `NPCID`는 대화 상대를 뜻한다.
4. 노드의 **Presentation → Expression**에서 표정을 고르고 **Expression Strength**를 `0~1`로 지정한다. 새 노드의 기본값은 `Neutral`이고 감정 강도의 초기값은 `0.5`다. 가벼운 표정은 `0.25~0.40`에서 시작해 캐릭터 얼굴을 보고 조절한다.
5. [DT_IC_Dialogue 에셋](../../Content/IsekaiClass/Data/DataTable/NPC/DT_IC_Dialogue.uasset)을 저장하고 새 PIE에서 해당 대화를 연다. 에셋 값만 조절할 때는 C++ 빌드가 필요 없다.

| 에디터 표시 | 데이터 값 | 얼굴 모프 접미사 |
|---|---|---|
| 기본 | `Neutral` | 기본 표정 |
| 기쁨 | `Joy` | `Joy` |
| 미소 | `Smile` | `Fun` |
| 화남 | `Angry` | `Angry` |
| 슬픔 | `Sad` | `Sorrow` |
| 놀람 | `Surprised` | `Surprised` |

## 적용된 예시

첫 메인 퀘스트 **이세계에 온 친구**에서 친구1(NPC 09)을 다시 만나는 장면이다. 행은 `DLG_SCN_R01_001_READY`다.

| 노드 | 화자 | 대사 | 표정 · 강도 |
|---|---|---|---|
| `N001` | `NPC_R01_009` | 야! 여기야! 진짜 너 맞네! | 기쁨 · `0.65` |
| `N002` | `CHR_PLAYER` | 목소리만 듣고도 알겠더라. 다친 데는 없어? | 가벼운 걱정에 슬픔 사용 · `0.28` |
| `N003` | `NPC_R01_009` | 없어. 숲에서 좀 헤매긴 했는데, 여관에서 재워 줬어. | 미소 · `0.40` |

`N004`부터는 감정 표정이 기본값이다. [첫 세 줄의 실제 게임 GIF](../../Saved/FirstDialogueExpressions_20261105/FirstDialogue_PlayerSpeech.gif)는 로컬 `Saved`에 있다.

## 게임에서 처리하는 흐름

- [ICDialogueRow.h](../../Source/IsekaiClass/Public/Data/Rows/NPC/ICDialogueRow.h)의 `FICDialogueNode`가 `Expression`과 `ExpressionStrength`를 저장한다.
- [ICNPCDialogueWidget.cpp](../../Source/IsekaiClass/Private/UI/Dialogue/ICNPCDialogueWidget.cpp)의 `TryShowNode`가 대사와 표정 값을 읽어 `SetSpokenLine`으로 넘긴다. 지정된 화자의 입 모양과 표정을 시작하고, 직전 화자의 입 모양을 정리한다.
- [ICFacialAnimationComponent.cpp](../../Source/IsekaiClass/Private/Character/Animation/ICFacialAnimationComponent.cpp)는 `Fcl_BRW_*`, `Fcl_EYE_*`, `Fcl_MTH_*`를 부드럽게 섞는다. 대사 글자에서 모음을 읽어 `Fcl_MTH_A/I/U/E/O`를 재생한다. 말할 때는 표정의 입 가중치를 낮추고, 눈을 깜빡일 때는 표정의 눈 가중치를 낮춘다. 대사가 끝나도 현재 줄의 감정은 다음 줄을 누를 때까지 남는다.
- [ICPlayerControllerDialogueCamera.cpp](../../Source/IsekaiClass/Private/Core/Controller/ICPlayerControllerDialogueCamera.cpp)는 근접 카메라가 켜져 있을 때 `CHR_PLAYER` 줄에서 플레이어를 비추고, NPC 대사로 넘어가면 NPC를 다시 비춘다. 근접 카메라 사용 여부와 거리는 `DT_IC_DialoguePresentation`에서 관리한다. 대화를 닫으면 얼굴 상태와 플레이 카메라가 복구된다.

## 여러 대사를 한 번에 작업할 때

2026-11-05에 게임이 읽는 에셋을 집계한 결과는 **177개 대화, 540줄**이다. [저장소 CSV](../../Data/NPC/DT_IC_Dialogue.csv)는 당시 **210개 대화, 573줄**로 범위가 달랐다. 일괄 작업은 최신 게임 에셋을 먼저 내보내고 `대화 행 ID + NodeID`로 대상을 찾아 필요한 필드만 병합한다. 전체 CSV를 재임포트하기 전에는 행과 노드 수, `TextKey`, 다음 노드 연결이 맞는지 대조한다.

CSV에서 표정 값은 `Nodes` 안의 구조체에 `Expression=Smile,ExpressionStrength=0.40`처럼 기록한다. 화자 전환까지 의도한 줄에만 `SpeakerNPCID`를 바꾼다. 변경한 데이터와 실제 에셋을 함께 맞춘다.

## 확인 순서

- 새 게임의 첫 메인 퀘스트에서 친구1과 대화한다. 1줄은 NPC의 기쁨, 2줄은 플레이어의 걱정과 말하는 입 모양, 3줄은 NPC의 미소를 확인한다. 첫 퀘스트를 완료한 세이브는 진행 상황에 따라 다른 대사를 열 수 있으므로 미완료 세이브나 새 게임으로 확인한다.
- 대사를 빠르게 넘기고, 플레이어 대사 도중 `Esc`로 닫아 입 모양이나 표정이 남지 않는지 확인한다. 플레이어를 비추는 줄은 남녀 프리셋으로 한 번씩 확인한다.
- 표정이 움직이지 않으면 먼저 노드의 화자 ID, `Expression`, `ExpressionStrength`를 확인한다. 모델을 교체했다면 Skeletal Mesh의 Morph Target Previewer에서 해당 눈썹·눈·입 모프가 실제로 움직이는지 확인한다.
- 얼굴 처리나 카메라 코드를 바꿨다면 에디터 빌드 후 Output Log에서 아래 검사를 실행한다. 첫 퀘스트의 세 줄과 대화 종료는 `IC.Characters.Face.FirstQuestExpressions`가 확인한다.

```text
Automation RunTests IC.Characters.Face+IC.UI.DialogueFaceCamera+IC.Runtime.QuestObjectiveDialogue
```
