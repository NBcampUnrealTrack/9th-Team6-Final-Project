#include "ICDebugCheatManager.h"
#include "../Player/PlayerPrototypeController.h"
#include "../Player/PlayerInventory.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"

void UICDebugCheatManager::ICAddGameMinutes(double Minutes)
{
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<APlayerPrototypeController>(GetOuterAPlayerController()))
        PC->Show(PC->GetTime() && PC->GetTime()->JumpGameMinutes(Minutes) ? TEXT("게임 시간 점프 완료") : TEXT("0 이상의 유효한 분을 입력하세요."));
#endif
}

void UICDebugCheatManager::ICGiveItem(FName ItemId, int32 Count)
{
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<APlayerPrototypeController>(GetOuterAPlayerController()))
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
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<APlayerPrototypeController>(GetOuterAPlayerController()))
    {
        auto* State = PC->GetWorldState();
        if (!State || !UICWorldStateSubsystem::IsCharacterIdValid(NpcId) || NpcId == UICWorldStateSubsystem::PlayerId() || Value < 0 || Value > 100)
        { PC->Show(TEXT("유효한 NPC ID와 0~100 호감도를 입력하세요.")); return; }
        const int32 Serial = FCString::Atoi(*NpcId.ToString().Right(3));
        const EICCharacterKind Kind = Serial < 100 ? EICCharacterKind::Recruitable : Serial < 200 ? EICCharacterKind::Resident : EICCharacterKind::Background;
        if (Kind == EICCharacterKind::Background || !State->EnsureCharacter(NpcId, Kind) || !State->SetAffinity(NpcId, Value))
        { PC->Show(TEXT("해당 NPC 상태를 변경할 수 없습니다.")); return; }
        PC->Show(TEXT("호감도 상태 변경 완료. NPC 액터/영입은 담당 연결 후 확인하세요."));
    }
#endif
}

void UICDebugCheatManager::ICTeleport(float X, float Y, float Z)
{
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<APlayerPrototypeController>(GetOuterAPlayerController()))
    {
        if (!PC->GetPawn() || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z)) { PC->Show(TEXT("유효한 좌표를 입력하세요.")); return; }
        PC->GetPawn()->SetActorLocation(FVector(X, Y, Z)); PC->Show(TEXT("테스트 순간이동 완료"));
    }
#endif
}
