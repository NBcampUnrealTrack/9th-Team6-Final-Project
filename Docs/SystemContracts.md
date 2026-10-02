# 1차 공용 시스템 연결 계약

기준: 승인된 1차 구현, UE 5.8. 기획서의 1~8번은 담당 영역이며 실제 사람 이름은 팀장이 배정한다. 이 문서는 구현된 API와 후속 연결 작업을 구분한다.

## 상태 소유권

| 데이터 | 원본 및 변경 경로 | 담당 |
| --- | --- | --- |
| 플레이어 생성 정보, 주 스탯, NPC 호감도·모드·파티 | UICWorldStateSubsystem | 1번 공용 / 2·5번 호출 |
| HP/MP/SP·파생 최대치·디버프 | 현재는 WorldState 시험값. GAS 연결 후 활성 참여자에서 조회하고 저장 전 Flush | 3번 |
| 가방·장비·퀵슬롯·창고 | WorldState.Inventory; UPlayerInventory가 복사→규칙 적용→검증→확정 | 2번 |
| NPC 일과·배치 | WorldState의 ID별 기록 + GameClock. 액터 생성·AI는 후속 | 4번 |
| 퀘스트·워프·처치·자원·엔딩 기록 | WorldState의 구조체; ApplyWorldProgress로 확정 | 5·6번 |
| GameClock / PlayClock | UICTimeSubsystem | 1번 |
| 저장 파일 및 슬롯 | UICSaveSubsystem | 1번 |
| 화면 | Snapshot + 변경 이벤트. 위젯은 원본을 직접 수정하지 않음 | 7번 |

GameInstance에서 GetSubsystem<UICWorldStateSubsystem>(), GetSubsystem<UICTimeSubsystem>(), GetSubsystem<UICSaveSubsystem>()로 접근한다. 인벤토리 컴포넌트는 BeginPlay에서 BindWorldState하고 EndPlay에서 해제한다. 반환된 Snapshot은 복사본이므로 수정만 해서는 반영되지 않는다.

## WorldState API

| 실제 함수 | 동작 |
| --- | --- |
| GetSnapshot(FName CharacterId) | 미등록이면 CharacterId=None인 기본값. 활성 참여자의 검증된 HP/MP/SP·최대치·디버프를 병합. 플레이어 장비는 Inventory의 조회 뷰 |
| EnsureCharacter(ID, Kind) | 최초 상태 생성; 기존 종류 변경 거부 |
| SetCharacterState(Character) | 전체 캐릭터 기록 검증 후 확정 |
| SetAffinity(ID, Value) | 영입형·상주형 0~100. 배경형·플레이어는 거부 |
| SetMainStat(ID, Stat, Value) | 플레이어·영입형의 STR/DEX/INT/CON/CHA/LUK, 1~10000 |
| SetVitals(ID, HP, MP, SP) | 시험 상태 및 GAS write-back에 사용; 현재 최대치 범위 검사 |
| SetCharacterMode(ID, Mode) | 영입형의 Scheduled/Companion/Downed. Downed→Scheduled 직접 변경 거부 |
| SetPartySlot(ID, Slot) | -1 해고 또는 0~3. 신규 영입은 호감도 100. 슬롯 중복·쓰러진 동료 해고 거부 |
| GetInventorySnapshot() | 가방·퀘스트·장비·퀵슬롯·컨테이너 복사 |
| CommitInventoryState(Inventory) | 공용 구조·GUID·슬롯 검증 후 확정. 카탈로그별 규칙은 Inventory.ValidateSnapshot으로 먼저 검사 |
| CommitCharacterAndInventory(Character, Inventory) | 시험 효과와 소비품 소모를 한 번에 확정(C++). GAS 효과 처리는 별도 계약 필요 |
| SetPlayerTransform(Position, Rotation) | 저장용 위치·회전 |
| ExportState() / ValidateState(Candidate) | 기록 추출 및 공용 검증 |
| CanRestoreState / RestoreState | 전체 검증·참여자 사전검증 후 원본 교체 및 참여자 복원 |
| ApplyWorldProgress(Candidate) | 퀘스트·워프·처치·보스·자원·드롭·엔딩 필드만 확정(C++) |

캐릭터 ID는 CHR_PLAYER 또는 NPC_R00~R04_NNN이다. NPC 종류별 시리얼은 영입형 001~099, 상주형 101~199, 배경형 201~299이다. 파티 슬롯은 내부 0~3이고 UI는 1~4로 표시한다. Companion1~4는 UI 대상 별칭이며 캐릭터 ID로 저장하지 않는다.

