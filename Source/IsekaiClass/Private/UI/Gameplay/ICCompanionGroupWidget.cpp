#include "UI/Gameplay/ICCompanionGroupWidget.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/PanelWidget.h"
#include "Internationalization/StringTable.h"

void UICCompanionGroupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RefreshCompanions();
}

void UICCompanionGroupWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshCompanions();
}

void UICCompanionGroupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BoundState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (BoundState)
    {
        BoundState->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::CharacterChanged);
        BoundState->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::RefreshCompanions);
    }
    RefreshCompanions();
}

void UICCompanionGroupWidget::NativeDestruct()
{
    if (BoundState)
    {
        BoundState->OnCharacterChanged.RemoveDynamic(this, &ThisClass::CharacterChanged);
        BoundState->OnWorldRestored.RemoveDynamic(this, &ThisClass::RefreshCompanions);
        BoundState = nullptr;
    }
    Super::NativeDestruct();
}

void UICCompanionGroupWidget::CharacterChanged(FName CharacterID)
{
    if (CharacterID != UICWorldStateSubsystem::PlayerId()) RefreshCompanions();
}

void UICCompanionGroupWidget::RefreshCompanions()
{
    if (bRefreshing) return;
    TGuardValue<bool> Guard(bRefreshing, true);
    auto* GI = GetGameInstance();
    auto* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    auto* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    const FICWorldState Snapshot = State ? State->ExportState() : FICWorldState();
    FName Party[3] = {NAME_None, NAME_None, NAME_None};
    for (const auto& Pair : Snapshot.Characters)
        if (Pair.Value.Kind == EICCharacterKind::Recruitable && Pair.Value.Mode == EICCharacterMode::Companion &&
            Pair.Value.PartySlot >= 0 && Pair.Value.PartySlot < UE_ARRAY_COUNT(Party))
            Party[Pair.Value.PartySlot] = Pair.Key;

    auto* Names = Definitions ? LoadObject<UStringTable>(nullptr,
        TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")) : nullptr;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Party); ++Index)
    {
        auto* Card = Cast<UICCharacterStatusWidget>(GetWidgetFromName(FName(*FString::Printf(TEXT("Companion%d"), Index + 1))));
        if (!Card) continue;
        FICDefinitionResult Result;
        const auto* Definition = Definitions && !Party[Index].IsNone() ? Definitions->Find<FICNPCRow>(Party[Index], Result) : nullptr;
        if (Definition && State)
        {
            const auto Member = State->GetSnapshot(Party[Index]);
            const auto Name = UICTextLibrary::ResolveText(Names, Definition->NameKey);
            Card->CharacterID = Party[Index];
            Card->DisplayName = Name.bSuccess ? Name.Text : FText::FromName(Party[Index]);
            Card->Portrait = Definition->Portrait.LoadSynchronous();
            Card->UpdateVitals(Member);
            Card->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            Card->CharacterID = NAME_None;
            Card->DisplayName = FText::GetEmpty();
            Card->Portrait = nullptr;
            Card->HPPercent = Card->MPPercent = 0.f;
            Card->SetVisibility(ESlateVisibility::Collapsed);
        }
        // The size wrapper must collapse too, so empty slots leave no gap above the player.
        if (auto* Wrapper = Card->GetParent()) Wrapper->SetVisibility(Card->GetVisibility());
        Card->RefreshPresentation();
    }
}
