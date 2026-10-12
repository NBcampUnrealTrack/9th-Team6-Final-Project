#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICGameplayWidget.generated.h"

class AICPlayerController;

// Blueprint owns the widget tree, HUD and menu layouts.
UCLASS(Abstract, Blueprintable, meta=(ToolTip="플레이 UI WBP의 컨트롤러 조회/갱신 이벤트 기반입니다. 화면 데이터는 Controller/PlayerState/WorldState에서 읽고 지속 상태를 WBP에 별도로 저장하지 마세요.", ShortToolTip="플레이 UI WBP의 컨트롤러 조회/갱신 이벤트 기반입니다."))
class ISEKAICLASS_API UICGameplayWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|UI", meta=(ToolTip="이 위젯의 소유 플레이어를 실제 ICPlayerController로 조회합니다.\n@return 소유 ICPlayerController입니다. 다른 컨트롤러나 미연결 상태이면 None입니다. 자료형: AICPlayerController*.")) AICPlayerController* GetICPlayerController() const;
    UFUNCTION(BlueprintCallable, Category="IC|UI", meta=(ToolTip="현재 소유 컨트롤러의 상태를 읽어 게임 UI 갱신 BP 이벤트를 요청합니다. UI 자체에 지속 게임 상태를 만들지 마세요.")) void RefreshCurrentScreen();
    UFUNCTION(BlueprintImplementableEvent, Category="IC|UI", meta=(ToolTip="WorldState·메뉴·인벤토리 변경 후 실제 게임 UI를 갱신하는 BP 표시 이벤트입니다. 자식 WBP에서 조회 함수로 원본을 읽어 표시하세요.\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요.")) void OnRefreshGameplayUI();
};
