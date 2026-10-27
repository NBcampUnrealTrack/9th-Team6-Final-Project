#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Combat/VFX/ICCombatVFX.h"
#include "Debug/ICCombatVFXSetupLibrary.h"
#include "Data/Rows/Character/ICCharacterRows.h"
#include "Combat/Actors/ICAttackEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraMeshRendererProperties.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCombatVFXCheck,"IC.Combat.NiagaraDataTable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCombatVFXCheck::RunTest(const FString&)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX.DT_IC_CombatVFX"));
    if(!TestNotNull(TEXT("Editable combat VFX DT exists"),Table))return false;
    TestEqual(TEXT("Twelve combo and four skill styles"),Table->GetRowNames().Num(),16);
    for(FName Name:Table->GetRowNames())
    {
        const auto* Row=Table->FindRow<FICCombatVFXRow>(Name,TEXT("VFX test"));
        for(const auto* Effect:{&Row->Slash,&Row->Range,&Row->Projectile,&Row->Trail,&Row->Cast,&Row->Impact})if(Effect->bEnabled)
        {
            auto* System=Effect->System.LoadSynchronous();
            TestTrue(Name.ToString()+TEXT(" uses a compiled project Niagara"),UICCombatVFXSetupLibrary::IsCombatNiagaraReady(System));
            if(System&&!System->GetName().Contains(TEXT("Project")))for(const auto& Handle:System->GetEmitterHandles())if(auto* Data=Handle.GetEmitterData())
                for(auto* Renderer:Data->GetRenderers())if(auto* Mesh=Cast<UNiagaraMeshRendererProperties>(Renderer))
                    TestTrue(TEXT("Default mesh index zero must contain the visible shape"),!Mesh->Meshes.IsEmpty()&&Mesh->Meshes[0].Mesh!=nullptr);
        }
    }
    auto* Classes=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Character/DT_IC_Class.DT_IC_Class"));
    int32 Stages=0;
    if(Classes)for(FName Name:Classes->GetRowNames())
    {
        const auto* Class=Classes->FindRow<FICClassRow>(Name,TEXT("VFX test"));
        for(const auto& Stage:Class->NormalAttackStages)
        {FICCombatPresentation Presentation;Presentation.Visuals=Stage.Visuals;TestNotNull(TEXT("Normal attack stage resolves its VFX row"),ICCombatVFX::Resolve(Presentation));++Stages;}
    }
    TestEqual(TEXT("All four professions use the DT in all three stages"),Stages,12);
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* Owner=World->SpawnActor<AICAttackEffect>();
    auto Style=Table->FindRow<FICCombatVFXRow>(TEXT("NORMAL_STAFF_1"),TEXT("VFX test"))->Projectile;
    Style.Color=FLinearColor(.7f,.1f,.2f);Style.Intensity=2;Style.Scale=FVector(2,3,4);Style.Duration=.8f;Style.WidthCm=4;
    auto* FX=ICCombatVFX::Attach(Owner,Owner->GetRootComponent(),Style,2);
    if(TestNotNull(TEXT("Runtime creates the configured Niagara component"),FX))
    {
        bool Valid=false;const auto Tint=FX->GetVariableColor(TEXT("User.IC_Color"),Valid);TestTrue(TEXT("DT color and intensity reach Niagara"),Valid&&Tint.Equals(Style.Color*2));
        const auto Scale=FX->GetVariableVec3(TEXT("User.IC_Scale"),Valid);TestTrue(TEXT("DT size reaches Niagara"),Valid&&Scale.Equals(Style.Scale));
        const auto Duration=FX->GetVariableFloat(TEXT("User.IC_Duration"),Valid);TestTrue(TEXT("DT lifetime overrides projectile default"),Valid&&FMath::IsNearlyEqual(Duration,.8f));
        const auto Width=FX->GetVariableFloat(TEXT("User.IC_Width"),Valid);TestTrue(TEXT("DT trail thickness reaches Niagara"),Valid&&FMath::IsNearlyEqual(Width,4.f));
    }
    Style.bEnabled=false;TestNull(TEXT("Disabled role creates no component"),ICCombatVFX::Attach(Owner,Owner->GetRootComponent(),Style));
    Owner->Destroy();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
