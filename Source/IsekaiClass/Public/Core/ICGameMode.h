#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ICGameMode.generated.h"
// BP_ICGameMode handles both the configured main menu map and gameplay maps.
UCLASS(Blueprintable, meta=(ToolTip="메인 메뉴와 플레이 레벨이 함께 사용하는 게임모드입니다. MainMenuMap에 지정한 레벨에서는 기본 Pawn을 생성하지 않고 공통 컨트롤러가 메인 메뉴 WBP를 표시합니다. 다른 레벨에서는 기본 Pawn·컨트롤러·PlayerState·GameState 클래스를 사용합니다. 자식 BP에서 프로젝트의 IC BP와 실제 메인 메뉴 레벨을 연결하세요.", ShortToolTip="메인 메뉴와 플레이 레벨의 공통 게임모드입니다."))
class ISEKAICLASS_API AICGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AICGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Main Menu", meta=(ToolTip="메인 메뉴로 실행할 레벨의 소프트 참조입니다. BP_ICGameMode에서 실제 L_IC_MainMenu를 지정하세요. 지정한 레벨에서는 기본 플레이어 Pawn을 생성하지 않으며 공통 ICPlayerController가 메인 메뉴를 엽니다. 비워 두면 모든 레벨을 플레이 레벨로 처리합니다. 레벨 이름이 아니라 전체 패키지 경로를 비교하므로 다른 폴더의 동명 레벨은 메뉴로 인식하지 않습니다. 에디터 PIE 접두사는 비교 전에 제거합니다.\n입력 자료형: 레벨 에셋 선택(UWorld)."))
    TSoftObjectPtr<UWorld> MainMenuMap;

    UFUNCTION(BlueprintPure, Category="IC|Main Menu", meta=(ToolTip="현재 월드가 MainMenuMap에 지정한 메인 메뉴 레벨인지 확인합니다. 메인 메뉴 전용 게임모드/BP를 따로 만들지 않고 같은 게임모드로 시작 모드를 분기할 때 사용합니다.\n@return 현재 월드와 MainMenuMap의 전체 패키지 경로가 같으면 true입니다. 미지정 또는 플레이 레벨이면 false입니다."))
    bool IsMainMenuLevel() const;
};
