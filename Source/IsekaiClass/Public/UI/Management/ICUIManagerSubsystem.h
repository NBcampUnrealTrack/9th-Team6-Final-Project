#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "ICUIManagerSubsystem.generated.h"
class UUserWidget;
class APlayerController;
UCLASS(meta=(ToolTip="로컬 플레이어의 화면 스택·포커스·입력 모드와 정지 요청을 관리합니다. 게임 상태 원본은 보관하지 않으며 마지막 정지 화면이 닫힐 때 시간이 재개됩니다."))
class ISEKAICLASS_API UICUIManagerSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintCallable, Category="IC|UI", meta=(ToolTip="이미 생성한 위젯을 화면 스택에 등록하고 표시합니다. 같은 위젯의 재등록은 기존 설정을 갱신합니다.\n@param Widget 표시할 실제 위젯 인스턴스입니다.\n@param PauseWorld 화면이 열려 있는 동안 시간을 정지할지 지정합니다.\n@param ZOrder 화면 표시 우선순위입니다.\n@return 등록과 표시가 완료되면 참입니다.")) bool OpenScreen(UUserWidget* Widget,bool PauseWorld,int32 ZOrder=10);
    UFUNCTION(BlueprintCallable, Category="IC|UI", meta=(ToolTip="해당 위젯의 표시와 정지 요청을 해제하고 이전 화면으로 포커스를 돌립니다.\n@param Widget 닫을 위젯 인스턴스입니다.")) void CloseScreen(UUserWidget* Widget);
    UFUNCTION(BlueprintCallable, Category="IC|UI", meta=(ToolTip="기존 컨트롤러 메뉴의 포커스와 정지 요청을 한 경로로 관리합니다.\n@param Controller 화면을 소유한 로컬 컨트롤러입니다.\n@param Focus 포커스를 받을 위젯입니다. 비면 컨트롤러 입력만 갱신합니다.\n@param MenuOpen 메뉴가 열려 있으면 참입니다.")) void SetMenuContext(APlayerController* Controller,UUserWidget* Focus,bool MenuOpen);
    UFUNCTION(BlueprintCallable, Category="IC|UI", meta=(ToolTip="여행·종료 전에 모든 관리 화면과 해당 정지 요청을 정리합니다.")) void CloseAllScreens();
private:
    struct FScreen { TWeakObjectPtr<UUserWidget> Widget; bool Pause=false; int32 Z=0; };
    TArray<FScreen> Screens;
    TWeakObjectPtr<APlayerController> MenuController;
    TWeakObjectPtr<UUserWidget> MenuFocus;
    bool bMenuOpen=false;
    void RefreshContext();
};
