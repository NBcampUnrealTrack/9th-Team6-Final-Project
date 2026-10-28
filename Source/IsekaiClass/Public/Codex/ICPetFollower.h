#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICPetFollower.generated.h"

class AICCharacterBase;
class USkeletalMeshComponent;
class UAnimSequence;
struct FICMonsterRow;

/** Cosmetic companion built from the equipped pet's existing monster presentation. */
UCLASS(NotBlueprintable, Transient)
class ISEKAICLASS_API AICPetFollower : public AActor
{
    GENERATED_BODY()
public:
    AICPetFollower();
    bool InitializePet(AICCharacterBase* Character,const FICMonsterRow& Definition);
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintPure,Category="Pet") AICCharacterBase* GetFollowedCharacter() const {return FollowedCharacter.Get();}
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Pet") TObjectPtr<USkeletalMeshComponent> Visual;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Pet") int32 PetIndex=INDEX_NONE;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Pet") FName CharacterID;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Pet|Movement") bool bMoving=false;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Pet|Movement") int32 AnimationSwitchCount=0;
private:
    TWeakObjectPtr<AICCharacterBase> FollowedCharacter;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> MoveAnimation;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentAnimation;
    float IdleSeconds=0;
    FVector Destination() const;
    void PlayAnimation(bool Moving);
};
