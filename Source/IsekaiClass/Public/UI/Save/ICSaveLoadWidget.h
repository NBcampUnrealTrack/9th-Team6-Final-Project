#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveLoadWidget.generated.h"
class UButton;
class UTextBlock;
class UVerticalBox;
class UWorld;
class UICSaveSubsystem;
class UICSaveSlotWidget;
class UICLoadConfirmWidget;
class UImage;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICSaveLoadClosed);
UCLASS(Abstract, Blueprintable, meta=(ToolTip="기존 저장/로드 목록 WBP 기반입니다. 실제 슬롯 정보를 조회하고 슬롯별 위젯을 구성합니다. 저장/삭제는 확인과 서비스 결과를 확인해야 합니다.", ShortToolTip="기존 저장/로드 목록 WBP 기반입니다."))
class ISEKAICLASS_API UICSaveLoadWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="저장/로드 화면 닫힘 이벤트입니다. 부모 화면/입력/정지 요청을 복원하고 제거 시 구독을 해제합니다.\n입력 자료형: FICSaveLoadClosed.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICSaveLoadClosed OnClosed;
    // Main menu: false + StartMap. In-game manual saving: true + empty map.
    UFUNCTION(BlueprintCallable, Category="IC|Save UI", meta=(ToolTip="저장/불러오기 화면의 저장 허용 여부와 목적지 플레이 레벨을 지정합니다. 메인 메뉴에서는 저장을 금지하고 유효한 레벨을 연결하세요.\n@param bInAllowSave true는 저장과 불러오기를 허용하고 false는 불러오기만 허용합니다. 입력 핀. 자료형: 체크박스(true/false).\n@param InGameplayMap 불러오기 이후 이동할 실제 플레이 레벨의 Soft Object 참조입니다. 입력 핀. 자료형: Soft Object 에셋 참조 (UWorld)."))
    void Configure(bool bInAllowSave, TSoftObjectPtr<UWorld> InGameplayMap);
    UFUNCTION(BlueprintCallable, Category="IC|Save UI", meta=(ToolTip="실제 저장 서브시스템에서 슬롯 미리보기를 다시 읽어 목록·선택·스크린샷·파티 표시를 갱신합니다.")) void RefreshSlots();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="슬롯 행을 담을 VerticalBox입니다. SlotWidgetClass로 생성한 실제 슬롯 목록을 추가합니다.\n입력 자료형: 객체 참조 (UVerticalBox).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UVerticalBox> IC_SaveLoad_SlotList;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="선택 슬롯 로드 Button입니다. 유효 슬롯/확인 후 실제 로드를 요청합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_SaveLoad_Load;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="저장/로드 화면 닫기 Button입니다. 부모 UI와 입력/정지 요청을 정리합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_SaveLoad_Close;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="수동 저장 Button입니다. 화면의 저장 허용 모드에서 슬롯 1~4의 실제 저장을 요청합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_SaveLoad_Save;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장/로드 화면 제목 TextBlock입니다. 현재 저장 허용 모드에 맞춰 표시합니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_SaveLoad_Title;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="선택 슬롯 상세 TextBlock입니다. 실제 DisplaySlots 선택 정보를 표시합니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_SaveLoad_Detail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장/로드 실패/완료 안내 TextBlock입니다. 서비스 결과를 그대로 안내합니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_SaveLoad_Notice;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="선택 저장의 PNG Image입니다. 없는 썸네일/손상 정보를 정상 사진처럼 대체하지 않습니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Thumbnail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 1번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Party0;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 2번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Party1;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 3번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Party2;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 4번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Party3;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 5번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveLoad_Party4;
    UPROPERTY(EditDefaultsOnly, Category="IC|Save UI", meta=(ToolTip="UICLoadConfirmWidget 자식 확인 WBP 클래스입니다. 기존 WBP를 지정하며 런타임 인스턴스와 다릅니다.\n입력 자료형: 클래스 선택 (UICLoadConfirmWidget).")) TSubclassOf<UICLoadConfirmWidget> ConfirmWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|Portraits", meta=(ToolTip="캐릭터 ID→초상화 Texture2D 맵입니다. 슬롯/상세 UI의 PartyIds와 같은 ID를 사용합니다.\n입력 자료형: 이름/ID → 객체 참조 (UTexture2D) 맵.")) TMap<FName, TObjectPtr<UTexture2D>> PortraitTextures;
    UPROPERTY(EditDefaultsOnly, Category="IC|Save UI", meta=(ToolTip="UICSaveSlotWidget 자식 기존 슬롯 WBP 클래스입니다. 실제 슬롯 정보로 각 행을 구성합니다.\n입력 자료형: 클래스 선택 (UICSaveSlotWidget).")) TSubclassOf<UICSaveSlotWidget> SlotWidgetClass;
