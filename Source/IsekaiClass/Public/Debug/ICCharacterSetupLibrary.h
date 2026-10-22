#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ICCharacterSetupLibrary.generated.h"
class UAnimSequence;
class UAnimMontage;
class UBlendSpace1D;
class USkeletalMesh;
class UWorld;

UCLASS()
class ISEKAICLASS_API UICCharacterSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareCharacterMesh(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintPure, Category="IC|Editor") static TArray<FName> GetCharacterBones(USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UBlendSpace1D* CreateLocomotion(const FString& Path, UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Run);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UAnimMontage* CreateActionMontage(const FString& Path, UAnimSequence* Sequence, bool bCombatNotify);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool PrepareTestNavigation(UWorld* World);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString CaptureUIAspectSamples();
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static FString InspectCharacterPresentation(bool bApplyCalibration=false);
};
