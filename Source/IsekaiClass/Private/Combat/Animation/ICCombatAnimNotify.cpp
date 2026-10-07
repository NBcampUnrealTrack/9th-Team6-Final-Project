#include "Combat/Animation/ICCombatAnimNotify.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Character/Base/ICCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyQueue.h"
#include "Debug/ICLog.h"
void UICCombatAnimNotify::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference)
{
    IC_TRACE_FUNCTION();Super::Notify(MeshComp,Animation,EventReference);auto* C=MeshComp?Cast<AICCharacterBase>(MeshComp->GetOwner()):nullptr;auto* ASC=C?C->GetICAbilitySystem():nullptr;if(!ASC)return;
    const auto* Context=EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();if(!Context)return;
    ASC->NotifyFromMontage(Context->MontageInstanceID,Event,HitIndex);
}
FString UICCombatAnimNotify::GetNotifyName_Implementation() const { IC_TRACE_FUNCTION();return Event.ToString()+TEXT(" #")+FString::FromInt(HitIndex); }
