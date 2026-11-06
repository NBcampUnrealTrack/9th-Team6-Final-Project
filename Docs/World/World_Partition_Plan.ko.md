# L_OpenWorld 월드 파티션 전환 계획

2026-11-06 기준으로 **맵과 NPC 배치를 먼저 마무리한 뒤 월드 파티션을 검증한다.** 팀의 배치가 확정되면 최종 빌드 전에 검증 시간을 확보하고, `L_OpenWorld`를 보존한 상태에서 시험 맵을 전환한다.

## 현재 작업 기준

- 실제 플레이 맵은 `/Game/IsekaiClass/World/Maps/L_OpenWorld`다. 에디터에서 확인한 맵에는 World Partition이 없고, `NPCAnchor`와 `L_SmartObject`는 `Always Loaded` 서브레벨이다.
- 배치 작업은 현재 맵과 서브레벨에서 계속한다. NPC의 집·침대·작업 위치, 퀘스트 목표, 워프 도착 지점에는 고유 앵커 ID를 사용한다. 앵커를 옮기거나 바꾸면 배치 액터, 참조하는 데이터 테이블, 좌표가 함께 맞는지 확인한다.
- NPC 일과, 퀘스트 목표, 워프의 새 배치를 검토할 때는 목적지가 다른 구역에 있을 때도 필요할 위치 정보와, 현장에서 실제로 로드해야 하는 액터를 구분해 기록한다. [앵커 데이터](../../Data/World/DT_IC_Anchor.csv) · [워프 데이터](../../Data/World/DT_IC_Warp.csv)

## 전환 시 영향과 처리 방향

| 기능 | 현재 의존성 | 시험 맵에서 확인할 내용 |
| --- | --- | --- |
| NPC 생성과 일과 | 생성은 로드된 앵커, 일과는 현장에 등록된 생활 오브젝트를 사용한다. | 지역이 로드되면 생성하고 내려가면 정리한다. 앵커만 남아 있는 먼 구역에서는 바닥 없이 생성을 시도하지 않도록 한다. 다시 접근하면 일과와 상태가 이어지는지 본다. |
| 퀘스트 안내 | 살아 있는 NPC나 로드된 앵커에서 위치를 얻고, NavMesh에 경로를 요청한다. | 멀리 있는 목표의 위치는 앵커 ID와 데이터로 추적할 수 있게 한다. 안내선은 로드된 구역에서 경로가 확보되면 갱신한다. 경계 밖 목표, 경계 통과, 완료 보고를 각각 확인한다. |
| 워프 | 지도는 로드된 워프돌을 수집하고, 이동은 도착 앵커를 찾아 즉시 실행한다. | 먼 워프도 데이터에서 목록을 구성한다. 목적지에 임시 스트리밍 소스를 두고 셀과 바닥 충돌이 준비된 뒤 플레이어와 동료를 이동시킨다. 도착하지 못하면 위치와 비용을 보존한다. |
| 이어하기와 저장 불러오기 | 저장한 좌표를 읽어 플레이어 위치를 복원한다. | 시작점과 다른 셀의 기존 세이브로 검증한다. 저장 좌표의 지형·충돌이 준비될 때까지 로딩 화면을 유지한다. |

[NPC 생성](../../Source/IsekaiClass/Private/World/Spawn/ICSpawnSubsystem.cpp) · [생활 오브젝트](../../Source/IsekaiClass/Private/NPC/LifeObject/ICLifeObjectActor.cpp) · [퀘스트 안내](../../Source/IsekaiClass/Private/Quest/ICQuestGuideComponent.cpp) · [워프 지도](../../Source/IsekaiClass/Private/UI/Map/ICMapWidget.cpp) · [워프 이동](../../Source/IsekaiClass/Private/World/Warp/ICPartyWarpComponent.cpp) · [저장 위치 복원](../../Source/IsekaiClass/Private/Core/Controller/ICPlayerController.cpp)

## 배치 확정 후 전환 순서

1. 원본 맵과 서브레벨의 Git 기준을 남긴다. 에디터의 **Tools → Convert Level** 또는 변환 커맨드렛을 사용한다. 커맨드렛은 `-ReportOnly`로 예상 변경을 확인하고, `-ConversionSuffix`로 `_WP` 시험 맵을 만들 수 있다. [Epic 월드 파티션 변환 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)
2. 시험 맵에서 지형·장식·충돌을 셀로 나눈다. 낮밤 컨트롤러와 전역 조명처럼 월드 전체에서 필요한 액터를 분류하고, 서로 참조하는 액터가 한 묶음으로 로드되는지도 점검한다. 시험 맵을 실제 게임 흐름에서 열 때는 `PlayMap`, 패키징 맵 목록, 지역 데이터의 맵 경로를 함께 확인한다. [게임 맵 설정](../../Config/DefaultGame.ini) · [지역 데이터](../../Data/World/DT_IC_Region.csv)
3. NPC·퀘스트의 NavMesh와 워프 목적지 선로딩을 검증한다. 셀을 왕복하며 액터가 정상적으로 로드·언로드되는지 확인하고, 워프 4곳과 멀리서 저장한 기존 세이브로 복귀를 시험한다. [Epic 월드 파티션 NavMesh 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partitioned-navigation-mesh)
4. 같은 위치와 그래픽 설정에서 전후 메모리, 프레임 시간, 구역 진입 시 끊김을 비교한다. 팀이 검증 결과를 확인한 뒤 최종 플레이 맵에 반영한다.
