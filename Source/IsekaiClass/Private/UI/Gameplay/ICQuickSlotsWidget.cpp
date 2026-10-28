#include "UI/Gameplay/ICQuickSlotsWidget.h"
#include "UI/Gameplay/ICQuickSlotWidget.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"

void UICQuickSlotsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RefreshQuickSlots();
}

void UICQuickSlotsWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshQuickSlots();
}

void UICQuickSlotsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BoundState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (BoundState)
    {
        BoundState->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuickSlots);
        BoundState->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::RefreshQuickSlots);
    }
    RefreshQuickSlots();
}

void UICQuickSlotsWidget::NativeDestruct()
{
    if (BoundState)
    {
        BoundState->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshQuickSlots);
        BoundState->OnWorldRestored.RemoveDynamic(this, &ThisClass::RefreshQuickSlots);
        BoundState = nullptr;
    }
    Super::NativeDestruct();
}

void UICQuickSlotsWidget::RefreshQuickSlots()
{
    auto* GI = GetGameInstance();
    auto* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    auto* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    const FICInventoryState Inventory = State ? State->GetInventorySnapshot() : FICInventoryState();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Card = Cast<UICQuickSlotWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("QuickSlot%d"), Index + 1))));
        if (!Card) continue;
        Card->SetSlotNumber(Index+1);
        const FName ID = Inventory.QuickSlots.IsValidIndex(Index) ? Inventory.QuickSlots[Index] : NAME_None;
        int64 Count = 0;
        if (!ID.IsNone())
            for (const auto& Stack : Inventory.Bag.Slots)
                if (Stack.ItemID == ID && Stack.Count > 0 && !Stack.bLocked) Count += Stack.Count;
        FICDefinitionResult Result;
        const auto* Item = Definitions && Count > 0 ? Definitions->Find<FICItemRow>(ID, Result) : nullptr;
        const bool bUsable = Item && (Item->Kind == EPlayerItemKind::Consumable || Item->Kind == EPlayerItemKind::Expansion);
        Card->SetItem(bUsable?ID:NAME_None, bUsable?static_cast<int32>(FMath::Min<int64>(Count, MAX_int32)):0, bUsable ? Item->Icon.LoadSynchronous() : nullptr);
    }
}
