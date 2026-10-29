#pragma once
#include "CoreMinimal.h"
#include "ReferenceSkeleton.h"
#include "Data/Rows/Combat/ICCombatTypes.h"

namespace ICHandGrip
{
struct FPalm
{
    FTransform Hand;
    FVector Forward,Normal,Width;
    bool bValid=false;
};
inline TArray<FTransform> ReferenceComponentPose(const FReferenceSkeleton& Skeleton)
{
    TArray<FTransform> Pose;
    for(int32 I=0;I<Skeleton.GetNum();++I)
    {
        auto Transform=Skeleton.GetRefBonePose()[I];const int32 Parent=Skeleton.GetParentIndex(I);
        if(Parent!=INDEX_NONE)Transform*=Pose[Parent];Pose.Add(Transform);
    }
    return Pose;
}
inline FPalm Palm(const FReferenceSkeleton& Skeleton,const TArray<FTransform>& Pose,bool Left)
{
    const FString Prefix=Left?TEXT("J_Bip_L_"):TEXT("J_Bip_R_");
    auto Index=[&](const TCHAR* Name){return Skeleton.FindBoneIndex(FName(*(Prefix+Name)));};
    const int32 Hand=Index(TEXT("Hand")),Middle=Index(TEXT("Middle1")),Finger=Index(TEXT("Index1")),Little=Index(TEXT("Little1")),Thumb=Index(TEXT("Thumb2"));
    if(Hand==INDEX_NONE||Middle==INDEX_NONE||Finger==INDEX_NONE||Little==INDEX_NONE)return {};
    FPalm Result;Result.Hand=Pose[Hand];
    Result.Forward=(Pose[Middle].GetLocation()-Pose[Hand].GetLocation()).GetSafeNormal();
    Result.Width=(Pose[Finger].GetLocation()-Pose[Little].GetLocation()).GetSafeNormal();
    Result.Normal=FVector::CrossProduct(Result.Width,Result.Forward).GetSafeNormal();
    const double ThumbSide=Thumb!=INDEX_NONE?FVector::DotProduct(Pose[Thumb].GetLocation()-Pose[Finger].GetLocation(),Result.Normal):0;
    if(FMath::Abs(ThumbSide)>.1){if(ThumbSide<0)Result.Normal*=-1;}
    else if(FVector::DotProduct(Result.Normal,FVector(0,0,-1))<0)Result.Normal*=-1;
    Result.bValid=true;return Result;
}
inline FName Socket(EICWeaponFamily Family)
{
    switch(Family)
    {
    case EICWeaponFamily::Sword:return TEXT("weapon_grip_sword");
    case EICWeaponFamily::Bow:return TEXT("weapon_grip_bow");
    case EICWeaponFamily::Staff:return TEXT("weapon_grip_staff");
    case EICWeaponFamily::Wand:return TEXT("weapon_grip_wand");
    default:return NAME_None;
    }
}
struct FFinger
{
    int32 MeshIndex=INDEX_NONE;
    FQuat Reference,Closed;
    bool Left=false;
};
inline TArray<FFinger> Fingers(const FReferenceSkeleton& Skeleton)
{
    TArray<FFinger> Result;const auto Pose=ReferenceComponentPose(Skeleton);
    for(const bool Left:{false,true})
    {
        const auto Basis=Palm(Skeleton,Pose,Left);if(!Basis.bValid)continue;
        const FString Prefix=Left?TEXT("J_Bip_L_"):TEXT("J_Bip_R_");
        for(const TCHAR* Name:{TEXT("Index"),TEXT("Middle"),TEXT("Ring"),TEXT("Little"),TEXT("Thumb")})
        for(int32 Joint=1;Joint<=3;++Joint)
        {
            const FName Bone(*(Prefix+Name+FString::FromInt(Joint)));
            const int32 Index=Skeleton.FindBoneIndex(Bone);if(Index==INDEX_NONE)continue;
            const int32 Child=Joint<3?Skeleton.FindBoneIndex(FName(*(Prefix+Name+FString::FromInt(Joint+1)))):INDEX_NONE;
            const int32 Parent=Skeleton.GetParentIndex(Index);
            const FVector Direction=Child!=INDEX_NONE?(Pose[Child].GetLocation()-Pose[Index].GetLocation()).GetSafeNormal():Parent!=INDEX_NONE?(Pose[Index].GetLocation()-Pose[Parent].GetLocation()).GetSafeNormal():Basis.Forward;
            const FVector Axis=Pose[Index].InverseTransformVectorNoScale(FVector::CrossProduct(Direction,Basis.Normal).GetSafeNormal());
            const bool Thumb=FCString::Strcmp(Name,TEXT("Thumb"))==0;
            const float Degrees=Thumb?(Joint==1?20.f:Joint==2?32.f:22.f):(Joint==1?58.f:Joint==2?72.f:42.f);
            const FQuat Reference=Skeleton.GetRefBonePose()[Index].GetRotation();
            Result.Add({Index,Reference,(Reference*FQuat(Axis,FMath::DegreesToRadians(Degrees))).GetNormalized(),Left});
        }
    }
    return Result;
}
}
