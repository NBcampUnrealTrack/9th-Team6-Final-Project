#include "Data/Validation/ICDefinitionLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Rows/NPC/ICDialogueRow.h"
#include "Data/Rows/NPC/ICGiftPreferenceRow.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "Data/Rows/NPC/ICShopRow.h"
#include "Data/Rows/Presentation/ICCodexRow.h"
#include "Data/Rows/Presentation/ICHintRow.h"
#include "Data/Rows/Presentation/ICResourceReferenceRow.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "Data/Rows/World/ICHomeRow.h"
#include "Data/Rows/World/ICLandmarkRow.h"
#include "Data/Rows/World/ICMapRow.h"
#include "Data/Rows/World/ICRegionRow.h"
#include "Data/Rows/World/ICSpawnerRow.h"
#include "Data/Rows/World/ICWarpRow.h"

namespace
{
    template<typename RowType, typename VisitorType>
    void VisitRows(const TArray<FICDefinitionTable>& Tables, VisitorType&& Visitor)
    {
        IC_TRACE_FUNCTION();
        for (const FICDefinitionTable& Source : Tables)
        {
            const UDataTable* Table = Source.Table.Get();
            if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(RowType::StaticStruct())) continue;
            for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
            {
                if (!Entry.Value) continue;
                const RowType& Row = *reinterpret_cast<const RowType*>(Entry.Value);
                if (!Row.bDeprecated && !Row.bTestOnly) Visitor(Row, Table, Entry.Key);
            }
        }
    }

    template<typename RowType>
    TMap<FName, const RowType*> IndexRows(const TArray<FICDefinitionTable>& Tables)
    {
        IC_TRACE_FUNCTION();
        TMap<FName, const RowType*> Index;
        VisitRows<RowType>(Tables, [&](const RowType& Row, const UDataTable*, FName) { IC_TRACE_FUNCTION(); Index.Add(Row.ID, &Row); });
        return Index;
    }

    struct FRowChecks
    {
        const UDataTable* Table;
        FName Row;
        bool bComplete;
        TArray<FICDefinitionIssue>& Issues;

        void Add(const FString& Field, const FString& Reason, EICDefinitionSeverity Severity = EICDefinitionSeverity::Error) const
        {
            IC_TRACE_FUNCTION();
            FICDefinitionIssue Issue;
            Issue.Table = Table->GetPathName();
            Issue.Row = Row;
            Issue.Field = Field;
            Issue.Reason = Reason;
            Issue.Severity = Severity;
            Issues.Add(MoveTemp(Issue));
        }

        void Require(bool bSatisfied, const FString& Field, const FString& Reason) const
        {
            IC_TRACE_FUNCTION();
            if (!bSatisfied) Add(Field, Reason, bComplete ? EICDefinitionSeverity::Error : EICDefinitionSeverity::Warning);
        }

        void UniqueIDs(const TArray<FName>& IDs, const FString& Field) const
        {
            IC_TRACE_FUNCTION();
            TSet<FName> Seen;
            for (int32 Index = 0; Index < IDs.Num(); ++Index)
            {
                const FString Path = FString::Printf(TEXT("%s[%d]"), *Field, Index);
                Require(!IDs[Index].IsNone(), Path, TEXT("An assigned definition ID is required."));
                if (Seen.Contains(IDs[Index])) Add(Path, TEXT("The same ID appears more than once."));
                Seen.Add(IDs[Index]);
            }
        }
    };

    bool HasVolume(const FBox& Box)
    {
        IC_TRACE_FUNCTION();
        return Box.IsValid && !Box.Min.ContainsNaN() && !Box.Max.ContainsNaN() && Box.GetSize().GetMin() > 0.0;
    }
}

