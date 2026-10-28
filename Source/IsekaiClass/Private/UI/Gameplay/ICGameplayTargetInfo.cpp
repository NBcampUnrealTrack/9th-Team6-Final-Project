#include "UI/Gameplay/ICGameplayWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Text/ICLocalization.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UICGameplayWidget::RefreshTargetInfo()
{
    auto* Panel=GetWidgetFromName(TEXT("HUD_TargetInfo"));
    if(!Panel)return;
    auto* PC=GetICPlayerController();
    auto* Enemy=PC?Cast<AICEnemyBase>(PC->GetHUDCombatTarget()):nullptr;
    Panel->SetVisibility(Enemy&&PC->Menu.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
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
