#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "ICNPCBase.generated.h"

class AICNPCBase;
class AICPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FICNPCInteractionStarted, AICNPCBase*, NPC, AICPlayerController*, Requester, FName, ServiceID);

UENUM(BlueprintType, meta=(ToolTip="NPC와 상호작용할 때 표시하는 선택지 종류입니다. 모든 NPC는 대화하기·선물하기·대화 끝내기를 갖고, 상점 서비스가 있는 NPC는 거래하기가 추가됩니다."))
enum class EICNPCInteractionOption : uint8
{
    Talk UMETA(ToolTip="대화하기입니다. 모든 NPC에게 표시됩니다.\nNPC 상호작용 선택지 종류입니다."),
    Gift UMETA(ToolTip="선물하기입니다. 모든 NPC에게 표시됩니다.\nNPC 상호작용 선택지 종류입니다."),
    Trade UMETA(ToolTip="거래하기입니다. 정의에 상점 서비스가 있는 NPC에게만 표시되며, 근무 중이 아니면 비활성입니다.\nNPC 상호작용 선택지 종류입니다."),
    End UMETA(ToolTip="대화 끝내기입니다. 모든 NPC에게 표시되며 선택하면 EndInteraction을 호출해야 합니다.\nNPC 상호작용 선택지 종류입니다.")
};

USTRUCT(BlueprintType, meta=(ToolTip="NPC 상호작용 UI에 표시할 선택지 한 줄입니다. Option으로 동작을 구분하고 Label을 버튼 문구로 사용합니다.", ShortToolTip="NPC 상호작용 UI의 선택지 한 줄입니다."))
struct ISEKAICLASS_API FICNPCInteractionOption
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC", meta=(ToolTip="선택지 종류입니다. 선택했을 때 실행할 동작을 구분합니다.\n입력 자료형: 열거형 (EICNPCInteractionOption).\nC++ 초기값: Talk.\nBP에서는 조회 전용입니다.")) EICNPCInteractionOption Option = EICNPCInteractionOption::Talk;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC", meta=(ToolTip="버튼에 표시할 문구입니다.\n입력 자료형: 텍스트.\nBP에서는 조회 전용입니다.")) FText Label;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC", meta=(ToolTip="거래하기일 때 연결된 Service DT의 서비스 ID입니다. 그 외 선택지는 None입니다. 상점 UI를 열 때 UICShopSubsystem에 넘깁니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다.")) FName ServiceID;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC", meta=(ToolTip="지금 선택할 수 있는지 여부입니다. 거래하기는 상점 주인이 근무 중일 때만 true이며, false면 버튼을 비활성으로 표시하세요.\n입력 자료형: 참/거짓.\nC++ 초기값: true.\nBP에서는 조회 전용입니다.")) bool bEnabled = true;
};
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
    
    UFUNCTION(BlueprintPure,Category="IC|NPC",meta=(ToolTip="현재 일과 구간에서 제공하는 서비스 ID를 조회합니다. 이동·수면·동료 모드에서는 비어 있습니다.\n@return 현재 근무 중인 서비스 정의 ID 목록입니다."))
    TArray<FName> GetAvailableServices() const;
    
    UFUNCTION(BlueprintCallable,Category="IC|NPC",meta=(ToolTip="플레이어의 상호작용(F) 요청을 처리합니다. 쓰러졌거나 동료 모드이거나 이미 상호작용 중이면 거절합니다. 성공하면 NPC가 일과 이동을 멈추고 플레이어를 바라보며, 현재 근무 서비스 ID(없으면 None)와 함께 OnInteractionStarted를 발행합니다. UI를 닫을 때 반드시 EndInteraction을 호출하세요.\n@param Requester 상호작용을 요청한 플레이어 컨트롤러입니다. 입력 핀. 자료형: 객체 참조 (AICPlayerController).\n@return 상호작용을 시작했으면 true, 거절했으면 false입니다."))
    bool TryInteract(AICPlayerController* Requester);
    
    UFUNCTION(BlueprintCallable,Category="IC|NPC",meta=(ToolTip="상호작용을 끝내고 NPC를 일과로 복귀시킵니다. 상점·대화 UI를 닫을 때 호출하세요. 상호작용 중이 아니면 아무 일도 하지 않습니다."))
    void EndInteraction();
    
    UFUNCTION(BlueprintPure,Category="IC|NPC",meta=(ToolTip="상호작용 키 안내 문구를 반환합니다. 화면에 띄울 선택지 목록은 GetInteractionOptions를 사용하세요.\n@return 화면에 표시할 안내 문구입니다."))
    FText GetInteractionText() const;
    
    UFUNCTION(BlueprintPure,Category="IC|NPC",meta=(ToolTip="상호작용 UI에 표시할 선택지 목록을 반환합니다. 모든 NPC는 대화하기·선물하기·대화 끝내기가 기본이고, 정의에 상점 서비스가 있는 NPC는 거래하기가 추가됩니다. 거래하기는 상점 주인이 근무 중이 아니면 비활성(bEnabled=false)입니다.\n@return 표시 순서대로 정렬된 선택지 목록입니다."))
    TArray<FICNPCInteractionOption> GetInteractionOptions() const;
    
    UFUNCTION(BlueprintPure,Category="IC|NPC",meta=(ToolTip="현재 플레이어와 상호작용 중인지 조회합니다.\n@return 상호작용 중이면 true입니다."))
    bool IsInteracting() const { return bInteracting; }
    
    UPROPERTY(BlueprintAssignable,Category="IC|NPC",meta=(ToolTip="상호작용이 시작됐을 때 발행됩니다. 상점·대화 UI가 구독해서 화면을 엽니다. ServiceID가 None이면 제공 서비스가 없는 일반 대화입니다.\n입력 자료형: 이벤트 (NPC, 플레이어 컨트롤러, 서비스 ID)"))
    FICNPCInteractionStarted OnInteractionStarted;
    
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|NPC",meta=(ToolTip="월드에 직접 배치할 때 지정할 NPC DT의 ID입니다. 동일 ID의 실행 액터는 한 개만 허용됩니다. 스폰 서브시스템이 생성하면 자동 지정합니다."))
    FName NPCDefinitionID;
    
protected:
    virtual void StoredCharacterChanged(FName ID) override;
    
private:
    FICNPCRow Definition;
    bool bInitialized=false,bInteracting=false;
    FTimerHandle DefeatTimer;
};
