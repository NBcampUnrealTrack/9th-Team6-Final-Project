# 새 NPC VRM을 MCP로 설정하는 절차

VRM을 에디터에 미리 임포트하고, 다른 작업자에게 아래 프롬프트와 같은 프로젝트를
전달한다. 작업자는 Unreal MCP에서 프로젝트 도구를 호출해 NPC를 만든다.
VRM 임포트만으로 작업을 시작하는 이벤트나 자동 팝업은 등록하지 않는다.

## 전달할 프롬프트

다음 내용을 복사하고 대괄호 항목만 채운다.

```text
IsekaiClass 프로젝트에서 미리 임포트한 VRM을 새 NPC로 설정해줘.
Unreal MCP로 실제 에셋을 검사·수정하고 저장한 뒤 플레이 검수까지 진행해줘.

입력
- VRM 원본 Skeletal Mesh 또는 VRM AssetList 경로: [콘텐츠 브라우저의 /Game/... 경로]
- NPC ID: [예: NPC_R02_001 / 미정이면 중복 없는 ID를 제안]
- 표시 이름: [이름]
- 직업: [검사 / 궁수 / 마법사 / 힐러]
- 반 친구 여부: [예 / 아니오]
- 월드 등장 위치·집·시간표·대화·고유 기술·초상화: [준비된 참조 / 아직 미정]

진행 규칙
1. Docs/Characters/NPC_VRM_AutoSetup.ko.md와 KawaiiPhysics_Setup.ko.md를 읽고,
   연결된 MCP의 에셋 검색·에디터 Python 실행·PIE 제어·화면 캡처 기능을 확인해줘.
2. Tools/Characters/npc_import.py의 inspect_vrm과 plan으로 원본 메시,
   메타데이터, IK Rig, 필수 본과 스프링 체인, ID 및 출력 경로 충돌을 먼저 확인해줘.
3. build(config)로 해당 NPC 한 명의 리타겟 애니메이션, 이동 BlendSpace,
   공격·기술·피격 Montage, 애니메이션 프로필, NPC BP, 무기 소켓과 발 높이를 설정해줘.
4. 머리·옷은 반드시 Kawaii Physics를 사용해줘. VRM SpringBone과 중복 실행하지 말고,
   표정·Constraint는 유지해줘. 몸 충돌체와 머리·치마·소매의 연결도 확인해줘.
5. 기존 NPC와 수동 조정값을 보존하고 새 NPC 범위만 수정해줘.
   현재 13종 전체를 다시 생성하는 setup_assets.py의 main은 실행하지 마.
6. 생활 장소와 대사 등이 미정이면 외형 생성과 등록 초안까지 완료해줘.
   준비된 참조가 있으면 유효성과 해당 NPC 소유 관계를 검사한 뒤
   CSV·DataTable·ID Registry·지역 NPC 목록·표시 이름까지 일치시켜줘.
7. 새 NPC를 임시 검수 환경에서 실제 걷고 달리게 하고 앞·뒤 화면을 확인해줘.
   머리 고정·튐·늘어남, 옷 관통, 발 접지와 무기 그립을 검사하고 필요한 조정을 해줘.
   침대와 수면 모션은 변경하지 마.
8. 생성 경로, 사용한 설정, 실제 수행한 검사와 남은 조정 사항을 보고해줘.
   생성 성공과 플레이 검수 완료를 구분하고, 확인하지 못한 항목은 완료로 표시하지 마.

필수 입력이나 MCP 기능이 없으면 이미 확인 가능한 검사를 먼저 진행하고,
부족한 항목만 구체적으로 알려줘. 예시 ID나 임의의 직업·대사·생활 장소를 확정하지 마.
```

## MCP 실행 전제

작업자에게 프롬프트와 함께 **이 도구가 포함된 프로젝트**를 전달해야 한다.
Editor C++ 빌드가 완료되어 `unreal.ICCharacterSetupLibrary`의 새 NPC 함수들이
노출되어 있어야 한다. 사용하는 Unreal MCP에는 에디터 Python 실행 기능이 필요하다.
에셋 검색, PIE 제어와 캡처도 실제 도구 목록에서 확인한다. MCP 구현마다 이름이
다르므로 특정 도구 이름을 추측해서 호출하지 않는다.

생성·등록 검사는 Unreal Editor Python에서 수행했다. 아래 호출문을
MCP의 에디터 Python 실행 기능에 전달한다. 연결과 PIE·캡처 기능은 작업자 환경에서 확인한다.

## 에디터 Python 호출 예제

작업자는 프로젝트 절대 경로를 하드코딩하지 않고 다음과 같이 모듈을 읽는다.