플레이어의 저장용 Character.Equipment는 비워 둔다. 장비 원본은 Inventory.Equipment 하나다. 플레이어 Snapshot의 장비 조회 뷰를 그대로 SetCharacterState로 돌려보내지 말고 GetStoredCharacter의 기록을 복사하거나 전용 SetVitals/SetMainStat을 사용한다. 영입형 장비는 캐릭터 기록에 저장할 수 있다. 상주형·배경형에는 성장·장비·디버프 로직을 붙이지 않는다.

OnCharacterChanged(ID), OnInventoryChanged(), OnWorldRestored()를 구독한다. 스탯 변경을 구독한 3번이 파생값을 재계산한다. 공용 모델은 피해·회피·이동 속도 공식을 정하지 않는다. ApplyWorldProgress 이후 퀘스트 표시 이벤트는 해당 담당이 발행한다.

## 활성 액터 참여자: 3·4번

IICStateParticipant를 액터 또는 컴포넌트에 구현한다.

1. RegisterStateParticipant(ID, Object). 같은 객체의 재등록은 허용하고 같은 ID의 다른 살아 있는 객체는 거부한다.
2. ReadLiveState(ID, Snapshot): 저장된 기록을 기초로 GAS의 현재 전투 값과 디버프를 반환한다. 조회에서 상태를 변경하지 않는다.
3. FlushToWorldState(State): 저장 전에 실시간 값을 SetCharacterState/SetVitals 등으로 반영하고 성공 여부를 반환한다. 실패하면 공용 기록을 이전 상태로 되돌리고 저장을 중단한다.
4. CanRestoreWorldState(Candidate): 카탈로그·필수 에셋·NPC 참조를 검사하는 순수 검증 함수. 여기서 액터나 상태를 변경하지 않는다.
5. RestoreFromWorldState(Candidate): 사전검증이 끝난 데이터를 적용한다. 로드된 시계를 조회할 수 있다. 복원 단계에서 실패 가능한 추가 작업을 하지 않는다.
6. EndPlay에서 UnregisterStateParticipant(ID, Object).

현재 플레이어 컨트롤러가 참여자로 등록돼 위치·인벤토리 검증·외형을 복원한다. GAS는 연결되지 않아 ReadLiveState는 공용 시험값을 반환한다. 3번은 이 참여자에 GAS를 연결하거나 등록을 넘겨받아야 한다. 두 객체를 같은 플레이어 ID로 동시에 등록할 수 없다.

로드는 파일·WorldState·Clock·참여자를 모두 검증한 후 Clock→WorldState→참여자→변경 이벤트 순서로 적용한다. NPC는 저장 좌표 대신 일과와 GameClock으로 재배치하고 동료는 플레이어 근처에서 복원한다. 이 스폰·AI 로직은 4번이 구현한다.

## 인벤토리: 2번

TryAddItem(ItemId, Count)는 **수납되지 않은 수량**을 반환한다. 성공은 0이다. 알려지지 않은 ID는 수량 전체를 반환한다. Count는 양수를 전달한다. OnLootResult(ItemID, Accepted, Remainder)의 마지막 인자는 삭제량이 아니다.

- 전투 보상: 잔량을 2번의 드롭 액터로 생성해야 한다.
- 구매·제작: AddItem(ID, Count, false) 또는 ExchangeItems를 사용해 공간 부족 시 전체 거부한다.
- 퀘스트 보상: 담당이 지급 완료/보류 정책을 소유한다.
- 현재 버리기는 데모의 삭제 동작을 유지한다. 월드 드롭으로 전환하는 작업은 후속이다.
- 현재 프로토 상자의 목재 획득은 반복 가능한 QA 기능이다.
- 장비 InstanceID는 소유권 이동에도 유지한다. 스택 분할은 새 GUID를 만든다.
- 가방 20→100칸, 10칸씩 확장; 장비 4칸; 퀵슬롯 4칸; 컨테이너 40칸.
- HomeA·HomeB는 기존 PlayerHome ID 하나를 공유한다.
- 게임 중 원본에 직접 배열을 쓰지 말고 Inventory의 함수로 변경한다.

현재 ItemTable 미지정 시 기존 데모 Definitions를 사용한다. CSV 등록부만으로 새 아이템 효과나 DT 행이 생성되지는 않는다.

## 두 시계: 2·4·6번

GetClock()은 GameMinute(게임 분)와 PlaySecond(활성 플레이 초)를 반환한다.

