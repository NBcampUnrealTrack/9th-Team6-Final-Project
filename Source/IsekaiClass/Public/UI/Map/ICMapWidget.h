#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Rows/World/ICMapRow.h"
#include "Data/Rows/Presentation/ICMapPresentationRow.h"
#include "ICMapWidget.generated.h"

class AICWarpStone;
class AActor;
class UTextBlock;
class UWidgetTree;
class UTexture2D;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICMapWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") bool bMinimap=false;
    /** How much of the map the HUD minimap magnifies around the player. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map|Minimap",meta=(ClampMin="1.0",ClampMax="8.0")) float MinimapZoom=4.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") TSoftObjectPtr<UDataTable> MapTable;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") TSoftObjectPtr<UDataTable> PresentationTable;
    UFUNCTION(BlueprintCallable,Category="Map") bool RequestTravel(FName WarpID);
    UFUNCTION(BlueprintPure,Category="Map") bool IsTraveling() const{return bTraveling;}
    UFUNCTION(BlueprintPure,Category="Map") FVector2D WorldToMapUV(FVector Position) const;
    UFUNCTION(BlueprintPure,Category="Map|Minimap") int32 GetTrackedBuildingCount() const;
    UFUNCTION(BlueprintPure,Category="Map|Minimap") int32 GetTrackedNPCCount() const;
    UFUNCTION(BlueprintPure,Category="Map|Minimap") int32 GetTrackedMonsterCount() const;
    static void BuildLayout(UWidgetTree* Tree,bool Mini);
    static float MinimapOpacity(FVector2D UV);
    static FVector2D MinimapDisplayUV(FVector2D MapUV,FVector2D CenterUV,float Zoom);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UUserWidget>> PortalMarkers;
    UPROPERTY(Transient) TObjectPtr<UWidget> PlayerMarker;
    UPROPERTY(Transient) TMap<TObjectPtr<AActor>,TObjectPtr<UTextBlock>> WorldActorMarkers;
    FICMapLayerDefinition Layer;
    FICMapPresentationRow Rules;
    FVector2D MinimapCenterUV=FVector2D(.5,.5);
    double MapMinimumZoom=1.,MapMaximumZoom=4.;
    TArray<TWeakObjectPtr<AICWarpStone>> Portals;
    /** Authored roots use IC.Map.Building; known environment-kit building cores are also detected automatically. */
    TArray<TWeakObjectPtr<AActor>> Buildings;
    TArray<TWeakObjectPtr<AActor>> NPCs;
    TArray<TWeakObjectPtr<AActor>> Monsters;
    bool bTraveling=false,bMoved=false,bRegistered=false;
    float TravelElapsed=0,RefreshElapsed=1;
    int32 SelectedPortal=0;
    FName PendingWarp;
    FText Status;
    void RefreshMap();
    void RefreshVisuals(const FGeometry& Geometry);
    FVector2D DisplayUV(FVector Position) const;
    FSlateRect ImageRect(const FGeometry&) const;
    FVector2D MarkerPosition(const FSlateRect&,FVector) const;
    void RefreshLabels();
    static bool IsAutomaticBuildingMeshPath(const FString& AssetPath);
    UFUNCTION() void CloseMap();
#if WITH_DEV_AUTOMATION_TESTS
    friend class FICMinimapWorldMarkerCheck;
#endif
};
