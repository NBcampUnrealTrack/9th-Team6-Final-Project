#include "Debug/ICDebugCheatManager.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/LifeNPC/ICShopSubsystem.h"
#include "Quest/ICQuestSubsystem.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "NPC/LifeObject/ICLifeObjectSubsystem.h"
#include "HAL/IConsoleManager.h"

void UICDebugCheatManager::ICCreateTestPlayer()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
        PC->Show(PC->DebugCreatePlayer() ? TEXT("테스트 플레이어 생성 및 자동 저장을 요청했습니다.") : TEXT("테스트 플레이어 생성 요청이 거부되었습니다. Output Log의 Player.Create.Rejected를 확인하세요."));
#endif
}

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

void UICDebugCheatManager::ICAddGameHours(double Hours)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    if (!PC) return;
    const double Minutes = Hours * 60.;
    auto* Time = PC->GetTime();
    if (!FMath::IsFinite(Hours) || Hours < 0 || !FMath::IsFinite(Minutes)
        || !Time || !Time->JumpGameMinutes(Minutes))
    {
        PC->ShowLocalized(NSLOCTEXT("ICCheat", "InvalidGameHours", "0 이상의 유효한 시간을 입력하세요. 예: ICAddGameHours 2"));
        return;
    }
    UE_LOG(LogICTime, Log, TEXT("[Cheat.TimeHours] Hours=%.2f Minutes=%.2f GameMinute=%.2f"), Hours, Minutes, Time->GetClock().GameMinute);
    PC->ShowLocalized(FText::Format(NSLOCTEXT("ICCheat", "GameHoursAdded", "게임시간 {0}시간 경과"), FText::AsNumber(Hours)));
#endif
}

void UICDebugCheatManager::ICGiveStarterWeapons()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    if (!PC) return;
    auto* Inventory = PC->GetInventory();
    auto* State = PC->GetWorldState();
    if (!Inventory || !Inventory->GetDefinitions() || !State || !State->ExportState().bPlayerCreated)
    {
        PC->ShowLocalized(NSLOCTEXT("ICCheat", "PlayerNotReady", "플레이어를 생성한 뒤 사용하세요."));
        return;
    }
    auto* Staging = NewObject<UICInventoryComponent>(this);
    Staging->BeginStaging(Inventory->GetDefinitions(), Inventory->CurrentClass);
    if (!Staging->ApplySnapshot(Inventory->GetSnapshot())) return;
    TArray<FICItemAmount> Weapons;
    for (const auto* ID : {TEXT("EQP_WPN_SWORD_001"), TEXT("EQP_WPN_BOW_001"), TEXT("EQP_WPN_STAFF_001"), TEXT("EQP_WPN_WAND_001")})
    {
        if (!Staging->AddItem(ID, 1, false))
        {
            PC->ShowLocalized(NSLOCTEXT("ICCheat", "StarterWeaponsNoRoom", "초보 무기 4종을 넣을 가방 공간이 부족하거나 아이템 정의를 확인할 수 없습니다."));
            return;
        }
        auto& Weapon = Weapons.AddDefaulted_GetRef(); Weapon.ItemID = ID; Weapon.Count = 1;
    }
    if (!Inventory->ApplySnapshot(Staging->GetSnapshot())) return;
    State->OnItemsReceived.Broadcast(Weapons);
    UE_LOG(LogICInventory, Log, TEXT("[Cheat.GiveStarterWeapons] Sword=1 Bow=1 Staff=1 Wand=1"));
    PC->ShowLocalized(NSLOCTEXT("ICCheat", "StarterWeaponsGranted", "초보 무기 4종 지급 완료 · 검, 활, 마법스태프, 치유완드 각 1개"));
#endif
}