- 현실 2초 = 게임 1분; 2880초 = 게임 1440분.
- 프로토 시작은 06:00(360분), 날짜는 GameMinute/1440으로 계산한다.
- SetRunning과 SetMenuPaused로 생성·메뉴·사망에서 멈춘다. 월드 일시정지에서도 진행하지 않는다.
- JumpGameMinutes(Minutes)는 음수·비정상 숫자를 거부하고 GameClock만 진행한다.
- OnTimeJumped(From, To)를 NPC 일과·SP·농사가 구독한다. OnClockChanged는 분 변화 또는 복원 때 발행한다.
- 2번은 부족 SP로 수면 시간을 계산하고 회복을 적용한 다음 JumpGameMinutes를 호출한다. 최소 수면 규칙은 미정이다.
- MonsterKilledAt, ResourceReadyAt, Drop.CreatedPlaySecond는 PlayClock 기준 초로 사용한다. 재생·소멸 루프는 각 담당이 구현한다.
- 메뉴 중 저장은 가능하지만 시계는 정지한다.

## 저장·오토 트리거: 3·5·6번

SaveSlot(1~4)은 비동기 수동 저장, SaveSlot(0)은 초기 자동 저장에 사용한다. 반환 true는 **요청 접수**이고 완료는 OnSaveFinished(Slot, Success, Message)로 확인한다. bSaving 동안 추가 저장·로드·삭제를 거부한다. GetSaveBlockReason(bool Auto)는 전투·대화·이벤트·사망·효과·쓰기 중 등 구체적 사유를 반환한다. SaveSlotSynchronously는 자동 검증용 C++ 함수다.

SetSafetyState(FICSaveSafety)로 전투·대화·이벤트·사망·효과 응답 대기를 전달한다. 이 상태는 수동·자동 모두를 막는다. 오토는 추가로 HP 비율 25% 이하 또는 활성 디버프가 있으면 보류한다. 25%는 공개된 프로토 값이며 최종 밸런스가 아니다.

현재 컨트롤러가 매 프레임 bInCombat, bInDialogue, bInEvent, bDead, bEffectWaiting을 저장 시스템에 전달한다. 3번은 SetCombatState, 5번은 대화·이벤트 필드와 종료 처리를 연결한다. 활성 전투 값은 참여자가 공급한다.

RequestAutoSave(Reason)으로 수면·워프·지역 이동·퀘스트 수락/완료·보스 처치를 요청한다. 해당 이벤트 구현은 아직 연결 대기다. 현재 동작하는 트리거는 초기 생성과 PlayClock 300초 간격이다. 안전해지면 PollAutoSave가 보류 요청을 처리한다. 파일 쓰기 실패 후 30 PlayClock 초 뒤 재시도한다.

| 파일 | 의미 |
| --- | --- |
| Saved/SaveGames/IC_Team_Manual1~4.sav | 수동 네 슬롯 |
| IC_Team_Auto.sav | 표시되는 자동 한 슬롯 |
| IC_Team_Auto.sav.bak | 직전 정상 자동 저장 |
| *.tmp / *.rollback / *.bak.tmp | 교체 중 임시·복구 기록 |
| PlayerPrototype_Auto.sav | 기존 Version 1 원본; 가져오기 시 보존 |

Version 2 파일은 길이·CRC32를 검사한 뒤 Unreal SaveGame payload를 읽는다. 임시 파일 쓰기와 읽기 검증→이전 정상본 보존→교체→읽기 검증 순서다. 손상된 오토 원본은 백업/복구 파일에서 읽을 수 있다. 알 수 없는 미래 버전은 로드·덮어쓰기를 거부한다. CRC는 우발적인 손상 감지용이다.

GetSlotList()는 자동 0 + 수동 1~4, 정확히 5개를 반환하며 bValid와 bUsesBackup을 표시한다. LoadSlot은 사망·생성 화면에서도 가능하다. 초기 생성은 자동 저장 완료 후 플레이를 시작한다. 실패하면 지급품을 중복 생성하지 않고 같은 상태를 다시 저장한다.

ImportLegacySave()는 수동 1이 비어 있을 때 기존 Version 1을 새 파일로 가져온 뒤 로드한다. 원본을 변경하지 않는다. 테스트는 IC_Test_<GUID> 및 IC_Test_Smoke 공간을 사용하며 실제 저장 슬롯을 건드리지 않는다.

## 저장 화면·삭제·종료: 1·7번

