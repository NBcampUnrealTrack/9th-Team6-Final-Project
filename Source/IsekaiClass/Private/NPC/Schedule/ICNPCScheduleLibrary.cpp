#include "NPC/Schedule/ICNPCScheduleLibrary.h"

FICNPCScheduleResolution UICNPCScheduleLibrary::ResolveSchedule(const FICScheduleRow& Schedule, double GameMinute)
{
    FICNPCScheduleResolution Result;
    if (!FMath::IsFinite(GameMinute)) return Result;

    Result.GameMinuteOfDay = FMath::FloorToInt(GameMinute) % 1440;
    if (Result.GameMinuteOfDay < 0) Result.GameMinuteOfDay += 1440;
    const FICScheduleSegment* Segment = Schedule.Segments.FindByPredicate(
        [&Result](const FICScheduleSegment& Candidate)
        {
            return Result.GameMinuteOfDay >= Candidate.StartGameMinute
                && Result.GameMinuteOfDay < Candidate.EndGameMinute;
        });
    if (!Segment) return Result;

    Result.bFound = true;
    Result.SegmentID = Segment->SegmentID;
    Result.Activity = Segment->Activity;
    Result.DepartureAnchorID = Segment->DepartureAnchorID;
    Result.DestinationAnchorID = Segment->DestinationAnchorID;
    return Result;
}
