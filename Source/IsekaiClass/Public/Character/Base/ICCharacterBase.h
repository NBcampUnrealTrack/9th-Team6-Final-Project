#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "World/State/ICStateParticipant.h"
#include "ICCharacterBase.generated.h"
class UICAbilitySystemComponent;
class UICCharacterAnimationProfile;
class AICWeaponActor;
class UAnimMontage;
class USkeletalMesh;

UCLASS(Abstract)
class ISEKAICLASS_API AICCharacterBase : public ACharacter, public IAbilitySystemInterface, public IICStateParticipant
{
    GENERATED_BODY()
public:
    AICCharacterBase();
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Presentation") TSoftObjectPtr<UICCharacterAnimationProfile> AnimationProfile;
    UPROPERTY(Transient, BlueprintReadOnly, Category="IC|Presentation") TObjectPtr<AICWeaponActor> EquippedWeaponActor;
    UFUNCTION(BlueprintCallable, Category="IC|Presentation") void RefreshWeaponAppearance();
    UAnimMontage* ResolveActionMontage(FName Action);
    UICCharacterAnimationProfile* GetAnimationProfile();
    void UpdatePresentationTransform(float GroundOffsetCm, const FTransform* BedAnchor);
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    UICAbilitySystemComponent* GetICAbilitySystem() const;
    void EnableCombat(bool PrimaryStats, bool PlayerSP);
    void ReleaseForLoad();
    virtual bool InitializeState(FName ID);
    virtual bool ReadLiveState_Implementation(FName CharacterId, FICCharacterState& Snapshot) const override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* WorldState) override;
    virtual bool CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
    virtual void HandleZeroHP();
    virtual bool IsHostileTo(const AICCharacterBase* Other) const;
    UFUNCTION(BlueprintPure, Category="IC|Character") FName GetCharacterID() const;
    UFUNCTION(BlueprintNativeEvent, Category="IC|Combat") void OnDefeated();
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|GAS") TObjectPtr<UICAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IC|State") FName CharacterID;
    bool bRegisteredState = false;
    bool bSuppressPersistence = false;
    bool bZeroHPHandled = false;
    bool bUsePrimaryStats = false, bUsePlayerSP = false;
    UICWorldStateSubsystem* WorldState() const;
    UFUNCTION() virtual void StoredCharacterChanged(FName ID);
private:
    FName VisualEquipmentID;
    UPROPERTY(Transient) TObjectPtr<UICCharacterAnimationProfile> LoadedAnimationProfile;
    TWeakObjectPtr<USkeletalMesh> PresentationMesh;
    FTransform StandingMeshTransform;
    bool bSleepPresentation = false;
};
