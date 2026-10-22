#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "NPC/Character/ICNPCBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"

namespace
{
UWorld* TestWorld()
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    return World;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWeaponGeometryCheck,"IC.Characters.WeaponScaleAndMuzzle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICWeaponGeometryCheck::RunTest(const FString&)
{
    auto* World=TestWorld();auto* Weapon=World->SpawnActor<AICWeaponActor>();
    Weapon->Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Weapon->Visual->SetRelativeScale3D(FVector(1,1,2));
    const float Length=Weapon->GetScaledLengthCm();
    TestTrue(TEXT("Length includes mesh scale"),FMath::IsNearlyEqual(Length,200.f,1.f));
    Weapon->SetActorScale3D(FVector(2));
    TestTrue(TEXT("Length includes actor scale"),FMath::IsNearlyEqual(Weapon->GetScaledLengthCm(),Length*2,.1f));
    FVector Start,End;Weapon->GetBladeTrace(Start,End);
    TestTrue(TEXT("Trace excludes handle"),FMath::IsNearlyEqual(float(FVector::Distance(Start,End)),Length*2*(1-Weapon->HandleFraction),.1f));
    TestTrue(TEXT("Auto muzzle follows resized tip"),Weapon->GetMuzzleLocation().Equals(End,.01));
    World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGiftTransactionCheck,"IC.Characters.GiftTransaction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGiftTransactionCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(Definitions,0);
    TestTrue(TEXT("Gift item is a valid inventory item"),Staging->AddItem(TEXT("ITEM_P0_01"),3,false));
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("선물 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");Initial.Inventory=Staging->GetSnapshot();
    FICCharacterState PlayerState;PlayerState.CharacterId=UICWorldStateSubsystem::PlayerId();PlayerState.Kind=EICCharacterKind::Player;
    PlayerState.UnlockedSkillIDs.Add(Initial.FirstSkill);Initial.Characters.Add(PlayerState.CharacterId,PlayerState);
    TestTrue(TEXT("Initial world restored"),State->RestoreState(Initial));
    World->InitializeActorsForPlay(FURL(),false);
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->Inventory->BindWorldState(State);Player->InitializeState(PlayerState.CharacterId);
    auto* Controller=World->SpawnActor<AICPlayerController>();Controller->Possess(Player);
    auto* NPC=World->SpawnActor<AICNPCBase>();TestTrue(TEXT("NPC definition initialized"),NPC->InitializeNPC(TEXT("NPC_R01_004")));
    if(TestNotNull(TEXT("Equipment restoration creates weapon actor after mesh is ready"),NPC->EquippedWeaponActor.Get()))
        TestEqual(TEXT("Bow attaches to configured hand socket"),NPC->EquippedWeaponActor->GetRootComponent()->GetAttachSocketName(),FName(TEXT("weapon_l")));
    NPC->SetActorLocation(Player->GetActorLocation()+FVector(100,0,0));
    TestTrue(TEXT("Preferred gift succeeds"),NPC->TryGiveGift(Controller,TEXT("ITEM_P0_01")));
    TestEqual(TEXT("Gift adds 20 affinity"),State->GetSnapshot(TEXT("NPC_R01_004")).Affinity,20);
    TestEqual(TEXT("Gift consumes exactly one item"),Player->Inventory->CountItem(TEXT("ITEM_P0_01")),2);
    State->SetAffinity(TEXT("NPC_R01_004"),480);
    TestTrue(TEXT("Gift reaches upper bound"),NPC->TryGiveGift(Controller,TEXT("ITEM_P0_01")));
    TestFalse(TEXT("No-effect gift at 500 rejected"),NPC->TryGiveGift(Controller,TEXT("ITEM_P0_01")));
    TestEqual(TEXT("Rejected gift does not consume"),Player->Inventory->CountItem(TEXT("ITEM_P0_01")),1);
    State->SetAffinity(TEXT("NPC_R01_004"),80);
    TestTrue(TEXT("Last item reaches skill threshold"),NPC->TryGiveGift(Controller,TEXT("ITEM_P0_01")));
    TestTrue(TEXT("100 affinity unlocks NPC skill"),State->GetSnapshot(PlayerState.CharacterId).UnlockedSkillIDs.Contains(TEXT("SKL_NPC_R01_004")));
    TestFalse(TEXT("Insufficient gift rejected"),NPC->TryGiveGift(Controller,TEXT("ITEM_P0_01")));
    TestEqual(TEXT("Failed gift preserves affinity"),State->GetSnapshot(TEXT("NPC_R01_004")).Affinity,100);
    Player->ReleaseForLoad();NPC->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCharacterPoseCheck,"IC.Characters.NativeGraphAndProfiles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCharacterPoseCheck::RunTest(const FString&)
{
    auto* World=TestWorld();auto* NPC=World->SpawnActor<AICNPCBase>();auto* Component=NPC->GetMesh();
    for(int32 Index=0;Index<13;++Index)
    {
        const bool Player=Index<2;const FString ID=Player?(Index==0?TEXT("PLAYER_M"):TEXT("PLAYER_F")):FString::Printf(TEXT("NPC_R01_%03d"),Index-1);
        const FString Folder=Player?FString::Printf(TEXT("/Game/IsekaiClass/Animation/Player/Main%s"),Index==0?TEXT("Male"):TEXT("Female")):TEXT("/Game/IsekaiClass/Animation/NPC/")+ID;
        const FString MeshPath=Player?FString::Printf(TEXT("/Game/IsekaiClass/Player/SamplePlayer/%s/SK_Player_%s"),Index==0?TEXT("Male"):TEXT("Female"),Index==0?TEXT("M"):TEXT("F")):FString::Printf(TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/%s/SK_R01_%d"),*ID,Index-1);
        auto* Profile=LoadObject<UICCharacterAnimationProfile>(nullptr,*(Folder+TEXT("/DA_ANIM_")+ID));
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*MeshPath);
        if(!TestNotNull(*ID,Profile)||!TestNotNull(TEXT("Target mesh"),Mesh))continue;
        TestEqual(TEXT("Expected family count"),Profile->WeaponSets.Num(),Player?4:1);
        auto* Working=DuplicateObject<UICCharacterAnimationProfile>(Profile,GetTransientPackage());
        for(const auto& Pair:Profile->WeaponSets)
        {
            Working->DefaultFamily=Pair.Key;NPC->AnimationProfile=Working;
            Component->SetSkeletalMesh(Mesh);Component->SetAnimInstanceClass(UICCharacterAnimInstance::StaticClass());
            auto* Anim=Cast<UICCharacterAnimInstance>(Component->GetAnimInstance());
            if(!TestNotNull(TEXT("Native anim instance"),Anim))continue;
            for(int32 Frame=0;Frame<10;++Frame){Component->TickAnimation(.1f,false);Component->RefreshBoneTransforms();}
            TestNotNull(TEXT("Locomotion set selected"),Anim->Locomotion.Get());
            const auto& Pose=Component->GetBoneSpaceTransforms();const auto& Reference=Mesh->GetRefSkeleton();
            float Difference=0;
            for(const FName Bone:{FName(TEXT("J_Bip_L_UpperArm")),FName(TEXT("J_Bip_R_UpperArm")),FName(TEXT("J_Bip_C_Spine"))})
            {const int32 B=Reference.FindBoneIndex(Bone);if(Pose.IsValidIndex(B))Difference+=Pose[B].GetRotation().AngularDistance(Reference.GetRefBonePose()[B].GetRotation());}
            TestTrue(*(ID+TEXT(" produces an animated body pose")),Difference>.02f);
            TestNotNull(TEXT("Attack montage"),Pair.Value.Attack.Get());TestNotNull(TEXT("Skill montage"),Pair.Value.Skill.Get());TestNotNull(TEXT("Hit montage"),Pair.Value.Hit.Get());
            TestTrue(TEXT("Target montage plays in native slot graph"),Anim->Montage_Play(Pair.Value.Attack)>0);
            Anim->Montage_Stop(0);
        }
    }
    World->DestroyWorld(false);return true;
}
#endif
