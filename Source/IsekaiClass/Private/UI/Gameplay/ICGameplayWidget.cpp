#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICCompanionGroupWidget.h"
#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Gameplay/ICQuickSlotsWidget.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Rendering/DrawElementTypes.h"

AICPlayerController* UICGameplayWidget::GetICPlayerController() const
{
    IC_TRACE_FUNCTION();
    return Cast<AICPlayerController>(GetOwningPlayer());
}

void UICGameplayWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("HUD"));
    RefreshSkillCooldown();
    SetIsFocusable(true);
    if (auto* Menu = Cast<UICGameMenuWidget>(GetWidgetFromName(TEXT("GameMenu"))))
    {
        Menu->OnResumeRequested.AddUniqueDynamic(this, &ThisClass::Resume);
        Menu->OnBackRequested.AddUniqueDynamic(this, &ThisClass::Back);
        Menu->OnSaveRequested.AddUniqueDynamic(this, &ThisClass::Save);
        Menu->OnLoadRequested.AddUniqueDynamic(this, &ThisClass::Load);
        Menu->OnGameSettingsRequested.AddUniqueDynamic(this, &ThisClass::GameSettings);
        Menu->OnQuitRequested.AddUniqueDynamic(this, &ThisClass::Quit);
        return;
    }
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Resume"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Resume);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Save"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Save);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Load"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Load);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Settings"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Settings);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_GameSettings"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::GameSettings);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Quit);
    if (auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("MenuScroll")));Scroll&&!GetWidgetFromName(TEXT("WeaponSlots")))
    {
        auto* Body=GetWidgetFromName(TEXT("TXT_MenuBody"));
        Scroll->ClearChildren();
        auto* Slots=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("WeaponSlots"));
        Scroll->AddChild(Slots);
        for (int32 Index=0;Index<4;++Index)
        {
            auto* Border=WidgetTree->ConstructWidget<UBorder>();
            Border->SetPadding(FMargin(10));Border->SetBrushColor(FLinearColor(.07f,.10f,.15f,1));
            auto* Label=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*FString::Printf(TEXT("TXT_WeaponSlot%d"),Index)));
            Label->SetFont(ICFrontendLayout::Font(16,false));Label->SetAutoWrapText(true);
            Border->SetContent(Label);
            auto* CellSlot=Slots->AddChildToHorizontalBox(Border);
            CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));CellSlot->SetPadding(FMargin(3,8));
        }
        Slots->SetVisibility(ESlateVisibility::Collapsed);
        if (Body) Scroll->AddChild(Body);
    }
}

void UICGameplayWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    RefreshSkillCooldown();
}

int32 UICGameplayWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,
    FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool ParentEnabled) const
{
    const int32 Top=Super::NativePaint(Args,Geometry,Culling,Elements,Layer,Style,ParentEnabled);
    const auto* PC=GetICPlayerController();const auto* Inventory=PC?PC->GetInventory():nullptr;
    if(!PC||!PC->bCreated||PC->IsDead()||!PC->Menu.IsEmpty()||PC->IsPaused()||!PC->GetPawn()||!Inventory)return Top;
    const auto Family=Inventory->GetEquippedWeaponFamily();
    if(Family!=EICWeaponFamily::Bow&&Family!=EICWeaponFamily::Staff&&Family!=EICWeaponFamily::Wand)return Top;
    const FVector2D Centre=Geometry.GetLocalSize()*.5;
    for(FVector2D Axis:{FVector2D(1,0),FVector2D(-1,0),FVector2D(0,1),FVector2D(0,-1)})
    {
        const TArray<FVector2D> Points{Centre+Axis*3,Centre+Axis*9};
        FSlateDrawElement::MakeLines(Elements,Top+1,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(0,0,0,.85f),true,3);
        FSlateDrawElement::MakeLines(Elements,Top+2,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(1,1,1,.9f),true,1);
    }
    return Top+2;
}

void UICGameplayWidget::RefreshSkillCooldown()
{
    auto* Status = Cast<UICCharacterStatusWidget>(GetWidgetFromName(TEXT("HUD_PlayerStatus")));
    if (!Status) return;
    auto* PC = GetICPlayerController();
    const FName SkillID = PC ? PC->GetCurrentWeaponSkill() : NAME_None;
    const auto* Definitions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    FICDefinitionResult Result;
    const auto* Skill = Definitions ? Definitions->Find<FICSkillRow>(SkillID, Result) : nullptr;
    const double RemainingSeconds = FMath::Max(0., PC->GetCurrentWeaponSkillCooldown());
    const double Duration = Skill ? Skill->CooldownPlaySeconds : 0.;
    const float Progress = Duration > 0. ? FMath::Clamp(float(1. - RemainingSeconds / Duration), 0.f, 1.f) : 1.f;
    Status->UpdateSkillCooldown(!SkillID.IsNone(), Progress, RemainingSeconds);
}

