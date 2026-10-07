#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateTypes.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
namespace ICSavePreviewUI
{
inline FText SlotName(int32 Slot) { IC_TRACE_FUNCTION(); return Slot == 0 ? NSLOCTEXT("ICSaveUI", "AutoSlot", "자동 저장") : FText::Format(NSLOCTEXT("ICSaveUI", "ManualSlot", "수동 슬롯 {0}"), FText::AsNumber(Slot)); }
inline FText Date(const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return FText::FromString(Info.SavedAt.ToString(TEXT("%Y.%m.%d %H:%M UTC"))); }
inline FText Location(const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return Info.Preview.Location.IsEmpty() ? NSLOCTEXT("ICSaveUI", "UnknownLocation", "위치 정보 없음") : FText::FromString(Info.Preview.Location); }
inline void SetImage(UImage* Image, UTexture2D* Texture)
{
    IC_TRACE_FUNCTION();
    if (!Image) return;
    Image->SetBrushFromTexture(Texture, false);
    Image->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}
inline UTexture2D* Portrait(const FICSaveSlotInfo& Info, int32 Index, const TMap<FName, TObjectPtr<UTexture2D>>& Textures)
{
    IC_TRACE_FUNCTION();
    if (!Info.Preview.PartyIds.IsValidIndex(Index)) return nullptr;
    FName Key = Info.Preview.PartyIds[Index];
    if (Key == TEXT("CHR_PLAYER")) Key = Info.Preset == 0 ? TEXT("CHR_PLAYER_MALE") : TEXT("CHR_PLAYER_FEMALE");
    const TObjectPtr<UTexture2D>* Found = Textures.Find(Key);
    return Found ? Found->Get() : nullptr;
}
}
