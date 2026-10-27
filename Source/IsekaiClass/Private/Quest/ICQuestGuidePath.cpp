#include "ICQuestGuidePath.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "NavMesh/RecastNavMesh.h"

namespace ICQuestGuidePath
{
namespace
{
    // Bound fallback work per pulse so disconnected open-world data cannot cause an unbounded search.
    constexpr int32 MaxSearchPolygons = 512;
    constexpr int32 MaxCollisionChecks = 2048;
    constexpr float EffectRadius = 5.f;

    struct FSearchNode
    {
        NavNodeRef Ref = INVALID_NAVNODEREF;
        FVector Center = FVector::ZeroVector;
        double Cost = TNumericLimits<double>::Max();
        int32 Parent = INDEX_NONE;
        TArray<FVector> Connection;
        bool bClosed = false;
        bool bStepBridge = false;
    };

    FVector Flat(const FVector& P) { return FVector(P.X, P.Y, 0); }

    // Find points on the two boundary edges, preserving their original elevations.
    bool ClosestBoundary(const TArray<FNavigationPortalEdge>& A,
        const TArray<FNavigationPortalEdge>& B, float MaxGap, int32 Sample,
        const FVector& Approach, const FVector& Goal, FVector& From, FVector& To)
    {
        double Best = TNumericLimits<double>::Max();
        bool bFound = false;
        for (const FNavigationPortalEdge& EA : A)
        {
            for (const FNavigationPortalEdge& EB : B)
            {
                const FVector A0 = Flat(EA.Left), A1 = Flat(EA.Right);
                const FVector B0 = Flat(EB.Left), B1 = Flat(EB.Right);
                FVector PA, PB;
                if (Sample == 3)
                {
                    // Intersect the destination direction with this ledge in XY.
                    const FVector Direction = Flat(Goal - Approach);
                    const FVector Edge = A1 - A0;
                    const double Cross = Direction.X * Edge.Y - Direction.Y * Edge.X;
                    if (FMath::Abs(Cross) <= UE_SMALL_NUMBER) continue;
                    const FVector Delta = A0 - Flat(Approach);
                    const double T = (Delta.X * Edge.Y - Delta.Y * Edge.X) / Cross;
                    const double U = (Delta.X * Direction.Y - Delta.Y * Direction.X) / Cross;
                    if (T < 0 || U < 0 || U > 1) continue;
                    PA = A0 + Edge * U;
                    PB = FMath::ClosestPointOnSegment(PA, B0, B1);
                }
                else if (Sample == 4)
                {
                    PA = FMath::ClosestPointOnSegment(Flat(Goal), A0, A1);
                    PB = FMath::ClosestPointOnSegment(PA, B0, B1);
                }
                else if (Sample == 1)
                {
                    PA = (A0 + A1) * .5;
                    PB = FMath::ClosestPointOnSegment(PA, B0, B1);
                }
                else if (Sample == 2)
                {
                    PB = (B0 + B1) * .5;
                    PA = FMath::ClosestPointOnSegment(PB, A0, A1);
                }
                else FMath::SegmentDistToSegmentSafe(A0, A1, B0, B1, PA, PB);
                const double Distance = FVector::DistSquared(PA, PB);
                if (Distance > FMath::Square(double(MaxGap))) continue;
                const FVector DA = A1 - A0, DB = B1 - B0;
                const double TA = DA.SizeSquared() > UE_SMALL_NUMBER
                    ? FMath::Clamp(FVector::DotProduct(PA - A0, DA) / DA.SizeSquared(), 0., 1.) : 0.;
                const double TB = DB.SizeSquared() > UE_SMALL_NUMBER
                    ? FMath::Clamp(FVector::DotProduct(PB - B0, DB) / DB.SizeSquared(), 0., 1.) : 0.;
                const FVector CandidateFrom = FMath::Lerp(EA.Left, EA.Right, TA);
                const FVector CandidateTo = FMath::Lerp(EB.Left, EB.Right, TB);
                const double Score = FVector::Distance(Approach, CandidateFrom)
                    + FMath::Abs(CandidateFrom.Z - CandidateTo.Z) + FVector::Dist2D(CandidateFrom, CandidateTo)
                    + FVector::Distance(CandidateTo, Goal);
                if (Score >= Best) continue;
                From = CandidateFrom;
                To = CandidateTo;
                Best = Score;
                bFound = true;
            }
        }
        return bFound;
    }
}

bool BuildStepBridge(UWorld& World, const FVector& From, const FVector& To,
    float VisualHeight, const AActor* Owner, TArray<FVector>& Points)
{
    Points.Reset();
    if (From.ContainsNaN() || To.ContainsNaN() || !FMath::IsFinite(VisualHeight)) return false;
    // Rise beside the step, cross above its upper surface, then descend if needed.
    // There is deliberately no limit on vertical separation.
    const double Top = FMath::Max(From.Z, To.Z);
    const FVector Candidates[] = {From, FVector(From.X, From.Y, Top), FVector(To.X, To.Y, Top), To};
    FCollisionQueryParams Query(SCENE_QUERY_STAT(QuestGuideStepBridge), false, Owner);
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    const FVector Offset(0, 0, VisualHeight);
    for (const FVector& Candidate : Candidates)
    {
        if (!Points.IsEmpty() && Points.Last().Equals(Candidate, .1)) continue;
        const FVector End = Candidate + Offset;
        if (World.OverlapAnyTestByObjectType(End, FQuat::Identity, Objects,
            FCollisionShape::MakeSphere(EffectRadius), Query)
            || (!Points.IsEmpty() && World.SweepTestByObjectType(Points.Last() + Offset, End,
                FQuat::Identity, Objects, FCollisionShape::MakeSphere(EffectRadius), Query)))
        {
            Points.Reset();
            return false;
        }
        Points.Add(Candidate);
    }
    return Points.Num() >= 2;
}

bool FindAcrossSteps(UWorld& World, const ARecastNavMesh& Mesh,
    const FNavLocation& Start, const FNavLocation& Goal, float MaxHorizontalGap,
    float VisualHeight, const AActor* Owner, TArray<FVector>& Points)
{
    Points.Reset();
    if (!Start.HasNodeRef() || !Goal.HasNodeRef() || !FMath::IsFinite(MaxHorizontalGap)
        || MaxHorizontalGap <= 0 || !FMath::IsFinite(VisualHeight)) return false;
    const FBox Bounds = Mesh.GetNavMeshBounds();
    if (!Bounds.IsValid) return false;
    TArray<FSearchNode> Nodes;
    TMap<NavNodeRef, int32> Indices;
    auto AddNode = [&](NavNodeRef Ref) -> int32
    {
        if (const int32* Existing = Indices.Find(Ref)) return *Existing;
        FVector Center;
        if (Nodes.Num() >= MaxSearchPolygons || !Mesh.GetPolyCenter(Ref, Center)) return INDEX_NONE;
        const int32 Index = Nodes.AddDefaulted();
        Nodes[Index].Ref = Ref;
        Nodes[Index].Center = Center;
        Indices.Add(Ref, Index);
        return Index;
    };
    const int32 StartIndex = AddNode(Start.NodeRef);
    const int32 GoalIndex = AddNode(Goal.NodeRef);
    if (StartIndex == INDEX_NONE || GoalIndex == INDEX_NONE) return false;
    Nodes[StartIndex].Center = Start.Location;
    Nodes[GoalIndex].Center = Goal.Location;
    Nodes[StartIndex].Cost = 0;
    int32 CollisionChecks = 0;
    for (int32 Iteration = 0; Iteration < MaxSearchPolygons; ++Iteration)
    {
        int32 Current = INDEX_NONE;
        double Best = TNumericLimits<double>::Max();
        for (int32 Index = 0; Index < Nodes.Num(); ++Index)
        {
            const FSearchNode& Node = Nodes[Index];
            if (Node.bClosed || Node.Cost == TNumericLimits<double>::Max()) continue;
            const double Score = Node.Cost + FVector::Distance(Node.Center, Goal.Location);
            if (Score < Best) { Best = Score; Current = Index; }
        }
        if (Current == INDEX_NONE) return false;
        if (Current == GoalIndex)
        {
            TArray<int32> Route;
            for (int32 Index = Current; Index != StartIndex; Index = Nodes[Index].Parent)
            {
                if (!Nodes.IsValidIndex(Index) || Nodes[Index].Parent == INDEX_NONE) return false;
                Route.Add(Index);
            }
            // String-pull each connected corridor separately. Step bridge points
            // remain fixed, so simplification cannot cut across a gap or wall.
            TArray<FVector> Result;
            TArray<NavNodeRef> Corridor{Start.NodeRef};
            FVector SegmentStart = Start.Location;
            auto AppendPoint = [&Result](const FVector& Point)
            {
                if (Result.IsEmpty() || !Result.Last().Equals(Point, .1)) Result.Add(Point);
            };
            auto AppendCorridor = [&](const FVector& End)
            {
                TArray<FNavPathPoint> StraightPath;
                if (!Mesh.FindStraightPath(SegmentStart, End, Corridor, StraightPath)
                    || StraightPath.IsEmpty()) return false;
                for (const FNavPathPoint& Point : StraightPath) AppendPoint(Point.Location);
                return true;
            };
            for (int32 Index = Route.Num() - 1; Index >= 0; --Index)
            {
                const FSearchNode& Node = Nodes[Route[Index]];
                if (Node.bStepBridge)
                {
                    if (Node.Connection.Num() < 2 || !AppendCorridor(Node.Connection[0])) return false;
                    for (const FVector& Point : Node.Connection) AppendPoint(Point);
                    SegmentStart = Node.Connection.Last();
                    Corridor.Reset();
                }
                Corridor.Add(Node.Ref);
            }
            if (!AppendCorridor(Goal.Location) || Result.Num() < 2) return false;
            Points = MoveTemp(Result);
            return true;
        }
        Nodes[Current].bClosed = true;
        // Copy values before AddNode can reallocate Nodes.
        const FVector Center = Nodes[Current].Center;
        const NavNodeRef Ref = Nodes[Current].Ref;
        const double Cost = Nodes[Current].Cost;
        auto Relax = [&](NavNodeRef NextRef, const TArray<FVector>& Connection, bool bStepBridge = false)
        {
            const int32 Next = AddNode(NextRef);
            if (Next == INDEX_NONE || Nodes[Next].bClosed) return;
            double NewCost = Cost;
            FVector Previous = Center;
            for (const FVector& Point : Connection)
            {
                NewCost += FVector::Distance(Previous, Point);
                Previous = Point;
            }
            NewCost += FVector::Distance(Previous, Nodes[Next].Center);
            if (NewCost < Nodes[Next].Cost)
            {
                Nodes[Next].Cost = NewCost;
                Nodes[Next].Parent = Current;
                Nodes[Next].Connection = Connection;
                Nodes[Next].bStepBridge = bStepBridge;
            }
        };
        TArray<FNavigationPortalEdge> Neighbors;
        Mesh.GetPolyNeighbors(Ref, Neighbors);
        for (const FNavigationPortalEdge& Edge : Neighbors)
            Relax(Edge.ToRef, {Edge.GetMiddlePoint()});

        TArray<FNavigationPortalEdge> Walls;
        Mesh.GetPolyWallSegments(Ref, Mesh.GetDefaultQueryFilter(), Owner, Walls);
        if (Walls.IsEmpty() || CollisionChecks >= MaxCollisionChecks) continue;
        TArray<FVector> Vertices;
        if (!Mesh.GetPolyVerts(Ref, Vertices)) continue;
        FBox Search(ForceInit);
        for (const FVector& Vertex : Vertices) Search += Vertex;
        Search = Search.ExpandBy(FVector(MaxHorizontalGap, MaxHorizontalGap, 0));
        Search.Min.Z = Bounds.Min.Z;
        Search.Max.Z = Bounds.Max.Z;
        TArray<FNavPoly> Nearby;
        Mesh.GetPolysInBox(Search, Nearby, Mesh.GetDefaultQueryFilter(), Owner);
        for (const FNavPoly& Poly : Nearby)
        {
            if (Poly.Ref == Ref || Neighbors.ContainsByPredicate(
                [&](const FNavigationPortalEdge& Edge) { return Edge.ToRef == Poly.Ref; })) continue;
            if (const int32* Known = Indices.Find(Poly.Ref); Known && Nodes[*Known].bClosed) continue;
            TArray<FNavigationPortalEdge> OtherWalls;
            Mesh.GetPolyWallSegments(Poly.Ref, Mesh.GetDefaultQueryFilter(), Owner, OtherWalls);
            TArray<FVector> BestBridge;
            double BestBridgeScore = TNumericLimits<double>::Max();
            // Compare destination-facing crossings with midpoint/corner fallbacks.
            for (int32 Sample : {3, 4, 1, 0, 2})
            {
                FVector From, To;
                if (!ClosestBoundary(Walls, OtherWalls, MaxHorizontalGap, Sample, Center, Goal.Location, From, To)) continue;
                if (++CollisionChecks > MaxCollisionChecks) break;
                TArray<FVector> Bridge;
                if (BuildStepBridge(World, From, To, VisualHeight, Owner, Bridge))
                {
                    double Score = FVector::Distance(Center, Bridge[0])
                        + FVector::Distance(Bridge.Last(), Goal.Location);
                    for (int32 Index = 1; Index < Bridge.Num(); ++Index)
                        Score += FVector::Distance(Bridge[Index - 1], Bridge[Index]);
                    if (Score < BestBridgeScore)
                    {
                        BestBridgeScore = Score;
                        BestBridge = MoveTemp(Bridge);
                    }
                }
            }
            if (!BestBridge.IsEmpty()) Relax(Poly.Ref, BestBridge, true);
            if (CollisionChecks >= MaxCollisionChecks) break;
        }
    }
    return false;
}
}
