#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCraftingComponent.generated.h"

class UICInventoryComponent;

UENUM(BlueprintType, meta=(ToolTip="현재 생산 UI의 레시피 분류입니다."))
enum class EPlayerRecipeType : uint8 { Cook UMETA(ToolTip="요리 레시피입니다.\n현재 생산 UI의 레시피 분류입니다."), Craft UMETA(ToolTip="제작 레시피입니다.\n현재 생산 UI의 레시피 분류입니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="레시피 목록에 표시할 공통 정보입니다. 실제 재료/결과/생산 참조는 FICRecipeRow에서 정의합니다.", ShortToolTip="레시피 목록에 표시할 공통 정보입니다."))
struct FPlayerRecipeDefinition : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="Recipe DT ID입니다. 예: RCP_COOK_001. 재료·결과·ProductionID를 조회해 제작합니다.\n입력 자료형: 이름/ID.")) FName RecipeID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="실제 표시 FText 이름입니다. String Table 참조로 현지화 식별자를 유지합니다. NameKey 입력과 FText 연결은 별개입니다.\n입력 자료형: 현지화 가능한 텍스트.")) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EPlayerRecipeType).\nC++ 초기값: Cook — 요리 레시피입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerRecipeType Type=EPlayerRecipeType::Cook;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="제작 SP 비용입니다. 현재 시작 시 한 번 차감하며 Recipe/Production 값이 같아야 합니다. 취소에도 시작 SP는 환불하지 않고 재료는 성공 완료 시 소모합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float SPCost=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="레시피 최초 해금 허용입니다. 현재 StartRecipe에서 체크면 별도 UnlockedRecipeIDs 없이 시작 가능하고 해제면 WorldState 해금 기록이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bUnlockedByDefault=true;
};

UCLASS(ClassGroup=(Player),meta=(BlueprintSpawnableComponent, ToolTip="Recipe DT 조회와 실제 ProductionSubsystem 제작 시작을 연결하는 컴포넌트입니다. 생성자에 시험용 레시피를 넣지 않으며 정의 누락은 실패합니다.", ShortToolTip="Recipe DT 조회와 실제 ProductionSubsystem 제작 시작을 연결하는 컴포넌트입니다."))
class ISEKAICLASS_API UICCraftingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICCraftingComponent();
    UFUNCTION(BlueprintPure,Category="IC|Crafting", meta=(ToolTip="실제 Recipe DT의 요리/제작 종류별 유효 레시피 ID 목록을 조회합니다.\n@param Type 조회할 정의 종류 또는 레시피 분류입니다. 이 함수의 타입 선택 목록을 확인하세요. 입력 핀. 자료형: 선택 목록 (EPlayerRecipeType).\n@return 해당 분류의 레시피 ID 배열입니다. 자료형: 이름/ID 배열.")) TArray<FName> GetRecipes(EPlayerRecipeType Type) const;
    const struct FICRecipeRow* Find(FName RecipeID) const;
    UFUNCTION(BlueprintPure,Category="IC|Crafting", meta=(ToolTip="Recipe와 연결 Production·Item 참조를 검증하고 현재 UI용 레시피 구조를 반환합니다.\n@param RecipeID 실제 Recipe DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Definition 조회된 정의의 출력 복사본입니다. 성공 결과를 먼저 검사하세요. 복사본 수정은 DT 원본을 변경하지 않습니다. 입력/출력 핀. 자료형: 구조/객체 (FPlayerRecipeDefinition).\n@return 유효한 레시피 조회에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool GetRecipeDefinition(FName RecipeID,FPlayerRecipeDefinition& Definition) const;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="실제 레시피 생산 시작을 ProductionSubsystem에 요청합니다. 작업 위치·도구·재료 예약·SP·결과 정의를 함께 검사합니다.\n@param RecipeID 실제 Recipe DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@param WorkAnchorID 생산 작업 위치를 나타내는 Anchor DT ID입니다. 허용 앵커 목록과 일치해야 합니다. 입력 핀. 자료형: 이름/ID.\n@param Inventory 거래에 사용할 실제 인벤토리 컴포넌트입니다. 현재 WorldState에 연결된 컴포넌트를 전달하세요. 입력 핀. 자료형: UICInventoryComponent*.\n@param JobID 시작한 생산 작업의 출력 GUID입니다. 완료/취소 요청에서 동일 GUID를 사용합니다. 입력/출력 핀. 자료형: 인스턴스 GUID.\n@param Reason 생산 시작 실패의 출력 문자열입니다. 입력/출력 핀. 자료형: 문자열.\n@return 시작 거래가 성공하면 true입니다. JobID를 보관하고 완료 이벤트를 따로 확인하세요. 자료형: 체크박스(true/false).")) bool StartCraft(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason);
    UFUNCTION(BlueprintPure, meta=(ToolTip="현재 연결된 실제 DT를 검사하여 필요한 아이템/레시피 정의가 유효한지 확인합니다. 오류 안내는 컴포넌트 메시지 경로로 전달합니다.\n@return 필요 정의 검사에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool ValidateDefinitions() const;
};
