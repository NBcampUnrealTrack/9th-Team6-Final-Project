#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ICGameplayEffects.generated.h"

UCLASS()
class ISEKAICLASS_API UICHealthEffect : public UGameplayEffect { GENERATED_BODY() public: UICHealthEffect(); };
UCLASS()
class ISEKAICLASS_API UICManaEffect : public UGameplayEffect { GENERATED_BODY() public: UICManaEffect(); };
UCLASS()
class ISEKAICLASS_API UICSPChangeEffect : public UGameplayEffect { GENERATED_BODY() public: UICSPChangeEffect(); };
UCLASS()
class ISEKAICLASS_API UICStatusGameplayEffect : public UGameplayEffect { GENERATED_BODY() public: UICStatusGameplayEffect(); };
UCLASS()
class ISEKAICLASS_API UICSkillCooldownEffect : public UGameplayEffect { GENERATED_BODY() public: UICSkillCooldownEffect(); };
