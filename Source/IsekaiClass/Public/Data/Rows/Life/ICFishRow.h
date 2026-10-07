#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICFishRow.generated.h"

/** Species definition; the referenced item definition owns stacking, trade prices and cooking/gift identity. */
USTRUCT(BlueprintType, meta=(ToolTip="물고기 정의와 획득 아이템·서식 낚시터·미끼·외형 참조입니다. Fish ID와 인벤토리 Item ID를 구분하세요.", ShortToolTip="물고기 정의와 획득 아이템·서식 낚시터·미끼·외형 참조입니다."))
struct ISEKAICLASS_API FICFishRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID."))
    FName ItemID;

    /** R01-R04 each contain three species; R05-R08 each contain two. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID."))
    FName RegionID;

    /** Empty means every point that explicitly lists this species. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="출현 가능한 FishingPoint ID 목록입니다. 낚시터 후보 목록과 서로 맞춥니다.\n입력 자료형: 이름/ID 배열."))
    TArray<FName> AllowedFishingPointIDs;

    /** Empty accepts any bait allowed by the fishing point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ToolTip="허용 미끼 Item ID 목록입니다. 빈 목록의 제한 해제 규칙은 낚시 실행 측에서 확정해야 하며 현재 전용 실행은 미연결입니다.\n입력 자료형: 이름/ID 배열."))
    TArray<FName> AcceptedBaitItemIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D)."))
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh)."))
    TSoftObjectPtr<UStaticMesh> Mesh;
};
