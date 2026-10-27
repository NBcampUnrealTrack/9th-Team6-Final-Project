#include "Combat/GAS/Cues/ICCombatCueNotify.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Combat/VFX/ICCombatVFX.h"

bool UICCombatCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    const auto* Payload = Cast<UICCombatCuePayload>(Parameters.SourceObject.Get());
    if (!Payload || !MyTarget) return false;
    const FVector Location = Parameters.Location.IsNearlyZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
    const auto& ChosenVisual = Payload->bImpact ? Payload->Presentation.ImpactVFX : Payload->Presentation.CastVFX;
    const auto& ChosenSound = Payload->bImpact ? Payload->Presentation.ImpactSound : Payload->Presentation.CastSound;
    const auto& Visual = ChosenVisual.IsNull() ? DefaultVFX : ChosenVisual;
    const auto& Sound = ChosenSound.IsNull() ? DefaultSound : ChosenSound;
    if(const auto* Config=ICCombatVFX::Resolve(Payload->Presentation))
        ICCombatVFX::Spawn(MyTarget->GetWorld(),FTransform(MyTarget->GetActorRotation(),Location),Payload->bImpact?Config->Impact:Config->Cast,.5f);
    else if (auto* System = Visual.LoadSynchronous()) UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, System, Location);
    if (auto* Audio = Sound.LoadSynchronous()) UGameplayStatics::PlaySoundAtLocation(MyTarget, Audio, Location);
    return true;
}
