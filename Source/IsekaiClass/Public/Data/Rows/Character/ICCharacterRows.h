#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCharacterRows.generated.h"

class ACharacter;
class UAnimInstance;
class USkeletalMesh;

UENUM(BlueprintType, meta=(ToolTip="외형 프리셋의 성별 분류입니다. 외형 에셋 분류이며 전투 능력치 성별 보정은 이 선택만으로 적용되지 않습니다."))
enum class EICPlayerPresetGender : uint8 { Male UMETA(ToolTip="남성 외형 프리셋입니다.\n외형 프리셋의 성별 분류입니다. 외형 에셋 분류이며 전투 능력치 성별 보정은 이 선택만으로 적용되지 않습니다."), Female UMETA(ToolTip="여성 외형 프리셋입니다.\n외형 프리셋의 성별 분류입니다. 외형 에셋 분류이며 전투 능력치 성별 보정은 이 선택만으로 적용되지 않습니다.") };

/** A fixed character class and its four sequential skill slots. No level or skill-point data. */
USTRUCT(BlueprintType, meta=(ToolTip="직업의 무기 계열·일반 공격·스킬 슬롯·시작 장비를 정의합니다. RecruitCombat 초기화에서 직업 정보를 조회합니다. 일반 공격과 직업 스킬의 실제 전투 실행기는 별도로 연결해야 합니다.", ShortToolTip="직업의 무기 계열·일반 공격·스킬 슬롯·시작 장비를 정의합니다."))
struct ISEKAICLASS_API FICClassRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 분류 또는 Pawn 클래스 참조입니다. 열거형은 직업, 클래스 에셋은 생성할 ACharacter 자식 BP를 지정합니다.\n입력 자료형: 선택 목록 (EICCharacterClass).\nC++ 초기값: None — 직업을 지정하지 않은 상태입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICCharacterClass CharacterClass = EICCharacterClass::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="검·활·지팡이·완드 계열 분류입니다. 직업/장착/옵션 제한과 함께 확인합니다.\n입력 자료형: 선택 목록 (EICWeaponFamily).\nC++ 초기값: None — 무기 계열이 없는 장비 또는 미지정 값입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 일반 공격의 물리/마법 구분입니다. 무기/직업 규칙을 맞춥니다.\n입력 자료형: 선택 목록 (EICDamageType).\nC++ 초기값: Physical — 물리 피해 계열입니다. 물리 공격 능력치를 사용하는 효과에 지정합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICDamageType NormalAttackDamageType = EICDamageType::Physical;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 첫 스킬 Skill ID입니다. Q 대응을 의도하며 실제 입력 명령/소유/실행 연결을 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillSlot1ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 두 번째 스킬 Skill ID입니다. E 대응을 의도하며 같은 직업 소유 규칙을 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillSlot2ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 세 번째 스킬 Skill ID입니다. R 대응을 의도하며 선행 해금/조건을 함께 작성합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillSlot3ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 네 번째 스킬 Skill ID입니다. T 대응을 의도하며 실제 전투 실행 연결이 필요합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillSlot4ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 시작 무기 Item ID입니다. 현재 플레이어 실제 지급은 NewGame.StartingWeapons이므로 이 값만 수정해 지급을 바꾸지 않습니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName StartingWeaponID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="직업 시작 의복 Item ID입니다. 실제 지급 경로와 RecruitCombat.InitialEquipment/NewGame.StartingItems를 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName StartingClothesID;
    // Must have exactly three stages, initialized from tuning values 1.0 / 1.0 / 1.3.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="일반 공격 콤보 단계 배열입니다. 피해/타격/몽타주/취소 구간을 작성하고 실제 전투 실행에 연결합니다.\n입력 자료형: 구조/객체 (FICNormalAttackStage) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICNormalAttackStage> NormalAttackStages;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="오른손 부착 소켓입니다. 기본 weapon_r이며 실제 메시 소켓을 확인합니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"weapon_r\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName WeaponRightSocket = TEXT("weapon_r");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="왼손 부착 소켓입니다. 기본 weapon_l이며 실제 장착 실행과 맞춥니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"weapon_l\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName WeaponLeftSocket = TEXT("weapon_l");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="투사체 생성 소켓입니다. 기본 muzzle이며 실제 에셋 소켓과 일치해야 합니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"muzzle\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName MuzzleSocket = TEXT("muzzle");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<UTexture2D> Icon;
};

