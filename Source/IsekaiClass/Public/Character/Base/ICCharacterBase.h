#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "World/State/ICStateParticipant.h"
#include "ICCharacterBase.generated.h"
class UICAbilitySystemComponent;

UCLASS(Abstract, meta=(ToolTip="플레이어·NPC·몬스터의 공통 캐릭터 기반입니다. 식별·이동·상태 수명과 GAS 접근을 제공하며 카메라·가방은 플레이어 클래스에 둡니다. 전투하지 않는 NPC에는 ASC를 생성하지 않습니다."))
class ISEKAICLASS_API AICCharacterBase : public ACharacter, public IAbilitySystemInterface, public IICStateParticipant
{
    GENERATED_BODY()
public:
    AICCharacterBase();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    UICAbilitySystemComponent* GetICAbilitySystem() const;
    void EnableCombat(bool PrimaryStats, bool PlayerSP);
    void ReleaseForLoad();
    virtual bool InitializeState(FName ID);
    virtual bool ReadLiveState_Implementation(FName CharacterId, FICCharacterState& Snapshot) const override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* WorldState) override;
    virtual bool CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
    virtual void HandleZeroHP();
    virtual bool IsHostileTo(const AICCharacterBase* Other) const;
    UFUNCTION(BlueprintPure, Category="IC|Character", meta=(ToolTip="이 캐릭터의 지속 상태 식별 ID입니다. 플레이어는 CHR_PLAYER, NPC는 NPC ID이며 몬스터는 스폰 슬롯 키를 별도로 사용합니다.\n@return 현재 조회 결과입니다. 지속 원본을 직접 수정하는 반환값이 아닙니다.")) FName GetCharacterID() const;
    UFUNCTION(BlueprintNativeEvent, Category="IC|Combat", meta=(ToolTip="HP 0 전환을 한 번 알리는 연출 이벤트입니다. 게임 상태 반영은 C++ 공통 경로에서 먼저 수행합니다.")) void OnDefeated();
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|GAS", meta=(ToolTip="이 캐릭터의 GAS 컴포넌트입니다. 전투 대상에만 C++로 생성하고 InitAbilityActorInfo 후 상태를 복원합니다.")) TObjectPtr<UICAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="IC|State", meta=(ToolTip="실행 캐릭터의 지속 ID입니다. 중복 ID 등록은 실패하고 제거 전에 실행 캐시를 원본에 반영합니다.")) FName CharacterID;
    bool bRegisteredState = false;
    bool bSuppressPersistence = false;
    bool bZeroHPHandled = false;
    bool bUsePrimaryStats = false, bUsePlayerSP = false;
    UICWorldStateSubsystem* WorldState() const;
    UFUNCTION(meta=(ToolTip="지속 상태 거래가 확정된 뒤 실행 캐시를 갱신하는 내부 콜백입니다.\n@param ID 변경된 캐릭터의 지속 ID입니다.")) virtual void StoredCharacterChanged(FName ID);
};
