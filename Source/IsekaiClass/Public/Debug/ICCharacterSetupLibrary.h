#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "ICCharacterSetupLibrary.generated.h"
class UAnimSequence;
class UAnimMontage;
class UBlendSpace1D;
class UBlendSpace;
class USkeletalMesh;
class UWorld;
class UICCharacterAnimationProfile;
class AICEnemyBase;

UCLASS()
class ISEKAICLASS_API UICCharacterSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool SetUIReviewAnalog(UWorld* World,FKey Key,float Value);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareCharacterMesh(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintPure, Category="IC|Editor") static TArray<FName> GetCharacterBones(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UBlendSpace1D* CreateLocomotion(const FString& Path, UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Run);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UBlendSpace* CreateCombatLocomotion(const FString& Path,UAnimSequence* Idle,const TArray<UAnimSequence*>& Directions);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UAnimMontage* CreateActionMontage(const FString& Path, UAnimSequence* Sequence, bool bCombatNotify);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UAnimMontage* CreateNormalAttackMontage(const FString& Path,UAnimSequence* Sequence,float HitTime,bool bProjectile);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UAnimMontage* CreateWeaponTransitionMontage(const FString& Path, UAnimSequence* Sequence, bool bDraw, float VisibilityFraction=0.55f);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareTestNavigation(UWorld* World);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString CaptureUIAspectSamples();
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString InspectCharacterPresentation(bool bApplyCalibration=false);
    /** Exercise secondary motion, pose resets and local bone lengths without saving assets. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool ReviewSecondaryMotion(const FString& Label, bool bCapture=false);
    /** Check that rear hair falls under gravity even at sustained 8 fps. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool ReviewHairDrape(const FString& Label);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString ConfigureKawaiiSecondaryMotion(bool bApply=false);
    /** Configure one newly imported VRM without relying on R01 asset names. Existing Kawaii tuning is preserved. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString ConfigureImportedNPCPhysics(USkeletalMesh* Mesh, UObject* Metadata, bool bApply=false);
    /** Resolve humanoid hand bones from VRM metadata and create weapon grip sockets. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareImportedNPCMesh(USkeletalMesh* Mesh, UObject* Metadata);
    /** Flat-ground calibration only; does not change sleep poses or beds. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool CalibrateImportedNPC(USkeletalMesh* Mesh, UICCharacterAnimationProfile* Profile);
    /** Start a PIE-only visual review with an isolated save namespace. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareLiveCharacterReview(UWorld* World);
    /** Spawn a normal monster at an isolated PIE review origin without saving map assets. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor",meta=(DevelopmentOnly)) static AICEnemyBase* SpawnLiveReviewEnemy(UWorld* World,FVector Origin);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool BeginMonsterTestReview();
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool CaptureMonsterTestHUD(UWorld* World,const FString& File);
    /** Set the outer PIE window size so visual comparisons use identical pixels. */
    UFUNCTION(BlueprintCallable, Category="IC|Editor",meta=(DevelopmentOnly)) static bool SetUIReviewWindowSize(UWorld* World,int32 Width,int32 Height);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PressUIReviewKey(UWorld* World,FKey Key);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool SetUIReviewKey(UWorld* World,FKey Key,bool Pressed);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static void CaptureLiveCharacterReview(UWorld* World, const FString& Name);
};
