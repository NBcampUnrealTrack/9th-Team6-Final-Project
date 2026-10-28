#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Rows/World/ICMapRow.h"
#include "Data/Rows/Presentation/ICMapPresentationRow.h"
#include "ICMapWidget.generated.h"

class AICWarpStone;
class UWidgetTree;
class UTexture2D;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICMapWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") bool bMinimap=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") TSoftObjectPtr<UDataTable> MapTable;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") TSoftObjectPtr<UDataTable> PresentationTable;
    UFUNCTION(BlueprintCallable,Category="Map") bool RequestTravel(FName WarpID);
    UFUNCTION(BlueprintPure,Category="Map") bool IsTraveling() const{return bTraveling;}
    UFUNCTION(BlueprintPure,Category="Map") FVector2D WorldToMapUV(FVector Position) const;
    static void BuildLayout(UWidgetTree* Tree,bool Mini);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
    FICMapLayerDefinition Layer;
    FICMapPresentationRow Rules;
    TArray<TWeakObjectPtr<AICWarpStone>> Portals;
    bool bTraveling=false,bMoved=false,bRegistered=false;
    float TravelElapsed=0,RefreshElapsed=1;
    int32 SelectedPortal=0;
    FName PendingWarp;
    FText Status;
    void RefreshMap();
    FSlateRect ImageRect(const FGeometry&) const;
    FVector2D MarkerPosition(const FSlateRect&,FVector) const;
    void RefreshLabels();
    UFUNCTION() void CloseMap();
};
