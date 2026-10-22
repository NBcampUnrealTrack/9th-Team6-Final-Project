# R01 캐릭터 밑작업 규약

사용자가 확정한 직업·외형·무기·선물 설정을 연결하는 기준이다. 정식 지역 제작,
스토리 대사, 최종 밸런스, 진엔딩 명단 완성은 별도 콘텐츠 작업이다.

| 표시 이름 | 영속 NPC ID | 직업 | 애니메이션 팩 | 반친구 |
|---|---|---|---|---|
| R01_01 | NPC_R01_001 | 마법사 | Mage | 아니오 |
| R01_02 | NPC_R01_002 | 검사 | Sword_Animations | 아니오 |
| R01_03 | NPC_R01_003 | 힐러 | Mage | 아니오 |
| R01_04 | NPC_R01_004 | 궁수 | Archer | 아니오 |
| R01_05 | NPC_R01_005 | 검사 | Sword_Animations | 아니오 |
| R01_06 | NPC_R01_006 | 마법사 | Mage | 아니오 |
| R01_07 | NPC_R01_007 | 궁수 | Archer | 아니오 |
| R01_08 | NPC_R01_008 | 마법사 | Mage | 아니오 |
| R01_09 | NPC_R01_009 | 힐러 | Mage | 예 |
| R01_10 | NPC_R01_010 | 검사 | Sword_Animations | 예 |
| R01_11 | NPC_R01_011 | 궁수 | Archer | 예 |

진엔딩용 bEndingTarget은 아직 설정하지 않는다. bClassmate만 반영하며,
R04까지 명단을 확정할 때 엔딩 정의와 함께 등록한다. 기존 NPC_R01_101은 기존
상점·퀘스트 참조가 있는 별도 시제품 인물로 유지한다.

## 코드·BP·데이터의 책임

- C++: 상태 원본, 스폰과 복원, 무기 장착·해제, 길이 계산·판정·발사점,
  애니메이션 선택과 공통 그래프, 선물의 원자적 차감·호감도 지급.
- BP: NPC별 기존 외형과 공통 부모 상속, 무기 메시·그립·길이 축·발사점 조정.
- DT: NPC 고정 직업, 무기 BP와 부착 변환, 선물 수치, 시간표·집·대화 연결.
- DA: Skeleton별 리타겟팅 결과와 무기 계열별 애니메이션 세트.

기존 GAS, Definition Catalog, WorldState, SpawnSubsystem, UIManager를 사용한다.
BP마다 별도 인벤토리나 호감도 원본을 만들지 않는다.
무기 외형은 장비·상태 변경 이벤트에서 즉시 갱신하며 캐릭터별 주기적 검색을 추가하지 않는다.

## 연구 과제와 기존 코드의 연결

첨부 연구 목록 17.1~17.9.4를 다시 대조했다. 아래의 코드 경로는
Source/IsekaiClass 기준이며, 이번 범위에서 실제 연결한 부분과 후속 연구를 구분한다.