namespace ICDefinitionValidation
{
    void ValidateNPCWorld(const TArray<FICDefinitionTable>& Tables, bool bComplete, TArray<FICDefinitionIssue>& Issues)
    {
        IC_TRACE_FUNCTION();
        const TMap<FName, const FICNPCRow*> NPCs = IndexRows<FICNPCRow>(Tables);
        const TMap<FName, const FICScheduleRow*> Schedules = IndexRows<FICScheduleRow>(Tables);
        const TMap<FName, const FICServiceRow*> Services = IndexRows<FICServiceRow>(Tables);
        const TMap<FName, const FICAnchorRow*> Anchors = IndexRows<FICAnchorRow>(Tables);

        VisitRows<FICNPCRow>(Tables, [&](const FICNPCRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            const bool bRecruit = Row.Type == EICNPCType::Recruitable;
            if (Row.bRecruitable != bRecruit) Check.Add(TEXT("bRecruitable"), TEXT("The recruitment flag must match NPC Type."));
            if (bRecruit) Check.Require(!Row.RecruitCombatID.IsNone(), TEXT("RecruitCombatID"), TEXT("Recruitable NPCs need a separate combat initial definition."));
            else if (!Row.RecruitCombatID.IsNone() || !Row.ClassID.IsNone()) Check.Add(TEXT("RecruitCombatID/ClassID"), TEXT("Resident and background NPCs cannot own growth/combat definitions."));
            if (Row.Type == EICNPCType::Background && (Row.InitialAffinity != 0 || Row.bEndingTarget || !Row.GiftPreferenceID.IsNone()))
                Check.Add(TEXT("Relationship"), TEXT("Background NPCs cannot have affinity, gifts or an ending target role."));
            Check.Require(!Row.RegionID.IsNone(), TEXT("RegionID"), TEXT("NPC region must be assigned."));
            Check.Require(!Row.InitialAnchorID.IsNone(), TEXT("InitialAnchorID"), TEXT("NPC initial placement requires a stable anchor."));
            Check.Require(!Row.ScheduleID.IsNone(), TEXT("ScheduleID"), TEXT("NPC daily activities require a schedule definition."));
            Check.Require(Row.DialogueIDs.Num() > 0, TEXT("DialogueIDs"), TEXT("NPCs need at least a dialogue or reusable background dialogue template."));
            Check.UniqueIDs(Row.DialogueIDs, TEXT("DialogueIDs"));
            Check.UniqueIDs(Row.ServiceIDs, TEXT("ServiceIDs"));
            if (Row.Type != EICNPCType::Background) Check.Require(!Row.HomeID.IsNone(), TEXT("HomeID"), TEXT("Recruitable/resident NPCs require their home definition."));
            if (Row.InitialAffinity < 0 || Row.InitialAffinity > 500) Check.Add(TEXT("InitialAffinity"), TEXT("Affinity must be an integer from 0 to 500."));
            if (bComplete)
            {
                if (Row.Mesh.IsNull() && Row.AppearanceResourceID.IsNone()) Check.Add(TEXT("Mesh/AppearanceResourceID"), TEXT("A character appearance asset or resource definition is required."));
                if (bRecruit && Row.Portrait.IsNull()) Check.Add(TEXT("Portrait"), TEXT("Recruitable NPCs require a distinct portrait."));
            }
            for (int32 Index = 0; Index < Row.ServiceIDs.Num(); ++Index)
            {
                if (const FICServiceRow* const* Service = Services.Find(Row.ServiceIDs[Index]))
                    if ((*Service)->NPCID != Row.ID) Check.Add(FString::Printf(TEXT("ServiceIDs[%d]"), Index), TEXT("The service belongs to a different NPC."));
            }
        });

        VisitRows<FICScheduleRow>(Tables, [&](const FICScheduleRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(Row.Segments.Num() > 0, TEXT("Segments"), TEXT("A daily schedule needs time intervals."));
            TArray<TPair<int32, int32>> Intervals;
            TSet<FName> SegmentIDs;
            for (int32 Index = 0; Index < Row.Segments.Num(); ++Index)
            {
                const FICScheduleSegment& Segment = Row.Segments[Index];
                const FString Path = FString::Printf(TEXT("Segments[%d]"), Index);
                Check.Require(!Segment.SegmentID.IsNone(), Path + TEXT(".SegmentID"), TEXT("Schedule segments need stable local IDs."));
                if (SegmentIDs.Contains(Segment.SegmentID)) Check.Add(Path + TEXT(".SegmentID"), TEXT("Duplicate schedule segment ID."));
                SegmentIDs.Add(Segment.SegmentID);
                if (Segment.StartGameMinute < 0 || Segment.StartGameMinute >= 1440 || Segment.EndGameMinute <= Segment.StartGameMinute || Segment.EndGameMinute > 1440)
                    Check.Add(Path + TEXT(".Time"), TEXT("Use [start,end), start 0..1439 and end 1..1440; split intervals across midnight."));
                else Intervals.Emplace(Segment.StartGameMinute, Segment.EndGameMinute);
                Check.Require(!Segment.DestinationAnchorID.IsNone(), Path + TEXT(".DestinationAnchorID"), TEXT("Every activity needs its destination/activity anchor."));
                if (Segment.Activity == EICScheduleActivity::Move)
                {
                    Check.Require(!Segment.DepartureAnchorID.IsNone(), Path + TEXT(".DepartureAnchorID"), TEXT("Movement reload starts from its departure anchor."));
                    if (!Segment.DepartureAnchorID.IsNone() && Segment.DepartureAnchorID == Segment.DestinationAnchorID)
                        Check.Add(Path, TEXT("A movement interval needs different departure and destination anchors."));
                }
                if (Segment.Activity != EICScheduleActivity::Work && Segment.ServiceIDs.Num() > 0)
                    Check.Add(Path + TEXT(".ServiceIDs"), TEXT("Services can only be offered during a Work activity."));
                Check.UniqueIDs(Segment.ServiceIDs, Path + TEXT(".ServiceIDs"));
                if (Segment.Activity == EICScheduleActivity::Sleep)
                    if (const FICAnchorRow* const* Anchor = Anchors.Find(Segment.DestinationAnchorID))
                        if ((*Anchor)->Type != EICAnchorType::Bed) Check.Add(Path + TEXT(".DestinationAnchorID"), TEXT("Sleep activities must target a Bed anchor."));
            }
            Intervals.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { IC_TRACE_FUNCTION(); return A.Key < B.Key; });
            int32 CoveredUntil = 0;
            for (const TPair<int32, int32>& Interval : Intervals)
            {
                if (Interval.Key < CoveredUntil) Check.Add(TEXT("Segments.Time"), TEXT("Daily schedule intervals overlap."));
                if (Interval.Key > CoveredUntil) Check.Require(false, TEXT("Segments.Time"), TEXT("Daily schedule contains an uncovered time interval."));
                CoveredUntil = FMath::Max(CoveredUntil, Interval.Value);
            }
            if (Intervals.Num() > 0 && CoveredUntil != 1440) Check.Require(false, TEXT("Segments.Time"), TEXT("Daily schedule must cover the day up to the exclusive 24:00 endpoint."));
            if (!FMath::IsFinite(Row.PathFailureRecoveryPlaySeconds) || Row.PathFailureRecoveryPlaySeconds < 0.0)
                Check.Add(TEXT("PathFailureRecoveryPlaySeconds"), TEXT("Path recovery timeout must be finite and nonnegative PlayClock seconds."));
        });