void UICDebugCheatManager::ICGiveGold()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC=Cast<AICPlayerController>(GetOuterAPlayerController());if(!PC)return;
    auto* State=PC->GetWorldState();
    if(!State||!State->ExportState().bPlayerCreated)
    {PC->ShowLocalized(NSLOCTEXT("ICCheat","PlayerNotReady","플레이어를 생성한 뒤 사용하세요."));return;}
    const int64 Total=static_cast<int64>(State->ExportState().Gold)+10000;
    if(Total>MAX_int32||!PC->SetGold(static_cast<int32>(Total)))
    {PC->ShowLocalized(NSLOCTEXT("ICCheat","GoldLimit","보유 골드 한도를 초과하여 지급할 수 없습니다."));return;}
    PC->ShowLocalized(NSLOCTEXT("ICCheat","GoldGranted","10,000G 지급 완료"));
    UE_LOG(LogICInventory,Log,TEXT("[Cheat.GiveGold] Added=10000 Total=%d"),PC->Gold);
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

void UICDebugCheatManager::ICCompleteActiveQuests()
{
#if !UE_BUILD_SHIPPING
    if (auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController()))
    {
        UGameInstance* GameInstance = PC->GetGameInstance();
        UICQuestSubsystem* Quests = GameInstance ? GameInstance->GetSubsystem<UICQuestSubsystem>() : nullptr;
        const int32 CompletedCount = Quests ? Quests->ForceCompleteActiveQuests() : 0;
        UE_LOG(LogICWorldState, Log, TEXT("[Cheat.CompleteActiveQuests] Completed=%d"), CompletedCount);
        PC->Show(FString::Printf(TEXT("진행 중인 퀘스트 %d개 완료"), CompletedCount));
    }
#endif
}

void UICDebugCheatManager::ICInteractNearestNPC()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    if (!PC || !PC->GetPawn() || !PC->GetWorld()) return;
    AICNPCBase* Nearest = nullptr; double Best = FMath::Square(500.);
    for (TActorIterator<AICNPCBase> It(PC->GetWorld()); It; ++It)
    { const double Distance = FVector::DistSquared(It->GetActorLocation(), PC->GetPawn()->GetActorLocation()); if (Distance < Best) { Best = Distance; Nearest = *It; } }
    if (!Nearest) { PC->Show(TEXT("500cm 이내에 NPC가 없습니다.")); return; }
    const TArray<FName> Services = Nearest->GetAvailableServices();
    if (!Nearest->TryInteract(PC)) { PC->Show(TEXT("NPC가 상호작용을 거절했습니다. (쓰러짐 / 동료 모드 / 이미 상호작용 중)")); return; }
    DebugInteractingNPC = Nearest;
    const FString ServiceText = Services.Num() > 0 ? Services[0].ToString() : FString(TEXT("(없음)"));
    FString OptionText;
    for (const auto& Option : Nearest->GetInteractionOptions()) OptionText += FString::Printf(TEXT("[%s%s] "), *Option.Label.ToString(), Option.bEnabled ? TEXT("") : TEXT(" 비활성"));
    PC->Show(FString::Printf(TEXT("상호작용 시작: NPC=%s 서비스=%s\n선택지: %s"), *Nearest->GetCharacterID().ToString(), *ServiceText, *OptionText));
#endif
}

void UICDebugCheatManager::ICEndNPCInteraction()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    if (!PC) return;
    if (!DebugInteractingNPC.IsValid()) { PC->Show(TEXT("진행 중인 개발용 NPC 상호작용이 없습니다.")); return; }
    DebugInteractingNPC->EndInteraction(); DebugInteractingNPC.Reset();
    PC->Show(TEXT("NPC 상호작용 종료"));
#endif
}

