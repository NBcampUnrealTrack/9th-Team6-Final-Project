#include "AI/ICNPCAIController.h"

#include "NPC/Character/ICNPCBase.h"
#include "NPC/LifeObject/ICLifeObjectUserComponent.h"

void AICNPCAIController::StopExecution()
{
    // NPC만 생활 오브젝트 예약/점유를 정리한다. 몬스터는 공용 컨트롤러 경로만 사용한다.
    if (AICNPCBase* NPC = Cast<AICNPCBase>(GetPawn()))
    {
        if (UICLifeObjectUserComponent* LifeObjectUser = NPC->GetLifeObjectUser())
        {
            LifeObjectUser->ReleaseAll(TEXT("StopExecution"));
        }
    }

    Super::StopExecution();
}
