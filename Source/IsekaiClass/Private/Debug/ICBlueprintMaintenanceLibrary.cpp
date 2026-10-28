#include "Debug/ICBlueprintMaintenanceLibrary.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Gameplay/ICGameplayLayout.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "SmartObjectComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Engine/Font.h"
#if WITH_EDITOR
#include "Engine/FontFace.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "UI/Framework/ICUIRootWidget.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "Editor.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Styling/CoreStyle.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/Dialogue/ICAffinityHeartsWidget.h"
#include "UI/Gameplay/ICCombatFeedbackWidget.h"
#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderingThread.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#endif

UFont* UICBlueprintMaintenanceLibrary::CreateFrontendFont(const FString& RegularFile,const FString& BoldFile)
{
#if WITH_EDITOR
    const TCHAR* PackageName=TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI");
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI.FONT_IC_UI"));
    if(Font)return Font;
    auto* Package=CreatePackage(PackageName);
    Font=NewObject<UFont>(Package,TEXT("FONT_IC_UI"),RF_Public|RF_Standalone);
    Font->FontCacheType=EFontCacheType::Runtime;
    for(int32 I=0;I<2;++I)
    {
        const FString& File=I==0?RegularFile:BoldFile;
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return nullptr;
        const FName Name=I==0?TEXT("Regular"):TEXT("Bold");
        auto* Face=NewObject<UFontFace>(Font,Name);
        Face->InitializeFromBulkData(File,EFontHinting::Default,Bytes.GetData(),Bytes.Num());
        Face->LoadingPolicy=EFontLoadingPolicy::Inline;
        auto& Entry=Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
        Entry.Name=Name;Entry.Font=FFontData(Face);
    }
    Package->MarkPackageDirty();return Font;
#else
    return nullptr;
#endif
}

bool UICBlueprintMaintenanceLibrary::EnsureFrontendFontFallback()
{
#if WITH_EDITOR
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI.FONT_IC_UI"));
    if (!Font) return false;
    auto& Composite=Font->GetMutableInternalCompositeFont();
    const bool bHasJapanese=Composite.SubTypefaces.ContainsByPredicate([](const FCompositeSubFont& Face)
    { return Face.Cultures == TEXT("ja"); });
    if (bHasJapanese && Composite.FallbackTypeface.Typeface.Fonts.Num()>0) return true;

    const FString File=FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf");
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes,*File)) return false;
    Font->Modify();
    auto* Face=NewObject<UFontFace>(Font,TEXT("IC_CJK_Fallback"));
    Face->InitializeFromBulkData(File,EFontHinting::Default,Bytes.GetData(),Bytes.Num());
    Face->LoadingPolicy=EFontLoadingPolicy::Inline;
    auto AddFaces=[Face](FTypeface& Typeface)
    {
        for (const FName Name : {FName(TEXT("Regular")),FName(TEXT("Bold"))})
        {
            auto& Entry=Typeface.Fonts.AddDefaulted_GetRef();
            Entry.Name=Name;Entry.Font=FFontData(Face);
        }
    };
    if (Composite.FallbackTypeface.Typeface.Fonts.IsEmpty()) AddFaces(Composite.FallbackTypeface.Typeface);
    if (!bHasJapanese)
    {
        auto& Japanese=Composite.SubTypefaces.AddDefaulted_GetRef();
        Japanese.EditorName=TEXT("Japanese");
        Japanese.Cultures=TEXT("ja");
        Japanese.CharacterRanges.Add(FInt32Range::Inclusive(0x3000,0x30FF));
        Japanese.CharacterRanges.Add(FInt32Range::Inclusive(0x3400,0x9FFF));
        Japanese.CharacterRanges.Add(FInt32Range::Inclusive(0xFF00,0xFFEF));
        AddFaces(Japanese.Typeface);
    }
    Composite.MakeDirty();
    Font->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool UICBlueprintMaintenanceLibrary::ImportUIStrings(UStringTable* Table,const FString& File)
{
    if (!Table) return false;
    Table->Modify();
    if (!Table->GetMutableStringTable()->ImportStringsFromCSVFile(File)) return false;
    Table->MarkPackageDirty();
    return true;
}

