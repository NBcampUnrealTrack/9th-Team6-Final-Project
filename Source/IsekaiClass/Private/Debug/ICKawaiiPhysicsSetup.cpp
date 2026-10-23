#include "Debug/ICCharacterSetupLibrary.h"
#if WITH_EDITOR
#include "AnimGraphNode_KawaiiPhysics.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/SkeletalMesh.h"
#include "VrmMetaObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

namespace
{
struct FICKawaiiGroup
{
    TArray<FName> Roots;
    TSet<int32> Colliders;
};
FVector ToUE(const FVector& Value){return FVector(Value.X,-Value.Z,Value.Y)*100;}

void ConfigureNode(FAnimNode_KawaiiPhysics& Node,const FString& Kind,const FICKawaiiGroup& Group,const FVRM1SpringBoneMeta& Meta)
{
    Node.RootBone.BoneName=Group.Roots[0];
    for(int32 Index=1;Index<Group.Roots.Num();++Index)
    {FKawaiiPhysicsRootBoneSetting Root;Root.RootBone.BoneName=Group.Roots[Index];Node.AdditionalRootBones.Add(Root);}
    const bool Long=Kind==TEXT("HairLong"),Short=Kind==TEXT("HairShort"),Skirt=Kind==TEXT("Skirt"),Bust=Kind==TEXT("Bust");
    Node.PhysicsSettings.Stiffness=Long?.025f:Short?.08f:Skirt?.08f:Bust?.15f:.14f;
    Node.PhysicsSettings.Damping=Long?.18f:Short?.30f:Skirt?.25f:.30f;
    Node.PhysicsSettings.WorldDampingLocation=.8f;Node.PhysicsSettings.WorldDampingRotation=.8f;
    Node.PhysicsSettings.Radius=Long?1.5f:Short?.8f:Skirt?2.f:1.f;
    Node.PhysicsSettings.LimitAngle=Long?55.f:Short?25.f:Skirt?45.f:Bust?15.f:35.f;
    Node.Gravity=FVector(0,0,Long?-200.f:Short?-120.f:Skirt?-220.f:Bust?-40.f:-160.f);
    Node.bUseWorldSpaceGravity=true;Node.bUseDefaultGravityZProjectSetting=false;Node.bAllowWorldCollision=false;
    Node.TargetFramerate=60;Node.TeleportDistanceThreshold=300;Node.TeleportRotationThreshold=60;
    // Keep one rigid segment per real bone. Subdivision bends the intermediate
    // path and can shorten the distance between actual skinned bone endpoints.
    Node.DummyBoneLength=0;Node.BoneSubdivisionCount=0;
    Node.StiffnessCurveData.GetRichCurve()->AddKey(0,1.5f);Node.StiffnessCurveData.GetRichCurve()->AddKey(1,.5f);
    Node.DampingCurveData.GetRichCurve()->AddKey(0,1.2f);Node.DampingCurveData.GetRichCurve()->AddKey(1,.9f);
    for(const int32 Index:Group.Colliders)
    {
        const auto& Collider=Meta.Colliders[Index];
        if(Collider.shapeType==TEXT("sphere"))
        {FSphericalLimit Limit;Limit.DrivingBone.BoneName=*Collider.boneName;Limit.OffsetLocation=ToUE(Collider.offset);Limit.Radius=Collider.radius*100;Node.SphericalLimits.Add(Limit);}
        else
        {
            const FVector A=ToUE(Collider.offset),B=ToUE(Collider.tail);
            FCapsuleLimit Limit;Limit.DrivingBone.BoneName=*Collider.boneName;
            Limit.OffsetLocation=(A+B)*.5f;Limit.OffsetRotation=FQuat::FindBetweenNormals(FVector::UpVector,(B-A).GetSafeNormal()).Rotator();
            Limit.Length=FVector::Distance(A,B);Limit.Radius=Collider.radius*100;Node.CapsuleLimits.Add(Limit);
        }
    }
}
}
#endif

