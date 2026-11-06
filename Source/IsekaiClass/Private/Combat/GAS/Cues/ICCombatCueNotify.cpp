#include "Combat/GAS/Cues/ICCombatCueNotify.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Combat/VFX/ICCombatVFX.h"
#include "Audio/ICBGMSubsystem.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

bool UICCombatCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    const auto* Payload = Cast<UICCombatCuePayload>(Parameters.SourceObject.Get());
    if (!Payload || !MyTarget||!UICBGMSubsystem::CanPlayEffects(MyTarget->GetWorld())) return false;
    const FVector Location = Parameters.Location.IsNearlyZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
    const auto& ChosenVisual = Payload->bImpact ? Payload->Presentation.ImpactVFX : Payload->Presentation.CastVFX;
    const auto& ChosenSound = Payload->bImpact ? Payload->Presentation.ImpactSound : Payload->Presentation.CastSound;
    const auto Visual = ChosenVisual.IsNull() && !Payload->Presentation.bSuppressDefaultVFX ? DefaultVFX : ChosenVisual;
    const auto& Sound = ChosenSound.IsNull() ? DefaultSound : ChosenSound;
    if(const auto* Config=ICCombatVFX::Resolve(Payload->Presentation))
    {
        auto* Follow=Payload->bImpact&&Config->bImpactFollowsTarget?MyTarget->GetRootComponent():nullptr;
        const FTransform Transform(MyTarget->GetActorRotation(),Location);
        ICCombatVFX::Spawn(MyTarget->GetWorld(),Transform,Payload->bImpact?Config->Impact:Config->Cast,.5f,Follow);
        const auto& Layers=Payload->bImpact?Config->ImpactLayers:Config->CastLayers;
        for(const auto& Layer:Layers)ICCombatVFX::Spawn(MyTarget->GetWorld(),Transform,Layer,.5f,Follow);
    }
    else if (auto* System = Visual.LoadSynchronous())
    {
        auto* Component=UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget,System,Location);
        if (Component && Payload->Presentation.MaxVFXLifetimeSeconds>0)
        {
            TWeakObjectPtr<UNiagaraComponent> WeakComponent=Component;
            FTimerHandle Handle;
            MyTarget->GetWorld()->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateLambda([WeakComponent]()
            {
                // DeactivateImmediate may auto-destroy and invalidate a weak pointer.
                // Destroy once using a single validated lookup instead.
                if (UNiagaraComponent* LiveComponent=WeakComponent.Get()) LiveComponent->DestroyComponent();
            }),Payload->Presentation.MaxVFXLifetimeSeconds,false);
        }
    }
    if (auto* Audio = Sound.LoadSynchronous()) UGameplayStatics::PlaySoundAtLocation(MyTarget, Audio, Location);
    return true;
}
