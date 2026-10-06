#include "PlayerPrototypeWidget.h"
#include "PlayerPrototypeController.h"
#include "PlayerInventory.h"
#include "PlayerCrafting.h"
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
    if(PC&&PC->bCreated && (E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::I||E.GetKey()==EKeys::O||E.GetKey()==EKeys::M||E.GetKey()==EKeys::P))
    {PC->HandleKey(E.GetKey());return FReply::Handled();}
    return Super::NativeOnKeyDown(G,E);
}
void UPlayerPrototypeWidget::NameChanged(const FText& T) {if(auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer()))PC->PlayerName=T.ToString();}
void UPlayerPrototypeWidget::Build()
{
    auto* PC=Cast<APlayerPrototypeController>(GetOwningPlayer());if(!PC)return;auto* Inv=PC->GetInventory();if(!Inv)return;
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
    {if(S.IsEmpty())return FString(TEXT("빈 칸"));const auto* D=Inv->Find(S.ItemID);return FString::Printf(TEXT("%s ×%d%s"),D?*D->DisplayName.ToString():*S.ItemID.ToString(),S.Count,S.bLocked?TEXT(" [잠금]"):TEXT(""));};
    auto* HUD=WidgetTree->ConstructWidget<UVerticalBox>();auto* HS=Canvas->AddChildToCanvas(HUD);HS->SetPosition(FVector2D(24,18));HS->SetSize(FVector2D(1100,170));
    const FPlayerEquipmentStats EquipmentStats=Inv->GetEquipmentStats();
    AddText(HUD,PC->bCreated?FString::Printf(TEXT("%s  |  HP %.0f/%.0f  MP %.0f/%.0f  SP %.0f/%.0f%s | 골드 %d | 공격 %.0f 방어 %.0f | STR %d DEX %d INT %d CON %d CHA %d LUK %d"),*PC->PlayerName,PC->Survival.HP,PC->Survival.MaxHP,PC->Survival.MP,PC->Survival.MaxMP,PC->Survival.SP,PC->Survival.MaxSP,PC->Survival.IsExhausted()?TEXT(" [탈진]"):TEXT(""),PC->Gold,EquipmentStats.Attack,EquipmentStats.Defense,PC->MainStats.Strength,PC->MainStats.Dexterity,PC->MainStats.Intelligence,PC->MainStats.Constitution,PC->MainStats.Charisma,PC->MainStats.Luck):TEXT("플레이어 시스템 프로토타입"),22);
    AddText(HUD,TEXT("WASD 이동 · 마우스 시점 · Space 점프 · F 상호작용 · I 가방 · ESC 메뉴 · F5 저장 · F9 로드"),14);
    FString Quick;
    for(int32 Q=0;Q<4;++Q){const auto* D=Inv->Find(Inv->QuickSlots[Q]);Quick+=FString::Printf(TEXT("  [%d] %s (%d)"),Q+1,D?*D->DisplayName.ToString():TEXT("미등록"),Inv->CountItem(Inv->QuickSlots[Q]));}
    AddText(HUD,Quick,14);AddText(HUD,PC->Notice,15);
    if(PC->Menu.IsEmpty())
    {
        auto* Cross=WidgetTree->ConstructWidget<UTextBlock>();Cross->SetText(FText::FromString(TEXT("+")));auto* CrossSlot=Canvas->AddChildToCanvas(Cross);CrossSlot->SetAnchors(FAnchors(0.5f,0.5f));CrossSlot->SetAlignment(FVector2D(0.5f,0.5f));CrossSlot->SetAutoSize(true);return;
    }
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(0.045f,0.055f,0.075f,0.98f));Panel->SetPadding(FMargin(20));
    auto* PS=Canvas->AddChildToCanvas(Panel);PS->SetAnchors(FAnchors(0.04f,0.21f,0.96f,0.95f));PS->SetOffsets(FMargin(0));
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Panel->SetContent(Scroll);auto* Body=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Body);
    if(PC->Menu==TEXT("Create"))
    {
        AddText(Body,TEXT("새 캐릭터 생성"),24); AddText(Body,TEXT("이름 1~12자 · 빈 이름과 줄바꿈 금지 / 로컬 남녀 캐릭터 사용"));
        auto* Name=WidgetTree->ConstructWidget<UEditableTextBox>();Name->SetHintText(FText::FromString(TEXT("플레이어 이름")));Name->SetText(FText::FromString(PC->PlayerName));Name->OnTextChanged.AddDynamic(this,&ThisClass::NameChanged);Body->AddChild(Name);
        auto* Gender=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Gender);Button(Gender,PC->Preset==0?TEXT("[남성 선택됨]"):TEXT("남성"),TEXT("Preset"),0);Button(Gender,PC->Preset==1?TEXT("[여성 선택됨]"):TEXT("여성"),TEXT("Preset"),1);
        const TCHAR* Jobs[]={TEXT("전사 / 검"),TEXT("궁사 / 활"),TEXT("마법사 / 지팡이"),TEXT("치료사 / 치유서")};
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Row);for(int32 J=0;J<4;++J)Button(Row,FString(PC->Job==J?TEXT("[선택] "):TEXT(""))+Jobs[J],TEXT("Job"),J);
        Button(Body,TEXT("생성 및 초기 저장 / 실패 시 재시도"),TEXT("Create"));Button(Body,TEXT("기존 프로토타입 저장 불러오기"),TEXT("Load"));
        AddText(Body,TEXT("시험 지급품: 포션·음식·경험의 서·확장권·목재·장비. 실제 시작 지급품은 팀 데이터로 교체합니다."));return;
    }
    auto* Nav=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Nav);
    if(PC->Menu!=TEXT("Load")) {Button(Nav,TEXT("닫기"),TEXT("Close"));Button(Nav,TEXT("가방"),TEXT("Menu"),0);Button(Nav,TEXT("퀘스트 가방"),TEXT("Menu"),1);Button(Nav,TEXT("저장"),TEXT("Save"));}
    Button(Nav,TEXT("불러오기"),TEXT("Load"));
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
        for(int32 N=0;N<4;++N)Button(CharacterBody,FString(Slots[N])+TEXT(": ")+ItemText(Inv->Equipment[N]),TEXT("Unequip"),N);
        AddText(Body,FString::Printf(TEXT("인벤토리 %d칸 / 최대 100칸"),Inv->Bag.Slots.Num()),22);
        auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Actions);
        Button(Actions,TEXT("사용"),TEXT("Use"));Button(Actions,TEXT("장착"),TEXT("Equip"));Button(Actions,TEXT("잠금 전환"),TEXT("Lock"));Button(Actions,TEXT("절반 분할"),TEXT("Split"));
        auto* MoreActions=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(MoreActions);
        Button(MoreActions,TEXT("이동/합치기"),TEXT("Move"));Button(MoreActions,TEXT("버리기"),TEXT("Discard"));Button(MoreActions,TEXT("정렬"),TEXT("Sort"));Button(MoreActions,TEXT("10칸 확장"),TEXT("Expand"));
        auto* QuickRow=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(QuickRow);for(int32 N=0;N<4;++N)Button(QuickRow,FString::Printf(TEXT("퀵슬롯 %d 등록"),N+1),TEXT("Quick"),N);
        if(Inv->Bag.Slots.IsValidIndex(PC->SelectedBag))
        {
            const auto& S=Inv->Bag.Slots[PC->SelectedBag];const auto* Definition=Inv->Find(S.ItemID);
            const FName TableItemID=Definition?Definition->ItemID:S.ItemID;
            FString OptionText;for(const auto& Option:S.Options)OptionText+=FString::Printf(TEXT(" %s +%.0f"),*Option.Key.ToString(),Option.Value);
            AddText(Body,FString::Printf(TEXT("선택: %d / %s / 아이템 ID %s / 등급 %d / 강화 +%d / 랜덤 옵션:%s / 소유자 %s / 인스턴스 ID %s"),PC->SelectedBag+1,*ItemText(S),*TableItemID.ToString(),static_cast<int32>(S.Grade),S.Upgrade,OptionText.IsEmpty()?TEXT(" 없음"):*OptionText,*S.OwnerID.ToString(),*S.InstanceID.ToString()),13);
        }
        auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>();Grid->SetSlotPadding(FMargin(3));Body->AddChild(Grid);
        for(int32 N=0;N<Inv->Bag.Slots.Num();++N)
        {auto* B=Button(Body,FString::Printf(TEXT("%s%02d  %s"),PC->SelectedBag==N?TEXT("> "):TEXT(""),N+1,*ItemText(Inv->Bag.Slots[N])),TEXT("Bag"),N);B->RemoveFromParent();Grid->AddChildToUniformGrid(B,N/4,N%4);}
        if(PC->Menu==TEXT("Storage"))
        {
            AddText(Body,FString::Printf(TEXT("창고 %s / 40칸"),*PC->OpenStorage.ToString()),22);
            Button(Body,TEXT("선택한 가방 아이템 넣기"),TEXT("Deposit"));Button(Body,TEXT("선택한 창고 아이템 꺼내기"),TEXT("Withdraw"));
            if(const auto* C=Inv->Storage.Find(PC->OpenStorage))for(int32 N=0;N<C->Slots.Num();++N)Button(Body,FString::Printf(TEXT("%s%02d %s"),PC->SelectedStorage==N?TEXT("> "):TEXT(""),N+1,*ItemText(C->Slots[N])),TEXT("Chest"),N);
        }
    }
    else if(PC->Menu==TEXT("Craft"))
    {
        AddText(Body,PC->ActiveStation==TEXT("Kitchen")?TEXT("요리"):TEXT("제작"),24);
        auto* Crafting=PC->GetCrafting();const FName RecipeID=PC->ActiveStation==TEXT("Kitchen")?FName(TEXT("RCP_COOK_001")):FName(TEXT("RCP_CRAFT_001"));
        if(const auto* Recipe=Crafting?Crafting->Find(RecipeID):nullptr)
        {
            FString Materials=FString::Printf(TEXT("재료: %s x%d"),*Recipe->IngredientID1.ToString(),Recipe->IngredientCount1);
            if(!Recipe->IngredientID2.IsNone())Materials+=FString::Printf(TEXT(" + %s x%d"),*Recipe->IngredientID2.ToString(),Recipe->IngredientCount2);
            AddText(Body,FString::Printf(TEXT("%s -> %s x%d / SP %.0f"),*Materials,*Recipe->ResultItemID.ToString(),Recipe->ResultCount,Recipe->SPCost),16);
            Button(Body,FString::Printf(TEXT("%s 만들기"),*Recipe->DisplayName.ToString()),TEXT("Craft"),PC->ActiveStation==TEXT("Kitchen")?0:1);
        }
        if(PC->ActiveStation==TEXT("Forge"))
        {
            AddText(Body,TEXT("장비 강화 - 가방에 해제된 장비만 가능"),18);
            for(int32 Index=0;Index<Inv->Bag.Slots.Num();++Index)
            {
                const auto& Item=Inv->Bag.Slots[Index];const auto* Definition=Inv->Find(Item.ItemID);
                if(!Item.IsEmpty()&&Definition&&Definition->Kind==EPlayerItemKind::Equipment)Button(Body,FString::Printf(TEXT("%s%s +%d"),PC->SelectedBag==Index?TEXT("> "):TEXT(""),*Definition->DisplayName.ToString(),Item.Upgrade),TEXT("Bag"),Index);
            }
            Button(Body,TEXT("선택 장비 강화"),TEXT("Enhance"));
        }
    }
    else if(PC->Menu==TEXT("Quest")) {AddText(Body,TEXT("별도 퀘스트 인벤토리"),22);for(const auto& S:Inv->QuestItems)AddText(Body,ItemText(S));if(Inv->QuestItems.IsEmpty())AddText(Body,TEXT("보관 중인 퀘스트 아이템이 없습니다. QUEST ITEM 상자에서 F로 시험 획득하세요."));}
    else if(PC->Menu==TEXT("Map")) AddText(Body,TEXT("지도 UI 연결 지점입니다. 이 구현 범위에는 실제 지도가 포함되지 않습니다."));
    else if(PC->Menu==TEXT("Party")) AddText(Body,TEXT("동료 시스템 연결 지점입니다. 아직 편성된 동료가 없습니다."));
    else AddText(Body,PC->Menu==TEXT("Load")?TEXT("사망 - 저장 파일을 불러오세요."):TEXT("일시정지 중입니다."),22);
}
