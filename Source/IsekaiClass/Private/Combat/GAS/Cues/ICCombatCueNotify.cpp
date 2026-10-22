#include "Combat/GAS/Cues/ICCombatCueNotify.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool UICCombatCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    const auto* Payload = Cast<UICCombatCuePayload>(Parameters.SourceObject.Get());
    if (!Payload || !MyTarget) return false;
    const FVector Location = Parameters.Location.IsNearlyZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
    const auto& ChosenVisual = Payload->bImpact ? Payload->Presentation.ImpactVFX : Payload->Presentation.CastVFX;
    const auto& ChosenSound = Payload->bImpact ? Payload->Presentation.ImpactSound : Payload->Presentation.CastSound;
    const auto& Visual = ChosenVisual.IsNull() ? DefaultVFX : ChosenVisual;
    const auto& Sound = ChosenSound.IsNull() ? DefaultSound : ChosenSound;
    if (auto* System = Visual.LoadSynchronous()) UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, System, Location);
    if (auto* Audio = Sound.LoadSynchronous()) UGameplayStatics::PlaySoundAtLocation(MyTarget, Audio, Location);
    return true;
}