        VisitRows<FICGiftPreferenceRow>(Tables, [&](const FICGiftPreferenceRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            if (Row.PreferredAffinityDelta < 0 || Row.DislikedAffinityDelta > 0 || Row.NeutralAffinityDelta != 0)
                Check.Add(TEXT("AffinityDelta"), TEXT("Preferred gifts cannot reduce affinity, disliked gifts cannot raise it, and neutral gifts have zero change."));
            TSet<FName> Items;
            for (int32 Index = 0; Index < Row.Items.Num(); ++Index)
            {
                const FName ItemID = Row.Items[Index].ItemID;
                const FString Path = FString::Printf(TEXT("Items[%d].ItemID"), Index);
                Check.Require(!ItemID.IsNone(), Path, TEXT("Gift preferences must reference an item."));
                if (Items.Contains(ItemID)) Check.Add(Path, TEXT("An item cannot have multiple preference entries."));
                Items.Add(ItemID);
            }
        });

        VisitRows<FICDialogueRow>(Tables, [&](const FICDialogueRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(Row.Nodes.Num() > 0, TEXT("Nodes"), TEXT("Dialogue needs presentation nodes."));
            TSet<FName> Nodes;
            for (int32 Index = 0; Index < Row.Nodes.Num(); ++Index)
            {
                const FICDialogueNode& Node = Row.Nodes[Index];
                const FString Path = FString::Printf(TEXT("Nodes[%d]"), Index);
                Check.Require(!Node.NodeID.IsNone(), Path + TEXT(".NodeID"), TEXT("Dialogue nodes need stable local IDs."));
                if (Nodes.Contains(Node.NodeID)) Check.Add(Path + TEXT(".NodeID"), TEXT("Duplicate dialogue node ID."));
                Nodes.Add(Node.NodeID);
                Check.Require(!Node.TextKey.IsEmpty(), Path + TEXT(".TextKey"), TEXT("Dialogue text must reference a StringTable key."));
                TSet<FName> Choices;
                for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
                {
                    const FICDialogueChoice& Choice = Node.Choices[ChoiceIndex];
                    const FString ChoicePath = FString::Printf(TEXT("%s.Choices[%d]"), *Path, ChoiceIndex);
                    Check.Require(!Choice.ChoiceID.IsNone(), ChoicePath + TEXT(".ChoiceID"), TEXT("Dialogue choices need local IDs."));
                    if (Choices.Contains(Choice.ChoiceID)) Check.Add(ChoicePath + TEXT(".ChoiceID"), TEXT("Duplicate dialogue choice ID within a node."));
                    Choices.Add(Choice.ChoiceID);
                    Check.Require(!Choice.TextKey.IsEmpty(), ChoicePath + TEXT(".TextKey"), TEXT("Choice text must reference a StringTable key."));
                }
            }
            Check.Require(!Row.EntryNodeID.IsNone(), TEXT("EntryNodeID"), TEXT("A dialogue entry node is required."));
            if (!Row.EntryNodeID.IsNone() && !Nodes.Contains(Row.EntryNodeID)) Check.Add(TEXT("EntryNodeID"), TEXT("Dialogue entry node does not exist in Nodes."));
            for (int32 Index = 0; Index < Row.Nodes.Num(); ++Index)
            {
                const FICDialogueNode& Node = Row.Nodes[Index];
                if (!Node.NextNodeID.IsNone() && !Nodes.Contains(Node.NextNodeID)) Check.Add(FString::Printf(TEXT("Nodes[%d].NextNodeID"), Index), TEXT("Next dialogue node does not exist."));
                for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
                    if (!Node.Choices[ChoiceIndex].NextNodeID.IsNone() && !Nodes.Contains(Node.Choices[ChoiceIndex].NextNodeID))
                        Check.Add(FString::Printf(TEXT("Nodes[%d].Choices[%d].NextNodeID"), Index, ChoiceIndex), TEXT("Next choice node does not exist."));
            }
        });

