#include "Debug/ICCombatVFXSetupLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraScript.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraDataSetAccessor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"
#if WITH_EDITOR
#include "NiagaraEmitter.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraMeshRendererProperties.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "NiagaraEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"

namespace ICCombatNiagaraAuthoring
{
void Expression(UNiagaraNodeFunctionCall* Module,FName Target,const FNiagaraTypeDefinition& Type,const FString& Code)
{
    const auto Handle=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
        FNiagaraParameterHandle::CreateModuleParameterHandle(Target),Module);
    auto& Override=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Module,Handle,Type,FGuid(),FGuid());
    auto* Graph=Module->GetGraph();
    auto* Node=NewObject<UNiagaraNodeCustomHlsl>(Graph,NAME_None,RF_Transactional);
    Node->ScriptUsage=ENiagaraScriptUsage::Function;
    Node->Signature.Name=FName(*(TEXT("IC_")+Target.ToString().Replace(TEXT("."),TEXT("_"))));
    Node->Signature.Inputs={FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(),TEXT("Map"))};
    Node->Signature.Outputs={FNiagaraVariable(Type,TEXT("CustomHLSLOutput"))};
    // CustomHlsl is an editor property; use reflection because its setter is
    // private to NiagaraEditor in the installed engine's MinimalAPI build.
    FindFProperty<FStrProperty>(Node->GetClass(),TEXT("CustomHlsl"))->SetPropertyValue_InContainer(Node,Code);
    Graph->AddNode(Node,false,false);Node->CreateNewGuid();
    static_cast<UEdGraphNode*>(Node)->AllocateDefaultPins();
    UEdGraphPin* Map=nullptr;UEdGraphPin* Value=nullptr;
    for(auto* Pin:Node->Pins){if(Pin->Direction==EGPD_Input&&Pin->PinName==TEXT("Map"))Map=Pin;if(Pin->Direction==EGPD_Output&&Pin->PinName==TEXT("CustomHLSLOutput"))Value=Pin;}
    auto* OverrideNode=Override.GetOwningNode();UEdGraphPin* Previous=nullptr;
    for(auto* Pin:OverrideNode->Pins)if(Pin->Direction==EGPD_Input&&Pin->LinkedTo.Num()&&Pin->PinType.PinSubCategoryObject==FNiagaraTypeDefinition::GetParameterMapDef().GetStruct())Previous=Pin->LinkedTo[0];
    check(Map&&Value&&Previous);Map->MakeLinkTo(Previous);Value->MakeLinkTo(&Override);
}
template<class T> void User(UNiagaraSystem* System,FName Name,const FNiagaraTypeDefinition& Type,const T& Value)
{
    FNiagaraVariable Variable(Type,Name);Variable.SetValue(Value);
    System->GetExposedParameters().AddParameter(Variable,true);
    System->GetExposedParameters().SetParameterValue(Value,Variable,true);
}
UNiagaraNodeFunctionCall* Module(UNiagaraNodeOutput* Output,const TCHAR* Path)
{
    auto* Script=LoadObject<UNiagaraScript>(nullptr,Path);check(Script);
    return FNiagaraStackGraphUtilities::AddScriptModuleToStack(Script,*Output);
}
void Parameters(UNiagaraSystem* System)
{
    User(System,TEXT("User.IC_Color"),FNiagaraTypeDefinition::GetColorDef(),FLinearColor(.5f,.8f,1,1));
    User(System,TEXT("User.IC_Duration"),FNiagaraTypeDefinition::GetFloatDef(),.24f);
    User(System,TEXT("User.IC_Radius"),FNiagaraTypeDefinition::GetFloatDef(),220.f);
    User(System,TEXT("User.IC_Angle"),FNiagaraTypeDefinition::GetFloatDef(),90.f);
    User(System,TEXT("User.IC_Slope"),FNiagaraTypeDefinition::GetFloatDef(),.22f);
    User(System,TEXT("User.IC_Width"),FNiagaraTypeDefinition::GetFloatDef(),2.f);
    User(System,TEXT("User.IC_Floor"),FNiagaraTypeDefinition::GetFloatDef(),-83.f);
    User(System,TEXT("User.IC_Scale"),FNiagaraTypeDefinition::GetVec3Def(),FVector3f(1));
}
void AddMeshEmitter(UNiagaraSystem* System,FName Shape,int32 Part=0)
{
    auto* Emitter=NewObject<UNiagaraEmitter>(System,NAME_None,RF_Transactional);
    UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter,false);
    auto* Data=Emitter->GetLatestEmitterData();Data->bLocalSpace=true;Data->bDeterminism=true;
    Data->InterpolatedSpawnMode=ENiagaraInterpolatedSpawnMode::NoInterpolation;
    Data->SpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::ParticleSpawnScript);
    auto* Graph=CastChecked<UNiagaraScriptSource>(Data->GraphSource)->NodeGraph.Get();
    auto* EmitterUpdate=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::EmitterUpdateScript);
    auto* Spawn=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleSpawnScript);
    auto* Update=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
    Module(EmitterUpdate,TEXT("/Niagara/Modules/Emitter/EmitterState.EmitterState"));
    auto* Burst=Module(EmitterUpdate,TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous"));
    const bool Arc=Shape==TEXT("Slash")||Shape==TEXT("Range"),Impact=Shape==TEXT("Impact");
    const int32 Num=Shape==TEXT("Range")?14:Arc||Impact?12:1;
    const auto CountHandle=FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(TEXT("Module.Spawn Count")),Burst);
    auto& Count=FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*Burst,CountHandle,FNiagaraTypeDefinition::GetIntDef(),FGuid(),FGuid());Count.DefaultValue=FString::FromInt(Num);
    auto Rapid=FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),CountHandle.GetParameterHandleString()),*Emitter->GetUniqueEmitterName(),Data->EmitterUpdateScriptProps.Script->GetUsage());
    Rapid.SetValue(Num);Data->EmitterUpdateScriptProps.Script->RapidIterationParameters.SetParameterData(Rapid.GetData(),Rapid,true);
    auto* Index=FNiagaraStackGraphUtilities::AddParameterModuleToStack({FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),TEXT("Particles.IC_Index"))},*Spawn,INDEX_NONE,{TEXT("0")});
    Expression(Index,TEXT("Particles.IC_Index"),FNiagaraTypeDefinition::GetIntDef(),TEXT("CustomHLSLOutput = ExecIndex();"));
    FString Code=TEXT("int i = Particles.IC_Index; float t = saturate(Engine.System.Age / max(User.IC_Duration, 0.001)); float3 a = float3(0,0,0); float3 b = float3(0,0,1); float expand = 1.0;\n");
    FString Position,Scale,Rotation;const TCHAR* MeshName=TEXT("Sphere");
    if(Arc)
    {
        MeshName=TEXT("Cylinder");
        Code+=TEXT("float halfAngle = User.IC_Angle * 0.008726646; float aa = -halfAngle + 2.0*halfAngle*i/12.0; float bb = -halfAngle + 2.0*halfAngle*(i+1)/12.0; a = float3(User.IC_Radius*cos(aa),User.IC_Radius*sin(aa),0); b = float3(User.IC_Radius*cos(bb),User.IC_Radius*sin(bb),0);\n");
        if(Shape==TEXT("Range"))Code+=TEXT("float edge=(i==12?-halfAngle:halfAngle); float ax=(i>=12?0.0:a.x); float ay=(i>=12?0.0:a.y); float bx=(i>=12?User.IC_Radius*cos(edge):b.x); float by=(i>=12?User.IC_Radius*sin(edge):b.y); a=float3(ax,ay,User.IC_Floor); b=float3(bx,by,User.IC_Floor);\n");
        else Code+=TEXT("a.z = -12.0+a.y*User.IC_Slope; b.z = -12.0+b.y*User.IC_Slope;\n");
    }
    else if(Impact)
    {
        MeshName=TEXT("Cylinder");
        Code+=TEXT("float pitch=fmod(float(i),3.0)*1.04719755; float yaw=i*2.39982772; float3 dir=float3(cos(pitch)*cos(yaw),cos(pitch)*sin(yaw),sin(pitch)); a=dir*3.0; b=dir*18.0; expand=1.0+t*1.6;\n");
    }
    if(Arc||Impact)
    {
        Position=TEXT("(a+b)*0.5*expand*User.IC_Scale");
        Scale=TEXT("float3(User.IC_Width/50.0,User.IC_Width/50.0,length(b-a)/100.0)*expand*User.IC_Scale");
        // The CPU VectorVM compiler cannot swizzle a normalize() expression.
        Code+=TEXT("float dx=b.x-a.x; float dy=b.y-a.y; float dz=b.z-a.z; float inv=rsqrt(max(dx*dx+dy*dy+dz*dz,0.0001)); float qw=1.0+dz*inv; float qlen=rsqrt(max(2.0*qw,0.0001));\n");
        Rotation=TEXT("float4(-dy*inv*qlen,dx*inv*qlen,0.0,qw*qlen)");
    }
    else if(Shape==TEXT("Trail"))
    {MeshName=TEXT("Cylinder");Position=TEXT("float3(-26,0,0)*User.IC_Scale");Scale=TEXT("float3(User.IC_Width/50.0,User.IC_Width/50.0,0.55)*User.IC_Scale");Rotation=TEXT("float4(0,0.70710678,0,0.70710678)");}
    else if(Shape==TEXT("Arrow")&&Part==1)
    {MeshName=TEXT("Cone");Position=TEXT("float3(6,0,0)*User.IC_Scale");Scale=TEXT("float3(0.09,0.09,0.16)*User.IC_Scale");Rotation=TEXT("float4(0,0.70710678,0,0.70710678)");}
    else
    {Position=TEXT("float3(0,0,0)");Scale=Shape==TEXT("Arrow")?TEXT("float3(0.28,0.035,0.035)*User.IC_Scale"):TEXT("float3(0.14,0.14,0.14)*User.IC_Scale");Rotation=TEXT("float4(0,0,0,1)");}
    FString Color=TEXT("User.IC_Color");
    if(Arc||Impact)Color=TEXT("User.IC_Color * (1.0-t)");
    if(Shape==TEXT("Slash"))Color+=TEXT(" * (float(i)/12.0 <= t*1.8 ? 1.0 : 0.0)");
    const TArray<FNiagaraVariable> Vars={FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(),TEXT("Particles.Position")),FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),TEXT("Particles.Scale")),FNiagaraVariable(FNiagaraTypeDefinition::GetQuatDef(),TEXT("Particles.MeshOrientation")),FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),TEXT("Particles.Color")),FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),TEXT("Particles.Lifetime")),FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(),TEXT("Particles.Alive"))};
    const TArray<FString> Defaults={TEXT("0,0,0"),TEXT("1,1,1"),TEXT("0,0,0,1"),TEXT("1,1,1,1"),TEXT("1"),TEXT("true")};
    for(auto* Output:{Spawn,Update})
    {
        auto* Values=FNiagaraStackGraphUtilities::AddParameterModuleToStack(Vars,*Output,INDEX_NONE,Defaults);
        Expression(Values,TEXT("Particles.Position"),FNiagaraTypeDefinition::GetPositionDef(),Code+TEXT("CustomHLSLOutput = ")+Position+TEXT(";"));
        Expression(Values,TEXT("Particles.Scale"),FNiagaraTypeDefinition::GetVec3Def(),Code+TEXT("CustomHLSLOutput = ")+Scale+TEXT(";"));
        Expression(Values,TEXT("Particles.MeshOrientation"),FNiagaraTypeDefinition::GetQuatDef(),Code+TEXT("CustomHLSLOutput = ")+Rotation+TEXT(";"));
        Expression(Values,TEXT("Particles.Color"),FNiagaraTypeDefinition::GetColorDef(),Code+TEXT("CustomHLSLOutput = ")+Color+TEXT(";"));
        Expression(Values,TEXT("Particles.Alive"),FNiagaraTypeDefinition::GetBoolDef(),TEXT("CustomHLSLOutput = Engine.System.Age < User.IC_Duration;"));
    }
    auto* Renderer=NewObject<UNiagaraMeshRendererProperties>(Emitter);
    FNiagaraMeshRendererMeshProperties Mesh;Mesh.Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),MeshName,MeshName));Renderer->Meshes.Reset();Renderer->Meshes.Add(Mesh);
    Renderer->bOverrideMaterials=true;FNiagaraMeshMaterialOverride Mat;Mat.ExplicitMat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IsekaiClass/VFX/CombatEffect/Materials/M_IC_CombatParticle.M_IC_CombatParticle"));Renderer->OverrideMaterials.Add(Mat);
    Emitter->AddRenderer(Renderer,Data->Version.VersionGuid);
    FNiagaraEditorUtilities::AddEmitterToSystem(*System,*Emitter,Data->Version.VersionGuid);
}
}
#endif

