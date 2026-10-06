# DataTable·ID·문자열 규칙

기획서 17장을 기준으로 작성한 1차 계약이다. 아래 DT는 팀원이 만들 스키마이며 이번 구현에서 .uasset이나 전체 DT 행을 생성하지 않았다.

## 등록부와 번호 배정

Data/ID_Registry.csv의 ID, Type, Owner, RangeStart, RangeEnd, Status, bDeprecated, Notes를 사용한다.

- ID는 영문 대문자·숫자·밑줄, 최대 40자. Row Name = ID. 참조는 FName 또는 Gameplay Tag.
- R00 공용, R01~R04 지역. 기본 시리얼 001~999; 일과는 01~99.
- 한 번 쓴 ID는 이름 변경·재사용·번호 당기기를 금지한다. 폐기는 Status=Deprecated, bDeprecated=true로 남긴다.
- Reserved는 예약이며 해당 콘텐츠 구현 완료를 뜻하지 않는다. Active도 스킬 ID 등록을 뜻하며 실제 Ability 완성을 보증하지 않는다.
- 이번 등록부는 프로토 최소 예약과 기존 코드 사용 ID를 담았다. 담당자는 신규 ID를 추가할 때 팀장이 배정한 범위만 사용한다.
- 현재 시드 대역: 플레이어 고정은 1번; 아이템·장비·레시피·컨테이너 001~099는 2번; 전투 ID 001~099는 3번; 일과 01~99 및 NPC 상주·배경 대역은 4번; 영입형 NPC·대화·퀘스트는 5번; 앵커·SO·워프는 6번.
- 확장 시 100번 이후 대역을 팀장이 등록한다. NPC는 001~099 영입, 101~199 상주, 201~299 배경을 유지한다.

| Type | 형식 |
| --- | --- |
| Character | CHR_PLAYER |
| NPC | NPC_R01_001 |
| Monster / Boss | MON_R01_001 / BOS_R01_001 / BOS_FIN_001 |
| Skill | SKL_WAR/ARC/MAG/HEL_001, SKL_NPC_001, SKL_MON_001 |
| Debuff | DBF_001 |
| Item | ITM_CONS/FOOD/BOOK/MAT/GATH/FISH/CRAFT/QST/TKT_001 |
| Equipment | EQP_WPN_SWORD/BOW/STAFF/WAND_001 또는 EQP_ARM/RNG/NCK_001 |
| Recipe | RCP_COOK/CRAFT_001 |
| Quest | QST_R01_MAIN/SIDE_001, QST_FIN_001 |
| Dialogue | DLG_NPC_R01_001_001 |
| Schedule | SCH_NPC_R01_001_01 |
| Anchor / Warp | ANC_R01_001 / WRP_R01_001 |
| SmartObject | SO_BED/WORK/KITCHEN/FORGE/SHOP/FARM/TREE/ORE/FISH/SEAT_R01_001 |
| Container | CNT_HOME/NPC/WORLD_R01_001 |
| Runtime drop | DRP_{Guid}; DT 행 없음. 공용 모델은 FGuid로 저장 |

슬래시로 나열한 부분은 허용 후보이며 실제 ID에는 슬래시를 넣지 않는다.

## 기존 데모 예외

Legacy_ID_Allowlist.csv의 Sword, Food, PlayerHome, SKL_HEAL_001 등을 명시적으로 허용한다. ReplacementID는 정식 ID 후보이고 자동 치환 기능이 아니다. 기존 저장과 장비 GUID를 보존한다. 정식 스킬 규격은 HEL이고 기존 HEAL은 이전 작업을 별도로 승인하기 전까지 유지한다.

Player·Companion1~4는 UI 대상 별칭이다. RestoreHP/RestoreSP/GrowSTR은 기존 시험 효과 코드다. 신규 콘텐츠에서 이 예외를 확대하지 말고 정식 ID와 Gameplay Tag를 사용한다.

## 공통 DT 컬럼

ID(FName), bDeprecated(bool), 표시 문자열의 키(NameKey 또는 DescKey)를 공통으로 사용한다. 표시 문자열 원문은 DT에 중복 저장하지 않는다. 공통 RowStruct는 각 기능 담당이 생성하며 복잡한 장비·스킬은 Primary Data Asset을 병용할 수 있다.

