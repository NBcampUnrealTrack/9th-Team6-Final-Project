# 전투 Niagara와 DT

`/Game/IsekaiClass/VFX/CombatEffect/NormalAttack`에는 일반 공격용 검 궤적·범위·명중·화살·마력탄·꼬리 Niagara가 있습니다.
`/Game/IsekaiClass/VFX/CombatEffect/Skill`에는 별도의 스킬용 파일과 프로젝트 마법진·검기 에셋을 복사한 변형이 있습니다.
일반 공격의 형태는 기존 C++ 기본 도형 이펙트를 Niagara Mesh Renderer로 옮긴 것입니다.
원본 C++ 메시 이펙트와 기존 프로젝트 VFX는 보존합니다.

## 수정 위치

`/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX`에서 수정합니다.

| 행 | 용도 |
|---|---|
| `NORMAL_SWORD_1` ~ `3` | 검 1·2·3단. 3단은 금색, 더 굵은 수평 궤적 |
| `NORMAL_BOW_1` ~ `3` | 궁수 일반 공격 |
| `NORMAL_STAFF_1` ~ `3` | 마법사 일반 공격 |
| `NORMAL_WAND_1` ~ `3` | 힐러 일반 공격 |
| `SKILL_SWORD/BOW/STAFF/WAND` | 각 직업 시작 스킬 |

| 블록 | 바꾸는 효과 |
|---|---|
| `Slash` | 검의 밝은 위쪽 궤적 |
| `Range` | 바닥의 공격 원뿔 테두리 |
| `Projectile` | 실제 발사체에 붙는 화살/마력탄 외형 |
| `Trail` | 발사체 뒤쪽 꼬리 |
| `Cast` | 스킬 시전 효과 |
| `Impact` | 적 또는 벽에 닿았을 때의 효과 |

각 블록의 `System`을 다른 Niagara로 교체할 수 있습니다. `bEnabled`는 해당 효과 표시 여부입니다.
`Color`, `Intensity`, `Scale`, `Duration`, `Offset`, `Rotation`, `WidthCm`으로 색·발광 강도·크기·지속시간·위치·회전·선 굵기를 설정합니다.
`SlashSlope`는 검 궤적의 경사입니다. 기본값은 1단 +0.22, 2단 -0.22, 3단 0입니다.
`Duration=0`이면 호출부 기본 시간 또는 발사체 수명을 따릅니다.
프로젝트에서 복사한 효과는 `ReferenceDuration`을 원래 애니메이션 길이로 두고 `Duration`으로 재생 속도를 조절합니다.

## 연결 구조

- 일반 공격: `DT_IC_Class → NormalAttackStages[n] → Visuals`가 전투 VFX 행을 참조합니다.
- 스킬: `DT_IC_Skill → Presentation → Visuals`가 행을 참조합니다.
- 개별 스킬 효과: `DT_IC_SkillEffect → Presentation → Visuals`에 지정하면 명중 효과를 덮어씁니다.
- 기존 `CastVFX`, `ImpactVFX`는 Visuals 행을 지정하지 않은 기존 콘텐츠에서 계속 동작합니다.

새로 만든 Niagara는 `User.IC_Color`, `User.IC_Scale`, `User.IC_Duration`, `User.IC_Width`, `User.IC_Radius`, `User.IC_Angle`, `User.IC_Floor`, `User.IC_Slope`를 사용합니다.
자체 Niagara로 교체할 때도 이 User 파라미터를 사용하면 DT 값이 반영됩니다. 파라미터가 없는 외부 Niagara는 에셋 자체 설정으로 재생됩니다.
복사본 마법진·검기에는 DT 색상용 모듈과 로컬 좌표 설정을 추가했습니다.

`bLegacyMeshFallback=true`이면 일반 공격 Niagara 참조가 없을 때 기존 메시 이펙트를 사용합니다.
예전 형태로 돌아가려면 클래스 콤보 단계의 `Visuals` 행 참조를 비우면 됩니다.
효과 크기·지속시간은 표시만 조절합니다. 실제 사거리·판정·속도·데미지는 클래스 콤보의 `Hit` 또는 스킬 `Hits`를 수정합니다.
기존 손에서 화면 중앙으로 향하는 조준, 콤보 모션, 3단 데미지 계수는 유지됩니다.

`Tools/Combat/setup_combat_vfx.py`는 초기 에셋과 DT 기본값을 생성합니다. 이미 존재하는 Niagara 그래프는 재생성하지 않지만 DT 기본값은 다시 채우므로 수동 편집 후 재실행에는 주의하세요.
