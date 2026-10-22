#include "Combat/GAS/ICGameplayTags.h"
namespace ICTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(DataAmount, "IC.Data.Amount", "효과 적용량. 회복은 양수, 비용과 피해는 음수입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attacking, "IC.State.Attacking", "일반 공격 실행 중입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "IC.State.Casting", "스킬 시전 또는 후딜 실행 중입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Guarding, "IC.State.Guarding", "전방 방어 자세입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dodging, "IC.State.Dodging", "수동 회피 동작입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invulnerable, "IC.State.Invulnerable", "피해와 차단 대상 상태 효과를 받지 않습니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Downed, "IC.State.Downed", "동료가 쓰러진 상태입니다. 파티 슬롯과 효과는 유지합니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead, "IC.State.Dead", "플레이어 또는 몬스터 사망 상태입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Returning, "IC.State.Returning", "몬스터 귀환 중입니다. 공격·피해·재어그로를 차단합니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Exhausted, "IC.State.Exhausted", "플레이어 SP 0의 탈진입니다. 전투 입력은 계속 허용합니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Staggered, "IC.State.Staggered", "유효 피격 경직입니다.");
    UE_DEFINE_GAMEPLAY_TAG(CombatEvent, "IC.Event.Combat");
    UE_DEFINE_GAMEPLAY_TAG(EffectCommitEvent, "IC.Event.Combat.EffectCommit");
    UE_DEFINE_GAMEPLAY_TAG(HitStartEvent, "IC.Event.Combat.HitStart");
    UE_DEFINE_GAMEPLAY_TAG(HitEndEvent, "IC.Event.Combat.HitEnd");
    UE_DEFINE_GAMEPLAY_TAG(ProjectileFireEvent, "IC.Event.Combat.ProjectileFire");
    UE_DEFINE_GAMEPLAY_TAG(CancelWindowStartEvent, "IC.Event.Combat.CancelWindowStart");
    UE_DEFINE_GAMEPLAY_TAG(CancelWindowEndEvent, "IC.Event.Combat.CancelWindowEnd");
    UE_DEFINE_GAMEPLAY_TAG(IFrameStartEvent, "IC.Event.Combat.IFrameStart");
    UE_DEFINE_GAMEPLAY_TAG(IFrameEndEvent, "IC.Event.Combat.IFrameEnd");
    UE_DEFINE_GAMEPLAY_TAG(CueCast, "GameplayCue.IC.Combat.Cast");
    UE_DEFINE_GAMEPLAY_TAG(CueImpact, "GameplayCue.IC.Combat.Impact");
}