UNiagaraSystem* UICCombatVFXSetupLibrary::CreateCombatNiagara(const FString& Path,FName Shape)
{
#if WITH_EDITOR
    using namespace ICCombatNiagaraAuthoring;
    if(!Path.StartsWith(TEXT("/Game/IsekaiClass/VFX/CombatEffect/")))return nullptr;
    if(auto* Existing=LoadObject<UNiagaraSystem>(nullptr,*Path))return Existing;
    auto* System=NewObject<UNiagaraSystem>(CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path),RF_Public|RF_Standalone|RF_Transactional);
    UNiagaraSystemFactoryNew::InitializeSystem(System,true);
    Parameters(System);AddMeshEmitter(System,Shape);if(Shape==TEXT("Arrow"))AddMeshEmitter(System,Shape,1);
    System->RequestCompile(true);System->WaitForCompilationComplete(false,false);
    FAssetRegistryModule::AssetCreated(System);System->MarkPackageDirty();
    return System;
#else
    return nullptr;
#endif
}
bool UICCombatVFXSetupLibrary::IsCombatNiagaraReady(UNiagaraSystem* System)
{
    if(!System)return false;
#if WITH_EDITOR
    if(System->HasOutstandingCompilationRequests())return false;
    for(const auto& Handle:System->GetEmitterHandles())if(auto* Data=Handle.GetEmitterData())
        if(Data->SpawnScriptProps.Script->GetLastCompileStatus()==ENiagaraScriptCompileStatus::NCS_Error||Data->UpdateScriptProps.Script->GetLastCompileStatus()==ENiagaraScriptCompileStatus::NCS_Error)return false;
    return System->GetSystemSpawnScript()->GetLastCompileStatus()!=ENiagaraScriptCompileStatus::NCS_Error;
#else
    return System->IsReadyToRun();
#endif
}

