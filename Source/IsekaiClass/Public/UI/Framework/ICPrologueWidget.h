#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "ICPrologueWidget.generated.h"

class AICPlayerController;
class UTextBlock;
class IInputProcessor;
UCLASS()
class ISEKAICLASS_API UICPrologueWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    static UICPrologueWidget* Show(AICPlayerController* PC,bool RevealWorld=false);
    static const TArray<FString>& Lines();
    UFUNCTION(BlueprintPure) int32 GetLineIndex() const { return LineIndex; }
    UFUNCTION(BlueprintPure) bool IsLineComplete() const { return State==EState::Reading&&Characters>=Lines()[LineIndex].Len(); }
    bool Advance();
    bool HandleProgressKey(const FKeyEvent& Event);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual void GoBack() override {}
private:
    friend struct FICPrologueTestAccess;
    enum class EState : uint8 { FadeIn,Reading,FadeOut,Revealing,Traveling };
    EState State=EState::FadeIn;
    int32 LineIndex=0,Characters=0;
    float Elapsed=0,Typing=0;
    TSharedPtr<IInputProcessor> ProgressInput;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Prompt;
};