| DT 및 소유자 | 필수 컬럼 | 검사·연결 규칙 |
| --- | --- | --- |
| DT_IC_NPC, 4·5번 | ID, bDeprecated, NameKey, Kind, RegionTag, ScheduleID, DialogueID, JobTag, SkillIDs, BaseStats, AppearanceAsset | Kind별 번호 대역. 성장 스탯은 영입형만. 외형 에셋은 1·8번 연결 |
| DT_IC_Item, 2번 | ID, bDeprecated, NameKey, DescKey, Kind, StackLimit, EffectID, Price, Icon | 장비 StackLimit=1, 일반 <=99. EffectID는 Ability/Effect 데이터 참조 |
| DT_IC_Equipment, 2·3·8번 | ID, bDeprecated, NameKey, Slot, WeaponFamily, AttributeModifiers, MeshAsset, SocketName, AnimationProfile | WPN/ARM/RNG/NCK. 강화·옵션은 아이템 인스턴스와 구분 |
| DT_IC_Quest, 5번 | ID, bDeprecated, NameKey, DescKey, PrerequisiteQuestIDs, ObjectiveTargetIDs, ObjectiveCounts, RewardItemIDs, RewardCounts, DialogueID | 수량 배열 길이 일치. 완료된 퀘스트는 수락 기록 필요 |
| DT_IC_Skill, 3번 | ID, bDeprecated, NameKey, DescKey, ClassTag, AbilityAsset, EffectIDs, MPCost, SPCost, CooldownPlaySeconds, TargetRule | Q/E/R/T 슬롯 대응은 클래스 데이터. AbilityAsset 유효성은 후속 에디터 Validator |
| DT_IC_Recipe, 2번 | ID, bDeprecated, NameKey, IngredientIDs, IngredientCounts, ResultItemID, ResultCount, RequiredObjectTag | 구매·제작 실패 시 재료/결과물 부분 변경 금지 |
| DT_IC_Schedule, 4번 | ID, bDeprecated, NpcID, StartGameMinute, EndGameMinute, AnchorID, SmartObjectID, ActivityTag | GameClock 기준. 일과 점프 시 재배치 |
| DT_IC_Warp/Anchor/SO, 6번 | ID, bDeprecated, RegionTag, LevelAsset, ActorReference | World Partition 스트리밍·레벨 참조는 6번 검증 |

현재 FPlayerItemDefinition의 실제 필드는 ItemID, Name(FText), Kind, EquipSlot, StackLimit, bCanDiscard, bCanSell, EffectID이다. 새 공용 타입은 기존 정의를 옮겼으며 정식 DT 스키마로 강제 변환하지 않았다. 2번은 기존 ItemTable의 RowName·ItemID 일치와 위 스키마의 가격·아이콘·외형·소켓 등 추가 컬럼을 단계적으로 적용한다.

## String Table과 에셋 이름

String Table 이름은 ST_IC_NPC, ST_IC_DLG, ST_IC_QST, ST_IC_ITM, ST_IC_SKL, ST_IC_UI, ST_IC_SYS이다. 키는 {ID}.{FIELD}, 예: NPC_R01_001.NAME, ITM_CONS_001.DESC. UI 키 예: UI_SAVE.TITLE, UI_SAVE.FAILED. 문서에서 FIELD는 대문자로 작성한다.

신규 대사·표시문자열은 처음부터 String Table에 입력한다. 기존 프로토 코드의 한국어 문자열은 이번 단계에서 유지했다. 전체 UI 이관은 7번의 후속 작업이다. 키나 테이블을 바꿀 때도 참조 변경을 명시적으로 검토한다.

에셋 접두사는 DT_IC_, DA_IC_, BP_IC_, ABP_IC_, WBP_IC_, NS_IC_, ST_IC_. 신규 공용 C++ 타입은 UIC/AIC/FIC/EIC로 시작한다. 기존 플레이어 타입은 호환을 위해 유지한다.

## 파일 기반 Validator 사용

Python 3 표준 라이브러리만 사용한다.

~~~powershell
python Tools/ValidateIds.py --self-test
python Tools/ValidateIds.py --tables "Data/Exports/*.csv"
python Tools/ValidateIds.py --baseline "C:/PreviousBuild/ID_Registry.csv" --tables "Data/Exports/*.csv"
~~~

기본 검사는 등록부·예외 목록이다. --tables를 주면 지정한 CSV의 행·참조까지 검사한다. 파일이 없으면 실패한다. 기본 이력은 git HEAD의 등록부이며 첫 추가 전에는 이력이 없어 'not yet committed'를 표시한다. 이미 배포한 빌드에 대해 검사하려면 --baseline으로 그 등록부를 제공한다. 이력 대비 삭제·폐기 ID 재활성화·종류/담당 변경을 거부한다.

테이블 CSV는 다음처럼 정규화한다.

~~~csv
ID,bDeprecated,NameKey,ResultItemID,IngredientIDs
RCP_COOK_001,false,RCP_COOK_001.NAME,ITM_FOOD_001,ITM_MAT_001
~~~

- 기본 키 컬럼은 ID, RowName 또는 Unreal 내보내기의 ---이다. 둘 이상 있으면 값이 같아야 한다.
- ItemID처럼 Id/IDs로 끝나는 컬럼은 등록부 또는 예외 목록의 참조로 검사한다.
- 복수 참조는 |로 구분하거나 JSON 문자열 배열로 내보낸다. CSV의 따옴표는 표준 CSV escaping을 적용한다.
- 빈 문자열 또는 None은 선택 참조를 뜻한다.
- Unreal 구조체/배열의 복잡한 괄호 직렬화는 이 검증기가 해석하지 않는다. 참조 ID를 별도 단순 컬럼으로 정규화한다.
- .uasset, SoftObjectPath, 실제 Ability/Socket 존재, 수량·밸런스 공식까지 검사하는 에디터 Validator는 2차 작업이다.

BalancePrototype.csv는 현재 코드 값과 미정 항목의 목록이다. 런타임이 CSV를 직접 읽는 기능은 없으며 값 변경 시 담당 코드/DT도 함께 갱신해야 한다. Pending의 빈 값을 임의 숫자로 채우지 않는다.
