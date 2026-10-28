#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCombatFeedbackWidget.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API UICDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Damage") bool bPlayerDamage=false;
    UFUNCTION(BlueprintCallable, Category="IC|Damage") void SetDamage(float Value);
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICMonsterHealthWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Monster") void SetEnemy(class AICEnemyBase* Value);
    UFUNCTION(BlueprintCallable, Category="IC|Monster") void RefreshHealth();
protected:
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
    TWeakObjectPtr<class AICEnemyBase> Enemy;
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICRewardNoticeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Reward") void SetReward(int32 Gold,const TArray<FICItemAmount>& Items,FName MonsterID=NAME_None,int32 Essence=0);
};

USTRUCT()
struct FICCombatFloatingEntry
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UUserWidget> Widget;
    FVector WorldPosition=FVector::ZeroVector;
    float Age=0;
    bool bReward=false;
};

/** A single HUD overlay owns damage numbers and the small reward feed. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICCombatFeedbackWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UICCombatFeedbackWidget(const FObjectInitializer& Initializer);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|UI") TSoftClassPtr<UICDamageNumberWidget> PlayerDamageClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|UI") TSoftClassPtr<UICDamageNumberWidget> MonsterDamageClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|UI") TSoftClassPtr<UICRewardNoticeWidget> RewardNoticeClass;
    UFUNCTION() void ShowDamage(AActor* Victim,float Amount);
    UFUNCTION(BlueprintCallable, Category="IC|Reward") void ShowReward(int32 Gold,const TArray<FICItemAmount>& Items,FName MonsterID,int32 Essence);
    UFUNCTION() void ShowItems(const TArray<FICItemAmount>& Items);
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
    UPROPERTY(Transient) TObjectPtr<class UICWorldStateSubsystem> BoundState;
    UPROPERTY(Transient) TArray<FICCombatFloatingEntry> FloatingEntries;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> LockMarker;
    UFUNCTION() void ClearEntries();
};
