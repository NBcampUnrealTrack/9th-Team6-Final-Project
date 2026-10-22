#pragma once
#include "UI/Framework/ICText.h"
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateTypes.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
namespace ICSavePreviewUI
{
inline FText SlotName(int32 Slot) { IC_TRACE_FUNCTION(); return Slot == 0 ? ICText::Get(TEXT("Native_f79024b509aa")) : FText::Format(ICText::Get(TEXT("Native_efbe5a4fda32")), FText::AsNumber(Slot)); }

inline FText Date(const FICSaveSlotInfo& Info)
{
    IC_TRACE_FUNCTION();

    const double OffsetMinutes=FMath::RoundToDouble((FDateTime::Now()-FDateTime::UtcNow()).GetTotalMinutes());
    return FText::AsDateTime(Info.SavedAt+FTimespan::FromMinutes(OffsetMinutes),
        EDateTimeStyle::Short,EDateTimeStyle::Short,FText::GetInvariantTimeZone());
}
inline FText PlayTime(const FICSaveSlotInfo& Info)
{
    IC_TRACE_FUNCTION();
    const int64 Seconds = Info.Clock.IsValid() ? FMath::FloorToInt64(FMath::Min(Info.Clock.PlaySecond, 9.e15)) : 0;
    return FText::FromString(FString::Printf(TEXT("%02lld:%02lld:%02lld"), Seconds / 3600, Seconds / 60 % 60, Seconds % 60));
}
inline FText Recovery(const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION();return Info.bUsesBackup?ICText::Get(TEXT("Native_139b2edfe429")):FText::GetEmpty(); }
inline FText Location(const FICSaveSlotInfo&) { IC_TRACE_FUNCTION(); return ICText::Get(TEXT("Save_Region1")); }
inline FText Summary(const FICSaveSlotInfo& Info)
{
    return FText::Format(ICText::Get(TEXT("Save_Summary")), FText::FromString(Info.PlayerName), Location(Info), PlayTime(Info), Date(Info));
}
inline void SetImage(UImage* Image, UTexture2D* Texture)
{
    IC_TRACE_FUNCTION();
    if (!Image) return;
    Image->SetBrushFromTexture(Texture, false);
    Image->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    if (auto* Parent = Cast<UOverlay>(Image->GetParent()))
        if (auto* Empty = Cast<UTextBlock>(Parent->GetChildAt(0)))
            Empty->SetVisibility(Texture ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
inline UTexture2D* Portrait(const FICSaveSlotInfo& Info, int32 Index, const TMap<FName, TObjectPtr<UTexture2D>>& Textures)
{
    IC_TRACE_FUNCTION();
    if (Index != 0 && !Info.Preview.PartyIds.IsValidIndex(Index)) return nullptr;
    FName Key = Index == 0 ? FName(TEXT("CHR_PLAYER")) : Info.Preview.PartyIds[Index];
    if (Key == TEXT("CHR_PLAYER")) Key = Info.Preset == 0 ? TEXT("CHR_PLAYER_MALE") : TEXT("CHR_PLAYER_FEMALE");
    const TObjectPtr<UTexture2D>* Found = Textures.Find(Key);
    return Found ? Found->Get() : nullptr;
}
}