TArray<FName> UICBlueprintMaintenanceLibrary::GetFrontendWidgetNames(UBlueprint* Blueprint)
{
    TArray<FName> Names;
#if WITH_EDITOR
    if(auto* Widget=Cast<UWidgetBlueprint>(Blueprint))
    {
        TArray<UWidget*> All;Widget->WidgetTree->GetAllWidgets(All);
        for(auto* Entry:All)Names.Add(Entry->GetFName());
    }
#endif
    return Names;
}

bool UICBlueprintMaintenanceLibrary::RenderFrontendPreview(UBlueprint* Blueprint,const FString& File,int32 Width,int32 Height,int32 Question)
{
#if WITH_EDITOR
    auto* BP=Cast<UWidgetBlueprint>(Blueprint);if(!BP||!BP->WidgetTree||Width<1||Height<1||!FSlateApplication::IsInitialized())return false;
    const bool bSettings=BP->GetFName()==TEXT("WBP_IC_Settings");
    UUserWidget* Preview=nullptr;
    if(bSettings)Preview=NewObject<UICSettingsWidget>(GetTransientPackage(),BP->GeneratedClass);
    else Preview=NewObject<UICUIRootWidget>(GetTransientPackage());
    if(!Preview)return false;
    if(bSettings)Preview->Initialize();
    if(!bSettings)Preview->WidgetTree=DuplicateObject<UWidgetTree>(BP->WidgetTree,Preview);Preview->SetIsEnabled(true);
    if(!bSettings&&Question>=0&&Question<5)
    {
        auto SetText=[Preview](const TCHAR* Name,FText Text){if(auto* Widget=Cast<UTextBlock>(Preview->WidgetTree->FindWidget(Name)))Widget->SetText(Text);};
        SetText(TEXT("TXT_Question"),ICFrontendLayout::Text(*FString::Printf(TEXT("Question%d"),Question+1)));
        if(auto* Name=Preview->WidgetTree->FindWidget(TEXT("EDIT_Name")))Name->SetVisibility(Question==0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(auto* Next=Preview->WidgetTree->FindWidget(TEXT("BTN_Next")))Next->SetVisibility(Question==0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CreationOptions.DT_IC_CreationOptions"));
        for(int32 I=0;I<4;++I)
        {
            auto* Button=Cast<UButton>(Preview->WidgetTree->FindWidget(FName(*FString::Printf(TEXT("BTN_Choice%d"),I))));
            if(!Button)continue;Button->SetVisibility(Question>0&&Question<5&&(Question!=1||I<2)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
            FText Value=ICFrontendLayout::Text(I==0?TEXT("Male"):TEXT("Female"));
            if(Table&&Question>1&&Question<5)if(auto* Row=Table->FindRow<FICCreationOptionRow>(FName(*FString::Printf(TEXT("Q%d_%d"),Question+1,I+1)),TEXT("Preview")))Value=Row->Label;
            if(auto* Text=Cast<UTextBlock>(Button->GetContent()))Text->SetText(Value);
        }
    }
    FWidgetRenderer Renderer(true);
    TSharedRef<SWidget> Surface=Preview->TakeWidget();
    if(auto* Settings=Cast<UICSettingsWidget>(Preview))Settings->SelectTab(FMath::Clamp(Question,0,4));
    if(BP->GetFName()==TEXT("WBP_GameHUD")||BP->GetFName()==TEXT("WBP_IC_GameMenu"))
        Surface=SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(.055f,.075f,.10f))
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(ICDisplayPolicy::DesignWidth).HeightOverride(ICDisplayPolicy::DesignHeight)[Surface]]];
    auto* Target=Renderer.DrawWidget(Surface,FVector2D(Width,Height));
    if(!Target)return false;
    Renderer.DrawWidget(Target,Surface,FVector2D(Width,Height),.1f);
    FlushRenderingCommands();
    TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
    if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))return false;
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Width,Height,Pixels,PNG);
    return FFileHelper::SaveArrayToFile(PNG,*File);
#else
    return false;
#endif
}

