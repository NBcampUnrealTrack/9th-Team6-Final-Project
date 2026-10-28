#include "NPC/LifeObject/ICLifeObjectTypes.h"

namespace ICLifeTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Activity, "IC.Life.Activity", "NPC 생활 연출 활동 루트 태그입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sleep, "IC.Life.Activity.Sleep", "침대 수면 연출입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sit, "IC.Life.Activity.Sit", "의자/벤치 앉기 연출입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rest, "IC.Life.Activity.Rest", "휴식·대기 연출입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Work, "IC.Life.Activity.Work", "근무 연출 공통 부모 태그입니다. 시간표에 이 태그만 쓰면 모든 작업 오브젝트가 후보입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(WorkShop, "IC.Life.Activity.Work.Shop", "상점 카운터 근무 연출입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(WorkCraft, "IC.Life.Activity.Work.Craft", "작업대 제작 연출입니다. 실제 생산은 하지 않습니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(WorkCook, "IC.Life.Activity.Work.Cook", "주방 조리 연출입니다. 실제 재료 소비는 하지 않습니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(WorkFarm, "IC.Life.Activity.Work.Farm", "농장·목축 작업 연출입니다. 실제 생산은 하지 않습니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Leisure, "IC.Life.Activity.Leisure", "비전투 여가 연출 공통 부모 태그입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeisureWalk, "IC.Life.Activity.Leisure.Walk", "산책 지점입니다.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeisureView, "IC.Life.Activity.Leisure.View", "구경 지점입니다.");
}

namespace ICLife
{
FGameplayTagContainer GetDefaultActivityTags(EICLifeObjectType Type)
{
    FGameplayTagContainer Tags;
    switch (Type)
    {
    case EICLifeObjectType::Bed:         Tags.AddTag(ICLifeTags::Sleep); break;
    case EICLifeObjectType::Seat:        Tags.AddTag(ICLifeTags::Sit); Tags.AddTag(ICLifeTags::Rest); break;
    case EICLifeObjectType::ShopCounter: Tags.AddTag(ICLifeTags::WorkShop); break;
    case EICLifeObjectType::Workbench:   Tags.AddTag(ICLifeTags::WorkCraft); break;
    case EICLifeObjectType::Kitchen:     Tags.AddTag(ICLifeTags::WorkCook); break;
    case EICLifeObjectType::FarmSpot:    Tags.AddTag(ICLifeTags::WorkFarm); break;
    case EICLifeObjectType::Leisure:     Tags.AddTag(ICLifeTags::Leisure); Tags.AddTag(ICLifeTags::Rest); break;
    default: break;
    }
    return Tags;
}

FGameplayTag ResolveActivityTag(EICScheduleActivity Activity, const FGameplayTag& SegmentTag)
{
    // 시간표에 명시한 태그가 IC.Life.Activity 계열이면 그대로 사용한다.
    if (SegmentTag.IsValid() && SegmentTag.MatchesTag(ICLifeTags::Activity)) return SegmentTag;
    switch (Activity)
    {
    case EICScheduleActivity::Sleep: return ICLifeTags::Sleep;
    case EICScheduleActivity::Work:  return ICLifeTags::Work;
    default: return FGameplayTag(); // Idle/Move는 오브젝트 없이 앵커에서 처리한다.
    }
}

const TCHAR* LexType(EICLifeObjectType Type)
{
    switch (Type)
    {
    case EICLifeObjectType::Bed: return TEXT("Bed");
    case EICLifeObjectType::Seat: return TEXT("Seat");
    case EICLifeObjectType::ShopCounter: return TEXT("ShopCounter");
    case EICLifeObjectType::Workbench: return TEXT("Workbench");
    case EICLifeObjectType::Kitchen: return TEXT("Kitchen");
    case EICLifeObjectType::FarmSpot: return TEXT("FarmSpot");
    case EICLifeObjectType::Leisure: return TEXT("Leisure");
    default: return TEXT("Unknown");
    }
}
}