void UICGameplayWidget::RefreshCurrentScreen()
{
    auto* PC = GetICPlayerController();
    if (!PC)
        return;
    if (auto* Status = Cast<UICCharacterStatusWidget>(GetWidgetFromName(TEXT("HUD_PlayerStatus"))))
    {
        auto* State = PC->GetWorldState();
        const bool HasPlayer = State && State->GetStoredCharacter(UICWorldStateSubsystem::PlayerId());
        const FICWorldState Info = State ? State->ExportState() : FICWorldState();
        const int32 Preset = HasPlayer ? Info.Preset : PC->Preset;
        Status->CharacterID = UICWorldStateSubsystem::PlayerId();
        Status->DisplayName = FText::FromString(HasPlayer ? Info.PlayerName : PC->PlayerName);
        Status->Portrait = nullptr;
        auto* Definitions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
        if (Definitions && !Status->DisplayName.IsEmpty() && Preset >= 0 && Preset <= 1)
        {
            FICDefinitionResult Result;
            const FName ID = Preset == 1 ? TEXT("PRESET_PLAYER_F") : TEXT("PRESET_PLAYER_M");
            if (const auto* Definition = Definitions->Find<FICPlayerPresetRow>(ID, Result))
                Status->Portrait = Definition->Portrait.LoadSynchronous();
        }
        FICCharacterState Snapshot;
        if (HasPlayer) Snapshot = PC->GetPlayerSnapshot();
        else Snapshot.HP = Snapshot.MP = Snapshot.SP = Snapshot.MaxHP = Snapshot.MaxMP = Snapshot.MaxSP = 0.f;
        Status->UpdateVitals(Snapshot);
    }
    if (auto* Companions = Cast<UICCompanionGroupWidget>(GetWidgetFromName(TEXT("HUD_Companions"))))
        Companions->RefreshCompanions();
    if (auto* QuickSlots = Cast<UICQuickSlotsWidget>(GetWidgetFromName(TEXT("HUD_QuickSlots"))))
        QuickSlots->RefreshQuickSlots();
    RefreshSkillCooldown();
    if (auto* Menu = Cast<UICGameMenuWidget>(GetWidgetFromName(TEXT("GameMenu"))))
    {
        if (auto* Hint = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Hint"))))
            Hint->SetText(FText::FromString(PC->InteractionHint.IsEmpty() ? TEXT("") : TEXT("[F] ") + PC->InteractionHint));
        if (auto* Notice = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Notice"))))
            Notice->SetText(PC->NoticeDisplayText);
        const bool Open = !PC->Menu.IsEmpty() && PC->Menu != TEXT("Death") && PC->Menu != TEXT("Loading") && PC->Menu != TEXT("Create") &&
            PC->Menu != TEXT("Settings") && PC->Menu != TEXT("Dialogue");
        Menu->SetVisibility(Open ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Menu->SetSessionPresentation(PC->Menu, PC->Menu == TEXT("Confirm") ? PC->ModalDisplayText : PC->NoticeDisplayText);
        OnRefreshGameplayUI();
        return;
    }
    auto SetText = [this](const TCHAR* Name, const FString& Value) {
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(Name)))
            Text->SetText(FText::FromString(Value));
    };
    SetText(TEXT("TXT_Player"), PC->PlayerName);
    const auto Snapshot = PC->GetPlayerSnapshot();
    const float Values[] = {Snapshot.HP, Snapshot.MP, Snapshot.SP};
    const float Maximums[] = {Snapshot.MaxHP, Snapshot.MaxMP, Snapshot.MaxSP};
    const TCHAR* Keys[] = {TEXT("HP"), TEXT("MP"), TEXT("SP")};
    for (int32 I = 0; I < 3; ++I)
    {
        SetText(*FString::Printf(TEXT("TXT_%s"), Keys[I]),
                FString::Printf(TEXT("%s  %d / %d"), Keys[I], FMath::FloorToInt(Values[I]),
                                FMath::FloorToInt(Maximums[I])));
        if (auto* Bar = Cast<UProgressBar>(GetWidgetFromName(FName(*FString::Printf(TEXT("BAR_%s"), Keys[I])))))
            Bar->SetPercent(Maximums[I] > 0 ? FMath::Clamp(Values[I] / Maximums[I], 0.f, 1.f) : 0);
    }
    const FString CombatHint=PC->bInCombat ? TEXT("전투 중") : TEXT("");
    SetText(TEXT("TXT_Hint"), CombatHint+(PC->InteractionHint.IsEmpty()?TEXT(""):(CombatHint.IsEmpty()?TEXT("[F] "):TEXT("\n[F] "))+PC->InteractionHint));
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Notice"))))
        Text->SetText(PC->NoticeDisplayText);
    const bool HasPanel = !PC->Menu.IsEmpty() && PC->Menu != TEXT("Death") && PC->Menu != TEXT("Loading") && PC->Menu != TEXT("Create") &&
                          PC->Menu != TEXT("Settings") && PC->Menu != TEXT("Dialogue");
    if (auto* Panel = GetWidgetFromName(TEXT("MenuPanel")))
        Panel->SetVisibility(HasPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (HasPanel)
    {
        const bool Confirm = PC->Menu == TEXT("Confirm");
        const bool Exit = PC->Menu == TEXT("Quit");
        const bool MainMenu = PC->Menu == TEXT("Pause");
        const bool SettingsMenu = PC->Menu == TEXT("MenuSettings");
        const bool ShowWeapons=PC->GetInventory()&&(PC->Menu==TEXT("Inventory")||PC->Menu==TEXT("Equipment"));
        if (auto* Slots=GetWidgetFromName(TEXT("WeaponSlots")))
            Slots->SetVisibility(ShowWeapons?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if (ShowWeapons)
        {
            auto* Inventory=PC->GetInventory();const auto Slots=Inventory->GetWeaponLoadout();
            const TCHAR* Names[]={TEXT("WeaponSlot_Sword"),TEXT("WeaponSlot_Bow"),TEXT("WeaponSlot_Staff"),TEXT("WeaponSlot_Wand")};
            for (int32 Index=0;Index<4;++Index)
            {
                FPlayerItemDefinition Item;
                const bool Filled=!Slots[Index].IsEmpty()&&Inventory->GetItemDefinition(Slots[Index].ItemID,Item);
                const bool Active=static_cast<int32>(Inventory->GetEquippedWeaponFamily())==Index+1;
                const FText Status=ICText::Get(!Filled?TEXT("WeaponSlot_Empty"):Active?TEXT("WeaponSlot_Active"):TEXT("WeaponSlot_Equipped"));
                SetText(*FString::Printf(TEXT("TXT_WeaponSlot%d"),Index),ICText::Get(Names[Index]).ToString()+TEXT("\n\n")
                    +(Filled?Item.DisplayName.ToString()+TEXT("\n"):TEXT(""))+Status.ToString());
            }
        }
        SetText(TEXT("TXT_Menu"), PC->IsDead()                ? ICText::Get(TEXT("Menu_Dead")).ToString()
                                  : Confirm                   ? ICText::Get(TEXT("Menu_Confirm")).ToString()
                                  : Exit                      ? ICText::Get(TEXT("Menu_Exit")).ToString()
                                  : MainMenu                  ? ICText::Get(TEXT("Menu")).ToString()
                                  : SettingsMenu              ? ICText::Get(TEXT("Settings")).ToString()
                                  : PC->Menu==TEXT("Equipment") ? ICText::Get(TEXT("Equipment")).ToString()
                                                              : PC->Menu);
        FString Body = Confirm ? PC->ModalText : PC->Notice;
        if (PC->Menu == TEXT("Inventory") && PC->GetInventory())
        {
            Body = ICText::Format(TEXT("Native_d52a2f45cb55"), PC->Gold);
            for (const auto& Item : PC->GetInventory()->GetSnapshot().Bag.Slots)
                if (!Item.IsEmpty())
                {
                    FICDefinitionResult Result;
                    const auto* Definition =
                        PC->GetInventory()->GetDefinitions()->Find<FICItemRow>(Item.ItemID, Result);
                    Body += FString::Printf(TEXT("%s  × %d\n"),
                                            Definition ? *Definition->DisplayName.ToString() : *Item.ItemID.ToString(),
                                            Item.Count);
                }
        }
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_MenuBody"))))
            Text->SetText(Confirm ? PC->ModalDisplayText : PC->Menu == TEXT("Inventory") ? FText::FromString(Body)
                : PC->Menu==TEXT("Equipment")?ICText::Get(TEXT("WeaponSlot_CycleHint")):PC->NoticeDisplayText);
        if (auto* Scroll = GetWidgetFromName(TEXT("MenuScroll")))
            Scroll->SetVisibility(MainMenu || SettingsMenu ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        auto Label = [this](const TCHAR* Name, const TCHAR* Key) {
            if (auto* Button = Cast<UButton>(GetWidgetFromName(Name)))
                if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
                    Text->SetText(ICFrontendLayout::Text(Key));
        };
        Label(TEXT("BTN_Resume"), Confirm ? TEXT("Accept") : Exit ? TEXT("Back") : MainMenu ? TEXT("BackToGame") : TEXT("Resume"));
        Label(TEXT("BTN_Quit"), Confirm ? TEXT("Back") : Exit ? TEXT("QuitWithoutSave") : TEXT("Quit"));
        for (const auto* Name : {TEXT("BTN_Party"), TEXT("BTN_Inventory"), TEXT("BTN_Quest"), TEXT("BTN_Codex")})
            if (auto* Button = GetWidgetFromName(Name))
                Button->SetVisibility(MainMenu ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        for (const auto* Name : {TEXT("BTN_Resume"), TEXT("BTN_Save"), TEXT("BTN_Load"),
                                TEXT("BTN_Settings"), TEXT("BTN_GameSettings"), TEXT("BTN_Quit")})
            if (auto* Button = GetWidgetFromName(Name))
            {
                const bool Visible = MainMenu ? FCString::Strcmp(Name, TEXT("BTN_Settings")) == 0 || FCString::Strcmp(Name, TEXT("BTN_Resume")) == 0
                    : SettingsMenu ? FCString::Strcmp(Name, TEXT("BTN_Settings")) != 0
                    : Confirm ? FCString::Strcmp(Name, TEXT("BTN_Resume")) == 0 || FCString::Strcmp(Name, TEXT("BTN_Quit")) == 0
                    : FCString::Strcmp(Name, TEXT("BTN_Settings")) != 0;
                Button->SetVisibility(Visible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            }
    }
    OnRefreshGameplayUI();
}
FReply UICGameplayWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    auto* PC = GetICPlayerController();
    if (PC && PC->HandleSessionKey(Event.GetKey(), Event.IsRepeat()))
        return FReply::Handled();
    if(auto* Menu=Cast<UICGameMenuWidget>(GetWidgetFromName(TEXT("GameMenu")));Menu&&Menu->IsVisible())
        if(Menu->HandleMenuKey(Event.GetKey(),Event.IsRepeat()))return FReply::Handled();
    if (PC &&
        (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Equals || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        if (!Event.IsRepeat()) PC->HandleInputCommand(EICInputCommand::Menu);
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}
void UICGameplayWidget::Resume()
{
    if (auto* PC = GetICPlayerController())
    {
        if (PC->Menu == TEXT("Confirm"))
            PC->ConfirmModal(true);
        else if (PC->Menu == TEXT("Quit"))
            PC->CloseSessionMenu();
        else
            PC->SetMenu(TEXT(""));
    }
}
void UICGameplayWidget::Back()
{
    if (auto* PC = GetICPlayerController()) PC->HandleInputCommand(EICInputCommand::Menu);
}
void UICGameplayWidget::Save()
{
    if (auto* PC = GetICPlayerController())
        PC->OpenSaveMenu(PC->Menu == TEXT("Quit"));
}
void UICGameplayWidget::Load()
{
    if (auto* PC = GetICPlayerController())
        PC->OpenLoadMenu();
}
void UICGameplayWidget::Settings()
{
    if (auto* PC = GetICPlayerController())
        PC->SetMenu(TEXT("MenuSettings"));
}
void UICGameplayWidget::GameSettings()
{
    if (auto* PC = GetICPlayerController())
        PC->SetMenu(TEXT("Settings"));
}
void UICGameplayWidget::Quit()
{
    if (auto* PC = GetICPlayerController())
    {
        if (PC->Menu == TEXT("Confirm"))
            PC->ConfirmModal(false);
        else if (PC->Menu == TEXT("Quit"))
            PC->QuitWithoutSave();
        else
            PC->RequestQuit();
    }
}
