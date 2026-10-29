#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "ICGameFlowSubsystem.generated.h"

class AICPlayerController;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "IsekaiClass Game Flow"))
class ISEKAICLASS_API UICGameFlowSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category = "Maps")
    TSoftObjectPtr<UWorld> TitleMap =
        TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/IsekaiClass/World/Maps/L_Title.L_Title")));
    UPROPERTY(Config, EditAnywhere, Category = "Maps")
    TSoftObjectPtr<UWorld> PlayMap =
        TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/IsekaiClass/World/L_TestMap.L_TestMap")));
    UPROPERTY(Config, EditAnywhere, Category = "Creation")
    TSoftObjectPtr<UDataTable> CreationOptions = TSoftObjectPtr<UDataTable>(
        FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CreationOptions.DT_IC_CreationOptions")));
};

UENUM(BlueprintType)
enum class EICGameFlowPhase : uint8
{
    Title,
    Questions,
    Loading,
    Initializing,
    Playing,
    Failed,
    Prologue
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCreationAnswers
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Gender = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Story = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Music = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Happiness = INDEX_NONE;
    bool IsComplete() const;
    static bool IsNameValid(const FString& Value);
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCreationOptionRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Question = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Option = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ResultText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SkillID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName WeaponItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RecipeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CodexID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTestReward = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICGameFlowChanged);

UCLASS()
class ISEKAICLASS_API UICGameFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) EICGameFlowPhase Phase = EICGameFlowPhase::Title;
    UPROPERTY(BlueprintReadOnly) FICCreationAnswers Answers;
    UPROPERTY(BlueprintReadOnly) FString LastError;
    UPROPERTY(BlueprintReadOnly) bool bPendingNewGame = false;
    UPROPERTY(BlueprintAssignable) FICGameFlowChanged OnChanged;
    UFUNCTION(BlueprintCallable) void BeginQuestions();
    UFUNCTION(BlueprintCallable) void SetAnswers(const FICCreationAnswers& Value);
    UFUNCTION(BlueprintCallable) bool BeginNewGame(AICPlayerController* Controller);
    UFUNCTION(BlueprintCallable) void ReturnToTitle(AICPlayerController* Controller);
    UFUNCTION(BlueprintPure) FICCreationOptionRow GetOption(int32 Question, int32 Option) const;
    const FICCreationOptionRow* FindOption(int32 Question, int32 Option) const;
    void BeginInitialization(bool bRetry = false);
    void CompleteNewGame();
    void Fail(const FString& Reason);

private:
    UPROPERTY(Transient) TObjectPtr<UDataTable> Options;
    UDataTable* GetOptions() const;
};
