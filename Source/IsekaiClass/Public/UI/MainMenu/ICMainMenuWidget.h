#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICMainMenuWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UWorld;
class UICSaveLoadWidget;

USTRUCT(BlueprintType, meta=(ToolTip="기존 메인 메뉴 버튼의 보통/호버/누름/비활성 이미지 묶음입니다. 각각 필요할 때 연결해 버튼 스타일을 구성합니다.", ShortToolTip="기존 메인 메뉴 버튼의 보통/호버/누름/비활성 이미지 묶음입니다."))
struct FICMainMenuImages
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly, Category="Images", meta=(ToolTip="버튼 보통 상태 Texture2D입니다. 실제 버튼 스타일 기본 이미지입니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> Normal;
    UPROPERTY(EditDefaultsOnly, Category="Images", meta=(ToolTip="호버 상태 버튼 Texture2D입니다. 마우스/키보드 선택 표현에 사용하는 실제 이미지입니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> Hover;
    UPROPERTY(EditDefaultsOnly, Category="Images", meta=(ToolTip="버튼 누름 상태 Texture2D입니다. 실제 버튼 스타일에서 사용합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> Pressed;
    UPROPERTY(EditDefaultsOnly, Category="Images", meta=(ToolTip="비활성 버튼 Texture2D입니다. 현재 버튼 스타일이 비활성 상태에서 사용하는 이미지입니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> Disabled;
};

UCLASS(Abstract, Blueprintable, meta=(ToolTip="기존 메인 메뉴의 시작·계속·로드·설정·종료 UI 기반입니다. 실제 저장 슬롯 목록을 조회하고 확인 팝업 후 각 동작을 요청합니다.", ShortToolTip="기존 메인 메뉴의 시작·계속·로드·설정·종료 UI 기반입니다."))
class ISEKAICLASS_API UICMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UICMainMenuWidget(const FObjectInitializer& ObjectInitializer);
    // Existing controller calls this name; prefer Continue when it is available.
    UFUNCTION(BlueprintCallable, Category="Main Menu", meta=(ToolTip="메인 메뉴 시작 버튼에 UI 포커스를 설정합니다.")) void FocusStartButton();
    UFUNCTION(BlueprintCallable, Category="Main Menu", meta=(ToolTip="유효한 이어하기 저장이 있는지 확인해 이어하기 버튼의 활성/비활성 표시를 갱신합니다.")) void RefreshContinueAvailability();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu", meta=(ToolTip="새 게임/계속하기에서 열 실제 플레이 UWorld 에셋입니다. 없으면 전환을 수행하지 않고 설정 누락을 안내합니다.\n입력 자료형: Soft Object 에셋 참조 (UWorld).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSoftObjectPtr<UWorld> StartMap;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu", meta=(ToolTip="UICSaveLoadWidget 자식 기존 WBP 클래스입니다. 슬롯 목록/서비스 연결 기반이며 새 위젯 생성 설정이 아닙니다.\n입력 자료형: 클래스 선택 (UICSaveLoadWidget).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSubclassOf<UICSaveLoadWidget> SaveLoadWidgetClass;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="기존 WBP 계속 버튼 바인딩입니다. 동일 이름/타입의 Button을 Is Variable로 두며 실제 유효 저장이 있을 때만 활성화합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."), Category="Main Menu") TObjectPtr<UButton> BTN_Continue;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="기존 WBP 새 게임 버튼 바인딩입니다. StartMap으로 이동하기 전 현재 메모리 세션을 준비하며 디스크 저장 삭제와 구분합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."), Category="Main Menu") TObjectPtr<UButton> BTN_Start;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="기존 WBP 로드 버튼 바인딩입니다. 실제 저장/로드 목록을 열며 동일 이름/타입의 위젯을 연결합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."), Category="Main Menu") TObjectPtr<UButton> BTN_Load;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="기존 WBP 설정 버튼 바인딩입니다. 현재 설정 화면 실제 연결은 별도이므로 관련 BP 이벤트에 화면을 연결해야 합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."), Category="Main Menu") TObjectPtr<UButton> BTN_Settings;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="기존 WBP 종료 버튼 바인딩입니다. 확인/저장 후 종료 흐름을 요청하며 동일 이름을 유지합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."), Category="Main Menu") TObjectPtr<UButton> BTN_Quit;

    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 계속 버튼 이미지 바인딩입니다. 상태별 ContinueImages를 적용합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_Continue;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 새 게임 버튼 이미지 바인딩입니다. 상태별 StartImages를 적용합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_Start;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 로드 버튼 이미지 바인딩입니다. 상태별 LoadImages를 적용합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_Load;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 종료 버튼 이미지 바인딩입니다. 상태별 QuitImages를 적용합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_Quit;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 첫 안개 이미지 바인딩입니다. 배경 UI 애니메이션에 사용하며 게임 시간과 별개입니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_MainFogA;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 둘째 안개 이미지 바인딩입니다. 첫 이미지와 겹친 배경 연출에 사용합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_MainFogB;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="기존 WBP 배경 빛 이미지 바인딩입니다. GlowPeriodSeconds로 연출합니다.\n입력 자료형: 객체 참조 (UImage).")) TObjectPtr<UImage> IMG_MainGlow;
    UPROPERTY(meta=(BindWidgetOptional, ToolTip="메인 메뉴 안내 TextBlock 바인딩입니다. 설정 누락/저장/전환 실패 이유를 표시합니다.\n입력 자료형: 객체 참조 (UTextBlock).")) TObjectPtr<UTextBlock> IC_Main_Notice;

    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images", meta=(ToolTip="계속 버튼 보통/호버/누름/비활성 이미지 묶음입니다. 실제 저장 유효 여부로 버튼 상태를 바꿉니다.\n입력 자료형: 구조/객체 (FICMainMenuImages).")) FICMainMenuImages ContinueImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images", meta=(ToolTip="새 게임 버튼 상태별 Texture2D입니다. 실제 생성/맵 전환 가능 상태와 함께 사용합니다.\n입력 자료형: 구조/객체 (FICMainMenuImages).")) FICMainMenuImages StartImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images", meta=(ToolTip="로드 버튼 상태별 Texture2D 묶음입니다. 실제 목록/선택 가능 상태와 함께 사용합니다.\n입력 자료형: 구조/객체 (FICMainMenuImages).")) FICMainMenuImages LoadImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images", meta=(ToolTip="종료 버튼 상태별 이미지입니다. 확인/저장 후 종료 흐름과 실제 버튼 상태를 맞춥니다.\n입력 자료형: 구조/객체 (FICMainMenuImages).")) FICMainMenuImages QuitImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ToolTip="메인 메뉴 안개/빛 배경 애니메이션 허용입니다. 체크 시 NativeTick에서 연출하며 게임 시계와 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bAnimateBackground = true;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="1", ToolTip="메뉴 안개 연출 반복 실제 초입니다. bAnimateBackground 체크 시 UI Tick에 사용합니다.\n입력 자료형: 소수.\nC++ 초기값: 24.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.")) float FogPeriodSeconds = 24.f;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="1", ToolTip="메뉴 빛 연출 반복 실제 초입니다. 게임 시계/수면 점프와 무관한 UI 연출입니다.\n입력 자료형: 소수.\nC++ 초기값: 8.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.")) float GlowPeriodSeconds = 8.f;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="0", ClampMax="1", ToolTip="메뉴 화면 종료 페이드 실제 초입니다. 게임 PlayClock과 별개인 UI 연출입니다.\n입력 자료형: 소수.\nC++ 초기값: 0.25. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) float ExitFadeSeconds = 0.25f;

    // Kept so the archived legacy WBP remains compilable. New menu does not call these.
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy", meta=(ToolTip="메인 메뉴의 불러오기 요청을 자식 WBP로 전달하는 BP 이벤트입니다. 기존 저장 화면 연결과 중복으로 창을 열지 마세요.\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요.")) void OnLoadRequested();
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy", meta=(ToolTip="메인 메뉴의 설정 화면 요청을 자식 WBP에 전달하는 BP 이벤트입니다. 실제 설정 위젯을 구현해 연결해야 합니다.\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요.")) void OnSettingsRequested();
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy", meta=(ToolTip="새 게임 목적지 레벨이 비어 있을 때 호출되는 BP 안내 이벤트입니다. GameplayMap을 실제 플레이 레벨로 지정하세요.\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요.")) void OnStartMapNotConfigured();
private:
    enum class EExitAction : uint8 { None, NewGame, Continue, Quit };
    EExitAction PendingAction = EExitAction::None;
    float FadeElapsed = 0.f;
    double AnimationSeconds = 0.0;
    FVector2D FogBaseA = FVector2D::ZeroVector;
    FVector2D FogBaseB = FVector2D::ZeroVector;
    bool bContinueAvailable = false;
    bool bContinueBusy = false;
    UPROPERTY(meta=(ToolTip="열린 실제 저장/로드 WBP 인스턴스입니다. 닫기/전환 때 구독과 입력/정지 요청을 정리합니다.\n입력 자료형: 객체 참조 (UICSaveLoadWidget).")) TObjectPtr<UICSaveLoadWidget> SaveLoadScreen;
    UPROPERTY(meta=(ToolTip="메뉴 스타일에 로드한 Texture2D 강한 참조 캐시입니다. 이미지 수명을 유지하며 직접 원본 에셋 목록 대신 편집하지 않습니다.\n입력 자료형: 객체 참조 (UTexture2D) 배열.")) TArray<TObjectPtr<UTexture2D>> ResolvedImages;
    TArray<int32> LastImageStates;
    void ResolveButtonImages();
    void UpdateButtonImages();
    void UpdateAmbientImages();
    void SetNotice(const FText& Text);
    bool HasValidStartMap() const;
    void BeginExit(EExitAction Action);
    void CompleteExit();
    UFUNCTION(meta=(ToolTip="메인 메뉴 이어하기 버튼의 내부 콜백입니다. 유효 저장 로드와 플레이 레벨 이동 흐름을 요청합니다.")) void HandleContinueClicked();
    UFUNCTION(meta=(ToolTip="메인 메뉴 시작 버튼의 내부 콜백입니다. 설정한 플레이 레벨의 새 게임 생성 흐름으로 이동합니다.")) void HandleStartClicked();
    UFUNCTION(meta=(ToolTip="불러오기 버튼의 내부 콜백입니다. 현재 선택 슬롯을 검사하고 필요한 확인 화면/로드 흐름을 요청합니다.")) void HandleLoadClicked();
    UFUNCTION(meta=(ToolTip="메인 메뉴에서 연 저장/불러오기 화면이 닫힐 때의 내부 콜백입니다. 메뉴 포커스와 버튼 상태를 복구합니다.")) void HandleSaveLoadClosed();
    UFUNCTION(meta=(ToolTip="설정 버튼의 내부 콜백입니다. 설정 요청 BP 이벤트를 호출합니다.")) void HandleSettingsClicked();
    UFUNCTION(meta=(ToolTip="메인 메뉴 종료 버튼의 내부 콜백입니다. 현재 연결된 종료 흐름을 수행합니다.")) void HandleQuitClicked();
    UFUNCTION(meta=(ToolTip="저장 완료 이벤트를 받는 위젯 내부 콜백입니다. 실제 성공 여부로 슬롯 미리보기·버튼·메시지를 갱신합니다.\n@param SlotIndex 저장 슬롯의 번호입니다. 0자동 저장, 1~4수동 저장입니다. 입력 핀. 자료형: 정수.\n@param bSuccess 실제 처리 성공이면 true입니다. 비동기 저장의 완료 결과를 확인하는 기준입니다. 입력 핀. 자료형: 체크박스(true/false).\n@param Message 처리 결과 안내 문구입니다. 실제 성공 여부와 함께 해석하세요. 입력 핀. 자료형: 문자열.")) void HandleSaveFinished(int32 SlotIndex, bool bSuccess, const FString& Message);
};
