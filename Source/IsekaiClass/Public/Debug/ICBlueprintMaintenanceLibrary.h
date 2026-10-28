#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ICBlueprintMaintenanceLibrary.generated.h"

class UBlueprint;
class UStringTable;
class UFont;
class AICLifeObjectActor;
class USmartObjectDefinition;

UCLASS()
class ISEKAICLASS_API UICBlueprintMaintenanceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static int32 ClearBlueprintDocumentation(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool BuildFrontendWidget(UBlueprint* Blueprint,FName Kind);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool AddDialogueAffinityHearts(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool AddCombatFeedbackHUD(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool AddMovementCooldownHUD(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool AddGameClockHUD(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable,Category="IC|Editor") static bool SetLifeObjectDefinition(AICLifeObjectActor* Object,USmartObjectDefinition* Definition);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool ImportUIStrings(UStringTable* Table,const FString& File);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static UFont* CreateFrontendFont(const FString& RegularFile,const FString& BoldFile);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool EnsureFrontendFontFallback();
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static TArray<FName> GetFrontendWidgetNames(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="IC|Editor") static bool RenderFrontendPreview(UBlueprint* Blueprint,const FString& File,int32 Width,int32 Height,int32 Question=-1);
};
