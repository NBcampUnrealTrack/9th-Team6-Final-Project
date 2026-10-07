#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCombatTypes.generated.h"

class AActor;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="피해의 능력치·방어 계산 계열입니다. 실제 피해/GAS 실행 연결이 필요합니다."))
enum class EICDamageType : uint8 { Physical UMETA(ToolTip="물리 피해 계열입니다. 물리 공격 능력치를 사용하는 효과에 지정합니다.\n피해의 능력치·방어 계산 계열입니다. 실제 피해/GAS 실행 연결이 필요합니다."), Magical UMETA(ToolTip="마법 피해 계열입니다. 마법 공격 능력치를 사용하는 효과에 지정합니다.\n피해의 능력치·방어 계산 계열입니다. 실제 피해/GAS 실행 연결이 필요합니다.") };

UENUM(BlueprintType, meta=(ToolTip="스킬의 정적 소유 주체입니다. 종류에 맞는 직업·NPC·몬스터 참조를 사용하세요."))
enum class EICSkillOwner : uint8 { CharacterClass UMETA(ToolTip="직업 공통 스킬입니다. 직업 정의의 스킬 슬롯과 연결합니다.\n스킬의 정적 소유 주체입니다. 종류에 맞는 직업·NPC·몬스터 참조를 사용하세요."), RecruitUnique UMETA(ToolTip="영입 NPC 고유 스킬입니다. 해당 NPC의 RecruitCombat 정의와 연결합니다.\n스킬의 정적 소유 주체입니다. 종류에 맞는 직업·NPC·몬스터 참조를 사용하세요."), Monster UMETA(ToolTip="몬스터 전용 스킬입니다. Monster와 BossPattern의 스킬 후보에서 사용합니다.\n스킬의 정적 소유 주체입니다. 종류에 맞는 직업·NPC·몬스터 참조를 사용하세요.") };