bool UICBlueprintMaintenanceLibrary::BuildFrontendWidget(UBlueprint* Blueprint,FName Kind)
{
#if WITH_EDITOR
    auto* Widget=Cast<UWidgetBlueprint>(Blueprint);if(!Widget||!Widget->WidgetTree)return false;
    Widget->WidgetTree=NewObject<UWidgetTree>(Widget,NAME_None,RF_Transactional);
    ICFrontendLayout::Build(Widget->WidgetTree,Kind);
    TSet<FName> Names;
    Widget->WidgetTree->ForEachWidget([&](UWidget* Child){Child->bIsVariable=true;Names.Add(Child->GetFName());Widget->WidgetVariableNameToGuidMap.FindOrAdd(Child->GetFName(),FGuid::NewGuid());});
    for(auto It=Widget->WidgetVariableNameToGuidMap.CreateIterator();It;++It)if(!Names.Contains(It.Key()))It.RemoveCurrent();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
    return Widget->WidgetTree->RootWidget!=nullptr;
#else
    return false;
#endif
}

bool UICBlueprintMaintenanceLibrary::SetLifeObjectDefinition(AICLifeObjectActor* Object,USmartObjectDefinition* Definition)
{
    if(!Object||!Object->SmartObject||!Definition)return false;Object->SmartObject->Modify();Object->SmartObject->SetDefinition(Definition);
    if(Object->ObjectType==EICLifeObjectType::Bed&&Object->Shape)
    {
        Object->Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Object->Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
        Object->Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    }
    return true;
}
bool UICBlueprintMaintenanceLibrary::AddGameClockHUD(UBlueprint* Blueprint)
{
#if WITH_EDITOR
    auto* BP=Cast<UWidgetBlueprint>(Blueprint);if(!BP||!ICGameplayLayout::EnsureGameClock(BP->WidgetTree))return false;
    if(auto* Clock=BP->WidgetTree->FindWidget(TEXT("TXT_GameClock")))
    {Clock->bIsVariable=true;BP->WidgetVariableNameToGuidMap.FindOrAdd(Clock->GetFName(),FGuid::NewGuid());}
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);return true;
#else
    return false;
