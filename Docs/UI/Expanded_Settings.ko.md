# 게임 설정 화면

`WBP_IC_Settings`를 C++ 설정 스키마로 생성한다. 5개 탭, 45개 옵션과 실제 Enhanced Input 키 매핑 목록을 제공한다. WBP 레이아웃은 에디터에서 수정할 수 있으며 동작·저장·검증은 C++이 담당한다.

| 탭 | 항목 |
| --- | --- |
| 전투 | 공격 시 자동 대상 선택, 전투 중 조준 방향 유지, 조준점 거리 보정, 피해량 숫자, 적 체력 막대 |
| 조작 · 키 설정 | 감도, 카메라 좌우·상하 반전, 시야각, 휠 확대·축소 속도, 실제 키보드·마우스 키 변경 |
| 그래픽 · 화면 | 모니터, 전체 화면/전체 창/창 모드, 해상도, 주사율, VSync, FPS 상한, 프리셋, 렌더링 비율, 동적 해상도, 11개 세부 품질, AA 방식, 모션 블러, 빛 번짐, 피사계 심도, 주변광 차폐, 감마 |
| 오디오 | 전체·음악·효과음·환경음·음성·UI 음량, 비활성 창 음소거 |
| 기타 | HUD·메뉴 글자 크기, FPS 표시 |

## 적용과 복원

- 화면에서 편집하는 값은 임시 값이다. **적용 · 저장**으로 엔진/플레이어에 반영하고 `GameUserSettings.ini`에 저장한다. 닫기와 ESC는 미적용 변경 사항을 버린다.
- 화면 모드·모니터·해상도·주사율은 적용 후 15초 내 **변경 유지**를 선택해야 한다. 되돌리기, 닫기 또는 시간 초과 시 원래 Windows 디스플레이 모드와 게임 설정으로 복원한다.
- Windows가 보고하는 지원 모드만 제공한다. 전체 창 모드에는 실제 바탕 화면 해상도를 표시하고 해상도 변경을 비활성화한다.
- 모니터/디스플레이 변경은 독립 실행(Standalone/패키지 게임)에서 사용한다. PIE에서는 에디터 화면 모드를 바꾸지 않으며 안내 메시지를 표시한다. 일반 그래픽·전투·조작·음량 옵션은 PIE에서도 적용된다.
- 기본값 버튼은 화면 모드·해상도를 유지하면서 옵션과 키를 초기화한 임시 값을 만든다. 적용 전에는 실제 설정이 바뀌지 않는다.

## 구현

- `UICGameUserSettings`: 세이브 게임과 분리된 사용자 설정, 값 검증, 렌더러 CVar, 사운드 믹스, 카메라, 시작 시 모니터 복원.
- `UICSettingsWidget` / `UICSettingBinding`: 편집 값, 컨트롤 이벤트, 탭, 키 교환, 적용/취소 및 화면 확인 시간 처리.
- `ICSettingsLayout`: C++과 WBP가 공유하는 옵션 목록과 편집 가능한 위젯 트리 생성.
- `ICDisplaySettings`: Windows 모니터/지원 모드 조회, 실제 주사율 변경과 게임 창 이동.
- 키 변경은 원본 IMC를 수정하지 않고 런타임 복사본에 적용한다. 이동 축 변환과 트리거를 보존하며 Ctrl 회피도 바뀐 이동 키를 따른다. 중복 키는 교환하고 ESC는 유지한다.
- 글자 크기는 UI 매니저가 원래 글꼴 크기를 기준으로 적용한다. 고정 디자인 해상도에 의해 DPI 크기 변경이 상쇄되는 문제를 피하며 메뉴가 정지된 동안에도 갱신된다.
- 기존 개발용 `DefaultEngine.ini`의 `sg.*`, `r.ScreenPercentage`, `t.MaxFPS` 강제값은 제거했다. 낮은 초기 품질은 `DefaultGameUserSettings.ini`로 옮겨 사용자가 변경할 수 있게 했다.
- AA 방식과 후처리 스위치는 `ECVF_SetByGameOverride`로 프로젝트 기본값보다 우선 적용한다. 모션 블러와 AO는 실제 기본 기능 플래그도 함께 갱신한다.
- 기존 감도 설정(`/Script/IsekaiClass.Input`)은 새 사용자 설정이 없는 경우 읽어온다.

오디오는 `Audio/Settings`의 Master 하위 Music/Effects/Ambient/Voice/UI 사운드 클래스와 `SM_UserSettings`에 연결한다. 현재 게임에서 참조하는 277개 사운드의 클래스 경로를 연결했고, 명시 클래스가 없는 소리의 기본 클래스는 Effects로 지정했다. 원본 음원 데이터는 변경하지 않는다. 사용하지 않는 샘플 라이브러리는 일괄 재저장하지 않는다.

## 생성 및 검증 도구

- `Tools/UI/setup_expanded_settings.py`: 설정 WBP 및 오디오 클래스/믹스 연결. `-ICRenderExpandedSettings`로 1600×900·1280×720의 탭별 PNG 생성.
- `Tools/UI/review_settings_widget.py`: 실제 제목 화면과 게임에서 탭 전환, 키 입력, 카메라·음량·렌더러 적용, 글자 크기, 키 변경과 재열기를 검증한다. 테스트 전에 사용자 설정을 보관하고 종료 시 복원한다.
- 자동 테스트: `IC.Settings.ConfigAndInputMapping`, `IC.Settings.WBPControlsAndDraft`.
- 생성 보고서: `Saved/ExpandedSettingsAssetsReport.json`, `Saved/ExpandedSettingsLiveReview/Review.json`.

실제 제목 화면과 격리 PIE 검증에서 다섯 탭, 실제 키 목록, ESC 복귀, 카메라·음량 적용, 렌더러 품질/AA/FPS/렌더링 비율, 글꼴 크기, Enhanced Input의 변경된 이동 키, 저장 후 재열기까지 9개 항목을 확인했다. 물리 모니터/주사율 변경은 사용자의 작업 화면을 바꾸므로 이 자동 검증에서는 실행하지 않는다.

최종 에디터 기본 빌드(`Tools/BuildProject.ps1`, 별도 Unity 비활성화 옵션 없음) 성공. `IC.Settings+IC.Combat.NormalAttack+IC.Input+IC.GAS` 필터의 프로젝트 테스트 22개가 모두 통과했다. 함께 선택된 엔진 RigLogic 입력 테스트 3개도 통과했다. 최신 결과는 `Saved/CentreAimTests.log` 및 `Saved/CentreAimTests`에서 확인할 수 있다.