| 연구 항목 | 재사용 또는 추가한 코드 | 이번 반영 및 남은 검증 |
|---|---|---|
| 17.1 리타겟팅·보조 물리 | 신규 Character/Animation/ICCharacterAnimInstance, ICCharacterAnimationProfile 및 Debug/ICCharacterSetupLibrary | 기존 대상 IK Rig로 Skeleton별 리타겟팅. 손바닥 소켓·무기 축과 배율·평지 높이·침대 자세를 보정. 기존 VRM SpringBone 유지 및 자세 전환 시 초기화. 경사면 IK·개별 의상 충돌·LOD 비용은 후속 검증. |
| 17.2 레벨 | 기존 World/Spawn/ICSpawnSubsystem, World/Anchors 및 Region/Anchor DT | NPC는 영속 ID와 정의로 연결. 신규 앵커는 초안. World Partition·Data Layers·PCG·HLOD·Nanite·MegaLights는 실제 월드 제작 및 성능 비교 단계에서 판단. |
| 17.3 플레이어 | 기존 Player/Character/ICPlayerCharacter, Core/Controller/ICPlayerControllerInput 및 공통 GAS | 남녀 프리셋을 DT에서 읽고 실제 장비 계열로 애니메이션 선택. 기존 입력·카메라·회피 대시 사용. 이동은 기존 Character Movement가 담당하고 현재 제공 클립의 이동 표현을 연결. |
| 17.4 몬스터 | 기존 AI/ICAIController, Character/Base/ICCharacterBase 및 Combat/GAS | 공통 전투 경로를 확장해 기존 몬스터 실행 경로를 보존. 몬스터별 BP·감지·BT/EQS·Mass·개체 수 성능 실험은 이번 NPC 외형 밑작업에 포함하지 않음. |
| 17.5 퀘스트 | 기존 Quest/ICQuestSubsystem, NPC 대화 DT 및 UI/Dialogue/ICNPCDialogueWidget | NPC ID와 대화 정의 연결. 시제품 상점·퀘스트 인물의 기존 ID 유지. 현재 인사말은 임시 문구이며 스토리 분기·목표·보상은 별도 작성. |
| 17.6 GAS·공통 개발 | 기존 Combat/GAS/ICAbilitySystemActions와 신규 Combat/Actors/ICWeaponActor | 기존 비용·스킬별 쿨타임·취소·중복 피격 관리를 사용. 대상 Skeleton의 Montage를 선택하고 기존 Combat Notify로 연결. 무기 길이·배율을 판정과 발사점에 반영. 네트워크 예측 및 복제 완성은 별도 검증. |
| 17.7 UI | 기존 UI/Management/ICUIManagerSubsystem 및 UI/Dialogue/ICNPCDialogueWidget | 기존 CommonUI 관리 안에 선물 동작 연결. 실제 위젯 6종을 7개 해상도로 렌더링해 배치 확인. 실제 3D 배경 대비·긴 콘텐츠·게임패드 포커스는 후속 검증. |
| 17.8 NPC 일정 | 기존 NPC/Character/ICNPCBase, AI/ICAIController, Schedule DT | 11명 정의에 하루 전체 일정 연결, 수면 활동을 공통 애니메이션에 전달. 작성된 StateTree가 없으므로 기존 C++ 일정 실행을 사용. Smart Object 점유·가구 사용·군중 회피는 실제 배치 시 확장. |
| 17.9.1 세이브·로드 | 기존 Save/ICSaveSubsystem, World/State/ICWorldStateSubsystem | 인벤토리·호감도·기술 해금은 기존 상태 원본에서 관리. 무기 Actor는 저장된 장비 ID로 재생성. 별도 BP 저장 원본 추가 없음. 실제 저장 파일 왕복·버전 이전 검증은 후속 작업. |
| 17.9.2 아이템·장비 | 기존 Inventory/Components/ICInventoryComponent, Inventory/Actors/ICDroppedItem 및 장비 DT | 원본 메시를 무기 BP로 감싸 DT에서 교체. 공통 선물의 Cube 외형도 아이템 DT로 교체. 장비 변경 이벤트로 즉시 외형 갱신. |
| 17.9.3 상호작용 | 기존 NPC 상호작용·대화 화면 및 WorldState 거래 처리 | 선물의 거리·상태·수량·호감도 상한을 검사하고 아이템 차감·호감도·기술 해금을 하나의 거래로 반영. 문·채집 등 다른 유형은 기존 구현을 유지. |
| 17.9.4 성능 | 기존 이벤트와 캐시 사용, 이번 자동화 검사 | 무기 외형 갱신용 주기 Tick 추가 없음. 프로필은 캐시하고 필요한 원본 클립만 대상별 생성. 프레임·메모리·로딩의 실제 성능 수치는 아직 측정하지 않음. |

Motion Matching·UAF·Motion Warping 등은 실제 애니메이션 결과, 플러그인 버전,
공격 이동 규칙을 비교한 뒤 채택한다. 연구 목록에 있다는 이유만으로 프로젝트의
기존 이동·전투 실행 체계를 교체하지 않는다. 이번 자산 생성과 검사 스크립트는
Tools/Characters에 두어 다른 지역과 무기에도 같은 절차를 재사용할 수 있게 했다.

## 애니메이션

실제 에셋 메타데이터에서 Mage/Sword의 Sequence1은 UE4 Skeleton,
Sequence2는 UE5 Manny 계열로 확인했다. 남녀 구분이 아니다. Archer 역시 자체
UE5 Skeleton을 사용한다. 원본 애니메이션은 보존하고 대상 Skeleton마다 필요한
동작만 복제·리타겟팅한다. 수면은 Sleep_Animation만 사용한다.

공통 그래프는 지상 이동 BlendSpace → 공중 동작 → 수면 → DefaultSlot 순서다.
대기·걷기·달리기는 속도로 섞고, 일반 공격·스킬·피격은 Montage를 사용한다.
플레이어는 실제 장착 무기 계열을 조회하고, NPC는 등록한 고정 계열을 사용한다.
마법사와 힐러는 Mage의 동작을 공유하되 스킬 Montage를 따로 지정할 수 있다.

Shift는 기존 회피 대시 규칙을 유지한다. 순간 이동과 대시를 구분하고,
대시 이동량도 이동 애니메이션 선택에 반영한다. 지속 달리기 입력으로 변경하려면
전투 기획의 대시 쿨타임과 함께 결정한다.

기존 VRM Post Process ABP에는 SpringBone이 있다. 동일 본에 Kawaii Physics를
중복 추가하지 않는다. Kawaii의 대상 본·충돌체·순간이동 초기화·LOD 적용은
대표 장면에서 비교한 뒤 적용한다. Motion Matching·Motion Warping·UAF는
기존 이동 및 GAS 동작을 검증한 후 별도 비교 대상으로 둔다.

## 무기 교체

장비 DT의 WeaponActorClass가 공통 AICWeaponActor를 상속한 BP를 가리킨다.
BP가 원본 메시를 보유하고, 기존 WeaponMesh 필드는 필요할 때의 명시적 메시
오버라이드로 남긴다. 초기 무기는 Arming_Sword, Bow_2, Bone_Spear_2,
Anciet_Staff이며 원본 파일은 이동하거나 변경하지 않는다.

