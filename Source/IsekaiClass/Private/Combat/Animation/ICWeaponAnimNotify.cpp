#include "Combat/Animation/ICWeaponAnimNotify.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyQueue.h"

void UICWeaponAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    auto* Player = MeshComp ? Cast<AICPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
    const auto* Context = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
    if (Player && Context) Player->NotifyWeaponVisibility(Context->MontageInstanceID, bDraw);
}

FString UICWeaponAnimNotify::GetNotifyName_Implementation() const
{
    return bDraw ? TEXT("Draw weapon") : TEXT("Put away weapon");
}