UENUM(BlueprintType, meta=(ToolTip="스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."))
enum class EICSkillPurpose : uint8 { Attack UMETA(ToolTip="적에게 피해를 주는 공격 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Heal UMETA(ToolTip="현재 HP 등 자원을 회복하는 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Buff UMETA(ToolTip="아군에게 능력치 강화나 유익한 상태 효과를 주는 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Shield UMETA(ToolTip="피해를 흡수하는 보호막 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Dispel UMETA(ToolTip="해제 가능한 상태 효과를 제거하는 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Revive UMETA(ToolTip="Downed 동료를 되살리는 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다."), Utility UMETA(ToolTip="이동·특수 상호작용 등 다른 분류에 속하지 않는 보조 용도입니다.\n스킬의 대표 용도입니다. 이 값은 분류이며 효과·대상·자동 사용 조건도 함께 작성해야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."))
enum class EICCombatTarget : uint8 { Self UMETA(ToolTip="시전자 자신을 대상으로 합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), Enemy UMETA(ToolTip="선택된 적을 대상으로 합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), PartyMember UMETA(ToolTip="선택한 파티원 하나를 대상으로 합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), LivingParty UMETA(ToolTip="살아 있는 파티원을 대상으로 합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), DownedCompanion UMETA(ToolTip="Downed 상태의 동료를 대상으로 합니다. 플레이어 사망과 구분합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), GroundPoint UMETA(ToolTip="지면 좌표를 지정하는 스킬입니다. 캐릭터 ID 대상과 구분합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다."), Forward UMETA(ToolTip="시전자 전방을 기준으로 판정합니다. 별도의 적 선택이 없어도 위치·방향 규칙이 필요합니다.\n스킬이 선택할 대상의 형태입니다. 생존 조건과 거리 조건은 별도 TargetRules 필드로 지정합니다.") };

UENUM(BlueprintType, meta=(ToolTip="선택 가능한 캐릭터의 생존 상태입니다. 대상 분류와 함께 검사해야 합니다."))
enum class EICCombatTargetState : uint8 { Living UMETA(ToolTip="살아 있는 대상만 허용합니다.\n선택 가능한 캐릭터의 생존 상태입니다. 대상 분류와 함께 검사해야 합니다."), Downed UMETA(ToolTip="Downed 동료만 허용합니다.\n선택 가능한 캐릭터의 생존 상태입니다. 대상 분류와 함께 검사해야 합니다."), LivingOrDowned UMETA(ToolTip="살아 있는 대상과 Downed 대상을 모두 허용합니다. 각 효과가 처리 가능한 상태인지 다시 검사하세요.\n선택 가능한 캐릭터의 생존 상태입니다. 대상 분류와 함께 검사해야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="판정 안에서 효과를 적용할 진영의 필터입니다."))
enum class EICEffectTargetSide : uint8 { Friendly UMETA(ToolTip="시전자와 우호 관계인 대상을 허용합니다.\n판정 안에서 효과를 적용할 진영의 필터입니다."), Hostile UMETA(ToolTip="시전자와 적대 관계인 대상을 허용합니다.\n판정 안에서 효과를 적용할 진영의 필터입니다."), Any UMETA(ToolTip="진영으로 제한하지 않습니다. 생존 상태와 기타 조건은 계속 검사합니다.\n판정 안에서 효과를 적용할 진영의 필터입니다.") };

UENUM(BlueprintType, meta=(ToolTip="타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."))
enum class EICHitShape : uint8 { Direct UMETA(ToolTip="선택한 대상에 직접 적용하는 방식입니다.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), WeaponTrace UMETA(ToolTip="무기 소켓의 이동 경로를 추적하는 판정입니다. 시작/끝 소켓과 중복 타격 방지가 필요합니다.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), Projectile UMETA(ToolTip="투사체 액터를 발사하여 충돌로 판정합니다. 투사체 클래스와 발사 이벤트를 연결하세요.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), Sphere UMETA(ToolTip="중심에서 반경으로 구형 범위를 판정합니다.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), Cone UMETA(ToolTip="전방 방향·거리·각도로 원뿔 범위를 판정합니다.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), Box UMETA(ToolTip="중심·회전·크기로 상자 범위를 판정합니다.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요."), GroundArea UMETA(ToolTip="지면의 지정 영역을 판정합니다. 경고 연출과 실제 타격 영역을 맞추세요.\n타격의 공간 판정 방식입니다. 해당 모양에 필요한 거리·반경·각도·크기·에셋을 함께 작성하세요.") };

UENUM(BlueprintType, meta=(ToolTip="스킬 비용을 최초 한 번 확정할 실제 실행 사건입니다. Notify 누락·중단에서도 시전 ID별 중복 확정을 막아야 합니다."))
enum class EICFirstEffectEvent : uint8 { ProjectileFire UMETA(ToolTip="첫 투사체가 실제 발사되는 시점입니다.\n스킬 비용을 최초 한 번 확정할 실제 실행 사건입니다. Notify 누락·중단에서도 시전 ID별 중복 확정을 막아야 합니다."), FirstHit UMETA(ToolTip="첫 타격이 실제 실행되는 시점입니다.\n스킬 비용을 최초 한 번 확정할 실제 실행 사건입니다. Notify 누락·중단에서도 시전 ID별 중복 확정을 막아야 합니다."), FirstRecovery UMETA(ToolTip="첫 회복 효과가 실제 실행되는 시점입니다.\n스킬 비용을 최초 한 번 확정할 실제 실행 사건입니다. Notify 누락·중단에서도 시전 ID별 중복 확정을 막아야 합니다."), EffectCommit UMETA(ToolTip="효과 실행기가 명시적으로 효과를 확정하는 시점입니다.\n스킬 비용을 최초 한 번 확정할 실제 실행 사건입니다. Notify 누락·중단에서도 시전 ID별 중복 확정을 막아야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."))
enum class EICSkillEffectKind : uint8 { Damage UMETA(ToolTip="피해 크기를 계산하여 자원을 감소시킵니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Heal UMETA(ToolTip="회복 크기를 계산하여 자원을 상한 내에서 증가시킵니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Shield UMETA(ToolTip="지속 시간과 흡수량을 가진 보호막을 적용합니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), ApplyStatus UMETA(ToolTip="StatusEffect ID가 가리키는 상태 효과를 적용합니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Dispel UMETA(ToolTip="해제 가능한 효과를 지정 개수·필터에 따라 제거합니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Revive UMETA(ToolTip="Downed 동료의 생존 상태와 자원을 복구합니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Movement UMETA(ToolTip="이동 거리·방향·충돌 규칙에 따라 위치를 변경합니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다."), Custom UMETA(ToolTip="공통 분류 외 전용 실행을 사용합니다. 전용 실행 코드를 연결하지 않으면 동작하지 않습니다.\n스킬 효과의 실행 분류입니다. 분류별 필요한 필드와 실제 실행 경로를 연결해야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."))
enum class EICAutoSkillType : uint8 { None UMETA(ToolTip="자동 사용 대상에서 제외합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."), Attack UMETA(ToolTip="적 대상과 전투 조건을 만족할 때 공격용으로 선택합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."), Heal UMETA(ToolTip="회복 대상과 HP 임계 조건을 검사합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."), Dispel UMETA(ToolTip="해제 가능한 효과를 가진 대상이 있는지 검사합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."), Revive UMETA(ToolTip="부활 가능한 Downed 동료가 있는지 검사합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다."), Buff UMETA(ToolTip="유익한 효과가 필요한 대상을 검사합니다.\n동료 AI 자동 사용의 용도입니다. 선택만으로 AI가 실행되지는 않으며 조건 판정과 비용 검사가 필요합니다.") };

UENUM(BlueprintType, meta=(ToolTip="지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."))
enum class EICStatusKind : uint8 { StatModifier UMETA(ToolTip="능력치의 고정값·비율을 수정합니다.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."), DamageOverTime UMETA(ToolTip="주기마다 피해를 적용합니다. 주기와 크기 정의가 필요합니다.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."), HealOverTime UMETA(ToolTip="주기마다 회복을 적용합니다. 주기와 크기 정의가 필요합니다.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."), Shield UMETA(ToolTip="남은 흡수량을 가진 보호막입니다.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."), Control UMETA(ToolTip="행동 제한 등 제어 효과입니다. 행동 태그와 해제 정책을 연결하세요.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다."), Custom UMETA(ToolTip="전용 상태 효과 실행을 사용합니다.\n지속 상태 효과의 대표 분류입니다. 실제 남은 시간·중첩 수는 WorldState/GAS 실행 상태에서 보존합니다.") };

UENUM(BlueprintType, meta=(ToolTip="같은 상태 효과를 다시 적용할 때의 중첩 정책입니다. 정의만으로 중첩 실행이 제공되지는 않습니다."))
enum class EICStackingPolicy : uint8 { KeepStrongerRefreshDuration UMETA(ToolTip="더 강한 수치를 유지하고 지속 시간을 갱신하는 정책입니다.\n같은 상태 효과를 다시 적용할 때의 중첩 정책입니다. 정의만으로 중첩 실행이 제공되지는 않습니다."), Replace UMETA(ToolTip="기존 효과를 새 효과로 교체하는 정책입니다.\n같은 상태 효과를 다시 적용할 때의 중첩 정책입니다. 정의만으로 중첩 실행이 제공되지는 않습니다."), AddStacks UMETA(ToolTip="최대 중첩 수 안에서 중첩 수를 늘리는 정책입니다.\n같은 상태 효과를 다시 적용할 때의 중첩 정책입니다. 정의만으로 중첩 실행이 제공되지는 않습니다."), Independent UMETA(ToolTip="각 적용을 독립된 효과 인스턴스로 유지하는 정책입니다.\n같은 상태 효과를 다시 적용할 때의 중첩 정책입니다. 정의만으로 중첩 실행이 제공되지는 않습니다.") };

UENUM(BlueprintType, meta=(ToolTip="동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."))
enum class EICCompanionRole : uint8 { Unspecified UMETA(ToolTip="역할을 따로 지정하지 않았습니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."), Frontline UMETA(ToolTip="근접 전선 역할입니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."), RangedDamage UMETA(ToolTip="원거리 물리 피해 역할입니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."), MagicDamage UMETA(ToolTip="마법 피해 역할입니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."), Healer UMETA(ToolTip="파티 회복 역할입니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다."), Support UMETA(ToolTip="강화·해제 등 파티 지원 역할입니다.\n동료 AI의 전투 역할 분류입니다. 장비·직업·스킬 및 실제 AI 판단과 함께 사용합니다.") };

UENUM(BlueprintType, meta=(ToolTip="동료의 기본 공격 성향입니다. 실제 AI 실행 연결이 필요합니다."))
enum class EICAttackDisposition : uint8 { Aggressive UMETA(ToolTip="허용된 적을 적극적으로 찾아 공격하는 성향입니다.\n동료의 기본 공격 성향입니다. 실제 AI 실행 연결이 필요합니다."), Defensive UMETA(ToolTip="방어·지원 우선 규칙으로 공격하는 성향입니다.\n동료의 기본 공격 성향입니다. 실제 AI 실행 연결이 필요합니다."), Wait UMETA(ToolTip="기본 공격을 대기하는 성향입니다.\n동료의 기본 공격 성향입니다. 실제 AI 실행 연결이 필요합니다.") };

UENUM(BlueprintType, meta=(ToolTip="HP가 낮을 때의 행동 성향입니다. 낮은 HP 판정 기준은 별도 규칙을 사용합니다."))
enum class EICLowHPDisposition : uint8 { Attack UMETA(ToolTip="낮은 HP에서도 공격을 계속하는 성향입니다.\nHP가 낮을 때의 행동 성향입니다. 낮은 HP 판정 기준은 별도 규칙을 사용합니다."), Wait UMETA(ToolTip="낮은 HP에서 공격을 대기하는 성향입니다.\nHP가 낮을 때의 행동 성향입니다. 낮은 HP 판정 기준은 별도 규칙을 사용합니다.") };

UENUM(BlueprintType, meta=(ToolTip="동료 AI의 적 대상 우선순위입니다."))
enum class EICTargetPriority : uint8 { PlayerTargetFirst UMETA(ToolTip="플레이어가 지정한 적을 우선합니다. 해당 적이 유효하지 않을 때의 대체 선택도 실행 측에서 처리해야 합니다.\n동료 AI의 적 대상 우선순위입니다."), NearbyEnemyFirst UMETA(ToolTip="가까운 유효 적을 우선합니다. 거리만으로 아군이나 무효 대상을 고르면 안 됩니다.\n동료 AI의 적 대상 우선순위입니다.") };

UENUM(BlueprintType, meta=(ToolTip="몬스터의 콘텐츠 분류입니다. 실제 리스폰·보상·진행 상태는 별도로 관리합니다."))
enum class EICMonsterKind : uint8 { Normal UMETA(ToolTip="일반 몬스터입니다.\n몬스터의 콘텐츠 분류입니다. 실제 리스폰·보상·진행 상태는 별도로 관리합니다."), RegionalBoss UMETA(ToolTip="지역 보스입니다. 지역 진행과 보스 처치 기록에 연결합니다.\n몬스터의 콘텐츠 분류입니다. 실제 리스폰·보상·진행 상태는 별도로 관리합니다."), FinalBoss UMETA(ToolTip="최종 보스입니다. 엔딩 진행과 연결할 실행 경로가 필요합니다.\n몬스터의 콘텐츠 분류입니다. 실제 리스폰·보상·진행 상태는 별도로 관리합니다.") };

UENUM(BlueprintType, meta=(ToolTip="부활 시 MP를 복구하는 방식입니다. 비율은 최대 MP에 대한 0~1 값입니다."))
enum class EICReviveMPPolicy : uint8 { Preserve UMETA(ToolTip="기존 MP를 유지하되 유효한 자원 범위로 제한합니다.\n부활 시 MP를 복구하는 방식입니다. 비율은 최대 MP에 대한 0~1 값입니다."), AtLeastMaxMPRatio UMETA(ToolTip="기존 MP와 최대 MP×설정 비율 중 큰 값을 사용합니다.\n부활 시 MP를 복구하는 방식입니다. 비율은 최대 MP에 대한 0~1 값입니다."), SetMaxMPRatio UMETA(ToolTip="기존 MP와 관계없이 최대 MP×설정 비율로 설정합니다.\n부활 시 MP를 복구하는 방식입니다. 비율은 최대 MP에 대한 0~1 값입니다.") };

/** Asset references only; these are presentation definitions, never running effect state. */
USTRUCT(BlueprintType, meta=(ToolTip="전투 아이콘·시전/명중 VFX·사운드의 정적 참조입니다. 효과 수치·실행 여부·남은 시간은 포함하지 않습니다. 표시 실행 측에서 필요한 에셋을 로드합니다.", ShortToolTip="전투 아이콘·시전/명중 VFX·사운드의 정적 참조입니다."))
struct ISEKAICLASS_API FICCombatPresentation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="시전 NiagaraSystem입니다. 실제 스킬 재생 실행 연결이 필요합니다.\n입력 자료형: Soft Object 에셋 참조 (UNiagaraSystem).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<UNiagaraSystem> CastVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="명중/효과 적용 NiagaraSystem입니다. 확정된 타격 뒤 표시하도록 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UNiagaraSystem).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<UNiagaraSystem> ImpactVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="시전 SoundBase입니다. 취소/반복 시 중복 재생을 피하는 실행 연결이 필요합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<USoundBase> CastSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="명중 SoundBase입니다. 실제 타격 확정과 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<USoundBase> ImpactSound;
};

/** Flat + coefficient times one source statistic. Personal CHA is applied once at execution. */
USTRUCT(BlueprintType, meta=(ToolTip="고정량 + 원본 능력치 × 계수로 효과 크기를 정의합니다. 개인 CHA 적용 여부를 별도로 지정해 중복 적용을 막습니다. 계산과 GAS 적용 실행기는 연결 작업이 남아 있습니다.", ShortToolTip="고정량 + 원본 능력치 × 계수로 효과 크기를 정의합니다."))
struct ISEKAICLASS_API FICCombatMagnitude
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ToolTip="효과량 계수의 원본 능력치입니다. Coefficient·FlatAmount와 함께 사용하도록 정의합니다.\n입력 자료형: 선택 목록 (EICStat).\nC++ 초기값: None — 능력치 대상을 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICStat SourceStat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ClampMin="0", ToolTip="SourceStat에 곱할 계수입니다. 1.5는 원본의 150%이며 FlatAmount와 합산하도록 정의합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double Coefficient = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ClampMin="0", ToolTip="효과의 고정 부분입니다. 정의 공식은 FlatAmount + SourceStat×Coefficient입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double FlatAmount = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ToolTip="개인 CHA 효과량 보정을 사용합니다. 같은 실행에서 한 번만 보정하며 실제 계산 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bApplyPersonalCHA = true;
};

USTRUCT(BlueprintType, meta=(ToolTip="스킬 대상의 종류·생존 상태·거리·시야·추가 조건을 정의합니다. 잘못된 대상을 고른 상태에서 비용을 확정하지 않도록 실행 측에서 재검사해야 합니다.", ShortToolTip="스킬 대상의 종류·생존 상태·거리·시야·추가 조건을 정의합니다."))
struct ISEKAICLASS_API FICSkillTargetRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="스킬 대상 분류입니다. 자기 자신/적/동료/살아 있는 파티/Downed 동료/지면/전방에 맞춰 생존·거리·시야 조건을 작성합니다.\n입력 자료형: 선택 목록 (EICCombatTarget).\nC++ 초기값: Enemy — 선택된 적을 대상으로 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICCombatTarget Target = EICCombatTarget::Enemy;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="허용 생존 상태입니다. Alive/Downed를 구분하고 HP만으로 Downed를 대신하지 않습니다.\n입력 자료형: 선택 목록 (EICCombatTargetState).\nC++ 초기값: Living — 살아 있는 대상만 허용합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICCombatTargetState TargetState = EICCombatTargetState::Living;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="실제 스킬 대상을 요구합니다. 해제 시에도 GroundPoint/Forward 등의 규칙을 실제 실행기에 맞춰 작성합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bRequiresTarget = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="시전자 자신을 대상으로 허용합니다. 해제하면 자기 대상 선택을 제외하도록 정의합니다. 실제 대상 실행은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bAllowCaster = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="스킬 대상 가림 검사 요구입니다. 해제 시 시야 예외이며 실제 거리/대상 검사와 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bRequireLineOfSight = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", ToolTip="최소 허용 거리 cm입니다. 0은 양수 최소 거리 요구가 없는 규격이며 MaximumRangeCm와 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double MinimumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", ToolTip="허용 최대 거리 cm입니다. 최소 거리보다 작지 않게 작성하고 0의 제한 해제 여부는 실제 대상/패턴 실행 규칙을 확인합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double MaximumRangeCm = 0.0;
    // Mandatory for revival: removed Downed actors must remain selectable without a distance check.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="Downed 대상만 거리 검사 예외를 허용합니다. 다른 생존/진영/참조 조건까지 무시하는 설정이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bIgnoreRangeForDowned = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ToolTip="상위 동작 조건 묶음입니다. All/Any와 종류별 필드를 작성합니다. 일반 조건 실행기는 별도 연결 대상이며 정의 검사 성공은 실행 성공과 다릅니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICConditionGroup Conditions;
};

USTRUCT(BlueprintType, meta=(ToolTip="시전과 다단 타격이 어떤 사건에서 중단되는지 정의합니다. 최초 효과 전 취소와 실행 후 남은 타격 취소를 구분하며 실제 전투 수신 경로 연결이 필요합니다.", ShortToolTip="시전과 다단 타격이 어떤 사건에서 중단되는지 정의합니다."))
struct ISEKAICLASS_API FICSkillCancelRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="사용자 이동으로 시전을 취소합니다. 스킬 자체 이동 예외는 bSkillMovementExempt로 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelOnUserMovement = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="스킬 자체 이동은 사용자 이동 취소에서 제외합니다. 실제 이동 원인을 구분하는 전투 실행 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSkillMovementExempt = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="방어 입력/동작으로 시전을 취소하도록 정의합니다. 실제 취소/비용/무적 종료는 한 번 처리해야 합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelOnGuard = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="회피 입력/동작으로 시전을 취소하도록 정의합니다. 실제 비용 확정 시점/후처리를 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelOnDodge = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="유효 피격으로 시전을 취소합니다. 무효/중복 타격을 실제 실행에서 구분해야 합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelOnEffectiveHit = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="최초 효과 전 대상 사망 시 취소 정의입니다. 최초 효과 이후 정책과 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelBeforeFirstEffectOnTargetDeath = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="최초 효과 전 사거리 이탈 시 취소 정의입니다. 이미 확정된 효과/비용의 반복 취소와 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelBeforeFirstEffectOnRangeExit = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="장비 교체 성공 시 시전을 취소합니다. 실패한 교체 시도까지 취소하는 규칙이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelOnSuccessfulEquipmentChange = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel", meta=(ToolTip="중단 시 아직 실행하지 않은 다단 타격도 취소합니다. 이미 확정된 타격/비용을 다시 취소하거나 실행하지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCancelRemainingHits = true;
};

USTRUCT(BlueprintType, meta=(ToolTip="스킬 타격 하나의 효과 ID·반복·판정 모양·투사체·몽타주 이벤트를 정의합니다. 공격/시전 ID와 타격 번호 검증, 이벤트 누락 처리 등은 실제 전투 실행 측의 책임입니다.", ShortToolTip="스킬 타격 하나의 효과 ID·반복·판정 모양·투사체·몽타주 이벤트를 정의합니다."))
struct ISEKAICLASS_API FICSkillHitDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="WeaponTrace의 무기 손잡이 쪽 소켓입니다. 캐릭터 메시 또는 부착 무기 컴포넌트에서 조회합니다. 소켓이 없으면 타격을 실행하지 않고 오류를 기록합니다.")) FName TraceStartSocket=TEXT("weapon_r");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="WeaponTrace의 무기 끝 소켓입니다. 매 프레임 시작·끝과 이전 프레임의 이동 경로를 구체 스윕하여 검사합니다. 실제 무기 메시의 소켓 이름에 맞추세요.")) FName TraceEndSocket=TEXT("weapon_tip");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ToolTip="무기 추적 구체의 반경입니다. 단위 cm이며 무기 두께에 맞춥니다. 벽 너머 대상과 같은 타격 번호의 중복 대상을 제외합니다.")) double TraceRadiusCm=8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ToolTip="무기 추적의 최대 PlayClock 실행 시간입니다. AN_HitEnd가 먼저 오면 즉시 끝나고 알림이 누락되면 이 시간에 종료합니다. 단위 초입니다.")) double TraceDurationPlaySeconds=.15;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="이 타격이 실행할 SkillEffect DT ID입니다. StatusEffect ID를 직접 넣지 않고 SkillEffect.AppliedStatusEffectIDs로 상태 적용을 정의합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName EffectID;
    // Offset from cast start; no absolute timestamps or remaining cooldowns belong here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ToolTip="작업/스킬/패턴 시작 기준 오프셋 초입니다. 절대 PlayClock 시각과 구분하고 실제 타임라인 이벤트를 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="1", ToolTip="단계/타격 반복 횟수입니다. 양수이며 같은 반복의 타격 번호/이벤트 중복 검증을 연결합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 RepeatCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ToolTip="반복 타격/단계 사이 PlayClock 초 간격입니다. 0은 같은 시간 기준 반복 규격이며 실제 중복 검증을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double RepeatIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="타격 판정 모양 또는 상호작용 메시 컴포넌트입니다. 타입에 따라 범위/충돌 또는 표시 메시를 설정합니다.\n입력 자료형: 선택 목록 (EICHitShape).\nC++ 초기값: Direct — 선택한 대상에 직접 적용하는 방식입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICHitShape Shape = EICHitShape::Direct;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ToolTip="구/원뿔/영역 타격 반지름 cm입니다. Shape에 맞는 실제 충돌 판정에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double RadiusCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ClampMax="360", ToolTip="원뿔 타격 총 각도입니다. 90은 전방 총 90도이며 Shape=Cone의 실제 판정에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 360.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double ConeAngleDegrees = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="박스 타격의 각 축 반쪽 크기 cm입니다. X=100이면 전체 X 길이는 200cm입니다. Shape=Box에서 사용하도록 정의합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FVector BoxHalfExtentCm = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="1", ToolTip="반복 타격마다 처리할 최대 대상 수입니다. 양수이며 실제 충돌/우선순위 실행과 연결합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MaximumTargets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="투사체 등 타격이 대상을 관통하도록 정의합니다. 최대 대상/중복/월드 충돌 규칙을 함께 작성합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bPierceTargets = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="투사체 월드 충돌 시 종료 정의입니다. 관통 대상 규칙과 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bStopAtWorldCollision = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="한 반복 타격에서 같은 대상은 한 번만 처리합니다. 다음 반복과 공격 ID/타격 번호를 구분해 실제 수신기에 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bHitSameTargetOncePerRepeat = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ToolTip="투사체 cm/s 속도입니다. 1000=10m/s이며 실제 ProjectileMovement 설정과 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double ProjectileSpeedCmPerSecond = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ToolTip="투사체 PlayClock 초 수명입니다. 실제 제거/판정 종료 실행을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double ProjectileLifetimePlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="투사체 AActor 자식 BP입니다. 충돌·속도·수명·공격 ID/타격 번호를 검증하는 실제 전투 경로가 필요합니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftClassPtr<AActor> ProjectileClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="선택 몽타주 내부 섹션 이름입니다. 실제 에셋에 존재해야 하며 정의 ID가 아닙니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName MontageSection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ToolTip="AnimNotify 이벤트 이름입니다. 몽타주 존재와 공격/시전 ID·타격 번호 검증 실행이 필요합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NotifyName;
};