- weapon_r: 검사·마법사·힐러. weapon_l: 궁수.
- BP의 Visual 상대 변환: 손으로 쥐는 위치와 방향.
- LengthAxis: 칼날/장대 길이 방향. 경계 상자와 실제 배율에서 길이를 계산한다.
- HandleFraction: 판정에서 제외할 손잡이 구간.
- MeleeReachAllowanceCm: 팔 길이 등을 보완하는 명시적 여유 거리.
- Muzzle: 활처럼 팁과 발사점이 다른 무기의 별도 발사 위치.

메시 경계만으로 손잡이의 의미나 날의 앞뒤를 추론할 수는 없다. 자동 계산값과
BP의 그립·축 설정을 함께 사용하며, 실제 자세에서 손·칼끝·발사점을 검증한다.

## 선물·데이터

공통 선물은 Item_P0_01 · 녹즙(테스트), 내부 ID는 대문자 규칙에 따른 ITEM_P0_01.
선호 선물 한 개를 소비하면 +20, 호감도 상한은 500이다. 아이템 부족·잘못된 대상·
효과 없음·거래 검증 실패에서는 차감하지 않는다. 초기 외형은 기본 Cube이며
아이템 DT의 WorldMesh·WorldMeshScale에서 바꾼다.

녹즙은 초록약초(ITM_MAT_GREEN_HERB) 1개로 만든다. 용사는 R01_08이며,
별도 용사 선물(ITEM_HERO_GIFT)을 돼지고기(ITM_MAT_PORK) 2개로 만든다.
R01_08은 용사 선물과 기존 공통 선물 모두 +20으로 받는다. 아이템·레시피·생산·
선호 선물·IDRegistry·StringTable을 연결하며, 기존 공통 아이템 ID는 유지한다.

음악 선택 1~4는 R01_02·R01_04·R01_01·R01_03에 대응한다. 선택 NPC 호감도
+300, 해당 인물의 고유 기술, 검·활·마법사 지팡이·힐러 지팡이의 초보 무기를 지급한다.
이야기 선택의 용사는 R01_08 도감 정보와 용사 레시피를 지급한다. 다른 이야기
지역의 NPC·도감은 비워 두고, 녹즙 레시피만 테스트 보상으로 지급한다.

NPC 초기 능력치는 임시 기본값 1로 구성한다. 고유 기술은 NPC별 ID로 분리하고
효과는 기존 계열별 시작 기술을 사용한다. 이름·직업 외의 전투 밸런스를 확정하는
작업은 아니다. 정식 지역의 생활 장소는 추후 정하며, 아래 테스트 배치는
L_TestMap에서 생성·이동을 확인하기 위한 임시 구성이다.

## L_TestMap의 한 줄 테스트 배치

정식 맵 파일명은 /Game/IsekaiClass/World/Maps/L_OpenWorld다. 정식 월드 완성 전에는
/Game/IsekaiClass/World/L_TestMap을 계속 사용한다. 플레이 맵 설정과 R01 Region의
참조를 L_OpenWorld로 앞당겨 바꾸지 않는다.

맵에는 BP_R01TestLayout 기준 액터 하나를 둔다. NPC BP를 맵에 직접 놓지 않는다.
기준 액터는 실행 시 NPC 정의와 시간표에서 집·활동·침대 ID를 읽어 위치 앵커
33개와 BP_Farm_Bed 11개를 만든다. 이후 NPC 11명 생성은 기존 SpawnSubsystem이
담당하고, 행동과 이동은 기존 Schedule DT와 AI Controller가 담당한다.

- 임시 1걸음 = 70cm로 가정. NPC 간격은 350cm, 침대는 각 NPC 뒤 700cm.
- 기준 위치 (2000, 0, 0), 회전 Yaw 180도. NPC는 플레이어 시작점 방향을 본다.
- 집과 활동 앵커는 줄의 같은 위치를 사용하고, 침대 앵커는 뒤쪽 침대 위치를 사용한다.
- RowSpacingCm, BedDistanceCm, NPCIDs, BedClass는 기준 BP/액터에서 변경 가능.
- 맵에 이동 영역 NavMeshBoundsVolume과 RecastNavMesh를 저장한다.
- 테스트 침대의 충돌·NavMesh 장애물 처리를 끈다. 도착하면 프로필의 침대 상대
  변환으로 수면 메시를 정렬하며 취침 중 무기를 숨긴다. 기상하면 원래 메시 상대
  변환과 무기를 복원한다. 가구 점유·접근 및 입상·퇴상 전환은 후속 제작 대상이다.
  침대 모델의 원본 에셋은 변경하지 않는다.
- 플레이어 생성 후 게임 시간이 실행되어야 NPC가 생성된다. 08:00에는 줄에서 활동,
  20:30에는 각자 침대 방향으로 이동, 다음 날 06:30에는 줄로 복귀하는 일정이다.

Tools/Characters/setup_test_layout.py는 기준 BP·맵·NavMesh·위치 DT를 재현한다.
이미 있는 기준 액터는 재사용하며 중복 생성하지 않는다. NPC Actor나 침대 Actor를
세이브의 새 상태 원본으로 추가하지 않는다.

