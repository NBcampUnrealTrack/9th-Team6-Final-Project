#pragma once
#include "CoreMinimal.h"
struct FICWorldState;
struct FICCharacterState;
struct FICSynergyRow;
class UICDefinitionSubsystem;
class UICAbilitySystemComponent;
namespace ICPartyStatus
{
    ISEKAICLASS_API TArray<const FICSynergyRow*> ActiveSynergies(const FICWorldState&,UICDefinitionSubsystem*,const UICAbilitySystemComponent* Recipient=nullptr);
    ISEKAICLASS_API uint32 CompositionKey(const FICWorldState&);
    ISEKAICLASS_API bool HasBuff(const FICCharacterState&,const FICWorldState&,UICDefinitionSubsystem*);
    ISEKAICLASS_API FText Describe(const FICCharacterState&,const FICWorldState&,UICDefinitionSubsystem*,double PlaySecond,const UICAbilitySystemComponent* Recipient=nullptr);
}