FString UICCharacterSetupLibrary::ConfigureKawaiiSecondaryMotion(bool bApply)
{
#if WITH_EDITOR
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("KawaiiPhysicsReview"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FString CSV=TEXT("Character,SpringRoots,KawaiiNodes,VRMNodes,Applied\n"),GraphLog;
    TArray<UPackage*> Packages;
    for(int32 Index=0;Index<13;++Index)
    {
        const bool Player=Index<2;const int32 Number=Index-1;
        const FString ID=Player?(Index==0?TEXT("PLAYER_M"):TEXT("PLAYER_F")):FString::Printf(TEXT("NPC_R01_%03d"),Number);
        const FString FolderPath=Player?TEXT("/Game/IsekaiClass/Player/SamplePlayer/")+(Index==0?FString(TEXT("Male")):FString(TEXT("Female"))):TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/")+ID;
        const FString Name=Player?(Index==0?TEXT("Player_M"):TEXT("Player_F")):FString::Printf(TEXT("R01_%d"),Number);
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*(FolderPath+TEXT("/SK_")+Name));
        auto* Meta=LoadObject<UVrmMetaObject>(nullptr,*(FolderPath+TEXT("/VM_")+Name+TEXT("_VrmMeta")));
        auto* Class=Mesh?Mesh->GetPostProcessAnimBlueprint().Get():nullptr;
        auto* Blueprint=Class?Cast<UAnimBlueprint>(Class->ClassGeneratedBy):nullptr;
        if(!Blueprint||!Meta)return TEXT("ERROR: Missing post process or metadata for ")+ID;
        const auto& Ref=Mesh->GetRefSkeleton();TArray<FTransform> Reference;
        for(int32 Bone=0;Bone<Ref.GetNum();++Bone)
        {auto Transform=Ref.GetRefBonePose()[Bone];const int32 Parent=Ref.GetParentIndex(Bone);if(Parent>=0)Transform*=Reference[Parent];Reference.Add(Transform);}
        TMap<FString,FICKawaiiGroup> Groups;int32 RootCount=0;
        for(const auto& Spring:Meta->VRM1SpringBoneMeta.Springs)
        {
            if(Spring.joints.Num()<2)continue;
            const auto& Root=Spring.joints[0];const FString Bone=Root.boneName;
            FString Kind=Bone.Contains(TEXT("Hair"))?TEXT("HairShort"):Bone.Contains(TEXT("Skirt"))?TEXT("Skirt"):Bone.Contains(TEXT("Sleeve"))?TEXT("Sleeve"):Bone.Contains(TEXT("Bust"))?TEXT("Bust"):TEXT("Accessories");
            float Length=0;
            for(int32 Joint=1;Joint<Spring.joints.Num();++Joint)Length+=Ref.GetRefBonePose()[Spring.joints[Joint].boneNo].GetTranslation().Size();
            if(Kind==TEXT("HairShort")&&Length>30)Kind=TEXT("HairLong");
            auto& Group=Groups.FindOrAdd(Kind);Group.Roots.Add(*Bone);++RootCount;
            for(const int32 GroupIndex:Spring.colliderGroups)
            {
                if(!Meta->VRM1SpringBoneMeta.ColliderGroups.IsValidIndex(GroupIndex))return TEXT("ERROR: Invalid collider group ")+ID;
                for(const int32 Collider:Meta->VRM1SpringBoneMeta.ColliderGroups[GroupIndex].colliders)
                {if(!Meta->VRM1SpringBoneMeta.Colliders.IsValidIndex(Collider))return TEXT("ERROR: Invalid collider ")+ID;Group.Colliders.Add(Collider);}
            }
        }
        TArray<UEdGraph*> Graphs;Blueprint->GetAllGraphs(Graphs);
        TArray<UEdGraphNode*> Old;TArray<UAnimGraphNode_KawaiiPhysics*> Kawaii;
        for(auto* Graph:Graphs)for(UEdGraphNode* Node:Graph->Nodes)
        {
            GraphLog+=ID+TEXT(": ")+Node->GetClass()->GetName()+TEXT("\n");
            if(Node->GetClass()->GetName()==TEXT("AnimGraphNode_VrmSpringBone"))Old.Add(Node);
            if(auto* Physics=Cast<UAnimGraphNode_KawaiiPhysics>(Node))Kawaii.Add(Physics);
        }
        if(bApply&&Old.Num()==1&&Kawaii.IsEmpty())
        {
            auto* Previous=Old[0];auto* Graph=Previous->GetGraph();
            auto* Input=Previous->FindPin(TEXT("ComponentPose"));auto* Output=Previous->FindPin(TEXT("Pose"));
            if(!Input||Input->LinkedTo.Num()!=1||!Output||Output->LinkedTo.IsEmpty())return TEXT("ERROR: Unsupported VRM graph ")+ID;
            UEdGraphPin* Upstream=Input->LinkedTo[0];const auto Downstream=Output->LinkedTo;
            const FString Package=Blueprint->GetOutermost()->GetName();
            const FString Source=FPackageName::LongPackageNameToFilename(Package,FPackageName::GetAssetPackageExtension());
            const FString Backup=Folder/TEXT("OriginalAssets")/(Package.RightChop(6)+TEXT(".uasset"));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup),true);
            if(!IFileManager::Get().FileExists(*Backup)&&IFileManager::Get().Copy(*Backup,*Source)!=COPY_OK)return TEXT("ERROR: Backup failed ")+ID;
            Blueprint->Modify();Graph->Modify();FBlueprintEditorUtils::RemoveNode(Blueprint,Previous,true);
            int32 Column=0;
            for(const FString Kind:{TEXT("HairLong"),TEXT("HairShort"),TEXT("Skirt"),TEXT("Sleeve"),TEXT("Bust"),TEXT("Accessories")})
            {
                const auto* Group=Groups.Find(Kind);if(!Group)continue;
                FGraphNodeCreator<UAnimGraphNode_KawaiiPhysics> Creator(*Graph);auto* Physics=Creator.CreateNode();
                ConfigureNode(Physics->Node,Kind,*Group,Meta->VRM1SpringBoneMeta);
                Physics->NodeComment=TEXT("IC Kawaii ")+Kind;Physics->bCommentBubbleVisible=true;
                Physics->NodePosX=Column++*360;Physics->NodePosY=0;Creator.Finalize();
                if(!Graph->GetSchema()->TryCreateConnection(Upstream,Physics->FindPinChecked(TEXT("ComponentPose"))))return TEXT("ERROR: Kawaii input link ")+ID;
                Upstream=Physics->FindPinChecked(TEXT("Pose"));Kawaii.Add(Physics);
            }
            for(auto* Pin:Downstream)if(!Graph->GetSchema()->TryCreateConnection(Upstream,Pin))return TEXT("ERROR: Kawaii output link ")+ID;
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
            if(Blueprint->Status==BS_Error)return TEXT("ERROR: Blueprint compile failed ")+ID;
            Blueprint->MarkPackageDirty();Packages.AddUnique(Blueprint->GetOutermost());Old.Reset();
        }
        if(bApply&&(Old.Num()!=0||Kawaii.IsEmpty()))return TEXT("ERROR: Physics graph not fully converted ")+ID;
        if(bApply)
        {
            for(auto* Physics:Kawaii)
            {
                const FString Kind=Physics->NodeComment.RightChop(10);
                const auto* Group=Groups.Find(Kind);if(!Group)return TEXT("ERROR: Unknown Kawaii group ")+ID;
                Physics->Modify();Physics->Node=FAnimNode_KawaiiPhysics();ConfigureNode(Physics->Node,Kind,*Group,Meta->VRM1SpringBoneMeta);
            }
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);FKismetEditorUtilities::CompileBlueprint(Blueprint);
            if(Blueprint->Status==BS_Error)return TEXT("ERROR: Kawaii tuning compile failed ")+ID;
            Blueprint->MarkPackageDirty();Packages.AddUnique(Blueprint->GetOutermost());
        }
        CSV+=FString::Printf(TEXT("%s,%d,%d,%d,%d\n"),*ID,RootCount,Kawaii.Num(),Old.Num(),bApply?1:0);
    }
    if(bApply&&!Packages.IsEmpty()&&!UEditorLoadingAndSavingUtils::SavePackages(Packages,true))return TEXT("ERROR: Could not save Kawaii graphs");
    const FString Path=Folder/(bApply?TEXT("SetupApplied.csv"):TEXT("SetupPlan.csv"));
    FFileHelper::SaveStringToFile(CSV,*Path);FFileHelper::SaveStringToFile(GraphLog,*(Folder/TEXT("GraphNodes.txt")));
    return Path;