CSV와 실제 DT를 함께 갱신한다. 신규 PlayerPreset은 기존 BP 배열보다 우선하며
정의가 없으면 기존 배열로 돌아간다. 기존 ID를 재사용하거나 삭제하지 않는다.

## UI 규약

- 기준 디자인 1920×1080, 실제 게임 시야와 UI 모두 16:9 고정.
- 16:10은 위아래, 21:9는 좌우에 검은 여백을 둔다. 세로로 긴 창도 같은 원칙이다.
- ICPlayerCameraManager가 타이틀·플레이·카메라 전환의 투영 비율을 고정한다.
- UI 루트는 AddToPlayerScreen으로 엔진의 constrained player view를 사용한다.
  별도의 마우스 좌표 보정을 겹치지 않는다. 검은 여백의 마우스 버튼 입력은 소비한다.
- Custom DPI 규칙은 창 전체 크기 대신 안쪽 게임 영역 높이를 1080으로 나눈다.
  Anchors·Safe Zone·클릭 판정이 같은 내부 영역을 사용한다.
- 메뉴 폭에는 상한을 두고 긴 내용에는 스크롤을 적용한다.
- 게임 화면은 내부 영역에 비례해 축소한다. 로딩 MoviePlayer도 1920×1080을
  ScaleToFit으로 표시하고 바깥은 검정으로 채운다.
- 초상화·미리보기 비율을 유지하고 신장 차이에 따른 잘림을 검증한다.
- 기존 CommonUI 계층과 UIManager를 통해 포커스·뒤로 가기·정지를 관리한다.
- HP/MP·호감도·장비 등은 상태 변경 이벤트를 사용하고, 쿨타임처럼 시간에 따라
  바뀌는 표시만 필요한 주기로 갱신한다.

## 재현과 검증

1. Tools/Characters/configure_data.py로 데이터 원본 갱신.
2. IsekaiClassEditor C++ 빌드.
3. Unreal Editor Python에서 Tools/Characters/setup_assets.py 실행.
4. 렌더링 에디터에서 Tools/Characters/calibrate_presentation.py 실행 후 보고서 확인.
5. NPC BP 11개, 남녀 프리셋, 무기 BP 4종, 선물 BP, 참조 무결성 확인.
6. 실제 장면에서 발 접지·그립·공격 중단·대시·수면·물리 안정성 확인.
7. 선물 차감/+20/상한/실패 무변경과 저장·로드, 화면비 및 패키징 검증.

기획 정합성 변경을 재현할 때는 configure_data.py 다음에 align_plan_data.py를
실행하고, 빌드 후 import_plan_alignment.py로 실제 DT와 StringTable을 갱신한다.
기존 캐릭터 생성 스크립트는 최신 선택 보상·레시피를 덮을 수 있으므로 이 순서를 지킨다.

### 기획 불일치 수정 내역 — 2026-10-22

| 항목 | 확인한 차이 | 반영한 규칙 |
|---|---|---|
| 수면 | SP가 가득 차면 0분, 최소 2시간 미적용, 플레이어만 회복 | 부족 SP 올림·정수 나눗셈, 최소 120분. 살아 있는 동행 HP/MP 회복, 쓰러진 동료 유지. GameClock만 이동. |
| 퀘스트 소지 | 창고 아이템까지 보유 수량으로 계산 | 가방·장착·퀘스트 전용 아이템만 계산. 창고 이동과 로드 후 현재 수량 재평가. |
| 능력치 | CHA와 동료 보정·효과 배율이 작동 | STR·DEX·CON·INT·LUK의 5종. CHA 열거값·저장 필드는 호환용 보존, 성장·조건·전투 효과는 비활성. 기존 CHA 책은 읽을 수 있으나 사용 불가. |
| 시작 선택 | NPC 미연결, 임시 공통 시작 기술 | 확정한 4명과 고유 기술 연결. 용사 정보·레시피와 다른 지역의 테스트 녹즙 레시피 분리. |
| 기술 애니메이션 | 프로필 공통 Montage가 개별 기술의 Montage를 덮음 | 기술·공격 단계에 명시한 Montage 우선. 비어 있을 때 계열 프로필 사용. |
| 창고 | 4개·160칸 규격 | 1·2·3단계, 40·80·120칸. SetHomeStorageStage 제공, 새 4단계 금지. 기존 초과 슬롯 내용은 보존하며 인출 허용, 신규 보관은 앞 120칸만 사용. |
| 드롭 | PlayClock 30분 유지 | PlayClock 600초(10분). 수면으로 수명 감소하지 않음. |
| 동료·관계 기본값 | DT는 3명/300인데 C++ 기본값은 4명/100 | 기본값·에디터 입력 범위·검증을 3명/300, 관계 범위 0~500으로 일치. NPC 집 접근 기본값 200. |
| UI 문구 | 설정·저장 화면과 알림 일부가 소스 문자열 | ST_IC_UI에서 공통 라벨·알림·서식 조회. 표시용 포맷은 FText::Format 사용. |