UNiagaraSystem* UICCombatVFXSetupLibrary::DuplicateCombatNiagara(const FString& Path,UNiagaraSystem* Source)
{
#if WITH_EDITOR
    using namespace ICCombatNiagaraAuthoring;
    if(!Source||!Path.StartsWith(TEXT("/Game/IsekaiClass/VFX/CombatEffect/")))return nullptr;
    if(auto* Existing=LoadObject<UNiagaraSystem>(nullptr,*Path))return Existing;
    auto* System=DuplicateObject<UNiagaraSystem>(Source,CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path));
    Parameters(System);
    for(const auto& Handle:System->GetEmitterHandles())if(auto* Data=Handle.GetEmitterData())
    {
        Data->bLocalSpace=true;
        auto* Graph=CastChecked<UNiagaraScriptSource>(Data->GraphSource)->NodeGraph.Get();
        auto* Spawn=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleSpawnScript);
        auto* Update=Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);
        if(!Spawn||!Update)continue;
        TArray<FNiagaraVariable> BaseVars={FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),TEXT("Particles.IC_UntintedColor"))};
        auto* Capture=FNiagaraStackGraphUtilities::AddParameterModuleToStack(BaseVars,*Spawn,INDEX_NONE,{TEXT("1,1,1,1")});
        Expression(Capture,TEXT("Particles.IC_UntintedColor"),FNiagaraTypeDefinition::GetColorDef(),TEXT("CustomHLSLOutput = Particles.Color;"));
        auto* Restore=FNiagaraStackGraphUtilities::AddParameterModuleToStack({FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),TEXT("Particles.Color"))},*Update,0,{TEXT("1,1,1,1")});
        Expression(Restore,TEXT("Particles.Color"),FNiagaraTypeDefinition::GetColorDef(),TEXT("CustomHLSLOutput = Particles.IC_UntintedColor;"));
        auto* Remember=FNiagaraStackGraphUtilities::AddParameterModuleToStack(BaseVars,*Update,INDEX_NONE,{TEXT("1,1,1,1")});
        Expression(Remember,TEXT("Particles.IC_UntintedColor"),FNiagaraTypeDefinition::GetColorDef(),TEXT("CustomHLSLOutput = Particles.Color;"));
        for(auto* Output:{Spawn,Update})
        {
            auto* Tint=FNiagaraStackGraphUtilities::AddParameterModuleToStack({FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),TEXT("Particles.Color"))},*Output,INDEX_NONE,{TEXT("1,1,1,1")});
            Expression(Tint,TEXT("Particles.Color"),FNiagaraTypeDefinition::GetColorDef(),TEXT("CustomHLSLOutput = Particles.IC_UntintedColor * User.IC_Color;"));
        }
    }
    System->RequestCompile(true);System->WaitForCompilationComplete(false,false);System->SetFlags(RF_Public|RF_Standalone);
    FAssetRegistryModule::AssetCreated(System);System->MarkPackageDirty();return System;
