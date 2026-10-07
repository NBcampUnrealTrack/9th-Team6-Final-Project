#include "ICCharacterCreateWidget.h"
#include "PlayerPrototypeController.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

void UICCharacterCreateWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (IC_Create_Back)
        IC_Create_Back->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
    UButton* JobButtons[] = { IC_Create_Warrior.Get(), IC_Create_Archer.Get(), IC_Create_Mage.Get(), IC_Create_Healer.Get() };
    DefaultJobStyles.Reset();
    for (UButton* Button : JobButtons)
        DefaultJobStyles.Add(Button ? Button->GetStyle() : FButtonStyle());
    if (IC_Create_Warrior)
        IC_Create_Warrior->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWarriorClicked);
    if (IC_Create_Archer)
        IC_Create_Archer->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleArcherClicked);
    if (IC_Create_Mage)
        IC_Create_Mage->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMageClicked);
    if (IC_Create_Healer)
        IC_Create_Healer->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHealerClicked);

    if (IC_Create_Name)
        IC_Create_Name->OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleNameChanged);
    if (IC_Create_Confirm)
        IC_Create_Confirm->OnClicked.AddUniqueDynamic(this, &ThisClass::CreateCharacter);
}
void UICCharacterCreateWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        if (IC_Create_Name) IC_Create_Name->SetText(FText::FromString(PC->PlayerName));
    RefreshState();
}
void UICCharacterCreateWidget::RefreshState()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
    {
        const bool bCanEdit = PC->CanEditCreation();
        if (IC_Create_Name) IC_Create_Name->SetIsReadOnly(!bCanEdit);
        if (IC_Create_Confirm) IC_Create_Confirm->SetIsEnabled(PC->CanSubmitCreation());
        if (IC_Create_Notice) IC_Create_Notice->SetText(FText::FromString(PC->Notice));
        OnCreationStateChanged(PC->Preset, PC->Job, bCanEdit);
        UpdateJobButtons(PC->Job, bCanEdit);
    }
}
void UICCharacterCreateWidget::HandleNameChanged(const FText& Text)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        if (PC->CanEditCreation()) PC->PlayerName = Text.ToString();
}
void UICCharacterCreateWidget::SelectPreset(int32 Index)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        PC->ExecuteUI(TEXT("Preset"), Index);
}
void UICCharacterCreateWidget::SelectJob(int32 Index)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
    {
        if (!PC->CanEditCreation() || Index < 0 || Index > 3) return;
        PC->ExecuteUI(TEXT("Job"), Index);
        RefreshState();
    }
}
void UICCharacterCreateWidget::CreateCharacter()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
    {
        if(PC->CanEditCreation() && IC_Create_Name) PC->PlayerName=IC_Create_Name->GetText().ToString();
        PC->RequestCreatePlayer();
    }
}
void UICCharacterCreateWidget::OpenLoadScreen()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer())) PC->OpenLoadMenu();
}
void UICCharacterCreateWidget::OpenQuitScreen()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer())) PC->RequestQuit();
}
int32 UICCharacterCreateWidget::GetSelectedPreset() const
{
    const auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer());
    return PC ? PC->Preset : 0;
}
int32 UICCharacterCreateWidget::GetSelectedJob() const
{
    const auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer());
    return PC ? PC->Job : 0;
}

void UICCharacterCreateWidget::UpdateJobButtons(int32 Job, bool bCanEdit)
{
    UButton* JobButtons[] = { IC_Create_Warrior.Get(), IC_Create_Archer.Get(), IC_Create_Mage.Get(), IC_Create_Healer.Get() };
    // Localizable defaults; String Table entries can replace these when the table is supplied.
    const FText JobNames[] = {
        NSLOCTEXT("ICCharacterCreation", "JobWarrior", "전사"),
        NSLOCTEXT("ICCharacterCreation", "JobArcher", "궁사"),
        NSLOCTEXT("ICCharacterCreation", "JobMage", "마법사"),
        NSLOCTEXT("ICCharacterCreation", "JobHealer", "치료사")
    };
    const FText JobDescriptions[] = {
        NSLOCTEXT("ICCharacterCreation", "JobWarriorDescription", "검을 사용하는 근접 물리 직업.\n전방에서 적을 상대하며 동료를 보호합니다.\n시작 구성: 초보자 검 · 전사 1번 스킬"),
        NSLOCTEXT("ICCharacterCreation", "JobArcherDescription", "활을 사용하는 원거리 물리 직업.\n거리를 확보하며 적을 공격합니다.\n시작 구성: 초보자 활 · 궁사 1번 스킬"),
        NSLOCTEXT("ICCharacterCreation", "JobMageDescription", "마법스태프를 사용하는 마법 공격 직업.\n마법으로 적을 공격하며 전투를 이끕니다.\n시작 구성: 초보자 마법스태프 · 마법사 1번 스킬"),
        NSLOCTEXT("ICCharacterCreation", "JobHealerDescription", "치유완드를 사용하는 치유·지원 직업.\n동료를 치유하고 전투를 지원합니다.\n시작 구성: 초보자 치유완드 · 치료사 1번 스킬")
    };
    const bool bValidJob = Job >= 0 && Job < 4;
    if (IC_Create_WeaponName)
        IC_Create_WeaponName->SetText(bValidJob ? JobNames[Job] : FText::GetEmpty());
    if (IC_Create_WeaponDescription)
        IC_Create_WeaponDescription->SetText(bValidJob ? JobDescriptions[Job] : FText::GetEmpty());
    UTexture2D* WeaponImages[] = { IC_Create_Weapon_Warrior.Get(), IC_Create_Weapon_Archer.Get(), IC_Create_Weapon_Mage.Get(), IC_Create_Weapon_Healer.Get() };
    if (IC_Create_Weapon_Icon)
    {
        UTexture2D* WeaponTexture = (Job >= 0 && Job < 4) ? WeaponImages[Job] : nullptr;
        // Keep the Designer brush size and layout, including when clearing a missing icon.
        FSlateBrush WeaponBrush = IC_Create_Weapon_Icon->GetBrush();
        WeaponBrush.SetResourceObject(WeaponTexture);
        IC_Create_Weapon_Icon->SetBrush(WeaponBrush);
        IC_Create_Weapon_Icon->SetVisibility(WeaponTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UButton* Button = JobButtons[Index];
        if (!Button || !DefaultJobStyles.IsValidIndex(Index)) continue;
        FButtonStyle ButtonStyle = DefaultJobStyles[Index];
        if (Index == Job && IC_Create_Job_Selected.Get())
        {
            // Preserve the brush size, tint and margins authored in the Designer.
            ButtonStyle.Normal.SetResourceObject(IC_Create_Job_Selected.Get());
            ButtonStyle.Hovered.SetResourceObject(IC_Create_Job_Selected.Get());
            ButtonStyle.Pressed.SetResourceObject(IC_Create_Job_Selected.Get());
        }
        Button->SetStyle(ButtonStyle);
        Button->SetIsEnabled(bCanEdit);
    }
}
void UICCharacterCreateWidget::HandleWarriorClicked() { SelectJob(0); }
void UICCharacterCreateWidget::HandleArcherClicked() { SelectJob(1); }
void UICCharacterCreateWidget::HandleMageClicked() { SelectJob(2); }
void UICCharacterCreateWidget::HandleHealerClicked() { SelectJob(3); }

void UICCharacterCreateWidget::HandleBackClicked()
{
    if (!GetWorld() || MainMenuLevelName.IsNone()) return;
    // The character creation screen pauses gameplay; resume before changing levels.
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
