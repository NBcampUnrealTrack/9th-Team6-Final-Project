# 생활·아이템 DT 편집

## 실제 에셋 위치

콘텐츠 브라우저의 `/Game/IsekaiClass/Data/DataTable` 아래에서 편집합니다.
`Data/`의 같은 이름 CSV는 에셋을 내보낸 원본입니다. CSV만 바꾼 경우 해당 DT에 재임포트해야 반영됩니다.

| 대상 | DT | 조절할 필드 |
|---|---|---|
| 채집물·광석 | `Life/DT_IC_Resource` | `Yields`, `RegenerationGameMinutes`, `AvailableMesh`, `VisualTransform`, `OreCollection`, `MaterialOverride` |
| 작물 | `Life/DT_IC_Crop` | `bGrowsNaturally`, `GrowthStages`, `HarvestYields`, `RegrowthGameMinutes` |
| 닭 | `Life/DT_Ranching`의 `RAN_R01_01` | `Mesh`, `MeshTransform`, `IdleAnimation`, `AnimationClass`, `MaxHP`, `DropTableID`, `RespawnGameMinutes` |
| 닭 사냥 보상 | `Rewards/DT_IC_DropTable`의 `DRP_R01_CHICKEN` | `Entries`의 `ItemID`, `Probability`, `MinimumCount`, `MaximumCount` |
| 아이템 가격·회복 | `Items/DT_IC_Item` | `BuyPrice`, `SellPrice`, `bCanSell`, `RestoreHP`, `RestoreMP`, `RestoreSP`, 각 회복 비율 |
| 상점 취급 품목 | `NPC/DT_IC_Shop` | `Entries`의 `ItemID`, `DisplayOrder` |
| 제작·요리 | `Recipes/DT_IC_Recipe` | `Ingredients`, `Results`, `WorkDurationPlaySeconds`, `SPCost`, `bUnlockedByDefault` |
| 이름·설명 | `StringTables/ST_IC_Definitions` | `<ID>.NAME`, `<ID>.DESCRIPTION` |
| 생활 상태 문구 | `StringTables/ST_IC_UI` | `Life_HarvestReady`, `Life_Growing`, `Life_Regrowing`, `Life_BagFull` |

## 게임시간 기준

- 채집 후 재생은 `RegenerationGameMinutes=120`: 게임시간 2시간입니다. 기존 `RegenerationPlaySeconds`는 과거 데이터 호환용이며 신규 재생에는 사용하지 않습니다.
- 작물은 원본의 눈에 보이는 메시만 순서대로 등록했습니다. 첫 메시부터 시작하고, 마지막 메시를 제외한 각 단계의 `DurationGameMinutes=1440`입니다.
- 밀 5단계 → 4일 뒤 최종 단계, 외줄밀 4단계 → 3일 뒤, 양배추 5단계 → 4일 뒤, 유채·해바라기 각 2단계 → 1일 뒤 최종 단계입니다.
- 마지막 단계의 `bHarvestable=True`일 때만 F로 수확합니다. 이전 단계의 플래그만 켜서는 조기 수확되지 않습니다.
- 현재 R01 작물은 `bGrowsNaturally=True`: 씨앗을 소모하지 않고 자생합니다. 수확하면 120분 동안 숨겨지고, 다시 첫 단계부터 성장합니다. 수동 재배용으로 False를 설정하면 기존 씨앗 심기 방식을 사용합니다.
- 잠자기나 게임시간 건너뛰기도 반영됩니다. 성장 시작 시각·수확 후 재생 시각·채집 재생 시각을 저장합니다.
- 배치한 생활 오브젝트의 `PointID`는 서로 다르게 유지해야 각각 독립적으로 저장됩니다.

## 닭 사냥

`/Game/IsekaiClass/Life/Ranching/BP_R01_Chicken`은 `ICRanchAnimal`을 부모로 사용합니다. 장식용 생활 액터에서 실제 전투 피해를 받는 캐릭터로 변경했습니다.