```python
import sys, json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
tools = str(root / 'Tools' / 'Characters')
if tools not in sys.path:
    sys.path.insert(0, tools)
import npc_import

# 실제 모델 경로로 바꾼다. 원본 메시 또는 VRM AssetList 모두 가능하다.
vrm_path = '/Game/여기에/실제/VRM_에셋_경로'
print(json.dumps(npc_import.inspect_vrm(vrm_path), ensure_ascii=False, indent=2))
```

검사 결과와 사용자가 준 값으로 설정을 만든 뒤 계획과 생성을 순서대로 호출한다.

```python
config = {
    'vrm': vrm_path,
    'id': 'NPC_R02_001',       # 예시: 실제 지정한 새 ID로 변경
    'name': '표시 이름',
    'role': 'Mage',           # Warrior=검사, Archer=궁수, Mage=마법사, Healer=힐러
    'classmate': False,
    'register_world': False,
    'region': 'R02',          # 예시: 실제 지역으로 변경
}
print(json.dumps(npc_import.plan(config), ensure_ascii=False, indent=2))
report = npc_import.build(config)
print(json.dumps(report, ensure_ascii=False, indent=2))
```

`inspect_vrm`과 `plan`은 저장하지 않는다. `build`가 해당 NPC의 에셋을 생성·저장한다.
VRM4U 임포트 시 원본 메시용 Mannequin IK Rig와 Post Process ABP가 필요하다.
기존 공유 소스 IK Rig, 원본 애니메이션, 다른 NPC의 데이터는 다시 작성하지 않는다.
완료된 동일 요청은 중복 생성하지 않는다. ID가 다른 기존 콘텐츠와 겹치면 중단한다.

## 월드 등록과 검수

`register_world=False`는 BP·애니메이션·Kawaii 설정과 등록 초안을 만든다.
이 상태에서는 NPC가 자동으로 월드에 등장하지 않는다.
월드 등록은 작성된 `region`, `anchor`, `home`, `schedule`, `dialogue`,
`unique_skill`, `portrait`를 지정하고 `register_world=True`로 호출한다.
집·대화·고유 기술의 소유자는 새 NPC ID와 일치해야 한다.
외형 생성 후 같은 ID로 준비된 월드 참조만 추가할 수 있다.

NPC 생성 후 등록할 행은 `Saved/NPCImport/<NPC ID>/registration_draft.json`에,
결과와 검사 항목은 같은 폴더의 `report.json`에 남는다.
완료된 월드 등록을 다시 실행해도 새 행을 중복 추가하지 않는다.
등록 실패 시 변경한 CSV와 DataTable을 복구하며 원본 백업도 보관한다.

새 NPC의 실제 플레이 검수는 작업자가 MCP의 PIE 제어·캡처 기능으로 수행한다.
기존 `live_character_review.py`는 현재 R01 11명 전용이므로 새 NPC 검수에 그대로
사용하지 않는다. 해당 NPC를 대상으로 별도 임시 배치·이동과 촬영을 구성해야 한다.
기존 세이브를 사용하거나 정식 레벨에 테스트 배치를 저장하지 않는다.

## 생성 위치와 자동화 범위

- 애니메이션: `/Game/IsekaiClass/Animation/NPC/<NPC ID>`
- NPC BP: `/Game/IsekaiClass/NPC/Blueprints/Generated/BP_<NPC ID>`
- 물리 그래프 백업: `Saved/NPCImport/OriginalAssets`
- 등록 전 CSV 백업: `Saved/NPCImport/<NPC ID>/DataBeforeRegistration`

기존 직업별 무기를 사용하고, 무기 소켓과 발 높이를 기본 보정한다.
Kawaii의 기본 물리값과 충돌 연결을 적용하지만 모델별 시각 조정을 대신하지 않는다.
VRoid의 Hair/Skirt/Sleeve/Bust 이름을 분류하며 다른 이름은 장식으로 취급한다.
스프링 본 누락이나 겹치는 체인 등은 원인과 함께 중단해 작업자가 수정하도록 한다.

VRM 0.x/1.x 메타데이터 변환 경로가 있으며 통합 검사는 VRoid VRM 1.x 복제 에셋으로
수행한다. `test_npc_import.py`는 렌더링 가능한 에디터에서 실행하고,
`test_npc_import_registration.py`는 복제 CSV와 테이블에서 등록·실패 복구를 검사한다.
실제 NPC 데이터는 검사 대상에서 수정하지 않는다.

2026-10-23 Editor 빌드와 생성 검사 26개, 등록·실패 복구 검사 22개를 통과했다.
생성 검사는 기존 CSV·공유 소스 IK Rig·13종의 물리 그래프와 메타데이터가
변경되지 않았는지도 파일 해시로 확인했다. 결과는 `Saved/NPCImport`에 남긴다.
