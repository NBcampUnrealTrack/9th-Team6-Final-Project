#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICResourceReferenceRow.generated.h"

class USkeleton;

UENUM(BlueprintType, meta=(ToolTip="리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."))
enum class EICResourceKind : uint8
{
    SkeletalMesh UMETA(ToolTip="스켈레탈 메시입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), StaticMesh UMETA(ToolTip="스태틱 메시입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Material UMETA(ToolTip="머티리얼 또는 머티리얼 인스턴스입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Texture UMETA(ToolTip="텍스처입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Animation UMETA(ToolTip="애니메이션 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), AnimationBlueprint UMETA(ToolTip="애니메이션 BP입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."),
    Montage UMETA(ToolTip="애니메이션 몽타주입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Rig UMETA(ToolTip="리그 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Retargeter UMETA(ToolTip="리타게터 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), VFX UMETA(ToolTip="이펙트 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Sound UMETA(ToolTip="사운드 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), BGM UMETA(ToolTip="배경 음악 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), MapTexture UMETA(ToolTip="지도 이미지 텍스처입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), LevelSequence UMETA(ToolTip="레벨 시퀀스입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Blueprint UMETA(ToolTip="블루프린트 에셋입니다.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요."), Other UMETA(ToolTip="그 외 리소스입니다. Notes에 실제 종류를 적으세요.\n리소스 출처 기록의 에셋 종류입니다. 실제 Soft Object의 종류와 일치시키세요.")
};

/** Asset provenance and reusable presentation references. No invented path is required for an unassigned resource. */
USTRUCT(BlueprintType, meta=(ToolTip="콘텐츠 에셋의 종류·사용처·스켈레톤·소켓·축/크기·출처·라이선스·검증 기록입니다. 실제 파일 경로와 사용처를 작성해 리소스 오류를 검사합니다.", ShortToolTip="콘텐츠 에셋의 종류·사용처·스켈레톤·소켓·축/크기·출처·라이선스·검증 기록입니다."))
struct ISEKAICLASS_API FICResourceReferenceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="정의/아이템/효과 분류입니다. 선택한 종류가 요구하는 참조/수치/대상 규칙을 함께 작성합니다.\n입력 자료형: 선택 목록 (EICResourceKind).\nC++ 초기값: Other — 그 외 리소스입니다. Notes에 실제 종류를 적으세요.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICResourceKind Kind = EICResourceKind::Other;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="검증할 실제 리소스 에셋 소프트 참조입니다. Kind·사용처·스켈레톤 정보를 파일과 맞춥니다.\n입력 자료형: Soft Object 에셋 참조 (UObject).")) TSoftObjectPtr<UObject> Asset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="이 리소스를 사용할 정의 종류/ID 목록입니다. 실제 파일 사용처와 참조 검사를 기록합니다.\n입력 자료형: 구조/객체 (FICDefinitionRef) 배열.")) TArray<FICDefinitionRef> UsedBy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", meta=(ToolTip="리소스 Skeleton 참조입니다. 메시/애니메이션/Anim BP 간 호환성 검사에 사용합니다.\n입력 자료형: Soft Object 에셋 참조 (USkeleton).")) TSoftObjectPtr<USkeleton> Skeleton;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", meta=(ToolTip="호환 스켈레톤 그룹 키입니다. 리타깃/리소스 검증에 사용할 동일 그룹을 작성합니다.\n입력 자료형: 이름/ID.")) FName SkeletonGroup;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", meta=(ToolTip="리소스에 존재해야 하는 소켓 이름 목록입니다. 실제 메시/스켈레톤에서 확인합니다.\n입력 자료형: 이름/ID 배열.")) TArray<FName> RequiredSockets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", meta=(ToolTip="리소스에 존재해야 하는 Notify 이름 목록입니다. 실제 애니메이션/몽타주를 검사하고 이벤트 수신 경로와 맞춥니다.\n입력 자료형: 이름/ID 배열.")) TArray<FName> RequiredNotifyNames;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification", meta=(Units="cm", ToolTip="리소스 대표 크기 XYZ cm입니다. 임포트/스케일 검증용이며 100은 1m입니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector DimensionsCm = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification", meta=(ToolTip="리소스 임포트 배율 XYZ입니다. (1,1,1)은 원래 배율이며 DimensionsCm/축을 함께 검증합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::OneVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector ImportScale = FVector::OneVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification", meta=(ToolTip="리소스 축·정면·무기 손잡이/원점 보정 설명입니다. 수치/소켓 설정 시 참고할 실제 에셋 규칙을 기록합니다.\n입력 자료형: 문자열.")) FString AxisAndGripNotes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance", meta=(ToolTip="실제 리소스 출처 페이지 주소입니다. 다운로드 파일/제작자/라이선스 근거를 기록합니다.\n입력 자료형: 문자열.")) FString SourceURL;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance", meta=(ToolTip="리소스 사용 라이선스 이름/조건 기록입니다. 실제 출처/상업 사용/크레딧 요구와 맞춰 작성합니다.\n입력 자료형: 문자열.")) FString License;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance", meta=(ToolTip="리소스 제작자/출처에 표시할 크레딧 문구입니다. 라이선스 요구를 실제 파일 출처와 맞춰 기록합니다.\n입력 자료형: 문자열.")) FString Attribution;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Verification", meta=(ToolTip="대표 씬에서 외형/크기/축을 검증했다는 작성 기록입니다. 실제 확인 후 체크하고 실패 원인은 리소스 설명에 기록합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bVerifiedInRepresentativeScene = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Verification", meta=(ToolTip="리소스가 패키징/Cook에서 검증되었다는 작성 기록입니다. 체크만으로 검증/패키징 작업을 실행하지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bVerifiedForCooking = false;
};
