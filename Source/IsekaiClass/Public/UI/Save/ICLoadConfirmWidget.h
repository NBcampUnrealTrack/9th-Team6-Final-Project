#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICLoadConfirmWidget.generated.h"
class UButton;
class UTextBlock;
class UImage;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadCancelled);
UCLASS(Abstract, Blueprintable, meta=(ToolTip="기존 로드 확인 WBP 기반입니다. 선택된 슬롯의 정보와 로드/취소 버튼을 표시하고 성공 여부를 상위 화면에 알립니다.", ShortToolTip="기존 로드 확인 WBP 기반입니다."))
class ISEKAICLASS_API UICLoadConfirmWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="로드 성공 확인 이벤트입니다. 기존 슬롯/맵 전환 흐름을 진행하며 실패 메시지를 성공으로 처리하지 않습니다.\n입력 자료형: FICLoadConfirmed.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICLoadConfirmed OnConfirmed;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="로드 확인 취소 이벤트입니다. 상위 화면을 복원하고 확인 위젯 구독/참조를 정리합니다.\n입력 자료형: FICLoadCancelled.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICLoadCancelled OnCancelled;
    void Configure(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="로드 확정 Button 바인딩입니다. 실제 선택 슬롯 검증/로드를 요청하고 실패하면 안내를 표시합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_LoadConfirm_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="로드 취소 Button 바인딩입니다. 확인 창을 닫고 상위 선택 화면을 복원합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_LoadConfirm_Cancel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="로드 확인 PNG 미리보기 Image입니다. Info.Preview.ThumbnailPNG를 해석해 표시합니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_LoadConfirm_Thumbnail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="확인 대상 저장 요약 TextBlock입니다. 이름·시각·위치·플레이 시간을 실제 Info로 표시합니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_LoadConfirm_Summary;
private:
    UPROPERTY(meta=(ToolTip="현재 표시/확인하는 실제 저장 슬롯 정보입니다. Slot/bExists/bValid/bUsesBackup를 확인하고 로드 성공을 별도로 검사합니다.\n입력 자료형: 구조/객체 (FICSaveSlotInfo).")) FICSaveSlotInfo Info;
    UPROPERTY(meta=(ToolTip="현재 PNG에서 생성한 표시 Texture2D입니다. 저장 원본 바이트와 별개인 런타임 참조입니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> Thumbnail;
    UFUNCTION(meta=(ToolTip="불러오기 확인 팝업의 확정 버튼 콜백입니다. 확인 이벤트를 발행하고 팝업을 닫습니다.")) void HandleConfirm();
    UFUNCTION(meta=(ToolTip="불러오기 확인 팝업의 취소 버튼 콜백입니다. 취소 이벤트를 발행하고 팝업을 닫습니다.")) void HandleCancel();
};