창고 단계 API는 설비 업그레이드 거래가 성공한 뒤 호출하는 공통 처리다. 정식
플레이어 집의 설치 슬롯·제작/구매 비용·해금 조건을 임의로 만들지 않는다.
기존 저장의 4번째 설치 및 동료 슬롯을 읽는 호환 범위는 유지하고 새 생성 규칙을 제한한다.

연구 후속 작업은 앞의 17.1~17.9.4 표를 따른다. 우선순위는 정식 월드의 영속 앵커·
스트리밍 복원 → 가구 예약/점유·수면 접근 조건 → 경사면 발 IK·체형별 그립/의상 충돌
→ UI 긴 문구·게임패드 포커스 → NPC/몬스터 수·LOD별 실제 성능 측정이다.
StateTree·Smart Objects·Motion Matching·Motion Warping·UAF·Mass 도입 여부는 이
측정과 버전 검토 후 결정한다. R02~R04 인물, 진엔딩 대상 및 정식 콘텐츠는 아직 미정이다.

### 현재 UI 방식 — C++ 공통 루트, 2026-10-22

공통 `UICUIRootWidget`을 C++로 생성하고 `ScaleBox(ScaleToFit)` → 중앙 정렬된
`SizeBox(1920×1080)` 안에 기존 UI 레이어를 넣는다. HUD·퀘스트 표시·설정·저장창이
모두 이 루트를 사용한다. 기준 크기는 `ICDisplayPolicy`의 공통 상수에서 관리한다.
창 크기에 따른 UI 축소·확대와 클릭 판정은 UMG가 처리하고 버튼 동작은 C++ OnClicked다.
카메라는 기본 `PlayerCameraManager`, 뷰포트는 기본 `CommonGameViewportClient`,
DPI는 기본 `ShortestSide`를 사용한다. 카메라 생성 시 기본 화면비 제약과 플레이어
CameraComponent에 같은 비율을 지정해 게임·UI 영역을 일치시키고 여백은 검정색으로 둔다.
매 프레임 카메라 캐시를 덮어쓰거나 사용자 정의 DPI·뷰포트·렌더 타깃을 사용하지 않는다.
에디터의 기본 별도 창 크기는 1920×1080으로 정리했다.

타이틀 메뉴의 일시정지 전에 기본 카메라를 한 번 초기화한다. 로딩 화면도 공통
기준 크기를 사용하며, 맵 GC 중 글꼴이 제거되지 않도록 GameInstance 서브시스템이 참조한다.

창 크기 변경 중의 D3D12 복사 범위 충돌은 기본 엔진 설정에서도 재현됐다.
VRM4U가 캡처 대상이 없는데도 후처리 콜백을 등록해 오래된 크기로 화면을 복사하던
경로를 막았다. 캡처 대상이 없으면 해당 콜백을 등록하지 않는 작은 수정이다.
저장 UI 3종과 저장 처리는 유지한다. Source 전체 삭제·정리는 계속 보류한다.
아래 화면비 검사 결과는 변경 전 구현의 기록이다.

최종 Editor C++ 빌드와 관련 자동 검사 4종을 통과했다(실패·미실행 0종).
독립 게임 실행에서 12번의 창 크기 변경·최대화·전체 화면·창 모드 복귀를 검사했다.
게임 투영 영역과 UI 영역 일치, 실제 캡처의 검정 여백, 시작 버튼 동작을 확인했다.
1280×800의 위아래 40px, 1680×720의 좌우 200px 여백과 2560×1440 전체 화면의
캡처를 확인했다. 기존 D3D12 복사 범위 오류가 나던 크기 전환도 충돌 없이 완료했다.
추가로 PIE의 크기 변경·버튼, 저장 UI 3종 연결, 새 저장·덮어쓰기·삭제·불러오기와
확인/취소, 설정창의 공통 루트 사용·복귀, 퀘스트 표시의 HUD 연결을 검사했다.
테스트 저장은 별도 접두사만 사용하고 종료 시 정리했다. 전체 게임 검사 41종을
다시 실행한 결과가 아니라 이번 변경에 해당하는 4종의 최종 결과다.

결과: `Saved/DisplayRootBuild.log`, `Saved/DisplayRootStandaloneTests/index.json`,
`Saved/DisplayRootUITests/index.json`, `Saved/DisplayRootFinal.json`.
대표 화면은 `Saved/DisplayRootReview`와 `Saved/SaveUIReview`에 있다.

### 이전 고정 화면비·기획 정합성 검증 결과 — 2026-10-22

- Editor C++ 최종 빌드 성공. 실제 DT 12종, ST_IC_Definitions·ST_IC_UI,
  BP_ICPlayerController의 카메라 매니저 기본값을 갱신했다.
- 실제 PIE 창을 1280×720 → 약 1280×800 → 1680×720 → 800×1000 →
  1280×720으로 연속 변경했다. PIE 테두리 때문에 일부 실행의 실제 클라이언트는
  1274×794다. 검사는 요청 크기와 실제 뷰포트 크기를 구분하고 실제 픽셀을 사용한다.
- 게임 투영·UI 루트 영역 일치, 안쪽 버튼 위치, 검은 여백 클릭 무반응,
  실제 Slate 마우스 입력으로 시작 버튼이 열리는 것을 모두 통과했다.
  UButton.OnClicked를 직접 호출해서 클릭 성공으로 간주하지 않는다.
