#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
class UNiagaraComponent;
class USceneComponent;
namespace ICCombatVFX
{
    ISEKAICLASS_API const FICCombatVFXRow* Resolve(const FICCombatPresentation& Presentation);
    ISEKAICLASS_API UNiagaraComponent* Attach(AActor* Owner,USceneComponent* Parent,const FICNiagaraEffect& Style,
        float DefaultDuration=0,float Radius=220,float Angle=90,float Floor=-83,float Slope=.22f);
    ISEKAICLASS_API void Spawn(UWorld* World,const FTransform& Transform,const FICNiagaraEffect& Style,float DefaultDuration=.5f);
}
