#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICNPCRow.generated.h"

class AActor;
class UAnimInstance;
class USkeletalMesh;
class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="NPC의 종족 분류입니다. 외형과 콘텐츠 분류이며 이 선택만으로 능력치 보정은 적용되지 않습니다."))
enum class EICNPCRace : uint8 { Human UMETA(ToolTip="인간입니다.\nNPC의 종족 분류입니다. 외형과 콘텐츠 분류이며 이 선택만으로 능력치 보정은 적용되지 않습니다."), Elf UMETA(ToolTip="엘프입니다.\nNPC의 종족 분류입니다. 외형과 콘텐츠 분류이며 이 선택만으로 능력치 보정은 적용되지 않습니다."), Dwarf UMETA(ToolTip="드워프입니다.\nNPC의 종족 분류입니다. 외형과 콘텐츠 분류이며 이 선택만으로 능력치 보정은 적용되지 않습니다."), Beastfolk UMETA(ToolTip="수인입니다.\nNPC의 종족 분류입니다. 외형과 콘텐츠 분류이며 이 선택만으로 능력치 보정은 적용되지 않습니다.") };

/** Identity, presentation and definition links only. Growth belongs to RecruitCombat; live state belongs to WorldState. */
USTRUCT(BlueprintType, meta=(ToolTip="NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다. EnsureCharacter 초기화에서 사용하며 이후 관계·자원·모드·위치는 WorldState가 유지합니다.", ShortToolTip="NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다."))
struct ISEKAICLASS_API FICNPCRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICNPCType).\nC++ 초기값: Background — 배경 NPC입니다. 번호201~299를 사용합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) EICNPCType Type = EICNPCType::Background;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC 종족 분류입니다. 인간/엘프/드워프/수인을 고르며 메시/대사 규칙과 함께 작성합니다.\n입력 자료형: 선택 목록 (EICNPCRace).\nC++ 초기값: Human — 인간입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) EICNPCRace Race = EICNPCRace::Human;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC 최초 위치의 Anchor ID입니다. 런타임에는 같은 ID를 가진 레벨 앵커 액터의 위치·회전을 사용합니다. 이후 이동 위치는 WorldState가 원본입니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName InitialAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="하루 Schedule DT ID입니다. 시간 구간/위치가 겹치거나 누락되지 않도록 검사합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName ScheduleID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC의 Dialogue ID 목록입니다. 대사 NPCID와 소유자를 맞추세요. 실제 대사 선택/조건 실행은 별도입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TArray<FName> DialogueIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="GiftPreference DT ID입니다. 선호 아이템과 관계 증감량을 조회합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName GiftPreferenceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC/일과 구간에서 제공할 Service ID 목록입니다. 영업/조건 실행 연결도 필요합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TArray<FName> ServiceIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="RecruitCombat DT ID입니다. NPCID·ClassID·NPC 종류가 일치해야 초기화됩니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName RecruitCombatID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="Class DT의 직업 정의 ID입니다. 직업 열거형이나 0부터 시작하는 직업 인덱스와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName ClassID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC 직업/일과 역할 GameplayTag입니다. 실제 서비스/조건/AI에서 사용하는 등록 태그를 선택합니다.\n입력 자료형: GameplayTag.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FGameplayTag OccupationTag;
    // Validated against Type; these flags do not represent current recruitment or ending progress.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="NPC 영입 허용 정의입니다. 실제 종류/RecruitCombat/관계/모드 전환 실행을 함께 확인합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) bool bRecruitable = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC", meta=(ToolTip="숨겨진 엔딩 관계 대상 NPC 표시입니다. Ending.HiddenAffinityNPCIDs와 실제 관계 판정을 맞춥니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) bool bEndingTarget = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relationship", meta=(ClampMin="0", ClampMax="100", ToolTip="최초 NPC 관계입니다. EnsureCharacter가 초기화하며 이후 변화는 WorldState를 유지합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 100.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) int32 InitialAffinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="외형 ResourceReference ID입니다. 직접 메시/애니메이션 참조와 스켈레톤 검증을 함께 사용합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) FName AppearanceResourceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (USkeletalMesh).\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="Texture2D 초상화입니다. 게임 메시와 별개이며 실제 표시 UI에서 로드합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="AnimInstance 자식 Anim BP 클래스입니다. 메시 스켈레톤과 호환되어야 합니다.\n입력 자료형: Soft Class 에셋 참조 (UAnimInstance).\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="AActor 자식 BP 클래스입니다. 스폰 실행·충돌·컴포넌트 설정을 확인합니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TSoftClassPtr<AActor> ActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(AllowedClasses="/Script/StateTreeModule.StateTree", ToolTip="일과 StateTree 참조입니다. 현재 UObject 소프트 참조이며 실제 실행/앵커 이동은 연결 대상입니다.\n입력 자료형: Soft Object 에셋 참조 (UObject).\n허용 에셋 종류: /Script/StateTreeModule.StateTree.\n소속 정의/시스템: NPC 이름 키·종족·소속 지역·집·초기 위치·일과·대사·서비스·영입·외형의 정적 원본입니다.")) TSoftObjectPtr<UObject> ScheduleStateTree;
};
