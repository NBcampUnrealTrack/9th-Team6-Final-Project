#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ICSpawnSubsystem.generated.h"
class AICWorldAnchor;
class AICNPCBase;
class AICEnemyBase;
class AICCharacterBase;
struct FICWorldState;
UCLASS(meta=(ToolTip="현재 월드의 로드된 앵커·NPC·몬스터 실행 액터를 관리합니다. 지속 원본은 WorldState이며 상태 복원 후 AI를 시작합니다. 셀 해제·저장·로드에서 캐시 반영과 중복 생성 방지를 처리합니다."))
class ISEKAICLASS_API UICSpawnSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    bool RegisterAnchor(AICWorldAnchor* Anchor);
    void UnregisterAnchor(AICWorldAnchor* Anchor);
    bool ResolveLoadedAnchor(FName ID,FTransform& Transform) const;
    bool FindSafePartyPlacement(FName CharacterID,FTransform& Transform) const;
    AICNPCBase* ReserveReviveTarget(FName ID,class UICAbilitySystemComponent* Caster,FString& Reason);
    bool CanReviveAtCurrentPlacement(AICCharacterBase* Target) const;
    bool FlushActors();
    bool CanRestoreActors(const FICWorldState& State) const;
    void RestoreActors();
    UFUNCTION(BlueprintPure,Category="IC|Spawn",meta=(ToolTip="지속 캐릭터 ID에 연결된 활성 캐릭터를 조회합니다. 제거된 NPC는 WorldState로 조회하세요.\n@param CharacterID NPC 또는 플레이어의 지속 ID입니다.\n@return 활성 액터가 없으면 비어 있는 참조입니다.")) AICCharacterBase* FindCharacter(FName CharacterID) const;
    UFUNCTION(BlueprintCallable,Category="IC|Spawn",meta=(ToolTip="로드된 앵커와 지속 상태를 다시 검사하여 필요한 실행 액터를 생성·제거합니다. 비어 있는 DT에는 임의 캐릭터를 생성하지 않습니다.")) void RefreshSpawns();
private:
    TMap<FName,TWeakObjectPtr<AICWorldAnchor>> Anchors;
    TMap<FName,TWeakObjectPtr<AICNPCBase>> NPCs;
    TMap<FName,TWeakObjectPtr<AICEnemyBase>> Enemies;
    TMap<FName,TWeakObjectPtr<class UICAbilitySystemComponent>> ReviveReservations;
    bool bReady=false,bRefreshing=false,bTearingDown=false;
    double NextRefreshAt=-1;
    UFUNCTION(meta=(ToolTip="로드 뒤 기존 실행 캐시를 제거하고 복원 원본에 맞춰 스폰을 갱신하는 내부 콜백입니다.")) void WorldRestored();
};
