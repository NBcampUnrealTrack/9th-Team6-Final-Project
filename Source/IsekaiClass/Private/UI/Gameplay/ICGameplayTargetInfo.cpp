#include "UI/Gameplay/ICGameplayWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Text/ICLocalization.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UICGameplayWidget::RefreshTargetInfo()
{
    auto* PC=GetICPlayerController();
    auto* Boss=PC?PC->GetHUDBossTarget():nullptr;
    if(auto* BossPanel=GetWidgetFromName(TEXT("HUD_BossBar")))
        BossPanel->SetVisibility(Boss&&PC->Menu.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(Boss)
    {
        const auto& Definition=Boss->GetMonsterDefinition();const auto State=Boss->GetICAbilitySystem()->Capture();
        if(auto* Name=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_BossName"))))
            Name->SetText(Definition.NameKey.IsEmpty()?FText::FromName(Definition.ID):ICLocalization::Localize(FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Definition.NameKey)));
        if(auto* HP=Cast<UProgressBar>(GetWidgetFromName(TEXT("BAR_BossHP"))))HP->SetPercent(State.MaxHP>0?FMath::Clamp(State.HP/State.MaxHP,0.f,1.f):0.f);
        if(auto* Health=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_BossHP"))))Health->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"),FMath::CeilToInt(State.HP),FMath::CeilToInt(State.MaxHP))));
    }
    auto* Panel=GetWidgetFromName(TEXT("HUD_TargetInfo"));
    if(!Panel)return;
    auto* Enemy=PC?Cast<AICEnemyBase>(PC->GetHUDCombatTarget()):nullptr;
    Panel->SetVisibility(Enemy&&Enemy->GetMonsterDefinition().Kind==EICMonsterKind::Normal&&PC->Menu.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(!Enemy)return;
    const auto& Definition=Enemy->GetMonsterDefinition();
    if(auto* Name=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_49"))))
        Name->SetText(Definition.NameKey.IsEmpty()?FText::FromName(Definition.ID):ICLocalization::Localize(FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Definition.NameKey)));
    if(auto* HP=Cast<UProgressBar>(GetWidgetFromName(TEXT("ProgressBar_13"))))
    {
        const auto State=Enemy->GetICAbilitySystem()->Capture();
        HP->SetPercent(State.MaxHP>0?FMath::Clamp(State.HP/State.MaxHP,0.f,1.f):0.f);
    }
}
