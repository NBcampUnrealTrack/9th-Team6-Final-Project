#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ICCombatVFXSetupLibrary.generated.h"
class UNiagaraSystem;
class UNiagaraComponent;

/** Editor authoring for the editable combat effects; no editor code is cooked. */
UCLASS()
class ISEKAICLASS_API UICCombatVFXSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="IC|Editor")
    static UNiagaraSystem* CreateCombatNiagara(const FString& Path,FName Shape);
    UFUNCTION(BlueprintCallable,Category="IC|Editor")
    static UNiagaraSystem* DuplicateCombatNiagara(const FString& Path,UNiagaraSystem* Source);
    UFUNCTION(BlueprintPure,Category="IC|Editor")
    static bool IsCombatNiagaraReady(UNiagaraSystem* System);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool CompileCombatNiagara(UNiagaraSystem* System);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool RepairCombatNiagaraColor(UNiagaraSystem* System);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool DumpCombatNiagara(UNiagaraSystem* System);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static int32 GetCombatParticleCount(UNiagaraComponent* Component);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static FString DescribeCombatParticles(UNiagaraComponent* Component);
};
