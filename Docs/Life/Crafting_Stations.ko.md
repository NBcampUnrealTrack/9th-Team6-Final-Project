# 제작대와 주방

테스트맵 `/Game/IsekaiClass/World/L_TestMap`의 왼쪽 생활 테스트 구역에 배치합니다.
제작대 위치는 `(2000, -3100, 40)`, 주방은 `(2000, -3550, 40)`입니다.
채집 구역이 시작되는 부분이며, 외형은 충돌이 있는 기본 박스입니다.

| 용도 | 에셋 |
|---|---|
| 제작대 | `/Game/IsekaiClass/Life/Crafting/BP_IC_CraftingTable` |
| 제작 UI | `/Game/IsekaiClass/Life/Crafting/WBP_IC_CraftingStation` |
| 주방 | `/Game/IsekaiClass/Life/Cooking/BP_IC_Kitchen` |
| 요리 UI | `/Game/IsekaiClass/Life/Cooking/WBP_IC_CookingStation` |

접근 후 F로 열면 이름만 있는 레시피 목록이 나옵니다. 마우스로 클릭하거나 W/S·방향키로 선택한 뒤 Enter로 상세를 엽니다.
상세에는 이름·설명·결과·시간·SP·필요 재료와 보유량이 표시됩니다. 만들기 또는 Enter로 시작하고, 취소 또는 Esc로 목록에 돌아갑니다.
재료·SP가 부족하면 만들기를 비활성화합니다. 실제 시작에서는 잠금 재료, 진행 중인 다른 제작, 가방 공간 등도 다시 검증합니다.

제작을 시작하면 게이지가 차고, 완료 시 재료를 차감하고 결과를 지급한 뒤 화면을 닫습니다.
기존 몬스터 보상의 `WBP_IC_RewardNotice`로 `+ 아이템 이름 N개 획득`이 표시됩니다.
진행 중 창을 닫아도 제작은 계속되며, 같은 작업대를 다시 열면 같은 게이지로 복귀합니다.
완료 시 가방 공간 등이 부족해지면 결과를 중복 지급하지 않고 대기 상태로 보존합니다. 다시 열어 재수령할 수 있습니다.
재료는 시작 시 예약되고 SP는 시작 시 지불됩니다. 저장·불러오기는 기존 생산 작업 저장 기능을 사용합니다.

## 레시피 편집

`/Game/IsekaiClass/Data/DataTable/Recipes/DT_IC_Recipe`에서 변경합니다.

| 필드 | 동작 |
|---|---|
| `DisplayName` | 목록·상세·진행 화면의 레시피 이름 |
| `DescriptionKey` | `ST_IC_Definitions`의 설명 키 |
| `Type` | `Craft` 제작대 / `Cook` 주방 |
| `Ingredients` | 재료 ItemID와 개수 |
| `Tools` | 보유해야 하는 도구, 소모하지 않음 |
| `Results` | 결과 ItemID와 개수. 획득 알림도 결과 아이템의 DisplayName 사용 |
| `WorkDurationPlaySeconds` | 실제 제작 시간과 게이지 전체 시간. 기본 5초 |
| `SPCost` | 시작 시 소모할 SP |
| `bUnlockedByDefault` | 기본 공개 여부. 잠긴 레시피는 해금한 뒤 목록에 표시 |
| `ProductionID` | 작업 조건을 제공하는 `DT_IC_Production` 행 |

레시피의 시간과 SP는 이 DT가 기준입니다. `DT_IC_Production`의 중복 시간·SP 값을 함께 수정할 필요가 없습니다.
생산 행의 작업대 제한·도구·시작 조건 검증은 기존 규칙을 따릅니다.
기본 제작 목록은 검·활·마법지팡이·치유완드이며, 주방은 기존 요리 레시피를 사용합니다.

네이티브 구현: `AICCraftingStation`, `UICCraftingStationWidget`, `UICProductionSubsystem`.
WBP 위젯 트리는 편집 가능하며 `Tools/Life/setup_crafting_stations.py`는 최초 배치·초기 UI를 다시 만드는 도구입니다.
수동으로 편집한 WBP를 보존하려면 재생성 스크립트를 다시 실행하지 마세요.
