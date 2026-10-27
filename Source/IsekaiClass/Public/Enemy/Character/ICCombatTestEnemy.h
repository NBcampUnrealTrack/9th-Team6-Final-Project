#pragma once
#include "Enemy/Character/ICEnemyBase.h"
#include "ICCombatTestEnemy.generated.h"

/** Visible target for the test map; combat and detection use the normal enemy AI. */
UCLASS()
class ISEKAICLASS_API AICCombatTestEnemy : public AICEnemyBase
{
    GENERATED_BODY()
public:
    AICCombatTestEnemy();
};
