#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "ICCharacterCreateWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;
class UTexture2D;
class UImage;

UCLASS(Abstract, Blueprintable, meta=(ToolTip="기존 캐릭터 생성 WBP의 네이티브 연결 기반입니다. IC_Create 이름의 선택 위젯과 이벤트를 사용해 프리셋/직업/이름을 컨트롤러에 반영합니다. 새 위젯을 생성할 필요는 없습니다.", ShortToolTip="기존 캐릭터 생성 WBP의 네이티브 연결 기반입니다."))
class ISEKAICLASS_API UICCharacterCreateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void RefreshState();
    UFUNCTION(BlueprintCallable, Category="Character Creation", meta=(ToolTip="캐릭터 생성 화면의 0 기반 외형 선택 인덱스를 변경합니다. 선택 가능 상태와 유효 프리셋 범위를 검사합니다.\n@param Index AppearancePresets 목록의 0 기반 외형 인덱스입니다. 입력 핀. 자료형: 정수.")) void SelectPreset(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation", meta=(ToolTip="캐릭터 생성 화면의 직업 인덱스를 변경합니다. 0전사·1궁수·2마법사·3힐러이며 생성 확정 후에는 편집할 수 없습니다.\n@param Index 직업 선택 인덱스입니다. 0전사·1궁수·2마법사·3힐러입니다. 입력 핀. 자료형: 정수.")) void SelectJob(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation", meta=(ToolTip="현재 생성 위젯의 이름·외형·직업을 컨트롤러 생성 요청으로 제출합니다. 실제 생성 결과는 WorldState/단계 변경을 확인하세요.")) void CreateCharacter();
    UFUNCTION(BlueprintCallable, Category="Character Creation", meta=(ToolTip="캐릭터 생성 화면에서 저장 불러오기 화면을 요청합니다.")) void OpenLoadScreen();
    UFUNCTION(BlueprintCallable, Category="Character Creation", meta=(ToolTip="캐릭터 생성 화면에서 종료 확인 화면을 요청합니다.")) void OpenQuitScreen();
    UFUNCTION(BlueprintPure, Category="Character Creation", meta=(ToolTip="현재 생성 위젯의 외형 선택을 조회합니다.\n@return 현재 0 기반 외형 인덱스입니다. 자료형: 정수.")) int32 GetSelectedPreset() const;
    UFUNCTION(BlueprintPure, Category="Character Creation", meta=(ToolTip="현재 생성 위젯의 직업 선택을 조회합니다.\n@return 0전사·1궁수·2마법사·3힐러의 선택 인덱스입니다. 자료형: 정수.")) int32 GetSelectedJob() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="이름 EditableTextBox 바인딩입니다. 현재 이름/입력 허용 상태를 연결하며 1~12자 검사는 실제 생성 경로가 수행합니다.\n입력 자료형: 객체 참조 (UEditableTextBox).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UEditableTextBox> IC_Create_Name;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ToolTip="생성 확정 Button 바인딩입니다. 이름/직업/프리셋 검증 후 Controller 생성/최초 저장을 요청합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="생성 안내 TextBlock 바인딩입니다. 실패 이유/저장 상태를 표시하며 지속 상태로 사용하지 않습니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_Create_Notice;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="전사 선택 Button 바인딩입니다. 직업 인덱스 0을 선택합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Warrior;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="궁수 선택 Button 바인딩입니다. 같은 이름/타입·Is Variable을 유지하며 직업 인덱스 1을 선택합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Archer;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="마법사 선택 Button 바인딩입니다. 직업 인덱스 2를 선택합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Mage;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="힐러 선택 Button 바인딩입니다. 직업 인덱스 3을 선택합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Healer;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Job Images", meta=(ToolTip="선택 직업 표시 Texture2D입니다. 현재 선택 버튼/강조 연출에 사용합니다.\n입력 자료형: 객체 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTexture2D> IC_Create_Job_Selected;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="현재 선택 무기 Image 바인딩입니다. 직업별 무기 안내 텍스처를 표시합니다.\n입력 자료형: 객체 참조 (UImage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UImage> IC_Create_Weapon_Icon;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images", meta=(ToolTip="전사 무기 안내 Texture2D입니다. 직업 인덱스 0 선택 때 표시합니다.\n입력 자료형: 객체 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTexture2D> IC_Create_Weapon_Warrior;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images", meta=(ToolTip="궁수 무기 안내 Texture2D입니다. 직업 인덱스 1 선택 때 표시합니다.\n입력 자료형: 객체 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTexture2D> IC_Create_Weapon_Archer;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images", meta=(ToolTip="마법사 무기 안내 Texture2D입니다. 직업 인덱스 2 선택 때 표시합니다.\n입력 자료형: 객체 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTexture2D> IC_Create_Weapon_Mage;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images", meta=(ToolTip="힐러 무기 안내 Texture2D입니다. 직업 인덱스 3 선택 때 표시합니다.\n입력 자료형: 객체 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTexture2D> IC_Create_Weapon_Healer;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="선택 직업 무기 이름 TextBlock 바인딩입니다. 실제 시작 무기 표시 이름과 맞춥니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_Create_WeaponName;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="선택 직업 무기 설명 TextBlock 바인딩입니다. 실제 지급 정의/표시 설명과 맞춥니다.\n입력 자료형: 객체 참조 (UTextBlock).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UTextBlock> IC_Create_WeaponDescription;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional, ToolTip="생성 화면 뒤로 Button 바인딩입니다. MainMenuLevelName 전환/상태 정리를 실행합니다.\n입력 자료형: 객체 참조 (UButton).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UButton> IC_Create_Back;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Navigation", meta=(ToolTip="캐릭터 생성 화면에서 돌아갈 메인 메뉴 레벨 이름입니다. 기본 L_IC_MainMenu이며 실제 레벨 에셋/전환 경로를 확인합니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"L_IC_MainMenu\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName MainMenuLevelName = TEXT("L_IC_MainMenu");
    // Gender and other presentation can still be updated in Blueprint.
    UFUNCTION(BlueprintImplementableEvent, Category="Character Creation", meta=(ToolTip="외형·직업 선택이나 편집 가능 상태가 바뀔 때 호출되는 BP 표시 이벤트입니다. 자식 WBP에서 선택 테두리·미리보기·버튼 상태를 갱신하세요.\n@param Preset BP AppearancePresets 배열의 0 기반 외형 인덱스입니다. 입력 핀. 자료형: 정수.\n@param Job 생성 화면 직업 인덱스입니다. 0전사·1궁수·2마법사·3힐러입니다. 입력 핀. 자료형: 정수.\n@param bCanEdit 현재 생성 선택을 편집할 수 있으면 true입니다. 입력 핀. 자료형: 체크박스(true/false).\nBP 재정의 이벤트입니다. 자식 BP/WBP에서 표시 또는 실행을 구현하세요."))
    void OnCreationStateChanged(int32 Preset, int32 Job, bool bCanEdit);
private:
    TArray<FButtonStyle> DefaultJobStyles;
    void UpdateJobButtons(int32 Job, bool bCanEdit);
    UFUNCTION(meta=(ToolTip="전사 선택 버튼의 내부 클릭 콜백입니다. 직업 인덱스0을 선택합니다.")) void HandleWarriorClicked();
    UFUNCTION(meta=(ToolTip="궁수 선택 버튼의 내부 클릭 콜백입니다. 직업 인덱스1을 선택합니다.")) void HandleArcherClicked();
    UFUNCTION(meta=(ToolTip="마법사 선택 버튼의 내부 클릭 콜백입니다. 직업 인덱스2를 선택합니다.")) void HandleMageClicked();
    UFUNCTION(meta=(ToolTip="힐러 선택 버튼의 내부 클릭 콜백입니다. 직업 인덱스3을 선택합니다.")) void HandleHealerClicked();
    UFUNCTION(meta=(ToolTip="캐릭터 생성 뒤로 가기 버튼의 내부 콜백입니다. 현재 메뉴 흐름에 맞는 종료/취소 요청을 전달합니다.")) void HandleBackClicked();
    UFUNCTION(meta=(ToolTip="이름 입력 변경의 내부 콜백입니다. 컨트롤러의 현재 이름과 생성 가능 표시를 갱신합니다.\n@param Text 현재 표시/입력할 문구입니다. FText는 현지화 가능 텍스트, FString은 직접 문자열입니다. 입력 핀. 자료형: 현지화 가능한 텍스트.")) void HandleNameChanged(const FText& Text);
};
