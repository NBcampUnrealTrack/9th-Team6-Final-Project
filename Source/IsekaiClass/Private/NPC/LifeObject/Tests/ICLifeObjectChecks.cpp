#include "Misc/AutomationTest.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifeObjectActivityTagCheck, "IC.NPC.LifeObject.ActivityTags",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICLifeObjectActivityTagCheck::RunTest(const FString&)
{
    // 시간표 Activity → 탐색 태그
    TestTrue(TEXT("Sleep 구간은 Sleep 태그로 찾는다"), ICLife::ResolveActivityTag(EICScheduleActivity::Sleep, FGameplayTag()) == ICLifeTags::Sleep.GetTag());
    TestTrue(TEXT("Work 구간은 Work 부모 태그로 찾는다"), ICLife::ResolveActivityTag(EICScheduleActivity::Work, FGameplayTag()) == ICLifeTags::Work.GetTag());
    TestFalse(TEXT("Idle 구간은 오브젝트를 찾지 않는다"), ICLife::ResolveActivityTag(EICScheduleActivity::Idle, FGameplayTag()).IsValid());
    TestTrue(TEXT("구간 ActivityTag가 있으면 우선한다"), ICLife::ResolveActivityTag(EICScheduleActivity::Idle, ICLifeTags::Sit.GetTag()) == ICLifeTags::Sit.GetTag());

    // 오브젝트 기본 태그 매칭(부모 태그 검색)
    TestTrue(TEXT("카운터는 Work 검색에 걸린다"), ICLife::GetDefaultActivityTags(EICLifeObjectType::ShopCounter).HasTag(ICLifeTags::Work));
    TestTrue(TEXT("작업대는 Work 검색에 걸린다"), ICLife::GetDefaultActivityTags(EICLifeObjectType::Workbench).HasTag(ICLifeTags::Work));
    TestFalse(TEXT("작업대는 Shop 검색에 걸리지 않는다"), ICLife::GetDefaultActivityTags(EICLifeObjectType::Workbench).HasTag(ICLifeTags::WorkShop));
    TestTrue(TEXT("침대는 Sleep 검색에 걸린다"), ICLife::GetDefaultActivityTags(EICLifeObjectType::Bed).HasTag(ICLifeTags::Sleep));
    TestFalse(TEXT("의자는 Sleep 검색에 걸리지 않는다"), ICLife::GetDefaultActivityTags(EICLifeObjectType::Seat).HasTag(ICLifeTags::Sleep));

    // 같은 구간 판정
    FICLifeRoutine A;
    A.bValid = true; A.SegmentID = TEXT("Work"); A.ActivityTag = ICLifeTags::Work; A.StartGameMinute = 360;
    FICLifeRoutine B = A;
    TestTrue(TEXT("같은 구간"), A.IsSameSegment(B));
    B.SegmentID = TEXT("Sleep");
    TestFalse(TEXT("구간이 바뀌면 다르다"), A.IsSameSegment(B));
    return true;
}
#endif