USTRUCT(BlueprintType, meta=(ToolTip="동료 자동 스킬의 분류와 사용 조건입니다. 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.", ShortToolTip="동료 자동 스킬의 분류와 사용 조건입니다."))
struct ISEKAICLASS_API FICAutoSkillRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICAutoSkillType).\nC++ 초기값: None — 자동 사용 대상에서 제외합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.")) EICAutoSkillType Type = EICAutoSkillType::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ToolTip="상위 동작 조건 묶음입니다. All/Any와 종류별 필드를 작성합니다. 일반 조건 실행기는 별도 연결 대상이며 정의 검사 성공은 실행 성공과 다릅니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.")) FICConditionGroup Conditions;
    // Inclusive boundary: healing targets at exactly 50% are eligible.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ClampMin="0", ClampMax="1", ToolTip="자동 회복을 시도할 HP/MaxHP 상한입니다. 0.5는 50% 이하이며 실제 AI/스킬 실행에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.5. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.")) double HealAtOrBelowHPRatio = 0.5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ToolTip="자동 버프 등에서 해당 효과가 없는 경우만 사용하도록 정의합니다. 실제 효과 목록/강도 판정을 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.")) bool bRequiresMissingStatusEffect = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ToolTip="자동 해제 스킬에 해제 가능한 효과 존재를 요구합니다. 무효과 낭비를 막는 실제 AI 판정 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 회복 임계값·해제 가능한 효과 존재 여부 등을 판정하는 AI/GAS 실행 연결은 아직 필요합니다.")) bool bRequiresDispellableStatusEffect = true;
};