#else
    return TEXT("ERROR: Editor only");
#endif
}

FString UICCharacterSetupLibrary::ConfigureImportedNPCPhysics(USkeletalMesh* Mesh, UObject* Metadata, bool bApply)
{
#if WITH_EDITOR
    auto* Source = Cast<UVrmMetaObject>(Metadata);
    if (!Mesh || !Source || Source->SkeletalMesh != Mesh) return TEXT("ERROR: VRM metadata does not belong to the selected mesh");
    auto* Class = Mesh->GetPostProcessAnimBlueprint().Get();
    auto* Blueprint = Class ? Cast<UAnimBlueprint>(Class->ClassGeneratedBy) : nullptr;
    if (!Blueprint || !Blueprint->GetOutermost()->GetName().StartsWith(TEXT("/Game/")))
        return TEXT("ERROR: Import the VRM with its Post Process Anim Blueprint enabled");
    const auto& Ref = Mesh->GetRefSkeleton();
    FVRM1SpringBoneMeta Meta = Source->VRM1SpringBoneMeta;
    // Normalize VRM 0.x metadata to the VRM 1 collider coordinate convention.
    // VRM4U uses (-X,Z,Y) for 0.x and (X,-Z,Y) for 1.x, in centimetres.
    if (Meta.Springs.IsEmpty() && !Source->VRMSpringMeta.IsEmpty())
    {
        for (const auto& Old : Source->VRMColliderMeta)
        {
            FVRM1SpringColliderGroups Group;
            for (const auto& Sphere : Old.collider)
            {
                FVRM1SpringCollider Collider;
                Collider.boneName = Old.boneName; Collider.boneNo = Ref.FindBoneIndex(*Old.boneName);
                Collider.shapeType = TEXT("sphere"); Collider.radius = Sphere.radius;
                Collider.offset = FVector(-Sphere.offset.X, Sphere.offset.Y, -Sphere.offset.Z);
                Group.colliders.Add(Meta.Colliders.Add(Collider));
            }
            Meta.ColliderGroups.Add(Group);
        }
        for (const auto& Old : Source->VRMSpringMeta) for (const FString& Root : Old.boneNames)
        {
            FVRM1SpringMeta Spring; FVRM1SpringJointMeta Joint;
            Joint.boneName = Root; Joint.boneNo = Ref.FindBoneIndex(*Root);
            Spring.joints.Add(Joint); Spring.colliderGroups = Old.ColliderIndexArray;
            Meta.Springs.Add(Spring);
        }
    }
    TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
    TArray<UEdGraphNode*> OldNodes; TArray<UAnimGraphNode_KawaiiPhysics*> Existing;
    for (auto* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node->GetClass()->GetName() == TEXT("AnimGraphNode_VrmSpringBone")) OldNodes.Add(Node);
        if (auto* Kawaii = Cast<UAnimGraphNode_KawaiiPhysics>(Node)) Existing.Add(Kawaii);
    }
    if (OldNodes.IsEmpty() && !Existing.IsEmpty())
        return FString::Printf(TEXT("OK: %d existing Kawaii nodes preserved"), Existing.Num());
    if (OldNodes.Num() != 1 || !Existing.IsEmpty()) return TEXT("ERROR: Expected one VRM SpringBone node and no mixed Kawaii simulation");
    if (Meta.Springs.IsEmpty()) return TEXT("ERROR: No VRM spring bones; author hair/cloth chains before automatic physics setup");
    auto Humanoid = [&](const FString& Key) -> FString
    { const FString* Value = Source->humanoidBoneTable.Find(Key); return Value ? *Value : FString(); };
    TSet<FName> BodyBones;
    for (const auto& Pair : Source->humanoidBoneTable) BodyBones.Add(*Pair.Value);
    TMap<FString, FICKawaiiGroup> Groups; TArray<int32> Roots;
    for (const auto& Spring : Meta.Springs)
    {
        if (Spring.joints.IsEmpty()) continue;
        const FString Root = Spring.joints[0].boneName; const int32 Bone = Ref.FindBoneIndex(*Root);
        if (Bone == INDEX_NONE || BodyBones.Contains(*Root)) return TEXT("ERROR: Invalid or humanoid spring root: ") + Root;
        if (Roots.Contains(Bone)) continue;
        Roots.Add(Bone);
    }
    // All descendants of a Kawaii root are simulated: overlapping roots would
    // evaluate the same bone twice and can destabilize imported hairstyles.
    for (int32 A : Roots) for (int32 B : Roots)
        if (A != B && Ref.BoneIsChildOf(A, B)) return TEXT("ERROR: Overlapping VRM spring roots require manual chain review");
    int32 RootCount = 0;
    for (const auto& Spring : Meta.Springs)
    {
        if (Spring.joints.IsEmpty()) continue;
        const FString Root = Spring.joints[0].boneName; const int32 Bone = Ref.FindBoneIndex(*Root);
        float Length = 0;
        for (int32 Child = Bone + 1; Child < Ref.GetNum(); ++Child) if (Ref.BoneIsChildOf(Child, Bone))
        {
            float Distance = 0;
            for (int32 Step = Child; Step != Bone; Step = Ref.GetParentIndex(Step)) Distance += Ref.GetRefBonePose()[Step].GetTranslation().Size();
            Length = FMath::Max(Length, Distance);
        }
        if (Length < KINDA_SMALL_NUMBER) continue;
        const FString Kind = Root.Contains(TEXT("Hair")) ? (Length > 30 ? TEXT("HairLong") : TEXT("HairShort"))
            : Root.Contains(TEXT("Skirt")) ? TEXT("Skirt") : Root.Contains(TEXT("Sleeve")) ? TEXT("Sleeve")
            : Root.Contains(TEXT("Bust")) ? TEXT("Bust") : TEXT("Accessories");
        auto& Group = Groups.FindOrAdd(Kind);
        if (Group.Roots.Contains(*Root)) continue;
        Group.Roots.Add(*Root); ++RootCount;
        for (const int32 GroupIndex : Spring.colliderGroups)
        {
            if (!Meta.ColliderGroups.IsValidIndex(GroupIndex)) return TEXT("ERROR: Invalid VRM collider group");
            for (const int32 Index : Meta.ColliderGroups[GroupIndex].colliders)
            {
                if (!Meta.Colliders.IsValidIndex(Index)) return TEXT("ERROR: Invalid VRM collider index");
                Group.Colliders.Add(Index);
            }
        }
    }
    if (Groups.IsEmpty()) return TEXT("ERROR: Spring bones have no child segments");
    for (const auto& Collider : Meta.Colliders)
        if (Ref.FindBoneIndex(*Collider.boneName) == INDEX_NONE || !FMath::IsFinite(Collider.radius) || Collider.radius < 0
            || Collider.offset.ContainsNaN() || Collider.tail.ContainsNaN()
            || (Collider.shapeType != TEXT("sphere") && Collider.shapeType != TEXT("capsule")))
            return TEXT("ERROR: Unsupported or invalid VRM body collider");
    int32 AddedColliders = 0;
    // Supply body contacts even when the imported garment omitted its links.
    // Build fallback capsules from the actual humanoid segment proportions.
    auto AddBodyContacts = [&](FICKawaiiGroup& Group, const FString& Key, const FString& EndKey, float Ratio)
    {
        const FString BoneName = Humanoid(Key); const int32 Bone = Ref.FindBoneIndex(*BoneName);
        if (BoneName.IsEmpty() || Bone == INDEX_NONE) return;
        bool Found = false;
        for (int32 Index = 0; Index < Meta.Colliders.Num(); ++Index) if (Meta.Colliders[Index].boneName == BoneName)
        { Group.Colliders.Add(Index); Found = true; }
        if (Found) return;
        const int32 End = Ref.FindBoneIndex(*Humanoid(EndKey));
        if (End == INDEX_NONE || Ref.GetParentIndex(End) != Bone) return;
        const FVector Segment = Ref.GetRefBonePose()[End].GetTranslation();
        const float Length = Segment.Size(); if (Length < 1) return;
        auto ToVRM = [](FVector Value) { return FVector(Value.X, Value.Z, -Value.Y) / 100.f; };
        FVRM1SpringCollider Collider;
        Collider.boneName = BoneName; Collider.boneNo = Bone; Collider.shapeType = TEXT("capsule");
        Collider.offset = ToVRM(Segment*.12f); Collider.tail = ToVRM(Segment*.78f);
        Collider.radius = FMath::Clamp(Length*Ratio, 1.5f, 9.f)/100.f;
        Group.Colliders.Add(Meta.Colliders.Add(Collider)); ++AddedColliders;
    };
    for (auto& Pair : Groups)
    {
        auto& Group = Pair.Value;
        if (Pair.Key.StartsWith(TEXT("Hair")))
        {
            AddBodyContacts(Group, TEXT("neck"), TEXT("head"), .35f);
            AddBodyContacts(Group, TEXT("chest"), TEXT("upperChest"), .65f);
            AddBodyContacts(Group, TEXT("spine"), TEXT("chest"), .65f);
            // Head is often a leaf in the humanoid hierarchy: use an existing
            // authored head collider rather than infer a radius from hair bones.
            for (int32 Index=0; Index<Meta.Colliders.Num(); ++Index)
                if (Meta.Colliders[Index].boneName == Humanoid(TEXT("head"))) Group.Colliders.Add(Index);
        }
        if (Pair.Key == TEXT("Skirt")) for (const FString Side : {TEXT("left"), TEXT("right")})
            AddBodyContacts(Group, Side+TEXT("UpperLeg"), Side+TEXT("LowerLeg"), .16f);
        if (Pair.Key == TEXT("Sleeve")) for (const FString Side : {TEXT("left"), TEXT("right")})
        {
            AddBodyContacts(Group, Side+TEXT("UpperArm"), Side+TEXT("LowerArm"), .18f);
            AddBodyContacts(Group, Side+TEXT("LowerArm"), Side+TEXT("Hand"), .16f);
        }
    }
    auto* Previous = OldNodes[0]; auto* Graph = Previous->GetGraph();
    auto* Input = Previous->FindPin(TEXT("ComponentPose")); auto* Output = Previous->FindPin(TEXT("Pose"));
    if (!Input || Input->LinkedTo.Num()!=1 || !Output || Output->LinkedTo.IsEmpty()) return TEXT("ERROR: Unsupported post-process graph links");
    const FString Summary = FString::Printf(TEXT("%d roots, %d Kawaii nodes, %d fallback body colliders"), RootCount, Groups.Num(), AddedColliders);
    if (!bApply) return TEXT("PLAN: ") + Summary;
    const FString Package = Blueprint->GetOutermost()->GetName();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
    const FString Backup = FPaths::ProjectSavedDir()/TEXT("NPCImport/OriginalAssets")/(Package.RightChop(6)+TEXT(".uasset"));
    if (IFileManager::Get().FileExists(*Filename) && !IFileManager::Get().FileExists(*Backup))
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
        if (IFileManager::Get().Copy(*Backup, *Filename) != COPY_OK) return TEXT("ERROR: Could not back up the imported graph");
    }
    UEdGraphPin* Upstream = Input->LinkedTo[0]; const auto Downstream = Output->LinkedTo;
    Blueprint->Modify(); Graph->Modify(); FBlueprintEditorUtils::RemoveNode(Blueprint, Previous, true);
    int32 Column = 0;
    for (const FString Kind : {TEXT("HairLong"),TEXT("HairShort"),TEXT("Skirt"),TEXT("Sleeve"),TEXT("Bust"),TEXT("Accessories")})
    {
        const auto* Group = Groups.Find(Kind); if (!Group) continue;
        FGraphNodeCreator<UAnimGraphNode_KawaiiPhysics> Creator(*Graph); auto* Node = Creator.CreateNode();
        ConfigureNode(Node->Node, Kind, *Group, Meta); Node->NodeComment = TEXT("IC Kawaii ")+Kind;
        Node->bCommentBubbleVisible=true; Node->NodePosX=Column++*360; Creator.Finalize();
        if (!Graph->GetSchema()->TryCreateConnection(Upstream, Node->FindPinChecked(TEXT("ComponentPose")))) return TEXT("ERROR: Could not connect Kawaii input");
        Upstream=Node->FindPinChecked(TEXT("Pose"));
    }
    for (auto* Pin : Downstream) if (!Graph->GetSchema()->TryCreateConnection(Upstream, Pin)) return TEXT("ERROR: Could not connect Kawaii output");
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return TEXT("ERROR: Imported post-process graph did not compile; original package is backed up");
    Blueprint->MarkPackageDirty(); TArray<UPackage*> Packages{Blueprint->GetOutermost()};
    if (!UEditorLoadingAndSavingUtils::SavePackages(Packages, true)) return TEXT("ERROR: Could not save the post-process graph");
    return TEXT("OK: ") + Summary;
#else
    return TEXT("ERROR: Editor only");
#endif
}