- 타이틀 초기화·일시정지·카메라 전환에서도 SetCameraCachePOV에서 16:9를
  적용한다. 1280×800은 위아래 40px, 1680×720은 좌우 200px 여백이다.
- UI 6종×7개 해상도의 42개 렌더링에서 초기화된 글자·버튼의 내부 영역 이탈 0건.
  타이틀의 16:10·21:9, 설정·HUD·대화 화면의 대표 이미지를 직접 확인했다.
- 수면 정수 올림·최소 120분·살아 있는 동료 회복·쓰러진 동료 유지·PlayClock 유지,
  창고 제외 퀘스트 소지 수량, 120칸 상한·이전 160칸 인출, 시작 NPC/고유 기술,
  무기별 쿨타임, 기존 저장 v1/v2 및 현행 저장·복원 검사를 통과했다.
- 이 단계의 전체 40종 검사와 수정 후 재검사를 합치면 고유 검사 39종이 통과했다.
  당시 남았던 저장 UI 에셋 누락은 아래의 후속 저장 UI 작업으로 해결했다.

결과 위치:

- Saved/PlanAlignmentBuildFinal.log
- Saved/PlanAlignmentImport.json
- Saved/PlanAlignmentTests/index.json — 전체 실행 결과
- Saved/FixedAspectTests/index.json — 마지막 화면비·실제 클릭·저장 시각 재검사
- Saved/FixedAspectReview/Screens/LayoutChecks.csv — 42개 화면의 배치 검사
- Saved/FixedAspectReview/Title_*.png — 실제 PIE 창의 검은 여백과 UI

### 저장 UI 에셋 누락 해결 — 2026-10-22

기존 `IC.UI.SaveSessionAssets` 검사가 참조하던 WBP 3개를 생성하고 기존 C++에 연결했다.

- `/Game/IsekaiClass/UI/Widget/WBP_IC_SaveLoad` — `UICSaveLoadWidget` 상속.
  자동 저장·수동 저장 목록, 선택 슬롯 정보·미리보기·초상화, 저장·불러오기·삭제·뒤로 가기.
- `/Game/IsekaiClass/UI/Widget/WBP_IC_LoadConfirm` — `UICLoadConfirmWidget` 상속.
  덮어쓰기·불러오기·삭제의 내용 표시와 확인·취소.
- `/Game/IsekaiClass/UI/Widget/WBP_IC_SaveSlot` — `UICSaveSlotWidget` 상속.
  슬롯 상태·시각·미리보기·초상화 표시와 선택 강조.

`BP_ICPlayerController.SaveLoadWidgetClass`가 저장 화면을 참조하고,
저장 화면의 `ConfirmWidgetClass`·`SlotWidgetClass`가 나머지 두 WBP를 참조한다.
남녀 플레이어·NPC 11명의 초상화 13개는 현재 DT의 Portrait 필드를 사용한다.
저장 세션·슬롯 규칙·비동기 저장·복원은 기존 C++ 처리를 그대로 사용하며,
위젯은 공통 UI 루트에 올라가 게임과 같은 16:9 내부 영역을 사용한다.

레이아웃은 `ICFrontendLayout.cpp`에서 만들고 `Tools/Characters/setup_save_ui.py`로
WBP·참조·StringTable을 재생성할 수 있다. 이 스크립트는 저장 UI 3종의 레이아웃을
다시 만들므로, 이후 수작업 디자인을 추가했다면 재실행 전에 변경 내용을 확인한다.
Source 파일 삭제·정리는 사용자 요청대로 계속 보류한다.

최종 Editor C++ 빌드와 `^IC.` 전체 자동 검사 **41종이 모두 통과**했다.
이전 실패였던 `IC.UI.SaveSessionAssets`와 새 `IC.UI.SaveSessionInteraction`도 포함한다.
실제 PIE 창에서 1280×720·1280×800·1680×720으로 크기를 바꾸고 Slate 마우스
입력으로 슬롯을 선택했다. 각 크기에서 저장 UI 영역과 게임 투영 영역이 일치했다.
새 저장·썸네일 생성·덮어쓰기 확인/취소·삭제 확인/취소·불러오기 확인/취소,
저장된 골드 복원, 이전 메뉴 복귀와 일시정지·커서 해제까지 통과했다.
별도 임의 접두사의 테스트 저장만 사용하고 종료 시 정리했다.

최종 결과는 `Saved/SaveUIFinalTests/index.json`, 요약은 `Saved/SaveUIFinal.json`에 있다.
총 41종 중 경고 없이 25종, 경고를 동반한 성공 16종이며 실패·미실행은 0종이다.
경고는 초기 앵커·NavMesh 준비, 최소 테스트 환경의 미등록 테이블·입력/World 컨텍스트,
에디터 네트워크 검사 시간 초과 등이다. 경고 로그를 숨기거나 검사를 비활성화하지 않았다.
화면 이미지는 `Saved/SaveUIReview`의 7개 PNG이며, 16:10 저장 화면과 21:9 저장·확인
화면을 직접 확인했다. 게임패드 전체 탐색과 패키징은 이번 검사 범위에 포함하지 않는다.
현재 플레이 맵은 `L_TestMap`, 정식 맵은 추후 `L_OpenWorld`로 유지한다.

