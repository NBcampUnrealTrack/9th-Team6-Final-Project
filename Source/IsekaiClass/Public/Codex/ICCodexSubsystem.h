#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCodexSubsystem.generated.h"

class UICDefinitionSubsystem;
class UTexture2D;
struct FICMonsterRow;
struct FICSkillRow;
struct FICRecipeRow;

UCLASS()
class ISEKAICLASS_API UICCodexSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static constexpr int32 PetCount=55;
    static constexpr int32 EssenceRequired=100;
    UFUNCTION(BlueprintPure,Category="Codex") int32 GetPetEssence(int32 Index) const;
    UFUNCTION(BlueprintPure,Category="Codex") bool IsPetUnlocked(int32 Index) const;
    UFUNCTION(BlueprintCallable,Category="Codex") bool AddPetEssence(int32 Index,int32 Amount);
    UFUNCTION(BlueprintCallable,Category="Codex") bool EquipPet(int32 Index,FName CharacterID);
    UFUNCTION(BlueprintPure,Category="Codex") int32 GetEquippedPet(FName CharacterID) const;
    UFUNCTION(BlueprintPure,Category="Codex") TArray<FName> GetPartyMembers() const;
    UFUNCTION(BlueprintPure,Category="Codex") UTexture2D* GetEquippedPetPortrait(FName CharacterID) const;
    UFUNCTION(BlueprintPure,Category="Codex") bool HasAcquiredItem(FName ItemID) const;
    UFUNCTION(BlueprintPure,Category="Codex") bool IsRecipeKnown(FName RecipeID) const;
    UFUNCTION(BlueprintCallable,Category="Codex") bool SetPlayerSkill(FName SkillID,FText& Error);
    UFUNCTION(BlueprintCallable,Category="Codex") bool UnsetPlayerSkill(FName SkillID);
    UFUNCTION(BlueprintPure,Category="Codex") bool IsPlayerSkillEquipped(FName SkillID) const;
    UFUNCTION(BlueprintPure,Category="Codex") bool IsSkillUnlocked(FName SkillID) const;
    const FICMonsterRow* FindPet(int32 Index) const;
    TArray<const FICMonsterRow*> PetDefinitions() const;
    FText PetName(int32 Index) const;
    FText PetBuffText(int32 Index) const;
    FText CharacterName(FName ID) const;
    UTexture2D* CharacterPortrait(FName ID) const;
    FText ItemName(FName ID) const;
    FText DefinitionText(const FString& Key,FName Fallback=NAME_None) const;
    EICWeaponFamily SkillFamily(const FICSkillRow& Skill) const;
    TArray<FName> SkillCells() const;
    TArray<FName> EquippedSkills() const;
    TArray<FName> ProductItems(int32 Category) const;
    TArray<FName> Recipes(int32 Category) const;
    FText SkillDescription(FName ID) const;
    FText RecipeDescription(FName ID) const;
private:
    UICDefinitionSubsystem* Definitions() const;
};
