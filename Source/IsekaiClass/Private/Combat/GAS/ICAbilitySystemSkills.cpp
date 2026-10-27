#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/Effects/ICGameplayEffects.h"
#include "Combat/GAS/Cues/ICCombatCueNotify.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Enemy/Character/ICEnemyBase.h"

FGameplayAbilitySpecHandle UICAbilitySystemComponent::GetOrGrantSkillAbility(const FICSkillRow& Row)
{
    if (const auto* Existing = SkillAbilityHandles.Find(Row.ID)) return *Existing;
    TSubclassOf<UGameplayAbility> Class = Row.AbilityClass.IsNull() ? UICSkillGameplayAbility::StaticClass() : Row.AbilityClass.LoadSynchronous();
    if (!Class || !Class->IsChildOf(UICSkillGameplayAbility::StaticClass())) return FGameplayAbilitySpecHandle();
    const auto* Default = Cast<UICSkillGameplayAbility>(Class->GetDefaultObject());
    if (Default && !Default->SkillID.IsNone() && Default->SkillID != Row.ID) return FGameplayAbilitySpecHandle();
    const auto Handle = GiveAbility(FGameplayAbilitySpec(Class, 1));
    SkillAbilityHandles.Add(Row.ID, Handle);
    return Handle;
}

void UICAbilitySystemComponent::ApplySkillCooldown(const FICSkillRow& Row, double Duration)
{
    if (Duration <= 0) return;
    Runtime.Combat.SkillReadyAtPlaySecond.Add(Row.ID, PlaySecond() + Duration);
    if (auto* Old = CooldownEffectHandles.Find(Row.ID)) RemoveActiveGameplayEffect(*Old);
    TSubclassOf<UGameplayEffect> Class = Row.CooldownEffectClass.IsNull() ? UICSkillCooldownEffect::StaticClass() : Row.CooldownEffectClass.LoadSynchronous();
    if (!Class || !Class->IsChildOf(UICSkillCooldownEffect::StaticClass())) Class = UICSkillCooldownEffect::StaticClass();
    auto Spec = MakeOutgoingSpec(Class, 1, MakeEffectContext());
    if (!Spec.IsValid()) return;
    Spec.Data->SetDuration(FMath::Max(.01, Duration), true);
    if (Row.CooldownTag.IsValid()) Spec.Data->DynamicGrantedTags.AddTag(Row.CooldownTag);
    CooldownEffectHandles.Add(Row.ID, ApplyGameplayEffectSpecToSelf(*Spec.Data));
}

void UICAbilitySystemComponent::SyncCooldownEffects()
{
    const double Now = PlaySecond();
    // Applying a GE also refreshes the saved deadline; never mutate the map during iteration.
    const TMap<FName,double> Deadlines = Runtime.Combat.SkillReadyAtPlaySecond;
    for (const auto& Entry : Deadlines)
    {
        if (Entry.Value <= Now)
        {
            if (auto* Handle = CooldownEffectHandles.Find(Entry.Key)) { RemoveActiveGameplayEffect(*Handle); CooldownEffectHandles.Remove(Entry.Key); }
            continue;
        }
        if (const auto* Handle = CooldownEffectHandles.Find(Entry.Key))
            if (GetActiveGameplayEffect(*Handle)) continue;
        FICDefinitionResult Result;
        if (const auto* Row = Definitions() ? Definitions()->Find<FICSkillRow>(Entry.Key, Result) : nullptr)
        {
            const double Remaining = Entry.Value - Now;
            ApplySkillCooldown(*Row, Remaining);
        }
    }
}

void UICAbilitySystemComponent::PlayCombatCue(bool bImpact, const FICCombatPresentation& Presentation, AActor* Recipient, const FVector& Location)
{
    // Configured monster reactions already play once from the applied HP loss,
    // including periodic and projectile damage. Do not add the generic impact.
    if(bImpact)if(const auto* Enemy=Cast<AICEnemyBase>(Recipient))
        if(!Enemy->GetMonsterDefinition().HitVFX.IsNull()||!Enemy->GetMonsterDefinition().HitSound.IsNull())return;
    if(bImpact&&Cast<AICEnemyBase>(GetAvatarActor())&&Presentation.ImpactVFX.IsNull()&&Presentation.ImpactSound.IsNull())return;
    auto* Payload = NewObject<UICCombatCuePayload>(this);
    Payload->Presentation = Presentation;
    Payload->bImpact = bImpact;
    FGameplayCueParameters Params;
    Params.Instigator = GetAvatarActor();
    Params.EffectCauser = GetAvatarActor();
    Params.SourceObject = Payload;
    Params.Location = Location;
    auto* TargetCharacter = Cast<AICCharacterBase>(Recipient);
    auto* TargetASC = TargetCharacter ? TargetCharacter->GetICAbilitySystem() : nullptr;
    if (!TargetASC) TargetASC = this;
    TargetASC->ExecuteGameplayCue(bImpact ? ICTags::CueImpact : ICTags::CueCast, Params);
}
