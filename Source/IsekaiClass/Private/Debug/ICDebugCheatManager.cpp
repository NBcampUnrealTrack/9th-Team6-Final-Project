#include "Debug/ICDebugCheatManager.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"

void UICDebugCheatManager::ICAddGameMinutes(double Minutes)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    UE_LOG(LogICCore, Log, TEXT("[Cheat.Time] Minutes=%.2f"),Minutes);
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
        PC->Show(PC->GetTime() && PC->GetTime()->JumpGameMinutes(Minutes) ? TEXT("게임 시간 점프 완료") : TEXT("0 이상의 유효한 분을 입력하세요."));
#endif
}

void UICDebugCheatManager::ICGiveItem(FName ItemId, int32 Count)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    UE_LOG(LogICInventory, Log, TEXT("[Cheat.GiveItem] Item=%s Count=%d"),*ItemId.ToString(),Count);
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
    {
        auto* Inventory = PC->GetInventory();
        if (!Inventory || !Inventory->Find(ItemId) || Count <= 0) { PC->Show(TEXT("알려진 아이템 ID와 양의 수량을 입력하세요.")); return; }
        const int32 Left = Inventory->TryAddItem(ItemId, Count);
        PC->Show(FString::Printf(TEXT("아이템 지급: 수납 %d, 잔량 %d"), Count - Left, Left));
    }
#endif
}

void UICDebugCheatManager::ICSetAffinity(FName NpcId, int32 Value)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    UE_LOG(LogICWorldState, Log, TEXT("[Cheat.Affinity] NPC=%s Value=%d"),*NpcId.ToString(),Value);
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
    {
        auto* State = PC->GetWorldState();
        if (!State || !UICWorldStateSubsystem::IsCharacterIdValid(NpcId) || NpcId == UICWorldStateSubsystem::PlayerId() || Value < 0 || Value > 100)
        { PC->Show(TEXT("유효한 NPC ID와 0~100 호감도를 입력하세요.")); return; }
        const int32 Serial = FCString::Atoi(*NpcId.ToString().Right(3));
        const EICCharacterKind Kind = Serial < 100 ? EICCharacterKind::Recruitable : Serial < 200 ? EICCharacterKind::Resident : EICCharacterKind::Background;
        if (Kind == EICCharacterKind::Background || !State->EnsureCharacter(NpcId, Kind) || !State->SetAffinity(NpcId, Value))
        { PC->Show(TEXT("해당 NPC 상태를 변경할 수 없습니다.")); return; }
        PC->Show(TEXT("NPC 원본 호감도 변경 완료"));
    }
#endif
}

void UICDebugCheatManager::ICTeleport(float X, float Y, float Z)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    UE_LOG(LogICCore, Log, TEXT("[Cheat.Teleport] X=%.2f Y=%.2f Z=%.2f"),X,Y,Z);
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
    {
        if (!PC->GetPawn() || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z)) { PC->Show(TEXT("유효한 좌표를 입력하세요.")); return; }
        PC->GetPawn()->SetActorLocation(FVector(X, Y, Z)); PC->Show(TEXT("개발자 순간이동 완료"));
    }
#endif
}