리타겟팅 생성 성공과 시각 검증 완료는 별개다. ResourceReference의 대표 장면·
쿠킹 검증 플래그는 실제 확인을 마친 뒤에만 올린다.

### 이번 실행 결과 — 2026-10-22

- Unreal Engine 5.8.3 Development Editor C++ 빌드 성공.
- 실제 DT·NPC BP·프로필·대상 Skeleton 연결 검사: 321개 통과, 실패 0개.
- `IC.Characters.GiftTransaction` 통과: 실제 NPC 초기화와 활 소켓 장착,
  아이템 1개 차감/+20, 호감도 500 상한, 100 도달 시 기술 해금, 실패 시 무변경.
- `IC.Characters.NativeGraphAndProfiles` 통과: 남녀 플레이어와 NPC 11명의
  공통 그래프가 참조 자세와 다른 본 자세를 계산하고 대상 Montage를 재생.
- `IC.Characters.WeaponScaleAndMuzzle` 통과: 메시·Actor 배율에 따른 길이,
  손잡이 제외 판정 구간, 변경된 팁을 따르는 자동 발사점 계산.
- 초기 메시 준비 전 무기 부착 경고를 보완한 뒤 위 3개 테스트를 다시 통과.
- 애니메이션의 체력 조회는 GAS Attribute, 일정 모드 조회는 저장된 상태 참조를
  사용해 매 프레임 전체 캐릭터 상태 복사를 피한다.

추가로 `IC.Characters.RuntimeLayoutAndSchedule`이 실제 L_TestMap PIE에서 통과했다.
기준 액터 1개 → 앵커 33개·침대 11개 → NPC 11명 자동 생성, 각 침대까지의 완전한
NavMesh 경로, 20:30 침대 이동과 다음 날 06:30 활동 위치 복귀를 검사했다.
취침 시간이어도 침대 도착 전에는 수면 자세를 재생하지 않는 조건도 검사했다.
이 테스트의 저장 파일은 임의의 별도 접두사를 사용하며 테스트 후 정리한다.

캐릭터 자동 테스트는 NullRHI로 실행했다. 정식 지역 배치, 경사면 접지와 의상별
충돌 완성, 일반 저장 슬롯의 전체 왕복 및 패키징은 이 자동 테스트의 검증 범위에
포함하지 않는다. 렌더링을 켠 기본 자세 보정 결과는 아래에 별도로 기록한다.

UI는 별도 렌더링을 켜고 로컬 플레이어·뷰포트 컨텍스트에서 실제 위젯을 검사했다.
메인 메뉴, 생성 첫 화면, HUD, 일시 정지, NPC 대화 선택지, 설정의 6종을
1280×720, 1920×1080, 2560×1440, 1280×800, 1920×1200, 2560×1080,
3440×1440으로 렌더링했다. 프로젝트 DPI 곡선을 사용한 42개 샘플이 생성됐고,
표시된 버튼·텍스트의 화면 바깥 이탈은 0개였다. 16:9·16:10·21:9 대표 이미지를
직접 확인해 주요 글자·버튼이 표시되는 것을 확인했다.

이미지와 검사표는 Saved/UIAspectReview/에 있다. Tools/Characters/capture_ui_aspects.py를
렌더링이 켜진 에디터의 ExecutePythonScript 모드에서 실행하면 재현할 수 있다.
이는 UI 배치 검사이며 실제 3D 배경과의 대비, 모든 대화·긴 목록, 게임패드 포커스와
운영체제 전체 화면 전환까지 검증한 결과는 아니다. UI 디자인을 전면 변경하지 않았다.

### 캐릭터 자세와 무기 기본 보정

남녀 플레이어 각 4계열과 NPC 11명의 고정 계열, 총 19개 조합을 실제 메시로
렌더링했다. 대기·걷기·달리기·공격·스킬·피격·수면의 133개 자세와 손 확대
19개를 `Saved/CharacterReview`에 저장한다. 수치 표는 `PoseMeasurements.csv`다.

- 생성한 애니메이션의 루트 잠금으로 이동 컴포넌트와 애니메이션의 수평 이동이
  중복되는 현상을 보정했다. 원본 팩 애니메이션은 수정하지 않는다.
- 대기 자세의 실제 스킨 정점 하단을 측정해 계열별 `GroundOffsetCm`에 저장한다.
  현재 보정량은 약 -0.44~+1.56cm다. 평지 기준 보정이며 경사면 발 IK와 다르다.
- 손목 원점 대신 검지·새끼손가락 시작 본 사이에 그립 소켓을 둔다. 무기 메시의
  긴 방향을 분석하고 BP Visual의 회전·배율·위치를 보정한다. 검 95cm, 활 120cm,
  마법사 지팡이 170cm, 힐러 지팡이 150cm를 임시 시각 기준으로 사용한다.
  활은 중앙, 검은 손잡이 쪽, 지팡이는 아래쪽 35%를 기준으로 잡는다.
