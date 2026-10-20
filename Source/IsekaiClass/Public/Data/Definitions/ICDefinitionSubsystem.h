#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Definitions/ICDefinitionCatalog.h"
#include "Data/Rows/Items/ICItemRows.h"
#include "Data/Rows/Life/ICRecipeRow.h"
#include "Data/Rows/Life/ICProductionRow.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/Combat/ICSkillRows.h"
#include "Data/Rows/Quest/ICQuestRow.h"
#include "Data/Rows/Settings/ICSettingsRows.h"
#include "Data/Rows/Character/ICCharacterRows.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Data/Rows/Combat/ICStatusEffectRow.h"
#include "Data/Rows/Combat/ICSynergyRow.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Data/Rows/Life/ICCropRow.h"
#include "Data/Rows/Life/ICFishRow.h"
#include "Data/Rows/Life/ICFishingPointRow.h"
#include "Data/Rows/Life/ICResourceRow.h"
#include "Data/Rows/NPC/ICDialogueRow.h"
#include "Data/Rows/NPC/ICGiftPreferenceRow.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "Data/Rows/NPC/ICShopRow.h"
#include "Data/Rows/Presentation/ICCodexRow.h"
#include "Data/Rows/Presentation/ICHintRow.h"
#include "Data/Rows/Presentation/ICResourceReferenceRow.h"
#include "Data/Rows/Quest/ICEndingRow.h"
#include "Data/Rows/Quest/ICProgressionRow.h"
#include "Data/Rows/Rewards/ICRewardRows.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "Data/Rows/World/ICHomeRow.h"
#include "Data/Rows/World/ICLandmarkRow.h"
#include "Data/Rows/World/ICMapRow.h"
#include "Data/Rows/World/ICRegionRow.h"
#include "Data/Rows/World/ICSpawnerRow.h"
#include "Data/Rows/World/ICWarpRow.h"
#include "ICDefinitionSubsystem.generated.h"

UENUM(BlueprintType)
enum class EICDefinitionFailure : uint8
{
    None , InvalidID , CatalogUnavailable , TableUnregistered , TableLoadFailed , WrongRowStruct ,
    RowMissing , DuplicateID , IDMismatch , Deprecated , TestDefinition , InvalidDefinition
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) EICDefinitionFailure Code = EICDefinitionFailure::RowMissing;
    UPROPERTY(BlueprintReadOnly) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(BlueprintReadOnly) FName ID;
    UPROPERTY(BlueprintReadOnly) FString Table;
    UPROPERTY(BlueprintReadOnly) FName Row;
    UPROPERTY(BlueprintReadOnly) FString Field;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

UCLASS()
class ISEKAICLASS_API UICDefinitionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void ConfigureCatalog(UICDefinitionCatalog* InCatalog);
    const FICDefinitionRow* FindRow(FName ID, const UScriptStruct* ExpectedStruct, FICDefinitionResult& Result) const;
    template<class T> const T* Find(FName ID, FICDefinitionResult& Result) const
    { IC_TRACE_FUNCTION(); return static_cast<const T*>(FindRow(ID, T::StaticStruct(), Result)); }
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetDefinitionIDs(EICDefinitionType Type, TArray<FName>& IDs) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetItem(FName ID, FICItemRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetRecipe(FName ID, FICRecipeRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetProduction(FName ID, FICProductionRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetNPC(FName ID, FICNPCRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSkill(FName ID, FICSkillRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetQuest(FName ID, FICQuestRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetEquipment(FName ID, FICEquipmentRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetNewGame(FName ID, FICNewGameRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetClass(FName ID, FICClassRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetPlayerPreset(FName ID, FICPlayerPresetRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetRecruitCombat(FName ID, FICRecruitCombatRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetBossPattern(FName ID, FICBossPatternRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetMonster(FName ID, FICMonsterRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSkillEffect(FName ID, FICSkillEffectRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetStatusEffect(FName ID, FICStatusEffectRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSynergy(FName ID, FICSynergyRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetEnhancement(FName ID, FICEnhancementRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetOption(FName ID, FICOptionRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetOptionPool(FName ID, FICOptionPoolRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetCrop(FName ID, FICCropRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetFish(FName ID, FICFishRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetFishingPoint(FName ID, FICFishingPointRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetResource(FName ID, FICResourceRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetDialogue(FName ID, FICDialogueRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetGiftPreference(FName ID, FICGiftPreferenceRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSchedule(FName ID, FICScheduleRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetService(FName ID, FICServiceRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetShop(FName ID, FICShopRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetCodex(FName ID, FICCodexRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetHint(FName ID, FICHintRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetResourceReference(FName ID, FICResourceReferenceRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetEnding(FName ID, FICEndingRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetProgression(FName ID, FICProgressionRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetIDRegistry(FName ID, FICIDRegistryRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetDropTable(FName ID, FICDropTableRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetReward(FName ID, FICRewardRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetCombatSettings(FName ID, FICCombatSettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetInventorySettings(FName ID, FICInventorySettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetMapSettings(FName ID, FICMapSettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetPartySettings(FName ID, FICPartySettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSaveSettings(FName ID, FICSaveSettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetStatSettings(FName ID, FICStatSettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetTimeSettings(FName ID, FICTimeSettingsRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetAnchor(FName ID, FICAnchorRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetHome(FName ID, FICHomeRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetLandmark(FName ID, FICLandmarkRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetMap(FName ID, FICMapRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetRegion(FName ID, FICRegionRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetSpawner(FName ID, FICSpawnerRow& Definition) const;
    UFUNCTION(BlueprintCallable, Category="IC|Definitions") FICDefinitionResult GetWarp(FName ID, FICWarpRow& Definition) const;
private:
    UPROPERTY() TObjectPtr<UICDefinitionCatalog> Catalog;
    UPROPERTY(Transient) TArray<TObjectPtr<UDataTable>> LoadedTables;
    UDataTable* LoadTable(const FICDefinitionBinding& Binding, FICDefinitionResult& Result) const;
};
