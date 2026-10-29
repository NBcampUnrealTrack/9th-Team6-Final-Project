#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Core/Controller/ICPlayerController.h"
#include "NPC/Character/ICNPCBase.h"
#include "Debug/ICNPCTestLayout.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
class FICNPCLayoutCommand : public IAutomationLatentCommand
{
public:
    explicit FICNPCLayoutCommand(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>75){Test->AddError(TEXT("Runtime NPC layout and schedule timed out"));Cleanup();return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World?Cast<AICPlayerController>(UGameplayStatics::GetPlayerController(World,0)):nullptr;
        if(!PC)return false;
        auto* Spawns=World->GetSubsystem<UICSpawnSubsystem>();auto* State=PC->GetWorldState();
        auto* D=PC->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
        if(Stage==0)
        {
            if(PC->Menu!=TEXT("Create"))return false;
            if(!PC->GetSaves()->OnSaveFinished.Contains(PC,TEXT("SaveFinished")))return false;
            GI=PC->GetGameInstance();
            Test->TestTrue(TEXT("Layout test isolates save files"),PC->GetSaves()->ConfigureServices(State,PC->GetTime(),TEXT("IC_NPCLayout_")+FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            if(!PC->DebugCreatePlayer()){Test->AddError(TEXT("Cannot create test player"));Cleanup();return true;}
            Stage=1;return false;
        }
        if(Stage==1)
        {
            if(!PC->bCreated||PC->GetSaves()->bSaving)return false;
            auto* Navigation=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
            if(Navigation&&Navigation->IsNavigationBuildInProgress())return false;
            Spawns->RefreshSpawns();AICNPCTestLayout* Layout=nullptr;int32 Layouts=0;
            for(TActorIterator<AICNPCTestLayout> It(World);It;++It){Layout=*It;++Layouts;}
            if(!Test->TestEqual(TEXT("Map has one layout origin"),Layouts,1)){Cleanup();return true;}
            Test->TestEqual(TEXT("Runtime creates 33 location anchors"),Layout->GeneratedAnchors.Num(),33);
            Test->TestEqual(TEXT("Runtime creates 11 requested bed Blueprints"),Layout->GeneratedBeds.Num(),11);
            for(int32 Index=0;Index<Layout->NPCIDs.Num();++Index)
            {
                auto* NPC=Cast<AICNPCBase>(Spawns->FindCharacter(Layout->NPCIDs[Index]));
                if(!Test->TestNotNull(*Layout->NPCIDs[Index].ToString(),NPC))continue;
                Test->TestTrue(TEXT("NPC starts on its row anchor"),FVector::Dist2D(NPC->GetActorLocation(),Layout->StandingTransform(Index).GetLocation())<30);
                const auto Goal=Layout->BedTransform(Index).GetLocation();
                auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(World,NPC->GetActorLocation(),Goal,NPC);
                Test->TestTrue(TEXT("Complete walking path to own bed"),Path&&Path->IsValid()&&!Path->IsPartial());
                if(!Path||!Path->IsValid())Test->AddInfo(FString::Printf(TEXT("Path diagnostic: NPC=%s Start=%s End=%s"),*NPC->GetCharacterID().ToString(),*NPC->GetActorLocation().ToString(),*Goal.ToString()));
            }
            PC->GetTime()->JumpGameMinutes(1230-FMath::Fmod(PC->GetTime()->GetClock().GameMinute,1440.));
            for(FName ID:Layout->NPCIDs)if(auto* NPC=Cast<AICNPCBase>(Spawns->FindCharacter(ID)))
            {NPC->GetMesh()->TickAnimation(.1f,false);if(auto* Anim=Cast<UICCharacterAnimInstance>(NPC->GetMesh()->GetAnimInstance()))Test->TestFalse(TEXT("Sleep pose waits until the NPC reaches its bed"),Anim->bSleeping);}
            Stage=2;StageAt=Now;return false;
        }
        if(Now-StageAt<8)return false;
        for(int32 Number=1;Number<=11;++Number)
        {
            const FName ID(*FString::Printf(TEXT("NPC_R01_%03d"),Number));FICDefinitionResult Result;
            const auto* Definition=D->Find<FICNPCRow>(ID,Result);const auto* Schedule=D->Find<FICScheduleRow>(Definition->ScheduleID,Result);
            const EICScheduleActivity Activity=Stage==2?EICScheduleActivity::Sleep:EICScheduleActivity::Work;
            const auto* Segment=Schedule->Segments.FindByPredicate([Activity](const auto& S){return S.Activity==Activity;});
            FTransform Anchor;auto* NPC=Spawns->FindCharacter(ID);
            const bool Arrived=NPC&&Segment&&Spawns->ResolveLoadedAnchor(Segment->DestinationAnchorID,Anchor)&&FVector::Dist2D(NPC->GetActorLocation(),Anchor.GetLocation())<=165;
            Test->TestTrue(*(ID.ToString()+(Stage==2?TEXT(" walks to own bed at 20:30"):TEXT(" returns to row at 06:30"))),Arrived);
            if(!Arrived&&NPC)Test->AddInfo(FString::Printf(TEXT("Arrival diagnostic: %s At=%s Goal=%s GameMinute=%.2f"),*ID.ToString(),*NPC->GetActorLocation().ToString(),*Anchor.GetLocation().ToString(),PC->GetTime()->GetClock().GameMinute));
            if(auto* Character=Cast<AICNPCBase>(NPC))
            {
                Character->GetMesh()->TickAnimation(.1f,false);
                if(auto* Anim=Cast<UICCharacterAnimInstance>(Character->GetMesh()->GetAnimInstance()))Test->TestEqual(TEXT("Sleep pose follows arrival and time"),Anim->bSleeping,Stage==2);
                if(auto* Gear=Character->EquippedWeaponActor.Get())Test->TestEqual(TEXT("Weapon hides for sleep and returns on waking"),Gear->IsHidden(),Stage==2);
                if(Stage==2)if(auto* Profile=Character->GetAnimationProfile())
                    Test->TestTrue(TEXT("Sleeping mesh aligns with its own bed anchor"),Character->GetMesh()->GetComponentTransform().Equals(Profile->SleepMeshTransform*Anchor,.1f));
                if(Stage==3)Test->TestTrue(TEXT("Standing mesh returns to the character capsule after sleep"),Character->GetMesh()->GetRelativeLocation().Size()<150.f);
            }
            if(Segment)Test->TestEqual(TEXT("Arrival records the schedule anchor"),State->GetSnapshot(ID).CurrentAnchorID,Segment->DestinationAnchorID);
        }
        if(Stage==2){PC->GetTime()->JumpGameMinutes(600);Stage=3;StageAt=Now;return false;}
        Cleanup();return true;
    }
private:
    FAutomationTestBase* Test;TWeakObjectPtr<UGameInstance> GI;int32 Stage=0;double Started,StageAt=0;
    void Cleanup(){if(GI.IsValid())if(auto* Saves=GI->GetSubsystem<UICSaveSubsystem>())if(!Saves->bSaving)Saves->DeleteSlot(0);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCLayoutCheck,"IC.Characters.RuntimeLayoutAndSchedule",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCLayoutCheck::RunTest(const FString&)
{
    if(GEditor->PlayWorld){AddError(TEXT("Close PIE before the layout check"));return false;}
    if(!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/L_TestMap"),false,true)){AddError(TEXT("Cannot load test map"));return false;}
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICNPCLayoutCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));return true;
}
#endif
