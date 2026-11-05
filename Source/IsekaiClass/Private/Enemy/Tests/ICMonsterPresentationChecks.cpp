#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Enemy/ICMonsterPresentation.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Codex/ICPetFollower.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMonsterPresentationSizeCheck,"IC.Presentation.MonsterSizes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMonsterPresentationSizeCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();UWorld* World=GI->GetWorld();
    World->InitializeActorsForPlay(FURL(),false);World->CreateAISystem();
    auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();
    FICDefinitionResult SpawnerResult;const auto* Spawner=Definitions->Find<FICSpawnerRow>(TEXT("SPN_R01_001"),SpawnerResult);
    if(!TestNotNull(TEXT("Authored monster spawner is available"),Spawner)){GI->Shutdown();World->DestroyWorld(false);return false;}
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("Size review");
    FICCharacterState PlayerState;PlayerState.CharacterId=UICWorldStateSubsystem::PlayerId();PlayerState.Kind=EICCharacterKind::Player;
    Initial.Characters.Add(PlayerState.CharacterId,PlayerState);State->RestoreState(Initial);
    auto* Owner=World->SpawnActor<AICPlayerCharacter>();Owner->EnableCombat(true,true);Owner->InitializeState(PlayerState.CharacterId);
    TArray<FName> IDs;Definitions->GetDefinitionIDs(EICDefinitionType::Monster,IDs);
    TArray<TSharedPtr<FJsonValue>> Samples;int32 Normals=0,Bosses=0;
    for(int32 Index=0;Index<IDs.Num();++Index)
    {
        FICDefinitionResult Result;const auto* Row=Definitions->Find<FICMonsterRow>(IDs[Index],Result);
        if(!Row||Row->bTestOnly||Row->bDeprecated)continue;
        auto* Class=Row->PawnClass.LoadSynchronous();if(!TestNotNull(TEXT("Monster blueprint resolves"),Class))continue;
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const float ActorScale=Index%2?.8f:1.2f;
        const FTransform Origin(FRotator(0,35,0),FVector(1000+Index*2000,0,250),FVector(ActorScale));
        auto* Enemy=World->SpawnActor<AICEnemyBase>(Class,Origin,Params);if(!TestNotNull(TEXT("Monster spawns"),Enemy))continue;
        auto* Mesh=Enemy->GetMesh();const auto Before=ICMonsterPresentation::VisualBounds(Mesh);
        const auto BeforeScale=Mesh->GetRelativeScale3D();const float BeforeRadius=Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius();
        const float BeforeHalfHeight=Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const auto* AuthoredSlot=Spawner->Slots.FindByPredicate([&](const FICSpawnSlotDefinition& Entry){return Entry.MonsterID==Row->ID;});
        if(!TestNotNull(TEXT("Monster has its actual authored spawn slot"),AuthoredSlot)){Enemy->Destroy();continue;}
        const auto Slot=*AuthoredSlot;
        if(!TestTrue(TEXT("Actual monster initialization applies presentation"),Enemy->InitializeEnemy(Spawner->ID,Slot,Origin)))
        {Enemy->ReleaseForLoad();Enemy->Destroy();continue;}
        const double FieldHeight=ICMonsterPresentation::VisualHeight(Mesh);
        const double Multiplier=FMath::Max(.01f,Row->VisualScaleMultiplier);
        if(Row->Kind==EICMonsterKind::Normal)
        {
            ++Normals;
            TestTrue(TEXT("Normal field height is uniform despite mesh and actor scales"),FMath::IsNearlyEqual(FieldHeight,ICMonsterPresentation::NormalFieldHeightCm*Multiplier,.02));
            const auto After=ICMonsterPresentation::VisualBounds(Mesh);
            TestTrue(TEXT("Normal normalization preserves the existing foot position"),FMath::IsNearlyEqual(Before.Origin.Z-Before.BoxExtent.Z,After.Origin.Z-After.BoxExtent.Z,.02));
        }
        else
        {
            ++Bosses;
            TestTrue(TEXT("Field boss keeps exactly its authored multiplier and mesh scale"),Mesh->GetRelativeScale3D().Equals(BeforeScale*Multiplier,1.e-6));
            TestTrue(TEXT("Field boss retains its original authored visual height"),FMath::IsNearlyEqual(FieldHeight,Before.BoxExtent.Z*2.*Multiplier,.02));
        }
        TestEqual(TEXT("Presentation never changes capsule radius"),Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius(),BeforeRadius);
        TestEqual(TEXT("Presentation never changes capsule height"),Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),BeforeHalfHeight);
        TestTrue(TEXT("Presentation never changes actor scale"),Enemy->GetActorScale3D().Equals(FVector(ActorScale),1.e-6));
        const auto Once=Mesh->GetRelativeTransform();Enemy->InitializeEnemy(Spawner->ID,Slot,Origin);
        TestTrue(TEXT("Repeated monster initialization does not accumulate scaling"),Mesh->GetRelativeTransform().Equals(Once));
        double PetHeight=0;
        if(Row->PetCodexIndex>=0)
        {
            auto* Pet=World->SpawnActor<AICPetFollower>(AICPetFollower::StaticClass(),FTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector(.8f)),Params);
            if(TestTrue(TEXT("Actual pet initialization succeeds"),Pet&&Pet->InitializePet(Owner,*Row)))
            {
                PetHeight=ICMonsterPresentation::VisualHeight(Pet->Visual);
                const double ExpectedPetHeight = Row->Kind == EICMonsterKind::RegionalBoss ? 80. : 56.;
                TestTrue(TEXT("Normal and regional boss pets use their respective reduced sizes"),FMath::IsNearlyEqual(PetHeight,ExpectedPetHeight,.02));
                const auto PetBounds=ICMonsterPresentation::VisualBounds(Pet->Visual);
                TestTrue(TEXT("Pets remain grounded at their following anchor"),FMath::IsNearlyEqual(PetBounds.Origin.Z-PetBounds.BoxExtent.Z,Pet->GetActorLocation().Z,.02));
                TestFalse(TEXT("Larger boss pet remains cosmetic and collision-free"),Pet->GetActorEnableCollision());
                TestTrue(TEXT("Reinitializing the pet remains stable"),Pet->InitializePet(Owner,*Row));
                TestTrue(TEXT("Pet size never depends on an earlier initialization"),FMath::IsNearlyEqual(ICMonsterPresentation::VisualHeight(Pet->Visual),PetHeight,.02));
                Pet->Destroy();
            }
        }
        auto Sample=MakeShared<FJsonObject>();Sample->SetStringField(TEXT("id"),Row->ID.ToString());
        Sample->SetBoolField(TEXT("normal"),Row->Kind==EICMonsterKind::Normal);Sample->SetNumberField(TEXT("actor_scale"),ActorScale);
        Sample->SetNumberField(TEXT("field_height_cm"),FieldHeight);Sample->SetNumberField(TEXT("pet_height_cm"),PetHeight);
        Samples.Add(MakeShared<FJsonValueObject>(Sample));Enemy->ReleaseForLoad();Enemy->Destroy();
    }
    TestEqual(TEXT("All ten regular field monsters are covered"),Normals,10);TestEqual(TEXT("Regional boss is covered"),Bosses,1);
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("complete"),!HasAnyErrors());Report->SetArrayField(TEXT("samples"),Samples);
    FString JSON;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&JSON));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("MonsterSizes_20261105");IFileManager::Get().MakeDirectory(*Folder,true);
    FFileHelper::SaveStringToFile(JSON,*(Folder/TEXT("Verification.json")));
    Owner->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
