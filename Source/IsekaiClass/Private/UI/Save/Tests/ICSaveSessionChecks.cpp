#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Save/ICSaveSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Save/ICSavePreviewHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveSessionCheck,"IC.UI.SaveSessionRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveSessionCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto* Viewport=NewObject<UGameViewportClient>(GEngine);auto& Context=*GEngine->GetWorldContextFromWorld(World);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;auto* Local=GI->CreateLocalPlayer(0,Error,false);PC->SetPlayer(Local);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();auto* Saves=GI->GetSubsystem<UICSaveSubsystem>();
    FICWorldState Data;Data.bPlayerCreated=true;Data.PlayerName=TEXT("세션 검증");Data.FirstSkill=TEXT("SKL_TEST");FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Data.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Isolated state is valid"),State->RestoreState(Data));
    const FString Prefix=TEXT("IC_Test_Session_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Saves->ConfigureServices(State,Time,Prefix);PC->bCreated=true;
    PC->OpenSaveMenu();PC->HandleSessionKey(EKeys::Two);TestEqual(TEXT("Save number key selects manual slot"),PC->SelectedSaveSlot,2);
    PC->OpenLoadMenu();PC->HandleSessionKey(EKeys::Four);TestEqual(TEXT("Load number key selects manual slot"),PC->SelectedLoadSlot,4);TestEqual(TEXT("Load selection preserves save selection"),PC->SelectedSaveSlot,2);
    PC->HandleSessionKey(EKeys::Enter);TestEqual(TEXT("Empty load cannot open confirmation"),PC->Menu,FString(TEXT("Load")));
    TestTrue(TEXT("Isolated slot created"),Saves->SaveSlotSynchronously(1));
    TestEqual(TEXT("Save stores a stable region ID"),Saves->GetSlotList()[1].Preview.Location,FString(TEXT("R01")));
    PC->RefreshSaveSlots();PC->SelectSessionSlot(1);
    PC->HandleSessionKey(EKeys::Enter);TestEqual(TEXT("Enter opens load confirmation"),PC->Menu,FString(TEXT("Confirm")));
    PC->HandleSessionKey(EKeys::Enter,true);TestEqual(TEXT("Held Enter cannot accept confirmation"),PC->Menu,FString(TEXT("Confirm")));
    PC->HandleSessionKey(EKeys::Escape);TestEqual(TEXT("Cancel returns to load list"),PC->Menu,FString(TEXT("Load")));TestTrue(TEXT("Cancel preserves save file"),Saves->GetSlotList()[1].bValid);
    PC->OpenSaveMenu();PC->SelectSessionSlot(1);PC->RequestManualSave();TestEqual(TEXT("Existing manual save opens common confirmation"),PC->Menu,FString(TEXT("Confirm")));
    PC->ConfirmModal(false);TestFalse(TEXT("Cancelled overwrite never starts a write"),Saves->bSaving);
    PC->RequestDelete();PC->ConfirmModal(false);TestTrue(TEXT("Cancelled deletion preserves slot"),Saves->GetSlotList()[1].bValid);
    PC->RequestDelete();PC->ConfirmModal(true);TestFalse(TEXT("Confirmed deletion removes isolated slot"),Saves->GetSlotList()[1].bExists);
    PC->OpenSaveMenu(true);PC->FinishSessionSave(1,false);TestEqual(TEXT("Write failure keeps quit screen open"),PC->Menu,FString(TEXT("Quit")));TestFalse(TEXT("Write failure cancels exit-after-save"),PC->IsQuitAfterSave());
    GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSavePreviewCheck,"IC.UI.SavePreviewClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSavePreviewCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();FICSaveSlotInfo Info;Info.SavedAt=FDateTime(2026,10,7,10,30);
    TestFalse(TEXT("Saved date formats for the active culture"),ICSavePreviewUI::Date(Info).IsEmpty());
    Info.Clock.PlaySecond=0;TestEqual(TEXT("Initial play duration"),ICSavePreviewUI::PlayTime(Info).ToString(),FString(TEXT("00:00:00")));
    Info.Clock.PlaySecond=3661.9;TestEqual(TEXT("Play duration uses cumulative hours minutes seconds"),ICSavePreviewUI::PlayTime(Info).ToString(),FString(TEXT("01:01:01")));
    Info.Clock.GameMinute=1445;TestEqual(TEXT("Sleeping does not change displayed play duration"),ICSavePreviewUI::PlayTime(Info).ToString(),FString(TEXT("01:01:01")));
    Info.Preview.Location=TEXT("L_TestMap");TestEqual(TEXT("Temporary location is always region one"),ICSavePreviewUI::Location(Info).ToString(),FString(TEXT("지역 1")));
    const FString Summary=ICSavePreviewUI::Summary(Info).ToString();
    TestTrue(TEXT("Slot labels both play time and save time"),Summary.Contains(TEXT("플레이 시간 01:01:01"))&&Summary.Contains(TEXT("저장 시간 ")));
    Info.bUsesBackup=true;TestTrue(TEXT("Backup preview identifies restored clock"),!ICSavePreviewUI::Recovery(Info).IsEmpty());return !HasAnyErrors();
}
#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/Save/ICSaveLoadWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveSessionAssetsCheck,"IC.UI.SaveSessionAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveSessionAssetsCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    auto* PCBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController"));
    auto* WidgetBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SaveLoad"));
    auto* ModalBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_LoadConfirm"));
    auto* SlotBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SaveSlot"));
    if(!TestNotNull(TEXT("Controller BP"),PCBP)||!TestNotNull(TEXT("Existing save WBP"),WidgetBP))return false;
    if(!TestNotNull(TEXT("Existing confirmation WBP"),ModalBP)||!TestNotNull(TEXT("Existing slot WBP"),SlotBP))return false;
    if(FParse::Param(FCommandLine::Get(),TEXT("ICApplySaveSessionAssets")))
    {

        auto* Tree=WidgetBP->WidgetTree.Get();auto* Close=Cast<UButton>(Tree->FindWidget(TEXT("IC_SaveLoad_Close")));
        if(!TestNotNull(TEXT("Existing close button"),Close))return false;
        const TCHAR* Names[]={TEXT("IC_SaveLoad_Save"),TEXT("IC_SaveLoad_Delete")};
        const TCHAR* Labels[]={TEXT("저장"),TEXT("삭제")};
        for(int32 Index=0;Index<2;++Index)
        {
            if(Tree->FindWidget(Names[Index]))continue;
            auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Names[Index]);
            auto* Text=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());Text->SetText(FText::FromString(Labels[Index]));Button->AddChild(Text);
            Button->SetBackgroundColor(FLinearColor(.08f,.08f,.1f,1));Button->SetColorAndOpacity(FLinearColor::White);
            if(auto* Bar=Cast<UHorizontalBox>(Close->GetParent()))Bar->AddChildToHorizontalBox(Button);
            else if(auto* Canvas=Cast<UCanvasPanel>(Close->GetParent()))
            {
                auto* Original=Cast<UCanvasPanelSlot>(Close->Slot);auto* Added=Canvas->AddChildToCanvas(Button);
                Added->SetAnchors(Original->GetAnchors());Added->SetAlignment(Original->GetAlignment());
                FMargin Offsets=Original->GetOffsets();Offsets.Left-=(Index+1)*(FMath::Max(Offsets.Right,180.f)+20.f);Added->SetOffsets(Offsets);
            }
            else {AddError(TEXT("Unexpected save WBP action layout; author buttons in its existing action panel."));return false;}
            Button->bIsVariable=true;
        }


        auto* Load=Cast<UButton>(Tree->FindWidget(TEXT("IC_SaveLoad_Load")));
        auto* Save=Cast<UButton>(Tree->FindWidget(TEXT("IC_SaveLoad_Save")));
        auto* Delete=Cast<UButton>(Tree->FindWidget(TEXT("IC_SaveLoad_Delete")));
        auto* ActionPanel=Load?Cast<UCanvasPanel>(Load->GetParent()):nullptr;
        auto* CloseGroup=Cast<UCanvasPanel>(Close->GetParent());
        auto* DesignPanel=CloseGroup?Cast<UCanvasPanel>(CloseGroup->GetParent()):nullptr;
        if(ActionPanel&&DesignPanel)
        {
            Save->RemoveFromParent();auto* SaveLayout=ActionPanel->AddChildToCanvas(Save);SaveLayout->SetAnchors(FAnchors(0,0,1,1));SaveLayout->SetOffsets(FMargin());
            Delete->RemoveFromParent();auto* DeleteLayout=DesignPanel->AddChildToCanvas(Delete);DeleteLayout->SetOffsets(FMargin(350,980,240,64));
            if(auto* Notice=Tree->FindWidget(TEXT("IC_SaveLoad_Notice")))if(auto* NoticeLayout=Cast<UCanvasPanelSlot>(Notice->Slot))NoticeLayout->SetOffsets(FMargin(610,970,810,75));
            if(auto* Detail=Cast<UTextBlock>(Tree->FindWidget(TEXT("IC_SaveLoad_Detail"))))
                for(auto* Button:{Save,Delete})if(auto* Label=Cast<UTextBlock>(Button->GetChildAt(0))){auto Font=Detail->GetFont();Font.Size=26;Label->SetFont(Font);}
            if(!Tree->FindWidget(TEXT("IC_SaveLoad_Title")))
            {
                auto* Title=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("IC_SaveLoad_Title"));DesignPanel->AddChildToCanvas(Title)->SetOffsets(FMargin(410,65,760,64));
                if(auto* Detail=Cast<UTextBlock>(Tree->FindWidget(TEXT("IC_SaveLoad_Detail")))){auto Font=Detail->GetFont();Font.Size=36;Title->SetFont(Font);}
                Title->SetColorAndOpacity(FLinearColor::White);
            }
        }
        auto* List=Tree->FindWidget(TEXT("IC_SaveLoad_SlotList"));
        if(List&&!Tree->FindWidget(TEXT("IC_SaveLoad_Scroll")))
            if(auto* Panel=Cast<UCanvasPanel>(List->GetParent()))
            {
                const auto LayoutData=Cast<UCanvasPanelSlot>(List->Slot)->GetLayout();List->RemoveFromParent();
                auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("IC_SaveLoad_Scroll"));Panel->AddChildToCanvas(Scroll)->SetLayout(LayoutData);Scroll->AddChild(List);
            }
        Tree->ForEachWidget([WidgetBP](UWidget* Widget){if(!WidgetBP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))WidgetBP->OnVariableAdded(Widget->GetFName());});
        WidgetBP->BlueprintDescription=TEXT("저장·불러오기 공통 WBP입니다. ICPlayerControllerSession의 슬롯 선택·확인·삭제·뒤로가기 규칙을 표시하며 자체 저장 실행 규칙은 두지 않습니다. 날짜와 게임 시간은 실제 원본 또는 복구 백업의 미리보기에서 표시합니다.");
        ModalBP->BlueprintDescription=TEXT("컨트롤러의 세션 규칙을 표시하는 공통 확인 WBP입니다. 불러오기·덮어쓰기·삭제 요청 문구와 실제 슬롯 미리보기를 표시하며 확인/취소 결과를 ConfirmModal로 전달합니다. Enter로 확인하고 Esc로 취소하며 키를 계속 누르는 반복 입력은 승인하지 않습니다.");
        FBlueprintEditorUtils::MarkBlueprintAsModified(ModalBP);FKismetEditorUtilities::CompileBlueprint(ModalBP);
        auto* ConfirmField=FindFProperty<FClassProperty>(UICSaveLoadWidget::StaticClass(),TEXT("ConfirmWidgetClass"));
        auto* SlotField=FindFProperty<FClassProperty>(UICSaveLoadWidget::StaticClass(),TEXT("SlotWidgetClass"));
        ConfirmField->SetPropertyValue_InContainer(WidgetBP->GeneratedClass->GetDefaultObject(),ModalBP->GeneratedClass.Get());
        SlotField->SetPropertyValue_InContainer(WidgetBP->GeneratedClass->GetDefaultObject(),SlotBP->GeneratedClass.Get());
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);FKismetEditorUtilities::CompileBlueprint(WidgetBP);
        PCBP->GeneratedClass->GetDefaultObject<AICPlayerController>()->SaveLoadWidgetClass=WidgetBP->GeneratedClass;
        FBlueprintEditorUtils::MarkBlueprintAsModified(PCBP);FKismetEditorUtilities::CompileBlueprint(PCBP);
        for(UBlueprint* Blueprint:{static_cast<UBlueprint*>(WidgetBP),PCBP,static_cast<UBlueprint*>(ModalBP)})
        {
            FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
            const FString Filename=FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
            TestTrue(TEXT("Authorized session asset saved"),UPackage::SavePackage(Blueprint->GetOutermost(),Blueprint,*Filename,Args));
        }
    }
    TestEqual(TEXT("Controller references the existing save WBP"),PCBP->GeneratedClass->GetDefaultObject<AICPlayerController>()->SaveLoadWidgetClass.Get(),WidgetBP->GeneratedClass.Get());
    TestNull(TEXT("Save list screenshot removed"),WidgetBP->WidgetTree->FindWidget(TEXT("IC_SaveLoad_Thumbnail")));
    TestNull(TEXT("Slot screenshot removed"),SlotBP->WidgetTree->FindWidget(TEXT("IC_SaveSlot_Thumbnail")));
    TestNull(TEXT("Confirmation screenshot removed"),ModalBP->WidgetTree->FindWidget(TEXT("IC_LoadConfirm_Thumbnail")));
    for(int32 I=0;I<4;++I)
        TestNotNull(TEXT("Four portrait placeholders exist"),SlotBP->WidgetTree->FindWidget(FName(*FString::Printf(TEXT("IC_SaveSlot_Party%d_Empty"),I))));
    TestNull(TEXT("No fourth companion slot"),SlotBP->WidgetTree->FindWidget(TEXT("IC_SaveSlot_Party4")));
    for(const auto& Pair:TArray<TPair<FName,UClass*>>{{TEXT("ConfirmWidgetClass"),ModalBP->GeneratedClass.Get()},{TEXT("SlotWidgetClass"),SlotBP->GeneratedClass.Get()}})
        TestTrue(TEXT("Save UI child widget class binding"),FindFProperty<FClassProperty>(UICSaveLoadWidget::StaticClass(),Pair.Key)->GetPropertyValue_InContainer(WidgetBP->GeneratedClass->GetDefaultObject()).Get()==Pair.Value);
    for(const TCHAR* Name:{TEXT("IC_SaveLoad_Save"),TEXT("IC_SaveLoad_Delete"),TEXT("IC_SaveLoad_Load"),TEXT("IC_SaveLoad_Close")})
    {
        auto* Widget=WidgetBP->WidgetTree->FindWidget(Name);TestNotNull(Name,Widget);
        if(Widget)if(auto* Layout=Cast<UCanvasPanelSlot>(Widget->Slot)){const auto Offsets=Layout->GetOffsets();AddInfo(FString::Printf(TEXT("%s Parent=%s Anchors=%s..%s Alignment=%s AutoSize=%d Offsets=%.0f,%.0f,%.0f,%.0f"),Name,*Widget->GetParent()->GetName(),*Layout->GetAnchors().Minimum.ToString(),*Layout->GetAnchors().Maximum.ToString(),*Layout->GetAlignment().ToString(),Layout->GetAutoSize(),Offsets.Left,Offsets.Top,Offsets.Right,Offsets.Bottom));}
    }
    TestTrue(TEXT("Save WBP compiled"),WidgetBP->Status==BS_UpToDate||WidgetBP->Status==BS_UpToDateWithWarnings);
    WidgetBP->WidgetTree->ForEachWidget([this](UWidget* Widget){if(auto* Layout=Cast<UCanvasPanelSlot>(Widget->Slot)){const auto O=Layout->GetOffsets();AddInfo(FString::Printf(TEXT("Widget %s Anchors=%s..%s Offsets=%.0f,%.0f,%.0f,%.0f Translation=%s"),*Widget->GetName(),*Layout->GetAnchors().Minimum.ToString(),*Layout->GetAnchors().Maximum.ToString(),O.Left,O.Top,O.Right,O.Bottom,*Widget->GetRenderTransform().Translation.ToString()));}});
    if(auto* Slots=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SaveSlot")))
        Slots->WidgetTree->ForEachWidget([this](UWidget* Widget){if(auto* Sizing=Cast<USizeBox>(Widget))AddInfo(FString::Printf(TEXT("Slot size %s %.0fx%.0f"),*Widget->GetName(),Sizing->GetWidthOverride(),Sizing->GetHeightOverride()));if(auto* Label=Cast<UTextBlock>(Widget)){const auto* Layout=Cast<UCanvasPanelSlot>(Label->Slot);if(Layout){const auto O=Layout->GetOffsets();AddInfo(FString::Printf(TEXT("Slot text %s Font=%.0f Rect=%.0f,%.0f,%.0f,%.0f"),*Label->GetName(),Label->GetFont().Size,O.Left,O.Top,O.Right,O.Bottom));}}});
    return !HasAnyErrors();
}
#endif
#endif

