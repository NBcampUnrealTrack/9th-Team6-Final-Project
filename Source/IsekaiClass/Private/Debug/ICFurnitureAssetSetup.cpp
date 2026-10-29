#include "Debug/ICBlueprintMaintenanceLibrary.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Components/SceneComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Life/Crafting/ICCraftingStation.h"
#endif

int32 UICBlueprintMaintenanceLibrary::CopyFurnitureAppearance(UBlueprint* Destination,UBlueprint* Source)
{
#if WITH_EDITOR
    if(!Destination||!Source||!Destination->SimpleConstructionScript||!Source->GeneratedClass||!GEditor)return 0;
    auto* Station=Cast<AICCraftingStation>(Destination->GeneratedClass->GetDefaultObject());
    auto* World=GEditor->GetEditorWorldContext().World();if(!Station||!World)return 0;
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Preview=World->SpawnActor<AActor>(Source->GeneratedClass,FTransform::Identity,Params);if(!Preview)return 0;
    auto* SCS=Destination->SimpleConstructionScript.Get();Destination->Modify();SCS->Modify();
    const auto Previous=SCS->GetAllNodes();for(auto* Node:Previous)
        if(Node&&(Node->ComponentTemplate->IsA<UStaticMeshComponent>()||Node->GetVariableName().ToString().StartsWith(TEXT("IC_Presentation_"))))SCS->RemoveNodeAndPromoteChildren(Node);
    TArray<AActor*> SourceActors{Preview};Preview->GetAllChildActors(SourceActors,true);SourceActors.AddUnique(Preview);
    int32 Count=0;
    for(auto* Actor:SourceActors)
    {
        TInlineComponentArray<USceneComponent*> Parts;Actor->GetComponents(Parts);
        for(auto* Part:Parts)
        {
            if(!Part->IsA<UStaticMeshComponent>())continue;
            const FName Name(*FString::Printf(TEXT("IC_Presentation_%02d_%s"),Count,*Part->GetName()));
            auto* Node=SCS->CreateNode(Part->GetClass(),Name);if(!Node)continue;
            // Spawned components often equal their source BP archetype. Copy the full
            // property set, rather than a delta against that unrelated archetype.
            UEngine::FCopyPropertiesForUnrelatedObjectsParams Copy;Copy.bDoDelta=false;
            UEngine::CopyPropertiesForUnrelatedObjects(Part,Node->ComponentTemplate,Copy);
            auto* Template=CastChecked<USceneComponent>(Node->ComponentTemplate);
            Template->SetupAttachment(nullptr);
            Template->SetAbsolute(false,false,false);
            Template->SetMobility(Station->Shape->Mobility);
            Template->SetRelativeTransform(Part->GetComponentTransform().GetRelativeTransform(Preview->GetActorTransform()));
            SCS->AddNode(Node);Node->SetParent(Station->Shape);++Count;
        }
    }
    World->DestroyActor(Preview);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Destination);
    FKismetEditorUtilities::CompileBlueprint(Destination);
    Station=CastChecked<AICCraftingStation>(Destination->GeneratedClass->GetDefaultObject());
    Station->Shape->SetStaticMesh(nullptr);Station->Shape->SetVisibility(false,false);
    Station->Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);Station->Shape->SetRelativeScale3D(FVector::OneVector);
    Station->Label->SetVisibility(false);Destination->MarkPackageDirty();return Count;
#else
    return 0;
#endif
}