/** Human male/female appearance choices; gameplay initial state belongs to new-game settings. */
USTRUCT(BlueprintType, meta=(ToolTip="플레이어 외형 프리셋의 성별·메시·애니메이션·초상화 참조입니다. 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.", ShortToolTip="플레이어 외형 프리셋의 성별·메시·애니메이션·초상화 참조입니다."))
struct ISEKAICLASS_API FICPlayerPresetRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="외형 프리셋 성별 분류입니다. 실제 적용 외형은 메시/Anim BP 참조로 결정합니다.\n입력 자료형: 선택 목록 (EICPlayerPresetGender).\nC++ 초기값: Male — 남성 외형 프리셋입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) EICPlayerPresetGender Gender = EICPlayerPresetGender::Male;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (USkeletalMesh).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="AnimInstance 자식 Anim BP 클래스입니다. 메시 스켈레톤과 호환되어야 합니다.\n입력 자료형: Soft Class 에셋 참조 (UAnimInstance).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="직업 분류 또는 Pawn 클래스 참조입니다. 열거형은 직업, 클래스 에셋은 생성할 ACharacter 자식 BP를 지정합니다.\n입력 자료형: Soft Class 에셋 참조 (ACharacter).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) TSoftClassPtr<ACharacter> CharacterClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="Texture2D 초상화입니다. 게임 메시와 별개이며 실제 표시 UI에서 로드합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset", meta=(ToolTip="외형 ResourceReference ID입니다. 직접 메시/애니메이션 참조와 스켈레톤 검증을 함께 사용합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 PlayerCharacter는 BP의 AppearancePresets를 사용하므로 이 DT와 자동 연결된 것으로 생각하면 안 됩니다.")) FName AppearanceResourceID;
};

/** Applied once to a new recruitable NPC state, before equipment/player-CHA/synergy calculation. */
USTRUCT(BlueprintType, meta=(ToolTip="영입 NPC의 초기 기본 능력치·장비·고유 스킬·전투 성향을 정의합니다. EnsureCharacter가 NPC·직업·장비 참조를 확인해 지속 상태를 초기화합니다. 이후 성장·자원·모드는 WorldState가 원본입니다.", ShortToolTip="영입 NPC의 초기 기본 능력치·장비·고유 스킬·전투 성향을 정의합니다."))
struct ISEKAICLASS_API FICRecruitCombatRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="NPC DT의 정의 ID입니다. R08 지역 규격의 같은 RowName/ID를 지정하고 액터 이름·인스턴스 GUID와 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="Class DT의 직업 정의 ID입니다. 직업 열거형이나 0부터 시작하는 직업 인덱스와 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) FName ClassID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="검·활·지팡이·완드 계열 분류입니다. 직업/장착/옵션 제한과 함께 확인합니다.\n입력 자료형: 선택 목록 (EICWeaponFamily).\nC++ 초기값: None — 무기 계열이 없는 장비 또는 미지정 값입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="동료 전투 역할입니다. 전열/원거리/마법/회복/지원 AI 분류이며 실제 행동 연결은 별도입니다.\n입력 자료형: 선택 목록 (EICCompanionRole).\nC++ 초기값: Unspecified — 역할을 따로 지정하지 않았습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) EICCompanionRole Role = EICCompanionRole::Unspecified;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="영입 NPC 최초 기본 능력치입니다. EnsureCharacter가 사용하며 기존 성장 원본을 매번 초기값으로 덮지 않습니다.\n입력 자료형: 구조/객체 (FICBaseStats).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) FICBaseStats InitialBaseStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="NPC 최초 장비 Item ID 묶음입니다. 아이템/장비/직업을 검증해 상태를 준비합니다.\n입력 자료형: 구조/객체 (FICInitialEquipment).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) FICInitialEquipment InitialEquipment;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="NPC 고유 Skill ID입니다. EnsureCharacter가 존재를 검사하고 실제 사용은 별도 전투 경로입니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) FName UniqueSkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="최초 공격 성향입니다. NPC 상태에 복사되며 이후 변경 상태와 실제 AI 실행을 구분합니다.\n입력 자료형: 선택 목록 (EICAttackDisposition).\nC++ 초기값: Defensive — 방어·지원 우선 규칙으로 공격하는 성향입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) EICAttackDisposition InitialAttackDisposition = EICAttackDisposition::Defensive;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="최초 저체력 행동입니다. NPC 상태에 복사되며 실제 AI 연결은 별도입니다.\n입력 자료형: 선택 목록 (EICLowHPDisposition).\nC++ 초기값: Wait — 낮은 HP에서 공격을 대기하는 성향입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) EICLowHPDisposition InitialLowHPDisposition = EICLowHPDisposition::Wait;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="최초 적 선택 우선순위입니다. NPC 상태에 복사되며 실제 타깃 AI 연결은 별도입니다.\n입력 자료형: 선택 목록 (EICTargetPriority).\nC++ 초기값: PlayerTargetFirst — 플레이어가 지정한 적을 우선합니다. 해당 적이 유효하지 않을 때의 대체 선택도 실행 측에서 처리해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) EICTargetPriority InitialTargetPriority = EICTargetPriority::PlayerTargetFirst;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit", meta=(ToolTip="최초 자동 스킬 허용 기록입니다. 체크=허용, 해제=비활성입니다. 실제 자동 AI/GAS 사용 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 이후 성장·자원·모드는 WorldState가 원본입니다.")) bool bInitiallyAutoSkillEnabled = false;
};
