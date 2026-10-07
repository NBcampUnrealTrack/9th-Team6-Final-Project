#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ICProductionSubsystem.generated.h"
class UICDefinitionSubsystem;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICInventoryComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FICProductionChanged,FGuid,JobID,const FString&,Message);

UCLASS(meta=(ToolTip="PlayClock 기준 제작 작업을 WorldState에 보존합니다. 시작 때 SP를 확정하고 재료를 예약하며 완료 때 재료 소모와 결과 지급을 함께 처리합니다. 메뉴 정지 중 진행하지 않고 저장 후 남은 시간을 이어갑니다.", ShortToolTip="PlayClock 기준 제작 작업을 WorldState에 보존합니다."))
class ISEKAICLASS_API UICProductionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void ConfigureServices(UICDefinitionSubsystem* Definitions,UICWorldStateSubsystem* State,UICTimeSubsystem* Clock);
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="제작 시작/완료/취소/완료 대기 이벤트입니다. JobID와 Message로 UI를 갱신하며 결과를 다시 지급하지 않습니다.\n입력 자료형: FICProductionChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICProductionChanged OnProductionChanged;
    UFUNCTION(BlueprintCallable,Category="IC|Production", meta=(ToolTip="레시피와 작업 앵커를 검사해 재료 예약과 SP 지불을 한 거래로 확정합니다. 현재 BagItems 생산만 지원하며 일반 조건·특수 생산은 미연결이면 거부합니다. PlayClock이 진행되어야 완료됩니다.\n@param RecipeID 실제 Recipe DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@param WorkAnchorID 생산 작업 위치를 나타내는 Anchor DT ID입니다. 허용 앵커 목록과 일치해야 합니다. 입력 핀. 자료형: 이름/ID.\n@param Inventory 거래에 사용할 실제 인벤토리 컴포넌트입니다. 현재 WorldState에 연결된 컴포넌트를 전달하세요. 입력 핀. 자료형: UICInventoryComponent*.\n@param JobID 성공한 생산 작업의 출력 GUID입니다. 실패하면 유효하지 않으며 같은 작업 완료/취소에 이 GUID를 사용합니다. 입력/출력 핀. 자료형: 인스턴스 GUID.\n@param Reason 시작 실패의 출력 문자열입니다. false 반환이면 확인하세요. 입력/출력 핀. 자료형: 문자열.\n@return 시작 거래에 성공하면 true입니다. 실패 시 JobID가 유효하지 않으며 Reason을 확인하세요. 자료형: 체크박스(true/false).")) bool StartRecipe(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason);
    UFUNCTION(BlueprintCallable,Category="IC|Production", meta=(ToolTip="Ready 작업의 예약 재료를 소모하고 결과를 지급하는 완료 거래를 수행합니다. 공간 부족 등 실패에서는 재료와 작업을 보존하여 재시도할 수 있습니다. 완료된 JobID는 중복 지급하지 않습니다.\n@param JobID 생산 작업의 고유 GUID입니다. 시작에서는 출력이며 완료/취소에서는 동일 작업의 GUID를 전달합니다. 입력 핀. 자료형: 인스턴스 GUID.\n@param Inventory 거래에 사용할 실제 인벤토리 컴포넌트입니다. 현재 WorldState에 연결된 컴포넌트를 전달하세요. 입력 핀. 자료형: UICInventoryComponent*.\n@param Reason 완료 실패의 출력 문자열입니다. 작업이 Ready로 남으면 수정 후 재시도할 수 있습니다. 입력/출력 핀. 자료형: 문자열.\n@return 완료 거래를 확정하면 true입니다. false이면 Reason과 현재 작업 상태를 확인하세요. 자료형: 체크박스(true/false).")) bool CompleteJob(FGuid JobID,UICInventoryComponent* Inventory,FString& Reason);
    UFUNCTION(BlueprintCallable,Category="IC|Production", meta=(ToolTip="진행/대기 생산을 취소하고 예약된 재료를 해제합니다. 현재 시작 시 지불한 SP는 환급하지 않습니다. 완료 작업을 다시 취소해 보상을 회수하지 않습니다.\n@param JobID 생산 작업의 고유 GUID입니다. 시작에서는 출력이며 완료/취소에서는 동일 작업의 GUID를 전달합니다. 입력 핀. 자료형: 인스턴스 GUID.\n@return 취소 상태를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool CancelJob(FGuid JobID);
    UFUNCTION(meta=(ToolTip="PlayClock 진행을 받는 생산 내부 콜백입니다. 남은 시간과 Ready 전환을 갱신하며 메뉴 정지 중에는 진행시키지 않습니다.\n@param Seconds 이번에 진행한 PlayClock 초입니다. 누적 총시간과 구분하며 메뉴 정지 중에는 진행하지 않습니다. 입력 핀. 자료형: 배정밀도 소수.")) void PlaySecondsAdvanced(double Seconds);
private:
    UPROPERTY(meta=(ToolTip="실제 DefinitionSubsystem 의존 참조입니다. 초기화에서 연결하며 수동으로 시험 데이터를 대신 넣지 않습니다.\n입력 자료형: 객체 참조 (UICDefinitionSubsystem).")) TObjectPtr<UICDefinitionSubsystem> Definitions;
    UPROPERTY(meta=(ToolTip="WorldState 원본 또는 서브시스템 참조입니다. 초기화/거래 경로를 통해 접근하며 별도 복제 원본을 만들지 않습니다.\n입력 자료형: 객체 참조 (UICWorldStateSubsystem).")) TObjectPtr<UICWorldStateSubsystem> State;
    UPROPERTY(meta=(ToolTip="실제 시계 서브시스템 참조 또는 저장 시계 스냅샷입니다. GameMinute 분과 PlaySecond 초를 구분해 저장/복원합니다.\n입력 자료형: 객체 참조 (UICTimeSubsystem).")) TObjectPtr<UICTimeSubsystem> Clock;
    TWeakObjectPtr<UICInventoryComponent> RuntimeInventory;
};