        VisitRows<FICServiceRow>(Tables, [&](const FICServiceRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(!Row.NPCID.IsNone(), TEXT("NPCID"), TEXT("A service provider NPC is required."));
            Check.Require(!Row.WorkAnchorID.IsNone(), TEXT("WorkAnchorID"), TEXT("A fixed service workplace anchor is required."));
            Check.Require(Row.OpeningHours.Num() > 0, TEXT("OpeningHours"), TEXT("Service opening intervals must be authored."));
            TArray<TPair<int32, int32>> Hours;
            for (int32 Index = 0; Index < Row.OpeningHours.Num(); ++Index)
            {
                const FICServiceHours& Interval = Row.OpeningHours[Index];
                if (Interval.OpenGameMinute < 0 || Interval.OpenGameMinute >= 1440 || Interval.CloseGameMinute <= Interval.OpenGameMinute || Interval.CloseGameMinute > 1440)
                    Check.Add(FString::Printf(TEXT("OpeningHours[%d]"), Index), TEXT("Use daily [open,close) intervals and split midnight; 24:00 is allowed only as the closing endpoint."));
                else Hours.Emplace(Interval.OpenGameMinute, Interval.CloseGameMinute);
            }
            Hours.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { IC_TRACE_FUNCTION(); return A.Key < B.Key; });
            for (int32 Index = 1; Index < Hours.Num(); ++Index)
                if (Hours[Index].Key < Hours[Index - 1].Value) Check.Add(TEXT("OpeningHours"), TEXT("Service opening intervals overlap."));
            int32 PreviousAffinity = -1;
            for (int32 Index = 0; Index < Row.AffinityDiscounts.Num(); ++Index)
            {
                const FICAffinityDiscountTier& Tier = Row.AffinityDiscounts[Index];
                const FString Path = FString::Printf(TEXT("AffinityDiscounts[%d]"), Index);
                if (Tier.MinimumAffinity < 0 || Tier.MinimumAffinity > 100 || Tier.MinimumAffinity <= PreviousAffinity)
                    Check.Add(Path + TEXT(".MinimumAffinity"), TEXT("Discount thresholds must be unique and strictly increasing within 0..100."));
                if (!FMath::IsFinite(Tier.DiscountRatio) || Tier.DiscountRatio < 0.0 || Tier.DiscountRatio > 1.0)
                    Check.Add(Path + TEXT(".DiscountRatio"), TEXT("Discount ratio must be finite in 0..1."));
                PreviousAffinity = Tier.MinimumAffinity;
            }
            if (Row.BaseCostGold < 0) Check.Add(TEXT("BaseCostGold"), TEXT("Service cost cannot be negative."));
            if (const FICAnchorRow* const* Anchor = Anchors.Find(Row.WorkAnchorID))
            {
                if ((*Anchor)->Type != EICAnchorType::Work) Check.Add(TEXT("WorkAnchorID"), TEXT("Service workplaces must use a Work anchor."));
                if (!Row.RegionID.IsNone() && (*Anchor)->RegionID != Row.RegionID) Check.Add(TEXT("WorkAnchorID"), TEXT("Service workplace belongs to a different region."));
            }
            if (Row.Type == EICServiceType::Shop) Check.Require(!Row.ShopID.IsNone(), TEXT("ShopID"), TEXT("Shop service needs its fixed item list."));
            else if (!Row.ShopID.IsNone()) Check.Add(TEXT("ShopID"), TEXT("Only Shop services can link a shop list."));
            if (Row.Type == EICServiceType::Warp) Check.Require(!Row.WarpID.IsNone(), TEXT("WarpID"), TEXT("Warp service needs its endpoint definition."));
            else if (!Row.WarpID.IsNone()) Check.Add(TEXT("WarpID"), TEXT("Only Warp services can link a warp endpoint."));
            if (Row.Type == EICServiceType::Inn) Check.Require(!Row.HomeID.IsNone(), TEXT("HomeID"), TEXT("Inn service needs the home/bed definition."));
            else if (!Row.HomeID.IsNone()) Check.Add(TEXT("HomeID"), TEXT("Only Inn services can link a lodging home."));
            if (const FICNPCRow* const* NPC = NPCs.Find(Row.NPCID))
            {
                if (!(*NPC)->ServiceIDs.Contains(Row.ID)) Check.Add(TEXT("NPCID"), TEXT("Provider NPC must include this service ID in ServiceIDs."));
                if (const FICScheduleRow* const* Schedule = Schedules.Find((*NPC)->ScheduleID))
                {
                    bool bHasWorkplace = false;
                    for (const FICScheduleSegment& Segment : (*Schedule)->Segments)
                        if (Segment.Activity == EICScheduleActivity::Work && Segment.DestinationAnchorID == Row.WorkAnchorID && (Segment.ServiceIDs.IsEmpty() || Segment.ServiceIDs.Contains(Row.ID))) bHasWorkplace = true;
                    Check.Require(bHasWorkplace, TEXT("WorkAnchorID"), TEXT("Provider schedule has no Work interval at this service anchor."));
                }
            }
        });

        VisitRows<FICShopRow>(Tables, [&](const FICShopRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(Row.Entries.Num() > 0, TEXT("Entries"), TEXT("A shop needs a fixed list of items; stock quantities are unlimited runtime rules."));
            TArray<FName> ItemIDs;
            for (const FICShopEntry& Entry : Row.Entries) ItemIDs.Add(Entry.ItemID);
            Check.UniqueIDs(ItemIDs, TEXT("Entries.ItemID"));
            if (const FICServiceRow* const* Service = Services.Find(Row.ServiceID))
                if ((*Service)->Type != EICServiceType::Shop || (*Service)->ShopID != Row.ID) Check.Add(TEXT("ServiceID"), TEXT("The linked service must be a Shop service pointing back to this list."));
        });

        VisitRows<FICRegionRow>(Tables, [&](const FICRegionRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            if (Row.BoundsCm.IsValid && !HasVolume(Row.BoundsCm)) Check.Add(TEXT("BoundsCm"), TEXT("Region bounds must be finite with positive dimensions in centimeters."));
            else Check.Require(Row.BoundsCm.IsValid != 0, TEXT("BoundsCm"), TEXT("Region world bounds must be authored."));
            Check.UniqueIDs(Row.AnchorIDs, TEXT("AnchorIDs"));
            Check.UniqueIDs(Row.SpawnerIDs, TEXT("SpawnerIDs"));
            Check.UniqueIDs(Row.WarpIDs, TEXT("WarpIDs"));
            Check.UniqueIDs(Row.NPCIDs, TEXT("NPCIDs"));
            if (!Row.BoundaryPolygonCm.IsEmpty())
            {
                if (Row.BoundaryPolygonCm.Num() < 3) Check.Add(TEXT("BoundaryPolygonCm"), TEXT("A polygon boundary needs at least three points."));
                double TwiceArea = 0.0;
                for (int32 Index = 0; Index < Row.BoundaryPolygonCm.Num(); ++Index)
                {
                    const FVector2D& A = Row.BoundaryPolygonCm[Index];
                    const FVector2D& B = Row.BoundaryPolygonCm[(Index + 1) % Row.BoundaryPolygonCm.Num()];
                    if (A.ContainsNaN()) Check.Add(FString::Printf(TEXT("BoundaryPolygonCm[%d]"), Index), TEXT("Region boundary coordinates must be finite."));
                    TwiceArea += A.X * B.Y - B.X * A.Y;
                }
                if (!FMath::IsFinite(TwiceArea) || FMath::Abs(TwiceArea) <= UE_SMALL_NUMBER) Check.Add(TEXT("BoundaryPolygonCm"), TEXT("Region boundary polygon has zero or invalid area."));
            }
            if (bComplete && Row.World.IsNull()) Check.Add(TEXT("World"), TEXT("Region must reference the shared World Partition world."));
        });

        VisitRows<FICAnchorRow>(Tables, [&](const FICAnchorRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            if (Row.WorldTransform.ContainsNaN() || !Row.WorldTransform.GetRotation().IsNormalized()) Check.Add(TEXT("WorldTransform"), TEXT("Anchor transform must be finite with a normalized rotation."));
            if (!FMath::IsFinite(Row.ClearanceRadiusCm) || Row.ClearanceRadiusCm < 0.0 || !FMath::IsFinite(Row.ClearanceHalfHeightCm) || Row.ClearanceHalfHeightCm < 0.0)
                Check.Add(TEXT("Clearance"), TEXT("Anchor clearance dimensions must be finite and nonnegative centimeters."));
            if (Row.SmartObjectSlotIndex < 0) Check.Add(TEXT("SmartObjectSlotIndex"), TEXT("Smart Object slot index cannot be negative."));
        });

        VisitRows<FICSpawnerRow>(Tables, [&](const FICSpawnerRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(Row.Slots.Num() > 0, TEXT("Slots"), TEXT("A spawner needs stable creature slots."));
            TSet<int32> Slots;
            for (int32 Index = 0; Index < Row.Slots.Num(); ++Index)
            {
                const FICSpawnSlotDefinition& Slot = Row.Slots[Index];
                const FString Path = FString::Printf(TEXT("Slots[%d]"), Index);
                if (Slot.SpawnSlotIndex < 0 || Slots.Contains(Slot.SpawnSlotIndex)) Check.Add(Path + TEXT(".SpawnSlotIndex"), TEXT("Spawn slot indices must be nonnegative and unique within this spawner; existing keys cannot be renumbered."));
                Slots.Add(Slot.SpawnSlotIndex);
                Check.Require(!Slot.MonsterID.IsNone(), Path + TEXT(".MonsterID"), TEXT("Spawn slot must identify a monster definition."));
                Check.Require(!Slot.OriginAnchorID.IsNone(), Path + TEXT(".OriginAnchorID"), TEXT("Spawn slot must identify its stable origin anchor."));
                if (const FICAnchorRow* const* Anchor = Anchors.Find(Slot.OriginAnchorID))
                {
                    if ((*Anchor)->Type != EICAnchorType::MonsterOrigin) Check.Add(Path + TEXT(".OriginAnchorID"), TEXT("Creature origins must use a MonsterOrigin anchor."));
                    if (!Row.RegionID.IsNone() && (*Anchor)->RegionID != Row.RegionID) Check.Add(Path + TEXT(".OriginAnchorID"), TEXT("Creature origin belongs to a different region."));
                }
                if (Slot.LeashShape == EICLeashShape::Radius && (!FMath::IsFinite(Slot.LeashRadiusCm) || Slot.LeashRadiusCm <= 0.0)) Check.Add(Path + TEXT(".LeashRadiusCm"), TEXT("Circular leash radius must be positive and finite centimeters."));
                if (Slot.LeashShape == EICLeashShape::Box && !HasVolume(Slot.OriginRelativeLeashBoundsCm)) Check.Add(Path + TEXT(".OriginRelativeLeashBoundsCm"), TEXT("Box leash bounds must have finite positive dimensions."));
                if (Slot.bOverrideRespawnTime && (!FMath::IsFinite(Slot.RespawnPlaySeconds) || Slot.RespawnPlaySeconds < 0.0)) Check.Add(Path + TEXT(".RespawnPlaySeconds"), TEXT("Respawn override must be finite, nonnegative PlayClock seconds."));
            }
        });

        VisitRows<FICWarpRow>(Tables, [&](const FICWarpRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(!Row.ServiceID.IsNone(), TEXT("ServiceID"), TEXT("Warp endpoint requires its provider service."));
            Check.Require(!Row.VisitAnchorID.IsNone(), TEXT("VisitAnchorID"), TEXT("Warp visit unlock requires a stable anchor."));
            Check.Require(!Row.ArrivalAnchorID.IsNone(), TEXT("ArrivalAnchorID"), TEXT("Warp endpoint requires a safe arrival anchor."));
            if (!FMath::IsFinite(Row.VisitRadiusCm) || Row.VisitRadiusCm <= 0.0) Check.Add(TEXT("VisitRadiusCm"), TEXT("Warp visit radius must be positive and finite centimeters."));
            if (const FICServiceRow* const* Service = Services.Find(Row.ServiceID))
                if ((*Service)->Type != EICServiceType::Warp || (*Service)->WarpID != Row.ID) Check.Add(TEXT("ServiceID"), TEXT("The linked service must be a Warp service pointing back to this endpoint."));
            if (const FICAnchorRow* const* Anchor = Anchors.Find(Row.ArrivalAnchorID))
            {
                if (!(*Anchor)->bSafeForArrival || (*Anchor)->Type != EICAnchorType::WarpArrival) Check.Add(TEXT("ArrivalAnchorID"), TEXT("Warp arrival must use a WarpArrival anchor marked safe for arrival."));
                if (!Row.RegionID.IsNone() && (*Anchor)->RegionID != Row.RegionID) Check.Add(TEXT("ArrivalAnchorID"), TEXT("Warp arrival belongs to a different region."));
            }
        });

        TSet<FName> ContainerIDs;
        VisitRows<FICHomeRow>(Tables, [&](const FICHomeRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(Row.BedAnchorIDs.Num() > 0, TEXT("BedAnchorIDs"), TEXT("A home/lodging definition requires its beds."));
            Check.UniqueIDs(Row.BedAnchorIDs, TEXT("BedAnchorIDs"));
            Check.UniqueIDs(Row.KitchenAnchorIDs, TEXT("KitchenAnchorIDs"));
            Check.UniqueIDs(Row.CraftingAnchorIDs, TEXT("CraftingAnchorIDs"));
            Check.UniqueIDs(Row.StorageInstallAnchorIDs, TEXT("StorageInstallAnchorIDs"));
            Check.UniqueIDs(Row.FarmPlotAnchorIDs, TEXT("FarmPlotAnchorIDs"));
            TSet<FName> UsedAnchors;
            auto AddAnchors = [&](const TArray<FName>& IDs, const FString& Field)
            {
                IC_TRACE_FUNCTION();
                for (const FName ID : IDs)
                {
                    if (ID.IsNone()) continue;
                    if (UsedAnchors.Contains(ID)) Check.Add(Field, TEXT("Bed, kitchen, crafting, farm and storage anchors must not overlap."));
                    UsedAnchors.Add(ID);
                }
            };
            AddAnchors(Row.BedAnchorIDs, TEXT("BedAnchorIDs"));
            AddAnchors(Row.KitchenAnchorIDs, TEXT("KitchenAnchorIDs"));
            AddAnchors(Row.CraftingAnchorIDs, TEXT("CraftingAnchorIDs"));
            AddAnchors(Row.StorageInstallAnchorIDs, TEXT("StorageInstallAnchorIDs"));
            AddAnchors(Row.FarmPlotAnchorIDs, TEXT("FarmPlotAnchorIDs"));
            auto CheckAnchorTypes = [&](const TArray<FName>& IDs, EICAnchorType Expected, const FString& Field)
            {
                IC_TRACE_FUNCTION();
                for (int32 Index = 0; Index < IDs.Num(); ++Index)
                    if (const FICAnchorRow* const* Anchor = Anchors.Find(IDs[Index]))
                    {
                        const FString Path = FString::Printf(TEXT("%s[%d]"), *Field, Index);
                        if ((*Anchor)->Type != Expected) Check.Add(Path, TEXT("Home anchor has an incompatible interaction type."));
                        if (!Row.RegionID.IsNone() && (*Anchor)->RegionID != Row.RegionID) Check.Add(Path, TEXT("Home anchor belongs to a different region."));
                    }
            };
            CheckAnchorTypes(Row.BedAnchorIDs, EICAnchorType::Bed, TEXT("BedAnchorIDs"));
            CheckAnchorTypes(Row.KitchenAnchorIDs, EICAnchorType::Kitchen, TEXT("KitchenAnchorIDs"));
            CheckAnchorTypes(Row.CraftingAnchorIDs, EICAnchorType::Crafting, TEXT("CraftingAnchorIDs"));
            CheckAnchorTypes(Row.StorageInstallAnchorIDs, EICAnchorType::StorageInstallation, TEXT("StorageInstallAnchorIDs"));
            CheckAnchorTypes(Row.FarmPlotAnchorIDs, EICAnchorType::FarmPlot, TEXT("FarmPlotAnchorIDs"));
            if (Row.Type == EICHomeType::Player)
            {
                Check.Require(Row.StorageInstallAnchorIDs.Num() == 3, TEXT("StorageInstallAnchorIDs"), TEXT("Player home must define exactly three fixed storage installation anchors."));
                Check.Require(Row.FarmPlotAnchorIDs.Num() == 6, TEXT("FarmPlotAnchorIDs"), TEXT("Player home must define exactly six fixed farm plots."));
                if (Row.StorageInstallAnchorIDs.Num() > 3 || Row.FarmPlotAnchorIDs.Num() > 6) Check.Add(TEXT("PlayerHome.Anchors"), TEXT("Player home cannot exceed three storage anchors or six farm plots."));
                if (Row.MaximumStorageInstallations != 3 || Row.StorageSlotsPerInstallation != 40) Check.Add(TEXT("PlayerHome.Capacity"), TEXT("Player storage is three installations, 40 slots each, sharing at most 120 slots."));
                Check.Require(!Row.SharedStorageGroupID.IsNone(), TEXT("SharedStorageGroupID"), TEXT("All player storage anchors must address one stable shared inventory group."));
                if (!Row.OwnerNPCID.IsNone() || !Row.NPCContainers.IsEmpty()) Check.Add(TEXT("OwnerNPCID/NPCContainers"), TEXT("Player shared storage is separate from NPC ownership and one-time NPC container contents."));
            }
            else if (!Row.StorageInstallAnchorIDs.IsEmpty() || !Row.FarmPlotAnchorIDs.IsEmpty()) Check.Add(TEXT("PlayerHome.Anchors"), TEXT("Player storage installation and farm plot lists belong only to the player home."));
            if (Row.Type == EICHomeType::NPC) Check.Require(!Row.OwnerNPCID.IsNone(), TEXT("OwnerNPCID"), TEXT("NPC home must identify its owner."));
            if (Row.Type == EICHomeType::Inn) Check.Require(!Row.InnServiceID.IsNone(), TEXT("InnServiceID"), TEXT("Inn beds require their lodging service definition."));
            if (Row.Type==EICHomeType::NPC && Row.ContainerAccessAffinity != 200) Check.Add(TEXT("ContainerAccessAffinity"), TEXT("NPC house containers require affinity 200."));
            for (int32 Index = 0; Index < Row.NPCContainers.Num(); ++Index)
            {
                const FICHomeContainerDefinition& Container = Row.NPCContainers[Index];
                const FString Path = FString::Printf(TEXT("NPCContainers[%d]"), Index);
                Check.Require(!Container.ContainerID.IsNone(), Path + TEXT(".ContainerID"), TEXT("NPC containers require stable individual IDs."));
                if (ContainerIDs.Contains(Container.ContainerID)) Check.Add(Path + TEXT(".ContainerID"), TEXT("Container ID is already used in another home/container."));
                ContainerIDs.Add(Container.ContainerID);
                Check.Require(!Container.AnchorID.IsNone(), Path + TEXT(".AnchorID"), TEXT("NPC container requires an interaction anchor."));
                if (!Container.AnchorID.IsNone() && UsedAnchors.Contains(Container.AnchorID)) Check.Add(Path + TEXT(".AnchorID"), TEXT("NPC container anchor overlaps another bed/work/container anchor."));
                UsedAnchors.Add(Container.AnchorID);
                if (Container.SlotCapacity <= 0) Check.Add(Path + TEXT(".SlotCapacity"), TEXT("NPC container capacity must be positive."));
            }
            if (const FICNPCRow* const* Owner = NPCs.Find(Row.OwnerNPCID))
            {
                if ((*Owner)->Type == EICNPCType::Background) Check.Add(TEXT("OwnerNPCID"), TEXT("Background NPCs do not own affinity-locked house containers."));
                if (Row.Type == EICHomeType::NPC && !(*Owner)->HomeID.IsNone() && (*Owner)->HomeID != Row.ID) Check.Add(TEXT("OwnerNPCID"), TEXT("NPC owner's HomeID points to a different home definition."));
            }
        });

        VisitRows<FICMapRow>(Tables, [&](const FICMapRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(!Row.Layers.IsEmpty(), TEXT("Layers"), TEXT("Map needs its outdoor/interior layer projection definitions."));
            TSet<FName> Floors;
            for (int32 Index = 0; Index < Row.Layers.Num(); ++Index)
            {
                const FICMapLayerDefinition& Layer = Row.Layers[Index];
                const FString Path = FString::Printf(TEXT("Layers[%d]"), Index);
                if (Floors.Contains(Layer.InteriorFloorID)) Check.Add(Path + TEXT(".InteriorFloorID"), TEXT("Map layers must have unique floor identifiers; None identifies the outdoor layer."));
                Floors.Add(Layer.InteriorFloorID);
                if (Layer.WorldAxisU.ContainsNaN() || Layer.WorldAxisV.ContainsNaN() || !FMath::IsNearlyEqual(Layer.WorldAxisU.SizeSquared(), 1.0, 0.001) || !FMath::IsNearlyEqual(Layer.WorldAxisV.SizeSquared(), 1.0, 0.001) || !FMath::IsNearlyZero(FVector2D::DotProduct(Layer.WorldAxisU, Layer.WorldAxisV), 0.001))
                    Check.Add(Path + TEXT(".WorldAxes"), TEXT("Map projection axes must be finite, perpendicular unit vectors."));
                if (Layer.WorldCmPerUV.ContainsNaN() || Layer.WorldCmPerUV.X <= 0.0 || Layer.WorldCmPerUV.Y <= 0.0) Check.Add(Path + TEXT(".WorldCmPerUV"), TEXT("Map projection scales must be finite, positive centimeters per UV unit."));
                if (Layer.WorldOriginCm.ContainsNaN() || Layer.MapOriginUV.ContainsNaN() || Layer.WorldBoundsMinCm.ContainsNaN() || Layer.WorldBoundsMaxCm.ContainsNaN()) Check.Add(Path + TEXT(".Projection"), TEXT("Map origins and bounds must contain only finite coordinates."));
                else if (Layer.WorldBoundsMaxCm.X <= Layer.WorldBoundsMinCm.X || Layer.WorldBoundsMaxCm.Y <= Layer.WorldBoundsMinCm.Y)
                    Check.Require(false, Path + TEXT(".WorldBounds"), TEXT("Map bounds must have positive horizontal dimensions."));
                if (bComplete && Layer.MapTexture.IsNull()) Check.Add(Path + TEXT(".MapTexture"), TEXT("Map layer needs its supplied texture asset."));
            }
            TSet<EICMapMarkerType> Markers;
            for (const EICMapMarkerType Marker : Row.VisibleMarkers)
            {
                if (Markers.Contains(Marker)) Check.Add(TEXT("VisibleMarkers"), TEXT("Map marker types must not repeat."));
                Markers.Add(Marker);
                const bool bAllowed = Row.Type == EICMapType::Minimap || Marker == EICMapMarkerType::Warp || Marker == EICMapMarkerType::Quest || (Row.Type == EICMapType::Town && (Marker == EICMapMarkerType::Player || Marker == EICMapMarkerType::Shop));
                if (!bAllowed) Check.Add(TEXT("VisibleMarkers"), TEXT("This marker is hidden by the planned large/town/minimap display policy."));
            }
            const int32 ExpectedMarkers = Row.Type == EICMapType::Large ? 2 : Row.Type == EICMapType::Town ? 4 : 6;
            Check.Require(Markers.Num() == ExpectedMarkers, TEXT("VisibleMarkers"), TEXT("Large map needs Warp/Quest; town needs Player/Shop/Warp/Quest; minimap needs all six types."));
            if (!FMath::IsFinite(Row.MinimumZoom) || !FMath::IsFinite(Row.MaximumZoom) || Row.MinimumZoom <= 0.0 || Row.MaximumZoom < Row.MinimumZoom) Check.Add(TEXT("Zoom"), TEXT("Map zoom must be finite with 0 < minimum <= maximum."));
        });

        TMap<FName, FName> LandmarkRegions;
        VisitRows<FICLandmarkRow>(Tables, [&](const FICLandmarkRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(!Row.RegionID.IsNone(), TEXT("RegionID"), TEXT("Landmark must identify its region."));
            Check.Require(!Row.AnchorID.IsNone(), TEXT("AnchorID"), TEXT("Landmark registration requires its level anchor."));
            if (!Row.RegionID.IsNone() && LandmarkRegions.Contains(Row.RegionID)) Check.Add(TEXT("RegionID"), TEXT("Only one landmark photograph is planned per region."));
            LandmarkRegions.Add(Row.RegionID, Row.ID);
            if (bComplete && Row.Photo.IsNull()) Check.Add(TEXT("Photo"), TEXT("Landmark needs the supplied prepared photo asset."));
        });

        VisitRows<FICCodexRow>(Tables, [&](const FICCodexRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            Check.Require(!Row.Subject.ID.IsNone(), TEXT("Subject.ID"), TEXT("Codex entry must reference its actual definition."));
            if (Row.Category == EICCodexCategory::Item && (Row.Subject.Type != EICDefinitionType::Item || Row.Registration != EICCodexRegistration::FirstItemInspectionOrAcquisition)) Check.Add(TEXT("Category/Subject/Registration"), TEXT("Item codex uses an Item definition and first inspection/acquisition registration."));
            if (Row.Category == EICCodexCategory::NPC && (Row.Subject.Type != EICDefinitionType::NPC || Row.Registration != EICCodexRegistration::FirstNPCConversation)) Check.Add(TEXT("Category/Subject/Registration"), TEXT("NPC codex uses an NPC definition and first conversation registration."));
            if (Row.Category == EICCodexCategory::Landmark && (Row.Subject.Type != EICDefinitionType::Landmark || Row.Registration != EICCodexRegistration::LandmarkInteraction)) Check.Add(TEXT("Category/Subject/Registration"), TEXT("Landmark codex uses a Landmark definition and interaction registration."));
        });

        VisitRows<FICResourceReferenceRow>(Tables, [&](const FICResourceReferenceRow& Row, const UDataTable* Table, FName Name)
        {
            IC_TRACE_FUNCTION();
            const FRowChecks Check{Table, Name, bComplete, Issues};
            if (Row.ImportScale.ContainsNaN() || Row.ImportScale.GetMin() <= 0.0) Check.Add(TEXT("ImportScale"), TEXT("Resource import scale must be finite and positive."));
            if (Row.DimensionsCm.ContainsNaN() || Row.DimensionsCm.GetMin() < 0.0) Check.Add(TEXT("DimensionsCm"), TEXT("Resource dimensions must be finite and nonnegative centimeters."));
            if (bComplete && Row.Asset.IsNull()) Check.Add(TEXT("Asset"), TEXT("Completed resource definitions require a supplied real asset path."));
        });
    }
}