void UICDebugCheatManager::ICShopOffers(FName ServiceID)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    auto* Shop = PC && PC->GetGameInstance() ? PC->GetGameInstance()->GetSubsystem<UICShopSubsystem>() : nullptr;
    if (!Shop) return;
    TArray<FICShopOffer> Offers; FString Reason;
    if (!Shop->GetOffers(ServiceID, Offers, Reason)) { PC->Show(FString::Printf(TEXT("상점 조회 실패: %s"), *Reason)); return; }
    FString Text = FString::Printf(TEXT("[%s] 판매 %d종 / 할인 %.0f%%"), *ServiceID.ToString(), Offers.Num(), Offers.Num() > 0 ? Offers[0].DiscountRatio * 100.f : 0.f);
    for (const auto& Offer : Offers) Text += FString::Printf(TEXT("\n%s 정가 %d -> %d (매입 %d)"), *Offer.ItemID.ToString(), Offer.BasePrice, Offer.FinalPrice, Offer.SellPrice);
    UE_LOG(LogICInventory, Log, TEXT("[Cheat.ShopOffers] %s"), *Text);
    PC->Show(Text);
#endif
}

void UICDebugCheatManager::ICShopBuy(FName ServiceID, FName ItemID, int32 Count)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    auto* Shop = PC && PC->GetGameInstance() ? PC->GetGameInstance()->GetSubsystem<UICShopSubsystem>() : nullptr;
    if (!Shop) return;
    FString Reason;
    const bool bOk = Shop->Buy(PC, ServiceID, ItemID, Count, Reason);
    PC->Show(bOk ? FString::Printf(TEXT("구매 완료. 보유 골드 %d"), PC->Gold) : FString::Printf(TEXT("구매 실패: %s"), *Reason));
#endif
}

void UICDebugCheatManager::ICShopSell(FName ServiceID, FName ItemID, int32 Count)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    auto* Shop = PC && PC->GetGameInstance() ? PC->GetGameInstance()->GetSubsystem<UICShopSubsystem>() : nullptr;
    if (!Shop) return;
    FString Reason;
    const bool bOk = Shop->Sell(PC, ServiceID, ItemID, Count, Reason);
    PC->Show(bOk ? FString::Printf(TEXT("판매 완료. 보유 골드 %d"), PC->Gold) : FString::Printf(TEXT("판매 실패: %s"), *Reason));
#endif
}

void UICDebugCheatManager::ICLifeObjectDump()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    if (UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(GetOuterAPlayerController()))
    {
        Life->DumpToLog();
        if (PC) PC->Show(FString::Printf(TEXT("생활 오브젝트 %d개 상태를 Output Log(LogICLifeObject)에 출력했습니다."), Life->GetObjectCount()));
    }
    else if (PC) PC->Show(TEXT("생활 오브젝트 서브시스템이 없습니다."));
#endif
}

void UICDebugCheatManager::ICLifeObjectDebug(int32 bEnabled)
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("ic.LifeObject.Debug")))
        Variable->Set(bEnabled != 0 ? 1 : 0, ECVF_SetByConsole);
#endif
}

void UICDebugCheatManager::ICDestroyNearestNPC()
{
    IC_TRACE_FUNCTION();
#if !UE_BUILD_SHIPPING
    auto* PC = Cast<AICPlayerController>(GetOuterAPlayerController());
    const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) return;
    AICNPCBase* Nearest = nullptr;
    double Best = TNumericLimits<double>::Max();
    for (TActorIterator<AICNPCBase> It(Pawn->GetWorld()); It; ++It)
    {
        if (It->IsActorBeingDestroyed()) continue;
        const double Distance = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
        if (Distance < Best) { Best = Distance; Nearest = *It; }
    }
    if (!Nearest) { PC->Show(TEXT("주변에 NPC가 없습니다.")); return; }
    UE_LOG(LogICLifeObject, Log, TEXT("[Cheat.DestroyNPC] NPC=%s"), *Nearest->GetCharacterID().ToString());
    PC->Show(FString::Printf(TEXT("%s 제거. ICLifeObjectDump로 점유가 남지 않았는지 확인하세요."), *Nearest->GetCharacterID().ToString()));
    Nearest->Destroy();
#endif
}