#else
    return nullptr;
#endif
}

bool UICCombatVFXSetupLibrary::DumpCombatNiagara(UNiagaraSystem* System)
{
#if WITH_EDITOR
    if(!System)return false;
    auto* Dump=IConsoleManager::Get().FindConsoleVariable(TEXT("fx.ForceNiagaraTranslatorDump"));
    const int32 OldDump=Dump?Dump->GetInt():0;if(Dump)Dump->Set(1);
    System->RequestCompile(true);System->WaitForCompilationComplete(false,false);
    if(Dump)Dump->Set(OldDump);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("CombatVFXDiagnostics")/System->GetName();IFileManager::Get().MakeDirectory(*Folder,true);
    auto Save=[&](UNiagaraScript* Script,const FString& Name){if(Script)FFileHelper::SaveStringToFile(Script->GetVMExecutableData().LastHlslTranslation,*(Folder/(Name+TEXT(".hlsl"))));};
    Save(System->GetSystemSpawnScript(),TEXT("SystemSpawn"));Save(System->GetSystemUpdateScript(),TEXT("SystemUpdate"));
    int32 Index=0;for(const auto& Handle:System->GetEmitterHandles())if(auto* Data=Handle.GetEmitterData())
    {const FString Prefix=FString::FromInt(Index++);Save(Data->EmitterSpawnScriptProps.Script,Prefix+TEXT("EmitterSpawn"));Save(Data->EmitterUpdateScriptProps.Script,Prefix+TEXT("EmitterUpdate"));Save(Data->SpawnScriptProps.Script,Prefix+TEXT("ParticleSpawn"));Save(Data->UpdateScriptProps.Script,Prefix+TEXT("ParticleUpdate"));}
    return true;
#else
    return false;
#endif
}
int32 UICCombatVFXSetupLibrary::GetCombatParticleCount(UNiagaraComponent* Component)
{
#if WITH_EDITOR
    if(Component)if(auto Controller=Component->GetSystemInstanceController())
    {Controller->WaitForConcurrentTickAndFinalize();if(auto* Instance=Controller->GetSystemInstance_Unsafe()){int32 Count=0;for(const auto& Emitter:Instance->GetEmitters())Count+=Emitter->GetNumParticles();return Count;}}
#endif
    return -1;
}
FString UICCombatVFXSetupLibrary::DescribeCombatParticles(UNiagaraComponent* Component)
{
    FString Result;
#if WITH_EDITOR
    if(Component)if(auto Controller=Component->GetSystemInstanceController())
    {
        Controller->WaitForConcurrentTickAndFinalize();if(auto* Instance=Controller->GetSystemInstance_Unsafe())for(const auto& Emitter:Instance->GetEmitters())if(Emitter->GetNumParticles()>0)
        {
            const auto& Data=Emitter->GetParticleData();
            FNiagaraDataSetAccessor<FVector3f> Scale(Data,TEXT("Scale"));auto ScaleReader=Scale.GetReader(Data.GetCurrentData());
            FNiagaraDataSetAccessor<FLinearColor> Color(Data,TEXT("Color"));auto ColorReader=Color.GetReader(Data.GetCurrentData());
            Result+=FString::Printf(TEXT("Count=%d Scale=%s Color=%s "),Emitter->GetNumParticles(),*ScaleReader.GetSafe(0,FVector3f(-1)).ToString(),*ColorReader.GetSafe(0,FLinearColor::Black).ToString());
        }
    }
#endif
    return Result;
}