/** Four static Equipment definition IDs, separate from inventory Item IDs and runtime GUIDs. */
USTRUCT(BlueprintType, meta=(ToolTip="초기 무기·의복·반지·목걸이의 아이템 ID 묶음입니다. Equipment DT의 장비 정의 ID가 아니라 Item DT의 장비 아이템 ID를 지정합니다.", ShortToolTip="초기 무기·의복·반지·목걸이의 아이템 ID 묶음입니다."))
struct ISEKAICLASS_API FICInitialEquipment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ToolTip="시작 무기 Item ID입니다. None이면 무기 없음입니다. 실제 Item/Equipment/직업 규칙을 검사하고 NPC 원본에 준비합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName WeaponID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ToolTip="시작 의복 Item ID입니다. None이면 없음이며 Clothes 슬롯 장비를 지정합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName ClothesID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ToolTip="시작 반지 Item ID입니다. None이면 없음이며 Ring 슬롯 장비를 지정합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName RingID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ToolTip="시작 목걸이 Item ID입니다. None이면 없음이며 Necklace 슬롯 장비를 지정합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NecklaceID;
};

/** One of the three normal attack stages shared by player and recruitable characters. */
USTRUCT(BlueprintType, meta=(ToolTip="일반 공격 콤보의 한 단계입니다. 피해 계수·고정 피해·타격 정의·몽타주와 방어/회피 취소 구간을 함께 작성합니다. 전투 실행기는 아직 연결이 필요합니다.", ShortToolTip="일반 공격 콤보의 한 단계입니다."))
struct ISEKAICLASS_API FICNormalAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0", ToolTip="일반 공격의 원본 능력치 배율입니다. 1=100%이며 FixedDamage와 합산하는 실행 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) double DamageCoefficient = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0", ToolTip="일반 공격의 고정 피해 부분입니다. 실제 방어/치명타 적용은 전투 실행 측에서 처리합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) double FixedDamage = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ToolTip="일반 공격 한 단계의 타격 규칙입니다. 효과 ID·판정·반복·Notify를 작성합니다.\n입력 자료형: 구조/객체 (FICSkillHitDefinition).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) FICSkillHitDefinition Hit;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ToolTip="AnimMontage입니다. 스켈레톤·섹션·Notify와 실제 재생 경로를 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (UAnimMontage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ToolTip="선택 몽타주 내부 섹션 이름입니다. 실제 에셋에 존재해야 하며 정의 ID가 아닙니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) FName MontageSection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0", ToolTip="일반 공격에서 방어/회피 취소 허용 시작 초입니다. 시작 기준 시간과 몽타주를 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) double GuardDodgeCancelStartPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0", ToolTip="일반 공격에서 방어/회피 취소 허용 종료 초입니다. 시작값 이상이며 실제 타임라인 실행에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 전투 실행기는 아직 연결이 필요합니다.")) double GuardDodgeCancelEndPlaySeconds = 0.0;
};
