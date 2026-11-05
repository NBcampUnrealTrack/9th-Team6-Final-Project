#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "Data/Rows/UI/ICPrologueRow.h"
#include "ICPrologueWidget.generated.h"

class AICPlayerController;
class UTextBlock;
class IInputProcessor;
class UICRadialCooldownBar;
UCLASS()
class ISEKAICLASS_API UICPrologueWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    static UICPrologueWidget* Show(AICPlayerController* PC,bool RevealWorld=false);
    UFUNCTION(BlueprintPure) int32 GetLineIndex() const { return LineIndex; }
    UFUNCTION(BlueprintPure) bool IsLineComplete() const { return State==EState::Reading&&Script.IsValidIndex(LineIndex)&&Characters>=Script[LineIndex].Text.ToString().Len(); }
    UFUNCTION(BlueprintPure) int32 GetLineCount() const { return Script.Num(); }
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Prologue") TSoftObjectPtr<UDataTable> ScriptTable;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Prologue") TSoftObjectPtr<UDataTable> PresentationTable;
    bool Advance();
    bool HandleProgressKey(const FKeyEvent& Event);
    bool HandleProgressKeyUp(const FKeyEvent& Event);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void GoBack() override {}
private:
    friend struct FICPrologueTestAccess;
    enum class EState : uint8 { FadeIn,Reading,FadeOut,Revealing,Traveling };
    EState State=EState::FadeIn;
    int32 LineIndex=0,Characters=0;
    float Elapsed=0,Typing=0;
    float SkipElapsed=0;
    float BaseFontSize=27;
    bool bSkipHeld=false,bSoundPlayed=false;
    UPROPERTY(Transient) TArray<FICPrologueRow> Script;
    FICProloguePresentationRow Presentation;
    void StartLine();
    void FinishReading();
    TSharedPtr<IInputProcessor> ProgressInput;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LineText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Prompt;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SkipPrompt;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UICRadialCooldownBar> SkipGauge;
};
