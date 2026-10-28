#include "Codex/ICPetFollowerSubsystem.h"
#include "Codex/ICPetFollower.h"
#include "Codex/ICCodexSubsystem.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UICPetFollowerSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{return Type==EWorldType::Game||Type==EWorldType::PIE;}
TStatId UICPetFollowerSubsystem::GetStatId() const
{RETURN_QUICK_DECLARE_CYCLE_STAT(UICPetFollowerSubsystem,STATGROUP_Tickables);}
void UICPetFollowerSubsystem::Tick(float DeltaTime)
{
    if(!GetWorld()||!GetWorld()->HasBegunPlay()||GetWorld()->IsPaused())return;
    RefreshIn-=DeltaTime;if(RefreshIn<=0){RefreshIn=.25f;RefreshFollowers();}
}
AICPetFollower* UICPetFollowerSubsystem::GetFollower(FName CharacterID) const
{auto* Pet=Followers.FindRef(CharacterID).Get();return IsValid(Pet)?Pet:nullptr;}
void UICPetFollowerSubsystem::RefreshFollowers()
{
    if(bRefreshing||!GetWorld())return;TGuardValue<bool> Guard(bRefreshing,true);
    auto* GI=GetWorld()->GetGameInstance();auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    auto* Codex=GI?GI->GetSubsystem<UICCodexSubsystem>():nullptr;auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>();
    if(!State||!Codex)return;const auto Snapshot=State->ExportState();TSet<FName> Active;
    if(Snapshot.bPlayerCreated)for(FName ID:Codex->GetPartyMembers())
    {
        const auto* Member=Snapshot.Characters.Find(ID);const auto* PetIndex=Snapshot.Codex.CharacterPets.Find(ID);
        if(!Member||Member->IsDowned()||!PetIndex||Snapshot.Codex.PetEssence.FindRef(*PetIndex)<UICCodexSubsystem::EssenceRequired)continue;
        auto* Character=ID==UICWorldStateSubsystem::PlayerId()?Cast<AICCharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(),0)):Spawns?Spawns->FindCharacter(ID):nullptr;
        if(!IsValid(Character)||!Character->GetICAbilitySystem()||!Character->GetICAbilitySystem()->IsAlive())continue;
        const auto* Definition=Codex->FindPet(*PetIndex);if(!Definition)continue;
        Active.Add(ID);auto* Existing=GetFollower(ID);
        if(Existing&&(Existing->PetIndex!=*PetIndex||Existing->GetFollowedCharacter()!=Character))
        {Existing->Destroy();Followers.Remove(ID);Existing=nullptr;}
        if(!Existing)
        {
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Params.ObjectFlags|=RF_Transient;
            auto* Pet=GetWorld()->SpawnActor<AICPetFollower>(AICPetFollower::StaticClass(),Character->GetActorTransform(),Params);
            if(Pet&&Pet->InitializePet(Character,*Definition))Followers.Add(ID,Pet);else if(Pet)Pet->Destroy();
        }
    }
    for(auto It=Followers.CreateIterator();It;++It)
        if(!Active.Contains(It.Key())||!IsValid(It.Value())){if(IsValid(It.Value()))It.Value()->Destroy();It.RemoveCurrent();}
}
void UICPetFollowerSubsystem::Deinitialize()
{
    for(auto& Pair:Followers)if(IsValid(Pair.Value))Pair.Value->Destroy();Followers.Empty();Super::Deinitialize();
}
