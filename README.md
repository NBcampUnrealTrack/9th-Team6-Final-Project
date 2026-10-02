# IsekaiClass

## 프로젝트 열기

- `IsekaiClass.uproject`를 Unreal Engine 5.8에서 엽니다.
- 플레이어 기능 확인용 맵은 `/Game/PlayerPrototype/L_PlayerPrototype`입니다.
- 첫 빌드에서는 프로젝트와 플러그인 바이너리가 다시 생성되므로 시간이 걸릴 수 있습니다.

## 플레이어 코드

- `Source/IsekaiClass/Player/PlayerCharacter.*`: 캐릭터와 외형 적용
- `PlayerPrototypeController.h`: 플레이어 입력과 시스템 연결 인터페이스
- `PlayerControllerInput.cpp`: 이동, 카메라, 키 입력, 상호작용
- `PlayerControllerPersistence.cpp`: 캐릭터 생성, 저장, 불러오기
- `PlayerControllerItems.cpp`: 아이템 사용, 사망, 보관함
- `PlayerControllerUI.cpp`: UI 명령 처리
- `PlayerGameMode.*`: 클래스 설정과 테스트 월드 구성
- `PlayerGameState.*`: 플레이 단계와 공유 창고 식별자
