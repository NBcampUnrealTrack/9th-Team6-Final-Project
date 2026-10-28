#include "Debug/ICDebugCheatManager.h"
#include "Core/Controller/ICPlayerController.h"
#include "Codex/ICCodexSubsystem.h"
#include "Codex/ICPetFollowerSubsystem.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Debug/ICLog.h"

void UICDebugCheatManager::ICGiveAllPetEssence()
{
#if !UE_BUILD_SHIPPING
    auto* PC=Cast<AICPlayerController>(GetOuterAPlayerController());
    auto* State=PC?PC->GetWorldState():nullptr;
    auto* Codex=PC&&PC->GetGameInstance()?PC->GetGameInstance()->GetSubsystem<UICCodexSubsystem>():nullptr;
    if(!State||!Codex||!State->ExportState().bPlayerCreated)
    {if(PC)PC->Show(TEXT("플레이어를 생성한 뒤 사용하세요."));return;}
    if(!State->FlushParticipants())return;
    const auto Pets=Codex->PetDefinitions();auto Next=State->ExportState();
    for(const auto* Pet:Pets)
    {
        auto& Count=Next.Codex.PetEssence.FindOrAdd(Pet->PetCodexIndex);
        Count=FMath::Max(Count,UICCodexSubsystem::EssenceRequired);
    }
    if(Pets.IsEmpty()||!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))
    {PC->Show(TEXT("펫 정의 또는 정수 지급 상태를 확인할 수 없습니다."));return;}
    PC->Show(FString::Printf(TEXT("펫 %d종의 정수를 각각 100개 이상으로 설정했습니다. ICEquipTestPets로 현재 파티에 장착할 수 있습니다."),Pets.Num()));
    UE_LOG(LogICCore,Log,TEXT("[Cheat.AllPetEssence] Pets=%d MinimumEssence=%d"),Pets.Num(),UICCodexSubsystem::EssenceRequired);
#endif
}

void UICDebugCheatManager::ICEquipTestPets()
{
#if !UE_BUILD_SHIPPING
    auto* PC=Cast<AICPlayerController>(GetOuterAPlayerController());auto* State=PC?PC->GetWorldState():nullptr;
    auto* Codex=PC&&PC->GetGameInstance()?PC->GetGameInstance()->GetSubsystem<UICCodexSubsystem>():nullptr;
    if(!State||!Codex||!State->ExportState().bPlayerCreated||!State->FlushParticipants())return;
    const auto Members=Codex->GetPartyMembers();TArray<int32> Unlocked;
    for(const auto* Pet:Codex->PetDefinitions())if(Codex->IsPetUnlocked(Pet->PetCodexIndex))Unlocked.Add(Pet->PetCodexIndex);
    if(Unlocked.IsEmpty()){PC->Show(TEXT("먼저 ICGiveAllPetEssence로 펫을 해금하세요."));return;}
    auto Next=State->ExportState();
    for(int32 Index=0;Index<Members.Num();++Index)Next.Codex.CharacterPets.Add(Members[Index],Unlocked[Index%Unlocked.Num()]);
    if(!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))return;
    if(auto* Followers=GetWorld()->GetSubsystem<UICPetFollowerSubsystem>())Followers->RefreshFollowers();
    PC->Show(FString::Printf(TEXT("플레이어와 현재 동료 %d명에게 동행 펫을 장착했습니다."),FMath::Max(0,Members.Num()-1)));
#endif
}
