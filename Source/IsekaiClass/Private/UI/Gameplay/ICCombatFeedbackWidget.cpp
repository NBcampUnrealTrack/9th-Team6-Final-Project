#include "UI/Gameplay/ICCombatFeedbackWidget.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "Settings/ICGameUserSettings.h"

void UICDamageNumberWidget::SetDamage(float Value)
{
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Damage"))))
    {
        Text->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
        Text->SetColorAndOpacity(bPlayerDamage?FLinearColor(1,.19f,.16f):FLinearColor(1,.88f,.46f));
    }
}
void UICMonsterHealthWidget::SetEnemy(AICEnemyBase* Value){Enemy=Value;RefreshHealth();}
void UICMonsterHealthWidget::RefreshHealth()
{
    auto* ASC=Enemy.IsValid()?Enemy->GetICAbilitySystem():nullptr;
    if(auto* Bar=Cast<UProgressBar>(GetWidgetFromName(TEXT("BAR_MonsterHP"))))
    {
        const auto Snapshot=ASC?ASC->Capture():FICCharacterState();
        Bar->SetPercent(Snapshot.MaxHP>0?FMath::Clamp(Snapshot.HP/Snapshot.MaxHP,0.f,1.f):0);
    }
    SetVisibility(ASC&&ASC->IsAlive()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}
void UICMonsterHealthWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{Super::NativeTick(Geometry,DeltaSeconds);RefreshHealth();}
void UICRewardNoticeWidget::SetReward(int32 Gold,const TArray<FICItemAmount>& Items,FName MonsterID,int32 Essence)
{
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Gold"))))
    {
        Text->SetText(FText::Format(NSLOCTEXT("ICCombat","GoldReceived","+ {0} 골드"),FText::AsNumber(Gold)));
        Text->SetVisibility(Gold>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    }
    FString Lines;
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    for(const auto& Item:Items)
    {
        FICDefinitionResult R;const auto* Definition=D?D->Find<FICItemRow>(Item.ItemID,R):nullptr;
        const FText Name=Definition&&!Definition->DisplayName.IsEmpty()?Definition->DisplayName:FText::FromName(Item.ItemID);
        if(!Lines.IsEmpty())Lines+=TEXT("\n");
        Lines+=FText::Format(NSLOCTEXT("ICCombat","ItemReceived","+ {0} {1}개 획득"),Name,FText::AsNumber(Item.Count)).ToString();
    }
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Essence"))))
    {
        FICDefinitionResult R;const auto* Monster=D?D->Find<FICMonsterRow>(MonsterID,R):nullptr;
        FText Name=FText::FromName(MonsterID);
        if(Monster)
        {
            const auto Lookup=UICTextLibrary::ResolveText(LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")),Monster->NameKey);
            if(Lookup.bSuccess)Name=Lookup.Text;
            else if(!Monster->PetName.IsEmpty())Name=Monster->PetName;
        }
        Text->SetText(FText::Format(NSLOCTEXT("ICCombat","EssenceReceived","+ {0} {1} 정수 획득"),Name,FText::AsNumber(Essence)));
        Text->SetVisibility(Essence>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    }
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Items"))))
    {
        Text->SetText(FText::FromString(Lines));
        Text->SetVisibility(Lines.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    }
}
UICCombatFeedbackWidget::UICCombatFeedbackWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
    PlayerDamageClass=FSoftClassPath(TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_PlayerDamage.WBP_IC_PlayerDamage_C"));
    MonsterDamageClass=FSoftClassPath(TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_MonsterDamage.WBP_IC_MonsterDamage_C"));
    RewardNoticeClass=FSoftClassPath(TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_RewardNotice.WBP_IC_RewardNotice_C"));
}
void UICCombatFeedbackWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BoundState=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(BoundState)
    {
        BoundState->OnCombatDamageReceived.AddUniqueDynamic(this,&ThisClass::ShowDamage);
        BoundState->OnMonsterRewardReceived.AddUniqueDynamic(this,&ThisClass::ShowReward);
        BoundState->OnItemsReceived.AddUniqueDynamic(this,&ThisClass::ShowItems);
        BoundState->OnWorldRestored.AddUniqueDynamic(this,&ThisClass::ClearEntries);
    }
}
void UICCombatFeedbackWidget::NativeDestruct()
{
    if(BoundState)
    {
        BoundState->OnCombatDamageReceived.RemoveDynamic(this,&ThisClass::ShowDamage);
        BoundState->OnMonsterRewardReceived.RemoveDynamic(this,&ThisClass::ShowReward);
        BoundState->OnItemsReceived.RemoveDynamic(this,&ThisClass::ShowItems);
        BoundState->OnWorldRestored.RemoveDynamic(this,&ThisClass::ClearEntries);
    }
    ClearEntries();BoundState=nullptr;Super::NativeDestruct();
}
void UICCombatFeedbackWidget::ClearEntries()
{
    for(auto& Entry:FloatingEntries)if(Entry.Widget)Entry.Widget->RemoveFromParent();
    FloatingEntries.Reset();
}
void UICCombatFeedbackWidget::ShowDamage(AActor* Victim,float Amount)
{
    if(!UICGameUserSettings::Get()->bDamageNumbers)return;
    auto* Canvas=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("FeedbackCanvas")));
    if(!Canvas||!IsValid(Victim)||Amount<=0||!GetOwningPlayer())return;
    const bool Player=Victim==GetOwningPlayerPawn();
    if(!Player&&!Victim->ActorHasTag(TEXT("IC.Enemy")))return;
    auto* Class=(Player?PlayerDamageClass:MonsterDamageClass).LoadSynchronous();
    auto* Number=Class?CreateWidget<UICDamageNumberWidget>(GetOwningPlayer(),Class):nullptr;
    if(!Number)return;
    Number->SetDamage(Amount);Number->SetVisibility(ESlateVisibility::HitTestInvisible);
    FVector Origin,Extent;Victim->GetActorBounds(true,Origin,Extent);
    const FVector Position=Origin+FVector(0,0,Extent.Z*.7f);
    auto* Placement=Canvas->AddChildToCanvas(Number);Placement->SetSize({140,60});Placement->SetAlignment({.5f,.5f});
    FVector2D Screen;
    Number->SetVisibility(UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),Position,Screen,false)?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    const auto ViewportGeometry=UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
    Placement->SetPosition(GetCachedGeometry().AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(Screen)));
    auto& Entry=FloatingEntries.AddDefaulted_GetRef();Entry.Widget=Number;Entry.WorldPosition=Position;
    if(FloatingEntries.Num()>64){FloatingEntries[0].Widget->RemoveFromParent();FloatingEntries.RemoveAt(0);}
}
void UICCombatFeedbackWidget::ShowReward(int32 Gold,const TArray<FICItemAmount>& Items,FName MonsterID,int32 Essence)
{
    auto* Canvas=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("FeedbackCanvas")));
    if(!Canvas||!GetOwningPlayer()||(Gold<=0&&Items.IsEmpty()&&Essence<=0))return;
    auto* Class=RewardNoticeClass.LoadSynchronous();
    auto* Notice=Class?CreateWidget<UICRewardNoticeWidget>(GetOwningPlayer(),Class):nullptr;
    if(!Notice)return;
    Notice->SetReward(Gold,Items,MonsterID,Essence);Notice->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* Placement=Canvas->AddChildToCanvas(Notice);Placement->SetAnchors(FAnchors(.6f,.62f));Placement->SetSize({460,132});
    auto& Entry=FloatingEntries.AddDefaulted_GetRef();Entry.Widget=Notice;Entry.bReward=true;
    int32 Count=0;
    for(int32 Index=FloatingEntries.Num()-1;Index>=0;--Index)if(FloatingEntries[Index].bReward&&++Count>4)
    {FloatingEntries[Index].Widget->RemoveFromParent();FloatingEntries.RemoveAt(Index);}
}
void UICCombatFeedbackWidget::ShowItems(const TArray<FICItemAmount>& Items)
{ ShowReward(0,Items,NAME_None,0); }
void UICCombatFeedbackWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);int32 RewardIndex=0;
    for(int32 Index=FloatingEntries.Num()-1;Index>=0;--Index)
    {
        auto& Entry=FloatingEntries[Index];Entry.Age+=DeltaSeconds;
        const float Lifetime=Entry.bReward?4.f:1.1f;
        if(!Entry.Widget||Entry.Age>=Lifetime)
        {if(Entry.Widget)Entry.Widget->RemoveFromParent();FloatingEntries.RemoveAt(Index);continue;}
        Entry.Widget->SetRenderOpacity(FMath::Clamp((Lifetime-Entry.Age)/.35f,0.f,1.f));
        auto* Placement=Cast<UCanvasPanelSlot>(Entry.Widget->Slot);
        if(!Placement)continue;
        if(Entry.bReward)Placement->SetPosition({0,RewardIndex++*132.f});
        else
        {
            FVector2D Screen;const bool Visible=GetOwningPlayer()&&UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),Entry.WorldPosition,Screen,false);
            Entry.Widget->SetVisibility(Visible?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
            const auto ViewportGeometry=UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
            const FVector2D Local=Geometry.AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(Screen));
            Placement->SetPosition(Local+FVector2D(0,-Entry.Age*60));
        }
    }
}
