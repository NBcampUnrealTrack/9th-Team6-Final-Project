# Source C++ 정리 검토

2026년 10월 22일 기준으로 `Source`의 헤더 128개와 CPP 102개를 검사했다.
CPP 수에는 이번 캐릭터 자세 검수용으로 추가한 파일 한 개가 포함된다.
내용이 완전히 같은 파일은 없었다. 아래는 삭제 실행 내역이 아니라 정리 후보와
유지 사유다. 이번 검토로 기존 소스 파일을 삭제하지 않았다.

## 불필요한 파일 껍데기

| 파일 | 확인한 내용 | 정리 방법 |
| --- | --- | --- |
| `Source/IsekaiClass/Private/Save/Compatibility/ICLegacyPlayerSaveGame.cpp` | 자기 헤더를 include하는 한 줄뿐이다. 함수·정적 변수·등록 코드가 없다. | CPP만 제거하는 후보. 같은 이름의 헤더와 저장 변환 코드는 유지한다. |
| `Source/IsekaiClass/Public/Core/ICModule.h` | pragma once와 CoreMinimal include만 있고 선언이 없다. 사용하는 곳은 `Private/Core/ICModule.cpp` 하나다. | CPP에서 CoreMinimal을 직접 include하면 이 헤더를 없앨 수 있다. 모듈 구현 CPP는 유지한다. |

빈 CPP는 빌드 입력을 한 개 늘리고, 빈 헤더는 경유 include를 한 단계 늘린다.
둘을 제거해도 큰 성능 개선이 생기지는 않는다. 역할이 없는 껍데기를 줄이는 정리다.

## 현재 참조가 없는 구형 기능 후보

| 파일 | 현재 상태 | 제거 전에 함께 처리할 부분 |
| --- | --- | --- |
| `Public/UI/Inventory/ICInventoryPreview.h`와 `Private/UI/Inventory/ICInventoryPreview.cpp` | 구형 인벤토리 초상화 Actor. 다른 C++ 실행 코드에서 생성하거나 호출하는 곳을 찾지 못했다. 현재 프로젝트 에셋에서 이 클래스의 부모·문자열 참조도 발견되지 않았다. | `Config/DefaultEngine.ini`의 PlayerInventoryPreview → ICInventoryPreview 리다이렉트를 검토한다. 향후 인벤토리 초상화를 사용할 계획이면 삭제보다 현재 프리뷰 방식에 연결하는 편이 낫다. |
| `Public/NPC/Dialogue/ICDialogueLibrary.h`와 `Private/NPC/Dialogue/ICDialogueLibrary.cpp` | DT를 직접 받아 대화 정의와 노드를 해석하는 이전 API. 현재 대화 위젯은 DefinitionSubsystem을 통해 정의를 얻는다. 이 라이브러리의 두 함수를 부르는 외부 C++ 호출과 현재 에셋 참조를 찾지 못했다. | `Private/Data/Validation/ICTextValidationLibrary.cpp`가 이 헤더를 include하며 대화 Row 타입을 간접 사용한다. 라이브러리를 없애려면 해당 include를 `Data/Rows/NPC/ICDialogueRow.h`로 교체한다. 헤더의 Blueprint 결과 구조체도 함께 확인한다. |

이 두 쌍은 코드 내용이 같은 중복 파일이 아니라, 현재 실행 경로에 연결되지 않은
기능 후보다. 프로젝트 안의 에셋·설정·도구를 기준으로 판단했으며, 외부에 보관한
옛 BP나 별도 DLC까지 사용하지 않는다고 보장하는 결과는 아니다.

## 삭제보다 에디터 모듈 분리가 맞는 코드

- `Public/Debug/ICBlueprintMaintenanceLibrary.h`와 `Private/Debug/ICBlueprintMaintenanceLibrary.cpp`
- `Public/Debug/ICCharacterSetupLibrary.h`와 `Private/Debug/ICCharacterSetupLibrary.cpp`
- `Private/Debug/ICUIAspectCapture.cpp`
- `Private/Debug/ICCharacterPresentationReview.cpp`