SelectedSaveSlot은 수동 1~4, SelectedLoadSlot은 자동 0~수동 4를 독립 선택한다. F5/가방 저장은 OpenSaveMenu, 숫자 1~4는 이 화면에서 선택, Enter는 RequestManualSave다. 기존 파일이 있으면 확인창의 캡처된 슬롯에 저장한다. F9는 SelectedLoadSlot을 읽는다. 불러오기 화면의 뒤로가기 버튼과 메뉴/취소 키는 진입 전 화면으로 돌아간다. 생성 전은 Create, 사망 후는 Pause로 복귀하며 사망 상태와 시간 정지를 유지한다. 슬롯 이름·UTC 저장 시각을 로컬 시각으로 표시하며 처리/완료 메시지에 슬롯을 포함한다.

DeleteSlot(0~4)은 해당 경로의 .sav, .bak, .tmp, .rollback, .bak.tmp만 삭제하고 잔존 파일을 확인한다. 부분 실패는 실패 메시지를 반환하고 목록을 다시 조회한다. GetSlotList.bExists는 원본 또는 관련 파일이 하나라도 있으면 true다. 자동 슬롯 삭제는 보류 요청·300초 기준을 현재 PlayClock으로 초기화한다. DeleteLegacySave는 기존 Version 1 원본만 삭제하며 가져온 수동 1과 독립이다.

저장·확인·종료 화면은 SetAutoSaveSuspended(true)로 자동 파일 쓰기를 보류한다. 요청 자체는 유지하고 화면을 나가면 재개한다. 종료는 추가 자동 저장을 요청하지 않는다. 저장 후 종료는 비동기 성공 이벤트를 기다리고 실패하면 종료 화면을 유지한다. 이미 쓰기 중인 저장 없이 종료 요청도 성공 완료까지 기다리며 실패하면 머문다. WITH_EDITOR의 PIE는 UEditorEngine.RequestEndPlayMap, 일반 게임은 QuitGame을 사용한다. UnrealEd 의존성은 Editor 빌드에만 포함한다.

상자는 APlayerPrototypeObject를 대상으로 중앙 카메라 trace를 우선하고 캐릭터 중심 250cm 안 전방 후보를 거리/방향으로 보완한다. 캐릭터 시점에서 대상까지 Visibility 가림 검사를 거친다. HUD 후보는 안내용이며 F 실행 시 다시 검증한다. Storage/Loot/Quest 행동 및 창고 권한을 처리한다. 다른 클래스 상자는 해당 클래스의 명시적인 상호작용 연결이 필요하다.

## 입력·효과 연결: 2·3·7번

우클릭 누름/해제는 BlockStart/BlockEnd, Shift는 Dodge, Q/E/R/T는 Skill 슬롯 0~3, Z/X/C/V는 동료 슬롯 0~3 요청을 보낸다. MenuCancelKey/Label이 같은 키를 제공한다. Development/Editor는 F6로 메뉴·대상 취소, Shipping은 ESC다. 에디터 StopPlaySession의 ESC 설정은 변경하지 않는다. 공격·회피·스킬은 OnCombatRequest 이벤트까지 구현됐으며 실제 전투는 3번 연결 대기다.

bUseTestEffects=true가 현재 기본이다. 포션 HP+25, 음식 SP+25, 근력의 서 STR+1은 상한을 확인해 소비와 함께 공용 상태에 적용한다. false로 전환하면 OnEffectRequest(RequestGuid, ItemId, TargetCharacterId)가 발생한다. 3번은 요청 GUID를 한 번만 처리하고 ResolveEffect(Guid, Success)로 응답한다. 응답 후 소모가 실패하면 안내하므로 실제 GAS 연동에서는 재고 예약·효과 롤백 정책을 함께 구현해야 한다.

OnPlayerInitialized(Job, FirstSkill)는 초기 저장 성공 또는 정상 로드 복원에서 한 번 발행한다. 초기 생성 정보는 드래프트이고 확정 후에는 WorldState가 원본이다.

## 팀장 치트

Development/Editor 콘솔에서 다음을 실행한다. Shipping 함수 본문은 비활성화된다.

| 명령 | 예시 |
| --- | --- |
| ICAddGameMinutes | ICAddGameMinutes 180 |
| ICGiveItem | ICGiveItem Food 3 |
| ICSetAffinity | ICSetAffinity NPC_R01_001 100 |
| ICTeleport | ICTeleport 0 0 130 |

등록 전 NPC의 유효한 ID에 호감도 치트를 사용하면 공용 상태만 생성한다. 실제 NPC 액터·영입 UI는 별도다. 신규 정식 아이템 ID는 ItemTable 또는 Definitions에 등록돼야 지급할 수 있다.
