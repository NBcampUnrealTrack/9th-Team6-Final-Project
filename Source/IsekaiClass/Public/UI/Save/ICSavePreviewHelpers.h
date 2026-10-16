#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateTypes.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
namespace ICSavePreviewUI
{
inline FText SlotName(int32 Slot) { IC_TRACE_FUNCTION(); return Slot == 0 ? NSLOCTEXT("ICSaveUI", "AutoSlot", "자동 저장") : FText::Format(NSLOCTEXT("ICSaveUI", "ManualSlot", "수동 슬롯 {0}"), FText::AsNumber(Slot)); }
// Save timestamps remain UTC on disk; display them in the player's local timezone.
inline FText Date(const FICSaveSlotInfo& Info)
{
    IC_TRACE_FUNCTION();
    // Locale-aware ICU skeletons change separators; format this fixed game UI explicitly.
    const double OffsetMinutes=FMath::RoundToDouble((FDateTime::Now()-FDateTime::UtcNow()).GetTotalMinutes());
    return FText::FromString((Info.SavedAt+FTimespan::FromMinutes(OffsetMinutes)).ToString(TEXT("%y.%m.%d %H:%M")));
}
inline FText GameTime(const FICSaveSlotInfo& Info)
{
    IC_TRACE_FUNCTION();
    if(!Info.Clock.IsValid())return NSLOCTEXT("ICSaveUI","InvalidClock","게임 시간 정보 없음");
    const double WholeMinute=FMath::FloorToDouble(Info.Clock.GameMinute);
    const FString Day=FString::Printf(TEXT("%.0f"),FMath::FloorToDouble(WholeMinute/1440)+1);
    const int32 Minute=static_cast<int32>(FMath::Fmod(WholeMinute,1440.0));
    const int32 Hour=Minute/60;const int32 DisplayHour=Hour%12==0?12:Hour%12;
    return FText::FromString(FString::Printf(TEXT("이세계 %s일차 %s %02d:%02d"),*Day,Hour<12?TEXT("오전"):TEXT("오후"),DisplayHour,Minute%60));
}
inline FText Recovery(const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION();return Info.bUsesBackup?NSLOCTEXT("ICSaveUI","RecoveryClock","백업 복구 · 표시된 날짜와 게임 시간은 실제 복구할 백업 기준입니다."):FText::GetEmpty(); }
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