#endif
}
bool UICBlueprintMaintenanceLibrary::AddDialogueAffinityHearts(UBlueprint* Blueprint)
{
#if WITH_EDITOR
    auto* BP = Cast<UWidgetBlueprint>(Blueprint);
    if (!BP || !BP->WidgetTree) return false;
    auto* Tree = BP->WidgetTree.Get();
    if (Tree->FindWidget(TEXT("AffinityHearts"))) return true;
    auto* Speaker = Cast<UTextBlock>(Tree->FindWidget(TEXT("SpeakerText")));
    auto* HeartsClass = LoadClass<UICAffinityHeartsWidget>(nullptr,
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_AffinityHearts.WBP_IC_AffinityHearts_C"));
    if (!Speaker || !Speaker->GetParent() || !HeartsClass) return false;
    BP->Modify();
    Tree->Modify();
    auto* Parent = Speaker->GetParent();
    Parent->Modify();
    const int32 Index = Parent->GetChildIndex(Speaker);
    auto* PreviousSlot = Speaker->Slot.Get();
    auto* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpeakerHeader"));
    Speaker->RemoveFromParent();
    Parent->InsertChildAt(Index, Header, PreviousSlot);
    auto* NameSlot = Header->AddChildToHorizontalBox(Speaker);
    NameSlot->SetPadding(FMargin(0, 0, 16, 0));
    NameSlot->SetVerticalAlignment(VAlign_Center);
    auto* Hearts = Tree->ConstructWidget<UICAffinityHeartsWidget>(HeartsClass, TEXT("AffinityHearts"));
    Hearts->SetVisibility(ESlateVisibility::HitTestInvisible);
    Header->AddChildToHorizontalBox(Hearts)->SetVerticalAlignment(VAlign_Center);
    for (UWidget* Child : {static_cast<UWidget*>(Header), static_cast<UWidget*>(Hearts)})
    {
        Child->bIsVariable = true;
        BP->WidgetVariableNameToGuidMap.FindOrAdd(Child->GetFName(), FGuid::NewGuid());
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return true;
#else
    return false;
#endif
}

bool UICBlueprintMaintenanceLibrary::AddCombatFeedbackHUD(UBlueprint* Blueprint)
{
#if WITH_EDITOR
    auto* BP=Cast<UWidgetBlueprint>(Blueprint);
    auto* Tree=BP?BP->WidgetTree.Get():nullptr;
    auto* Canvas=Tree?Cast<UCanvasPanel>(Tree->RootWidget):nullptr;
    auto* Class=LoadClass<UICCombatFeedbackWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CombatFeedback.WBP_IC_CombatFeedback_C"));
    if(!Canvas||!Class)return false;
    if(Tree->FindWidget(TEXT("HUD_CombatFeedback")))return true;
    BP->Modify();Tree->Modify();
    auto* Feedback=Tree->ConstructWidget<UICCombatFeedbackWidget>(Class,TEXT("HUD_CombatFeedback"));
    Feedback->SetVisibility(ESlateVisibility::HitTestInvisible);Feedback->bIsVariable=true;
    auto* Placement=Canvas->AddChildToCanvas(Feedback);Placement->SetAnchors(FAnchors(0,0,1,1));Placement->SetOffsets(FMargin(0));Placement->SetZOrder(5);
    BP->WidgetVariableNameToGuidMap.FindOrAdd(Feedback->GetFName(),FGuid::NewGuid());
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);return true;
#else
    return false;
#endif
}

bool UICBlueprintMaintenanceLibrary::AddMovementCooldownHUD(UBlueprint* Blueprint)
{
#if WITH_EDITOR
    auto* BP=Cast<UWidgetBlueprint>(Blueprint);
    auto* Tree=BP?BP->WidgetTree.Get():nullptr;
    auto* Canvas=Tree?Cast<UCanvasPanel>(Tree->RootWidget):nullptr;
    auto* Class=LoadClass<UICMovementCooldownWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_MovementCooldown.WBP_IC_MovementCooldown_C"));
    if(!Canvas||!Class)return false;
    BP->Modify();Tree->Modify();
    auto* Cooldown=Cast<UICMovementCooldownWidget>(Tree->FindWidget(TEXT("HUD_MovementCooldown")));
    if(!Cooldown)Cooldown=Tree->ConstructWidget<UICMovementCooldownWidget>(Class,TEXT("HUD_MovementCooldown"));
    Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);Cooldown->bIsVariable=true;
    auto* Placement=Cast<UCanvasPanelSlot>(Cooldown->Slot);
    if(!Placement)Placement=Canvas->AddChildToCanvas(Cooldown);
    Placement->SetAnchors(FAnchors(0,0));Placement->SetAlignment({.5f,.5f});
    Placement->SetPosition({0,0});Placement->SetSize(FVector2D(UICRadialCooldownBar::Diameter));
    BP->WidgetVariableNameToGuidMap.FindOrAdd(Cooldown->GetFName(),FGuid::NewGuid());
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);return true;
#else
    return false;
#endif
}

int32 UICBlueprintMaintenanceLibrary::ClearBlueprintDocumentation(UBlueprint* Blueprint)
{
    int32 Count=0;
#if WITH_EDITOR
    if(!Blueprint)return 0;
    if(!Blueprint->BlueprintDescription.IsEmpty()){Blueprint->BlueprintDescription.Empty();++Count;}
    for(auto& Variable:Blueprint->NewVariables)
        Count+=Variable.MetaDataArray.RemoveAll([](const FBPVariableMetaDataEntry& Entry)
        {return Entry.DataKey==TEXT("ToolTip")||Entry.DataKey==TEXT("ShortToolTip");});
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for(auto* Graph:Graphs)
    {
        const auto Nodes=Graph->Nodes;
        for(UEdGraphNode* Node:Nodes)
        {
            if(!Node)continue;
            if(Node->IsA<UEdGraphNode_Comment>()){Graph->RemoveNode(Node);++Count;continue;}
            if(!Node->NodeComment.IsEmpty()||Node->bCommentBubbleVisible)
            {Node->NodeComment.Empty();Node->bCommentBubbleVisible=false;++Count;}
        }
    }
    if(Count)FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
#endif
    return Count;
}