이 코드는 BP·문자열·캐릭터 에셋 생성과 UI·자세 검수에 사용한다.
`Tools/Characters` 등의 Python에서 실제로 호출한다. `WITH_EDITOR`로 구현이
제한되어 있지만 게임 모듈 안에 에디터 보조 API 선언이 함께 있다.
장기적으로 IsekaiClassEditor 모듈을 만들고 여기에 옮기는 편이 구조상 명확하다.
옮길 때 Build.cs, uproject 모듈 등록, Python의 반영 이름과 필요한 리다이렉트를
함께 갱신해야 한다. 이번에는 사용 중인 도구를 그대로 유지했다.

`ICNPCTestLayout.h/.cpp`는 같은 Debug 폴더에 있어도 실행 중 앵커와 침대를 만드는
런타임 클래스다. `BP_R01TestLayout`의 실제 부모이며 L_TestMap에서 사용하므로
에디터 전용으로 바꾸거나 삭제하면 안 된다. 폴더를 정리한다면 World/Testing
같은 위치로 구분할 수 있다.

## 오래되어 보여도 유지해야 하는 코드

| 파일 또는 묶음 | 유지 이유 |
| --- | --- |
| `Public/Save/Compatibility/ICLegacyPlayerSaveGame.h` | 이전 저장 데이터의 필드와 클래스를 읽는 형식이다. SaveSubsystem의 변환·가져오기와 호환성 테스트에서 사용한다. 빈 CPP와 용도가 다르다. |
| `Private/Save/Compatibility/ICSaveDataMigration.h/.cpp` | 이전 아이템 ID 등의 정규화를 실제 저장 변환에서 사용한다. |
| `Public/Player/Stats/ICPlayerStats.h`, `Public/Life/Survival/ICPlayerSurvival.h`, `Public/Player/State/ICPlayerStateConversions.h` | 비슷한 상태 표현이 남아 있지만 UI·인벤토리·저장 호환 경로가 사용한다. 통합하려면 데이터 변환과 저장 마이그레이션이 필요하다. 파일 단위 삭제 대상이 아니다. |
| `ICAbilitySystemComponent.cpp`와 Actions·Stats·Weapons·Effects CPP | 한 클래스의 기능별 구현 분할이다. 같은 클래스를 구현한다는 이유로 중복이 되지 않는다. |
| `ICPlayerController.cpp`와 Input·Items·UI·Session·Persistence·MainMenu CPP | 입력, 화면, 세션, 저장 등으로 나눈 동일 컨트롤러 구현이다. |
| Definition·Combat·NPCWorld 등의 Validation CPP | 서로 다른 정의·참조 관계를 검사한다. 비슷한 검사 형태만으로 제거할 수 없다. |
| `UI/Framework/ICScreenWidget.h/.cpp` | 여러 실제 화면이 상속하는 공통 UI 기반 클래스다. |
| `Private/**/Tests/*.cpp` | 자동 테스트 등록 코드다. 일반 호출이나 include가 없다는 이유로 미사용 파일이 되지 않는다. |
| Row·Enum·인라인 함수 중심 헤더 | CPP가 없는 정상적인 헤더도 있다. h/cpp 개수 차이는 잔재의 근거가 아니다. |

## 정리 순서

1. 빈 CPP 한 개와 빈 모듈 헤더 한 개를 먼저 정리하고 Editor 빌드를 확인한다.
2. 인벤토리 프리뷰와 대화 라이브러리를 사용할 계획이 있는지 정한 뒤, 관련 include·리다이렉트·결과 구조체를 함께 정리한다.
3. 에디터 보조 코드를 별도 Editor 모듈로 옮기고 캐릭터 생성·검수 Python을 실행한다.
4. 상태 타입 통합은 저장 호환 작업으로 따로 진행한다.

파일 해시·선언·include·호출 및 Config/Tools/Data 텍스트 참조를 검사하고,
Unreal Asset Registry의 BP 부모 정보와 에셋 패키지의 네이티브 심벌을 추가로 확인했다.
검사 원자료는 `Saved/SourceAudit.json`과 `Saved/SourceAssetRefs.json`에 있다.