private:
    UPROPERTY(meta=(ToolTip="생성된 슬롯 WBP 인스턴스 배열입니다. 목록 갱신 때 이전 위젯/구독을 정리합니다.\n입력 자료형: 객체 참조 (UICSaveSlotWidget) 배열.")) TArray<TObjectPtr<UICSaveSlotWidget>> SlotRows;
    UPROPERTY(meta=(ToolTip="실제 서비스에서 조회한 화면 슬롯 목록입니다. 디스크 원본을 대체하지 않고 선택/유효 여부를 표시합니다.\n입력 자료형: 구조/객체 (FICSaveSlotInfo) 배열.")) TArray<FICSaveSlotInfo> DisplaySlots;
    UPROPERTY(meta=(ToolTip="슬롯 PNG에서 생성한 Texture2D 수명 유지 배열입니다. 새 목록 갱신/화면 종료 시 이전 참조를 정리합니다.\n입력 자료형: 객체 참조 (UTexture2D) 배열.")) TArray<TObjectPtr<UTexture2D>> ThumbnailTextures;
    UPROPERTY(meta=(ToolTip="현재 로드 확인 WBP 인스턴스입니다. 확인/취소/제거 때 참조와 구독을 정리합니다.\n입력 자료형: 객체 참조 (UICLoadConfirmWidget).")) TObjectPtr<UICLoadConfirmWidget> ConfirmScreen;
    int32 PendingLoadSlot = INDEX_NONE;
    void PerformLoad();
    UFUNCTION(meta=(ToolTip="선택 슬롯 불러오기 확인 이후의 내부 콜백입니다. 실제 로드 성공 후 필요하면 설정한 플레이 레벨로 이동합니다.")) void HandleLoadConfirmed();
    UFUNCTION(meta=(ToolTip="불러오기 확인 취소의 내부 콜백입니다. 로드 없이 현재 슬롯 화면으로 복귀합니다.")) void HandleLoadCancelled();
    TSoftObjectPtr<UWorld> GameplayMap;
    bool bAllowSave = false;
    bool bWaitingForSave = false;
    int32 SelectedSlot = INDEX_NONE;
    int32 PendingOverwriteSlot = INDEX_NONE;
    UICSaveSubsystem* GetSaves() const;
    void UpdateSelection();
    void SetNotice(const FText& Text);
    UFUNCTION(meta=(ToolTip="저장 슬롯 선택의 내부 콜백입니다. 선택 슬롯 인덱스와 상세 미리보기를 갱신합니다.\n@param SlotIndex 저장 슬롯의 번호입니다. 0자동 저장, 1~4수동 저장입니다. 입력 핀. 자료형: 정수.")) void HandleSlotPicked(int32 SlotIndex);
    UFUNCTION(meta=(ToolTip="불러오기 버튼의 내부 콜백입니다. 현재 선택 슬롯을 검사하고 필요한 확인 화면/로드 흐름을 요청합니다.")) void HandleLoadClicked();
    UFUNCTION(meta=(ToolTip="저장 버튼의 내부 콜백입니다. 저장 허용·선택 슬롯·안전 상태를 검사하고 비동기 저장을 요청합니다.")) void HandleSaveClicked();
    UFUNCTION(meta=(ToolTip="저장/불러오기 닫기 버튼의 내부 콜백입니다. 화면과 이벤트 구독 및 해당 정지 요청을 정리합니다.")) void HandleCloseClicked();
    UFUNCTION(meta=(ToolTip="저장 완료 이벤트를 받는 위젯 내부 콜백입니다. 실제 성공 여부로 슬롯 미리보기·버튼·메시지를 갱신합니다.\n@param SlotIndex 저장 슬롯의 번호입니다. 0자동 저장, 1~4수동 저장입니다. 입력 핀. 자료형: 정수.\n@param bSuccess 실제 처리 성공이면 true입니다. 비동기 저장의 완료 결과를 확인하는 기준입니다. 입력 핀. 자료형: 체크박스(true/false).\n@param Message 처리 결과 안내 문구입니다. 실제 성공 여부와 함께 해석하세요. 입력 핀. 자료형: 문자열.")) void HandleSaveFinished(int32 SlotIndex, bool bSuccess, const FString& Message);
};
