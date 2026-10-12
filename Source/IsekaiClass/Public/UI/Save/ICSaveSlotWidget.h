#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveSlotWidget.generated.h"
class UButton;
class UTextBlock;
class UTexture2D;
class UImage;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICSlotPicked, int32, SlotIndex);
UCLASS(Blueprintable, meta=(ToolTip="기존 슬롯 한 칸의 표시/선택 WBP 기반입니다. 빈 슬롯·정상 저장·손상/복구 가능한 저장을 구분하고 썸네일·이름·위치·플레이 시간을 보여줍니다.", ShortToolTip="기존 슬롯 한 칸의 표시/선택 WBP 기반입니다."))
class ISEKAICLASS_API UICSaveSlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    int32 GetSlotIndex() const { IC_TRACE_FUNCTION();return SlotIndex; }
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="슬롯 선택 이벤트입니다. 선택 번호로 상세 화면을 갱신하며 클릭만으로 파일 삭제/덮어쓰기를 확정하지 않습니다.\n입력 자료형: FICSlotPicked.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICSlotPicked OnSlotPicked;
    void Configure(int32 InSlot, const FText& InLabel);
    void SetSelected(bool bSelected);
    void ConfigureData(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail, const TMap<FName, TObjectPtr<UTexture2D>>& Portraits);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="슬롯 선택 Button입니다. 클릭 시 OnSlotPicked로 Slot 번호를 알리고 실제 파일 변경은 별도 확정합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_SaveSlot_Button;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="슬롯 이름/시각/상태 TextBlock입니다. 빈/손상/백업 정보를 Info로 표시합니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_SaveSlot_Label;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="슬롯 한 칸 PNG 미리보기 Image입니다. 런타임 Thumbnail 참조를 표시합니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Thumbnail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 1번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Party0;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 2번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Party1;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 3번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Party2;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 4번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Party3;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="저장 미리보기의 5번째 파티 초상화 Image 바인딩입니다. 저장 미리보기에 기록된 파티 순서와 외형 인덱스를 사용합니다. 해당 인원이 없거나 슬롯이 비어 있으면 이미지를 비웁니다. WBP 디자이너에서 이 이름과 같은 Image를 만들 때 자동 연결됩니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_SaveSlot_Party4;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images", meta=(ToolTip="슬롯 보통 Texture2D입니다. 선택/누름/호버 외 기본 스타일입니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> IC_Load_Slot_Normal;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images", meta=(ToolTip="슬롯 호버 Texture2D입니다. 실제 선택 상태와 구분한 버튼 스타일입니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> IC_Load_Slot_Hover;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images", meta=(ToolTip="슬롯 누름 Texture2D입니다. 클릭/키 입력 중 표시합니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> IC_Load_Slot_Pressed;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images", meta=(ToolTip="슬롯 선택 Texture2D입니다. 현재 선택 상태의 강조 이미지입니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> IC_Load_Slot_Selected;
    UFUNCTION(BlueprintImplementableEvent, meta=(ToolTip="저장 슬롯의 선택 여부가 바뀔 때 호출되는 BP 표시 이벤트입니다. 자식 WBP에서 강조 테두리 등 표시를 갱신하세요.\n@param bSelected 현재 슬롯이 선택되어 있으면 true입니다. 입력 핀. 자료형: 체크박스(true/false).\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요.")) void OnSelectionChanged(bool bSelected);
private:
    int32 SlotIndex = INDEX_NONE;
    FText Label;
    bool bIsSelected = false;
    UPROPERTY(meta=(ToolTip="현재 표시/확인하는 실제 저장 슬롯 정보입니다. Slot/bExists/bValid/bUsesBackup를 확인하고 로드 성공을 별도로 검사합니다.\n입력 자료형: 구조/객체 (FICSaveSlotInfo).")) FICSaveSlotInfo Info;
    UPROPERTY(meta=(ToolTip="현재 PNG에서 생성한 표시 Texture2D입니다. 저장 원본 바이트와 별개인 런타임 참조입니다.\n입력 자료형: 객체 참조 (UTexture2D).")) TObjectPtr<UTexture2D> Thumbnail;
    UPROPERTY(meta=(ToolTip="캐릭터 ID→초상화 Texture2D 맵입니다. 슬롯/상세 UI의 PartyIds와 같은 ID를 사용합니다.\n입력 자료형: 이름/ID → 객체 참조 (UTexture2D) 맵.")) TMap<FName, TObjectPtr<UTexture2D>> PortraitTextures;
    void ApplyData();
    UFUNCTION(meta=(ToolTip="저장 슬롯 버튼의 내부 클릭 콜백입니다. 자신의 슬롯 인덱스로 선택 이벤트를 발행합니다.")) void HandleClicked();
};
