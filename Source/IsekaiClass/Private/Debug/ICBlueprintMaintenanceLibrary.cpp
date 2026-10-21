#include "Debug/ICBlueprintMaintenanceLibrary.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Engine/Font.h"
#if WITH_EDITOR
#include "Engine/FontFace.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "UI/Framework/ICUIRootWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
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

bool UICBlueprintMaintenanceLibrary::ImportUIStrings(UStringTable* Table,const FString& File)
{
    return Table&&Table->GetMutableStringTable()->ImportStringsFromCSVFile(File);
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
    auto* Preview=NewObject<UICUIRootWidget>(GetTransientPackage());
    Preview->WidgetTree=DuplicateObject<UWidgetTree>(BP->WidgetTree,Preview);Preview->SetIsEnabled(true);
    if(Question>=0)
    {
        auto SetText=[Preview](const TCHAR* Name,FText Text){if(auto* Widget=Cast<UTextBlock>(Preview->WidgetTree->FindWidget(Name)))Widget->SetText(Text);};
        SetText(TEXT("TXT_Step"),FText::FromString(FString::Printf(TEXT("%d / 5"),Question+1)));
        SetText(TEXT("TXT_Question"),ICFrontendLayout::Text(Question<5?*FString::Printf(TEXT("Question%d"),Question+1):TEXT("Review")));
        if(auto* Name=Preview->WidgetTree->FindWidget(TEXT("EDIT_Name")))Name->SetVisibility(Question==0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(auto* Next=Preview->WidgetTree->FindWidget(TEXT("BTN_Next")))Next->SetVisibility(Question==0||Question==5?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CreationOptions.DT_IC_CreationOptions"));
        for(int32 I=0;I<4;++I)
        {
            auto* Button=Cast<UButton>(Preview->WidgetTree->FindWidget(FName(*FString::Printf(TEXT("BTN_Choice%d"),I))));
            if(!Button)continue;Button->SetVisibility(Question>0&&Question<5&&(Question!=1||I<2)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
            FText Value=ICFrontendLayout::Text(I==0?TEXT("Male"):TEXT("Female"));
            if(Table&&Question>1&&Question<5)if(auto* Row=Table->FindRow<FICCreationOptionRow>(FName(*FString::Printf(TEXT("Q%d_%d"),Question+1,I+1)),TEXT("Preview")))Value=Row->Label;
            if(auto* Text=Cast<UTextBlock>(Button->GetContent()))Text->SetText(Value);
        }
        if(Question==3)SetText(TEXT("TXT_Detail"),ICFrontendLayout::Text(TEXT("MusicHelp")));
        if(Question==5)
        {
            SetText(TEXT("TXT_Step"),FText::FromString(TEXT("5 / 5 · 시작 준비")));
            SetText(TEXT("TXT_Detail"),FText::FromString(TEXT("테스트여행자 · 여성\n\n무자본 빙의 개척기 : 0원에서 거부까지\n엘프 재벌 상인의 정보·선물 레시피 연결 예정\n\n사랑하는 사람과 함께 듣던 노래 · 스태프 선택\n시작 무기: 스태프 / 테스트 기술 지급 · NPC 호감도 연결 예정\n\n누군가에게 인정받았던 순간")));
        }
    }
    FWidgetRenderer Renderer(true);
    auto* Target=Renderer.DrawWidget(Preview->TakeWidget(),FVector2D(Width,Height));
    if(!Target)return false;
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
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
    return Widget->WidgetTree->RootWidget!=nullptr;
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