- 수면은 같은 Sleep_Animation 팩의 `anim_Sleep_06_L`을 리타겟팅해 사용한다.
  무릎을 높게 세우는 기존 샘플보다 긴 치마에서 자세가 덜 들리는 옆누움이다.
  `SleepMeshTransform`은 현재 BP_Farm_Bed와 체형별 머리·골반 위치에 맞춘 값이다.
  침대 종류나 배율이 달라지면 해당 침대 기준 변환을 다시 보정해야 한다.
- 취침·기상과 큰 위치 이동에서 기존 애니메이션 물리를 초기화한다. 기존 VRM
  SpringBone 외에 Kawaii Physics 같은 두 번째 보조 물리를 겹쳐 넣지 않는다.
  30·60·120Hz로 계산한 133개 자세에서 NaN과 과도한 본 좌표는 발견되지 않았다.
  이 검사는 모든 의상·머리카락의 접촉 관통이 없음을 보장하지 않는다.
- 검사 임시 스킬은 지상 판정과 맞도록 Sword_Animations의
  `Combo_Attack_02_01_Seq`를 사용한다. 공중 도약이 포함된 Power_Attack_End 샘플은
  현재 스킬에 연결하지 않는다.

`calibrate_presentation.py`는 수면·검사 스킬을 준비하고 보정값과 이미지를 저장한다.
`review_presentation.py`는 저장 없이 현재 값을 검수한다. 설정을 다시 생성할 때
기존 무기 BP 그립과 프로필 접지 보정값을 보존하며, 리타겟터에 기본 연산을
중복 추가하지 않는다. 체형이 바뀌면 보정 스크립트를 다시 실행한다.

최종 보정 후 Editor 빌드, 에셋 연결 321개 검사와 캐릭터 자동 테스트 4종을
다시 통과했다. 실제 PIE 검사에는 침대 도착 후 메시 정렬, 취침 중 무기 숨김,
다음 날 기상 시 무기 표시와 캡슐 기준 메시 복원도 포함한다. 테스트 시작 시
아직 생성되지 않은 앵커를 기다리는 경고와 기존 상점 NPC의 앵커 경고가 있지만,
R01 테스트 대상 11명의 생성·경로·일정·복원 검사는 모두 성공했다.

소스의 삭제·통합 후보와 유지 사유는 `Docs/Characters/Source_Audit.ko.md`에 정리했다.

### 캐릭터 생성·ESC 메뉴·저장 슬롯 변경 — 2026-10-22

캐릭터 생성의 진행 숫자·돌아가기·보상 설명·최종 검토/시작 확인창을 제거했다.
이름 입력은 [이름 설정], 나머지는 선택 버튼으로 진행하며 행복 질문을 선택하면
기존 선택 보상을 적용하고 로딩 후 L_TestMap에서 시작한다. 요청한 질문·선택지
문구를 StringTable에 반영하고 한 줄로 표시한다. 새 게임은 경고 없이 자동 저장을
교체하며 기존 수동 저장은 유지한다.

ESC는 게임을 멈추고 [파티·인벤토리·퀘스트·도감·설정] 메뉴를 연다. 앞의 네 버튼은
클릭만 가능하다. 설정 하위에는 [계속하기·저장·불러오기·게임 설정·그만하기]를
두었다. 저장·불러오기·옵션·종료 취소는 이전 하위 메뉴로, 하위 메뉴 ESC는 첫
메뉴로 돌아간다. 첫 메뉴 ESC와 계속하기는 게임을 재개한다.

저장 시 SceneCapture·렌더 타깃·PNG 생성 및 저장 UI의 사진 읽기/표시를 제거했다.
기존 저장의 사진 필드는 호환을 위해 읽을 수 있지만 디코딩과 새 직렬화 시 비운다.
저장 화면·슬롯·확인창의 사진 영역도 제거했다. 슬롯에는 캐릭터 이름, 캐릭터 얼굴,
동료 3칸의 얼굴(비어 있으면 ?), 임시 지역명 [지역 1], PlayClock 누적 플레이 시간,
실제 저장 시각만 표시한다. 이전 저장에 파티 미리보기 ID가 없어도 플레이어 성별의
얼굴을 사용한다. 플레이 시간은 수면으로 건너뛰는 GameClock과 구분한다.

Editor 빌드 성공. 변경 관련 고유 자동 검사 13종의 최종 결과는 모두 성공이다.
키 입력·실제 마우스로 생성부터 맵 이동·자동 저장 교체·수동 저장 보존·메뉴 복귀를
확인했다. 사진 없는 저장·덮어쓰기·삭제·불러오기, 빈 동료칸, 화면 크기별 클릭,
저장 왕복·비동기 저장·기존 버전/백업 복원을 포함한다. 전체 게임 검사를 재실행한
결과는 아니다. 결과: `Saved/CreationMenuFinal.json`, `CreationMenuFinalTests`,
`CreationMenuPlaythroughTests`. 생성·메뉴 캡처는 `Saved/CreationMenuReview`에 있다.
Source 전체 삭제·정리는 계속 보류한다.
