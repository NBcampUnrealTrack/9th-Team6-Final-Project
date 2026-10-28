#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ICDefinitionTypes.generated.h"

UENUM(BlueprintType)
enum class EICDefinitionType : uint8
{
    Settings , PlayerPreset , Class , NPC , RecruitCombat , Skill , SkillEffect , StatusEffect ,
    Synergy , Monster , BossPattern , Region , Anchor , Spawner , Warp , Home , Item , Equipment ,
    Enhancement , Option , OptionPool , Reward , DropTable , Schedule , GiftPreference ,
    Dialogue , Service , Shop , Recipe , Production , Resource , Crop , Fish , FishingPoint ,
    Quest , Progression , Ending , Codex , Landmark , Map , Hint , ResourceReference , IDRegistry , Ranching , GachaMachine
};

UENUM(BlueprintType)
enum class EICStat : uint8
{
    None , STR , DEX , INT , CON , CHA UMETA(Hidden), LUK , MaxHP , MaxMP , MaxSP , PhysicalAttack ,
    MagicAttack , Defense , AttackSpeed , MoveSpeed , CriticalChance , CriticalMultiplier , AutoDodgeChance
};

UENUM(BlueprintType)
enum class EICCharacterClass : uint8 { None , Warrior , Archer , Mage , Healer  };
UENUM(BlueprintType)
enum class EICWeaponFamily : uint8 { None , Sword , Bow , Staff , Wand  };
UENUM(BlueprintType)
enum class EICNPCType : uint8 { Recruitable , Resident , Background  };
UENUM(BlueprintType)
enum class EICClock : uint8 { GameClock , PlayClock  };
UENUM(BlueprintType)
enum class EICComparison : uint8 { Equal , NotEqual , Greater , GreaterOrEqual , Less , LessOrEqual  };
UENUM(BlueprintType)
enum class EICConditionPolicy : uint8 { All , Any  };
UENUM(BlueprintType)
enum class EICConditionType : uint8
{
    None , QuestAccepted , QuestCompleted , ItemInBag , Stat , Affinity , VisitedAnchor ,
    BossDefeated , WarpUnlocked , RecipeUnlocked , Flag , HasGameplayTag , CurrentHPRatio ,
    CurrentMPRatio , InCombat , TargetExists , HasStatusEffect , CharacterMode
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionRef
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") FName ID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBaseStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 STR = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 DEX = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 INT = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 CON = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 CHA = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000")) int32 LUK = 1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICItemAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 Count = 1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICStatModifier
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double Flat = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) double Percent = 0;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCondition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICConditionType Type = EICConditionType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FICDefinitionRef Target;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICComparison Compare = EICComparison::GreaterOrEqual;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double Value = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bUseEffectiveStats = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowCurrentState = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bNegate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Flag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICConditionGroup
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICConditionPolicy Policy = EICConditionPolicy::All;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICCondition> Conditions;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") FString NameKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") FString DescriptionKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") FGameplayTagContainer Tags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ClampMin="1")) int32 DataVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") bool bDeprecated = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition") bool bTestOnly = false;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICIDRegistryRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICDefinitionType DefinitionType = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ReplacementID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UDataTable> DefinitionTable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Notes;
};
