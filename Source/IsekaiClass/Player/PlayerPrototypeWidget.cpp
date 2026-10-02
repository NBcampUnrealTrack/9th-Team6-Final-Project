#include "PlayerPrototypeWidget.h"
#include "PlayerPrototypeController.h"
#include "PlayerInventory.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"
#include "PlayerInventoryPreview.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/PanelWidget.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"

void UPlayerPrototypeButton::Invoke() {if(Controller) Controller->ExecuteUI(Command,Index);}
TSharedRef<SWidget> UPlayerPrototypeWidget::RebuildWidget()
{
    if(WidgetTree && !WidgetTree->RootWidget) WidgetTree->RootWidget=WidgetTree->ConstructWidget<UCanvasPanel>();
    return Super::RebuildWidget();
}
void UPlayerPrototypeWidget::NativeConstruct() {Super::NativeConstruct();SetIsFocusable(true); Build();}
void UPlayerPrototypeWidget::NativeDestruct()
{if(IsValid(Preview)){Preview->Destroy();Preview=nullptr;}Super::NativeDestruct();}
void UPlayerPrototypeWidget::NativeTick(const FGeometry& G,float D)
{
    Super::NativeTick(G,D);auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer());
    if(PC&&PC->bDirty){PC->bDirty=false;Build();ForceLayoutPrepass();}
    if(IsValid(Preview) && (PreviewRefresh-=D)<=0.f)
    {Preview->Capture->CaptureScene();PreviewRefresh=0.2f;}
}
FReply UPlayerPrototypeWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer());
    const FKey Key=E.GetKey();
    const bool bSession=PC&&(PC->Menu==TEXT("Save")||PC->Menu==TEXT("Confirm")||PC->Menu==TEXT("Quit"));
    if(PC&&(Key==PC->MenuCancelKey()||Key==EKeys::F5||Key==EKeys::F9||
        (bSession&&(Key==EKeys::One||Key==EKeys::Two||Key==EKeys::Three||Key==EKeys::Four||Key==EKeys::Enter))||
        (PC->bCreated&&(Key==EKeys::I||Key==EKeys::O||Key==EKeys::M||Key==EKeys::P))))
    {PC->HandleKey(Key);return FReply::Handled();}
    return Super::NativeOnKeyDown(G,E);
}
void UPlayerPrototypeWidget::NameChanged(const FText& T) {if(auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer()))if(PC->CanEditCreation())PC->PlayerName=T.ToString();}
void UPlayerPrototypeWidget::Build()
{
    auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer());if(!PC)return;auto* Inv=PC->GetInventory();if(!Inv)return;const auto Snapshot=Inv->GetSnapshot();const auto Player=PC->GetPlayerSnapshot();
    const bool bBag=PC->Menu==TEXT("Inventory")||PC->Menu==TEXT("Storage");
    if(!bBag&&IsValid(Preview)){Preview->Destroy();Preview=nullptr;}
    auto* Canvas=Cast<UCanvasPanel>(WidgetTree->RootWidget); if(!Canvas)return; Canvas->ClearChildren();
    auto AddText=[this](UPanelWidget* Parent,const FString& Text,int32 Size=16)
    {auto* W=WidgetTree->ConstructWidget<UTextBlock>();W->SetText(FText::FromString(Text));auto Font=W->GetFont();Font.Size=Size;W->SetFont(Font);W->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f,0.95f,0.97f)));W->SetAutoWrapText(true);Parent->AddChild(W);return W;};
    auto Button=[this,PC](UPanelWidget* Parent,const FString& Text,const FString& Cmd,int32 Index=0)
    {
        auto* B=WidgetTree->ConstructWidget<UPlayerPrototypeButton>();B->Controller=PC;B->Command=Cmd;B->Index=Index;
        B->OnClicked.AddDynamic(B,&UPlayerPrototypeButton::Invoke);B->SetBackgroundColor(FLinearColor(0.2f,0.24f,0.3f));
        auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Text));auto Font=T->GetFont();Font.Size=14;T->SetFont(Font);T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        B->AddChild(T);Parent->AddChild(B);return B;
    };
    auto ItemText=[Inv](const FPlayerItemInstance& S)
    {if(S.IsEmpty())return FString(TEXT("빈 칸"));const auto* D=Inv->Find(S.ItemID);return FString::Printf(TEXT("%s ×%d%s"),D?*D->Name.ToString():*S.ItemID.ToString(),S.Count,S.bLocked?TEXT(" [잠금]"):TEXT(""));};
    auto* HUD=WidgetTree->ConstructWidget<UVerticalBox>();auto* HS=Canvas->AddChildToCanvas(HUD);HS->SetPosition(FVector2D(24,18));HS->SetSize(FVector2D(1100,170));
    AddText(HUD,PC->bCreated?FString::Printf(TEXT("%s  |  HP %.0f / SP %.0f / STR %d"),*PC->PlayerName,Player.HP,Player.SP,Player.MainStats.STR):TEXT("플레이어 시스템 프로토타입"),22);
    AddText(HUD,FString::Printf(TEXT("WASD 이동 · 마우스 시점 · Space 점프 · F 상호작용 · I 가방 · %s 메뉴 · F5 저장 화면 · F9 선택 슬롯 로드"),*PC->MenuCancelLabel()),14);
    if(auto* Time=PC->GetTime())
    {
        const auto Clock=Time->GetClock();const int64 Minute=FMath::FloorToInt64(Clock.GameMinute);
        AddText(HUD,FString::Printf(TEXT("Day %lld  %02lld:%02lld%s"),Minute/1440+1,(Minute%1440)/60,Minute%60,PC->GetSaves()&&PC->GetSaves()->bSaving?TEXT("  저장 중..."):TEXT("")),14);
    }
    FString Quick;
    for(int32 Q=0;Q<4;++Q){const auto* D=Inv->Find(Snapshot.QuickSlots[Q]);Quick+=FString::Printf(TEXT("  [%d] %s (%d)"),Q+1,D?*D->Name.ToString():TEXT("미등록"),Inv->CountItem(Snapshot.QuickSlots[Q]));}
    AddText(HUD,FString::Printf(TEXT("F9 불러오기 대상: %s"),PC->SelectedLoadSlot==0?TEXT("자동 저장"):*FString::Printf(TEXT("수동 %d"),PC->SelectedLoadSlot)),13);
    AddText(HUD,PC->Menu==TEXT("Save")?TEXT("저장 화면: 키보드 1~4 슬롯 선택 · Enter 저장 실행"):Quick,14);AddText(HUD,PC->Notice,15);AddText(HUD,PC->InteractionHint,15);
    if(PC->Menu.IsEmpty())
    {
        auto* Cross=WidgetTree->ConstructWidget<UTextBlock>();Cross->SetText(FText::FromString(TEXT("+")));auto* CrossSlot=Canvas->AddChildToCanvas(Cross);CrossSlot->SetAnchors(FAnchors(0.5f,0.5f));CrossSlot->SetAlignment(FVector2D(0.5f,0.5f));CrossSlot->SetAutoSize(true);return;
    }
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(0.045f,0.055f,0.075f,0.98f));Panel->SetPadding(FMargin(20));
    auto* PS=Canvas->AddChildToCanvas(Panel);PS->SetAnchors(FAnchors(0.04f,0.21f,0.96f,0.95f));PS->SetOffsets(FMargin(0));
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Panel->SetContent(Scroll);auto* Body=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Body);
    auto* Saves=PC->GetSaves();const bool bBusy=Saves&&Saves->bSaving;
    auto SaveSlots=[&](bool bSave=false)
    {
        AddText(Body,bSave?TEXT("수동 저장: 버튼 또는 키보드 1~4로 선택하세요."):TEXT("불러오기 / 삭제: 자동 1개 · 수동 4개"),18);
        const int32 Selected=bSave?PC->SelectedSaveSlot:PC->SelectedLoadSlot;
        for(const auto& Slot:PC->SaveSlots)
        {
            if(bSave&&Slot.Slot==0)continue;
            const FString Label=Slot.Slot==0?TEXT("자동"):FString::Printf(TEXT("수동 %d"),Slot.Slot);
            FString Status=!Slot.bExists?TEXT("비어 있음"):Slot.bValid?Slot.PlayerName+(Slot.bUsesBackup?TEXT(" (백업 복구 가능)"):TEXT("")):TEXT("읽을 수 없음 / 삭제 가능");
            if(Slot.bValid)Status+=TEXT(" · ")+(Slot.SavedAt+(FDateTime::Now()-FDateTime::UtcNow())).ToString(TEXT("%Y-%m-%d %H:%M:%S"));
            auto* B=Button(Body,FString(Selected==Slot.Slot?TEXT("[선택] "):TEXT(""))+Label+TEXT(" : ")+Status,bSave?TEXT("SaveSlot"):TEXT("LoadSlot"),Slot.Slot);
            B->SetIsEnabled(!bBusy);if(Selected==Slot.Slot)B->SetBackgroundColor(FLinearColor(0.15f,0.4f,0.6f));
        }
        if(bSave)
        {
            PC->SyncSaveSafety();
            const FString Reason=Saves?Saves->GetSaveBlockReason(false):TEXT("저장 시스템 없음");
            if(!Reason.IsEmpty())AddText(Body,Reason);
            Button(Body,FString::Printf(TEXT("수동 슬롯 %d 저장 실행 (Enter)"),Selected),TEXT("SaveNow"))->SetIsEnabled(!bBusy&&Reason.IsEmpty());
        }
        else Button(Body,TEXT("선택 슬롯 불러오기"),TEXT("Load"))->SetIsEnabled(!bBusy&&PC->SaveSlots.IsValidIndex(Selected)&&PC->SaveSlots[Selected].bValid);
        Button(Body,TEXT("선택 슬롯 삭제"),TEXT("Delete"))->SetIsEnabled(!bBusy&&PC->SaveSlots.IsValidIndex(Selected)&&PC->SaveSlots[Selected].bExists);
        if(Saves&&Saves->HasLegacySave())
        {
            AddText(Body,TEXT("기존 Version 1 원본: PlayerPrototype_Auto (가져온 후에도 원본 유지)"));
            Button(Body,TEXT("기존 저장을 빈 수동 슬롯 1로 가져오기"),TEXT("ImportLegacy"))->SetIsEnabled(!bBusy&&PC->SaveSlots.IsValidIndex(1)&&!PC->SaveSlots[1].bExists);
            Button(Body,TEXT("기존 Version 1 원본 삭제"),TEXT("DeleteLegacy"))->SetIsEnabled(!bBusy);
        }
    };
    if(PC->Menu==TEXT("Confirm"))
    {
        AddText(Body,TEXT("확인"),24);AddText(Body,PC->ModalText,20);
        Button(Body,TEXT("확인 (Enter)"),TEXT("ConfirmYes"))->SetIsEnabled(!bBusy);
        Button(Body,FString(TEXT("취소 ("))+PC->MenuCancelLabel()+TEXT(")"),TEXT("ConfirmNo"))->SetIsEnabled(!bBusy);return;
    }
    if(PC->Menu==TEXT("Quit"))
    {
        AddText(Body,TEXT("게임 끝내기"),24);
        PC->SyncSaveSafety();const FString Reason=Saves?Saves->GetSaveBlockReason(false):TEXT("저장 시스템 없음");
        if(!Reason.IsEmpty())AddText(Body,Reason);
        Button(Body,TEXT("저장 후 종료"),TEXT("QuitSave"))->SetIsEnabled(PC->bCreated&&!bBusy&&Reason.IsEmpty());
        Button(Body,bBusy?TEXT("진행 중인 저장 완료 후 종료"):TEXT("저장하지 않고 종료"),TEXT("QuitWithout"));
        Button(Body,FString(TEXT("취소 ("))+PC->MenuCancelLabel()+TEXT(")"),TEXT("Close"));return;
    }
    if(PC->Menu==TEXT("Save"))
    {
        AddText(Body,PC->IsQuitAfterSave()?TEXT("저장 후 종료"):TEXT("저장"),24);SaveSlots(true);
        Button(Body,FString(TEXT("돌아가기 ("))+PC->MenuCancelLabel()+TEXT(")"),TEXT("Close"));
        return;
    }
    if(PC->Menu==TEXT("Create"))
    {
        AddText(Body,TEXT("새 캐릭터 생성"),24); AddText(Body,TEXT("이름 1~12자 · 빈 이름과 줄바꿈 금지 / 로컬 남녀 캐릭터 사용"));
        auto* Name=WidgetTree->ConstructWidget<UEditableTextBox>();Name->SetHintText(FText::FromString(TEXT("플레이어 이름")));Name->SetText(FText::FromString(PC->PlayerName));Name->SetIsReadOnly(!PC->CanEditCreation());Name->OnTextChanged.AddDynamic(this,&ThisClass::NameChanged);Body->AddChild(Name);
        auto* Gender=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Gender);Button(Gender,PC->Preset==0?TEXT("[남성 선택됨]"):TEXT("남성"),TEXT("Preset"),0);Button(Gender,PC->Preset==1?TEXT("[여성 선택됨]"):TEXT("여성"),TEXT("Preset"),1);
        const TCHAR* Jobs[]={TEXT("전사 / 검"),TEXT("궁사 / 활"),TEXT("마법사 / 지팡이"),TEXT("치료사 / 치유서")};
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Row);for(int32 J=0;J<4;++J)Button(Row,FString(PC->Job==J?TEXT("[선택] "):TEXT(""))+Jobs[J],TEXT("Job"),J);
        Button(Body,TEXT("생성 및 초기 저장 / 실패 시 재시도"),TEXT("Create"));
        AddText(Body,TEXT("시험 지급품: 포션·음식·경험의 서·확장권·목재·장비. 실제 시작 지급품은 팀 데이터로 교체합니다."));SaveSlots();Button(Body,TEXT("게임 끝내기"),TEXT("Quit"));return;
    }
    auto* Nav=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Nav);
    if(PC->Menu==TEXT("Load"))Button(Nav,FString(TEXT("뒤로가기 ("))+PC->MenuCancelLabel()+TEXT(")"),TEXT("Close"));
    else {Button(Nav,TEXT("닫기"),TEXT("Close"));Button(Nav,TEXT("가방"),TEXT("Menu"),0);Button(Nav,TEXT("퀘스트 가방"),TEXT("Menu"),1);Button(Nav,TEXT("저장"),TEXT("Save"));}
    Button(Nav,TEXT("불러오기 화면"),TEXT("LoadMenu"));Button(Nav,TEXT("게임 끝내기"),TEXT("Quit"));
    if(PC->Menu==TEXT("Pause")||PC->Menu==TEXT("Load"))SaveSlots();
    if(!PC->PendingItem.IsNone())
    {auto* Targets=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Targets);Button(Targets,TEXT("자신에게 사용"),TEXT("Target"),0);for(int32 N=1;N<=4;++N)Button(Targets,FString::Printf(TEXT("동료 %d"),N),TEXT("Target"),N);Button(Targets,TEXT("취소"),TEXT("Cancel"));}
    if(PC->Menu==TEXT("Inventory")||PC->Menu==TEXT("Storage"))
    {
        auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();
        Scroll->RemoveFromParent();Panel->SetContent(Columns);
        auto* CharacterPanel=WidgetTree->ConstructWidget<UBorder>();
        CharacterPanel->SetBrushColor(FLinearColor(0.025f,0.025f,0.025f,1.f));CharacterPanel->SetPadding(FMargin(16));
        auto* LeftSlot=Columns->AddChildToHorizontalBox(CharacterPanel);FSlateChildSize LeftSize(ESlateSizeRule::Fill);LeftSize.Value=0.36f;LeftSlot->SetSize(LeftSize);LeftSlot->SetPadding(FMargin(0,0,18,0));
        auto* RightSlot=Columns->AddChildToHorizontalBox(Scroll);FSlateChildSize RightSize(ESlateSizeRule::Fill);RightSize.Value=0.64f;RightSlot->SetSize(RightSize);
        auto* CharacterScroll=WidgetTree->ConstructWidget<UScrollBox>();CharacterPanel->SetContent(CharacterScroll);
        auto* CharacterBody=WidgetTree->ConstructWidget<UVerticalBox>();CharacterScroll->AddChild(CharacterBody);
        AddText(CharacterBody,PC->PlayerName,22);
        const TCHAR* Jobs[]={TEXT("전사"),TEXT("궁사"),TEXT("마법사"),TEXT("치료사")};
        AddText(CharacterBody,FString::Printf(TEXT("%s · %s"),Jobs[FMath::Clamp(PC->Job,0,3)],PC->Preset==1?TEXT("여성"):TEXT("남성")),15);
        if(!IsValid(Preview)) Preview=GetWorld()->SpawnActor<APlayerInventoryPreview>(FVector(0,0,-10000),FRotator::ZeroRotator);
        if(Preview)
        {
            Preview->UpdatePreset(PC->Preset);
            auto* PortraitSize=WidgetTree->ConstructWidget<USizeBox>();PortraitSize->SetHeightOverride(300);CharacterBody->AddChild(PortraitSize);
            auto* PortraitScale=WidgetTree->ConstructWidget<UScaleBox>();PortraitScale->SetStretch(EStretch::ScaleToFit);PortraitSize->SetContent(PortraitScale);
            auto* Portrait=WidgetTree->ConstructWidget<UImage>();FSlateBrush PortraitBrush;PortraitBrush.SetResourceObject(Preview->Target);PortraitBrush.ImageSize=FVector2D(512,640);Portrait->SetBrush(PortraitBrush);PortraitScale->SetContent(Portrait);
        }
        AddText(CharacterBody,TEXT("장비 · 클릭하면 해제"),18);
        const TCHAR* Slots[]={TEXT("무기"),TEXT("옷"),TEXT("반지"),TEXT("목걸이")};
        for(int32 N=0;N<4;++N)Button(CharacterBody,FString(Slots[N])+TEXT(": ")+ItemText(Snapshot.Equipment[N]),TEXT("Unequip"),N);
        AddText(Body,FString::Printf(TEXT("인벤토리 %d칸 / 최대 100칸"),Snapshot.Bag.Slots.Num()),22);
        auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Actions);
        Button(Actions,TEXT("사용"),TEXT("Use"));Button(Actions,TEXT("장착"),TEXT("Equip"));Button(Actions,TEXT("잠금 전환"),TEXT("Lock"));Button(Actions,TEXT("절반 분할"),TEXT("Split"));
        auto* MoreActions=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(MoreActions);
        Button(MoreActions,TEXT("이동/합치기"),TEXT("Move"));Button(MoreActions,TEXT("버리기"),TEXT("Discard"));Button(MoreActions,TEXT("정렬"),TEXT("Sort"));Button(MoreActions,TEXT("10칸 확장"),TEXT("Expand"));
        auto* QuickRow=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(QuickRow);for(int32 N=0;N<4;++N)Button(QuickRow,FString::Printf(TEXT("퀵슬롯 %d 등록"),N+1),TEXT("Quick"),N);
        if(Snapshot.Bag.Slots.IsValidIndex(PC->SelectedBag))
        {const auto& S=Snapshot.Bag.Slots[PC->SelectedBag];AddText(Body,FString::Printf(TEXT("선택: %d / %s / 등급 %d / 강화 +%d / 소유자 %s / ID %s"),PC->SelectedBag+1,*ItemText(S),S.Grade,S.Upgrade,*S.OwnerID.ToString(),*S.InstanceID.ToString()),13);}
        auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>();Grid->SetSlotPadding(FMargin(3));Body->AddChild(Grid);
        for(int32 N=0;N<Snapshot.Bag.Slots.Num();++N)
        {auto* B=Button(Body,FString::Printf(TEXT("%s%02d  %s"),PC->SelectedBag==N?TEXT("> "):TEXT(""),N+1,*ItemText(Snapshot.Bag.Slots[N])),TEXT("Bag"),N);B->RemoveFromParent();Grid->AddChildToUniformGrid(B,N/4,N%4);}
        if(PC->Menu==TEXT("Storage"))
        {
            AddText(Body,FString::Printf(TEXT("창고 %s / 40칸"),*PC->OpenStorage.ToString()),22);
            Button(Body,TEXT("선택한 가방 아이템 넣기"),TEXT("Deposit"));Button(Body,TEXT("선택한 창고 아이템 꺼내기"),TEXT("Withdraw"));
            if(const auto* C=Snapshot.Storage.Find(PC->OpenStorage))for(int32 N=0;N<C->Slots.Num();++N)Button(Body,FString::Printf(TEXT("%s%02d %s"),PC->SelectedStorage==N?TEXT("> "):TEXT(""),N+1,*ItemText(C->Slots[N])),TEXT("Chest"),N);
        }
    }
    else if(PC->Menu==TEXT("Quest")) {AddText(Body,TEXT("별도 퀘스트 인벤토리"),22);for(const auto& S:Snapshot.QuestItems)AddText(Body,ItemText(S));if(Snapshot.QuestItems.IsEmpty())AddText(Body,TEXT("보관 중인 퀘스트 아이템이 없습니다. QUEST ITEM 상자에서 F로 시험 획득하세요."));}
    else if(PC->Menu==TEXT("Map")) AddText(Body,TEXT("지도 UI 연결 지점입니다. 이 구현 범위에는 실제 지도가 포함되지 않습니다."));
    else if(PC->Menu==TEXT("Party")) AddText(Body,TEXT("동료 시스템 연결 지점입니다. 아직 편성된 동료가 없습니다."));
    else AddText(Body,PC->Menu==TEXT("Load")?TEXT("불러올 저장 슬롯을 선택하세요."):TEXT("일시정지 중입니다."),22);
}
