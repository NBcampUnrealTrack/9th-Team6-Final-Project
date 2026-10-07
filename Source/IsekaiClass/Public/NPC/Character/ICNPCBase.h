#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "ICNPCBase.generated.h"
UCLASS(Blueprintable,meta=(ToolTip="영입·주민·배경 NPC의 실행 캐릭터입니다. NPC 정의를 ID로 읽고 WorldState를 복원한 뒤 AI를 시작합니다. 영입형만 주 스탯·GAS를 생성하고 SP는 생성하지 않습니다."))
class ISEKAICLASS_API AICNPCBase : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICNPCBase();
    virtual void BeginPlay() override;
    virtual void HandleZeroHP() override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
    bool InitializeNPC(FName ID);
    void RefreshExecution();
    void HoldDownedActor();
    const FICNPCRow& GetNPCDefinition() const { return Definition; }
    UFUNCTION(BlueprintPure,Category="IC|NPC",meta=(ToolTip="현재 일과 구간에서 제공하는 서비스 ID를 조회합니다. 이동·수면·동료 모드에서는 비어 있습니다.\n@return 현재 근무 중인 서비스 정의 ID 목록입니다.")) TArray<FName> GetAvailableServices() const;
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|NPC",meta=(ToolTip="월드에 직접 배치할 때 지정할 NPC DT의 ID입니다. 동일 ID의 실행 액터는 한 개만 허용됩니다. 스폰 서브시스템이 생성하면 자동 지정합니다.")) FName NPCDefinitionID;
protected:
    virtual void StoredCharacterChanged(FName ID) override;
private:
    FICNPCRow Definition;
    bool bInitialized=false;
    FTimerHandle DefeatTimer;
};