- 닭고기 1개: Probability 1.0, MinimumCount 1, MaximumCount 1.
- 달걀: Probability 0.5, MinimumCount 1, MaximumCount 4. 확률과 수량은 모두 드롭 DT에서 수정합니다.
- 인벤토리에 들어온 물품은 기존 획득 알림을 사용합니다. 가방이 꽉 차면 넘친 보상은 월드 드롭으로 남습니다.
- 처치 여부·남은 HP·보상 지급 식별자를 저장하여 중복 지급을 방지합니다.
- 닭의 `RespawnGameMinutes` 기본값은 0이므로 죽인 닭은 자동 재생하지 않습니다. 양수를 입력하면 그 게임시간 후 재생합니다. 이 필드는 채집·작물의 2시간 재생과 별개입니다.
- 소·돼지·양은 기존 외형을 유지합니다. 이번 전투·처치 보상 연결은 닭에 적용했습니다.

## 아이템·레시피

요청한 기존 18개 재료·테스트·임시 아이템 행을 제거했습니다. 성장서는 사용 불가능한 옛 매력의 서 3종도 정리하여 요청한 15종만 남겼습니다. 유효한 아이템은 97종입니다. 상점·드롭·선물·캐릭터 생성 보상·테스트 퀘스트의 아이템 참조도 실제 생활 아이템으로 바꿨습니다. 기존 퀘스트의 이야기나 진행 순서는 재작성하지 않았습니다.

| 무기 | 레시피 | 재료 |
|---|---|---|
| 초보자 검 | `RCP_CRAFT_001` | 광석1×3 + 광석3×1 + 갈대×2 |
| 초보자 활 | `RCP_CRAFT_BOW_001` | 갈대×3 + 키 큰 풀×2 + 광석2×1 + 물고기1×1 |
| 초보자 마법스태프 | `RCP_CRAFT_STAFF_001` | 광석4×2 + 광석5×1 + 분홍꽃×2 + 해바라기×1 |
| 초보자 치유완드 | `RCP_CRAFT_WAND_001` | 광석6×1 + 흰꽃×2 + 유채×2 + 물고기4×1 |

기본 빵은 밀×2, R01 밀빵은 요청대로 밀×3입니다. R01 요리 30종은 요청한 재료 조합·결과물 1개·HP 25/SP 15 회복을 유지합니다. 기본 제작 시간은 5초입니다.

씨앗 구매가는 10/12/14/16/18, 판매가는 4/5/6/7/8로 초기 설정했습니다. 기존 상인의 상점에 씨앗 5종을 등록했습니다. 다른 생활 재료·요리의 기존 판매가 10은 유지했습니다.

축산물 기본 회복량(HP/SP)은 달걀 5/5, 우유 10/10, 돼지고기 10/5, 양털 0/0, 닭고기 10/5입니다. 양털은 기본 회복량이 없어 사용해도 소모되지 않습니다. 회복 수치와 판매가는 아이템 DT에서 변경할 수 있습니다.

퀘스트 반지의 퀘스트 탭·버리기/판매/선물 불가 설정은 유지합니다. 저장된 삭제 아이템은 수량과 인스턴스 식별자를 유지한 채 실제 생활 아이템으로 변환하며, 창고·퀵슬롯·드롭·도감·진행 중 제작 예약에도 적용합니다. 폐기된 테스트 제작법은 기존 R01 요리로 대응시킵니다.

## 번역과 도구

아이템과 레시피의 `DisplayName`은 `ST_IC_Definitions`의 이름 키를 참조하는 FText입니다. 아이템 ID는 언어에 따라 바꾸지 않습니다. 번역은 동일한 키에 대한 로컬라이제이션 자료로 관리합니다. 획득 알림도 해당 아이템 이름을 사용합니다.

이번 데이터 마이그레이션 도구는 `Tools/Life/configure_life_data.py`입니다. 초기 기본값을 다시 지정하는 일회성 정리 도구이므로, 이후 DT에서 밸런스를 수정한 뒤 일상적으로 재실행하지 마세요. 평소에는 DT 편집 또는 CSV 재임포트를 사용하세요. 과거 프로토타입 생성 도구의 재실행은 현재 데이터 관리 절차에 포함하지 않습니다.
