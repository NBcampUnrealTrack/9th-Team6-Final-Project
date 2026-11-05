#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Framework/ICWidgetTemplates.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Framework/ICInputPresentation.h"
#include "Core/Controller/ICPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Combat/Party/ICPartyStatus.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Data/Rows/Combat/ICStatusEffectRow.h"
#include "Data/Rows/Combat/ICSynergyRow.h"
#include "Data/Text/ICLocalization.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/Types/ICCharacterStateTypes.h"

void UICCharacterStatusWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshPresentation();
}

void UICCharacterStatusWidget::RefreshPresentation()
{
    if (auto* Name = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Name"))))
    {Name->SetText(bDowned?FText::Format(NSLOCTEXT("ICStatus","Downed","{0} · 전투불능"),DisplayName):DisplayName);Name->SetColorAndOpacity(bDowned?FLinearColor(.5f,.5f,.5f):FLinearColor(.92f,.94f,.98f));}
    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait"))))
    {
        Image->SetBrushFromTexture(Portrait);
        if(bDowned&&Portrait)if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IsekaiClass/UI/Materials/M_UI_DownedPortrait.M_UI_DownedPortrait")))
        {Image->SetBrushFromMaterial(Material);if(auto* Dynamic=Image->GetDynamicMaterial())Dynamic->SetTextureParameterValue(TEXT("Portrait"),Portrait);}
        Image->SetColorAndOpacity(bDowned?FLinearColor(.6f,.6f,.6f,1):FLinearColor::White);
        Image->SetVisibility(Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    const TCHAR* Names[] = {TEXT("BAR_HP"), TEXT("BAR_MP"), TEXT("BAR_SP")};
    const float Values[] = {HPPercent, MPPercent, SPPercent};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
        if (auto* Bar = Cast<UProgressBar>(GetWidgetFromName(Names[Index])))
        {Bar->SetPercent(bDowned?0.f:FMath::IsFinite(Values[Index]) ? FMath::Clamp(Values[Index], 0.f, 1.f) : 0.f);const FLinearColor Colors[]={FLinearColor(.76f,.17f,.18f),FLinearColor(.15f,.42f,.83f),FLinearColor(.76f,.63f,.18f)};Bar->SetFillColorAndOpacity(bDowned?FLinearColor(.28f,.28f,.28f):Colors[Index]);Bar->SetRenderOpacity(bDowned?.5f:1.f);}
    for (int32 Index=0; Index<3; ++Index)
        if (auto* Segment=Cast<UProgressBar>(GetWidgetFromName(FName(*FString::Printf(TEXT("BAR_MP%d"),Index+1)))))
        {
            Segment->SetPercent(bDowned?0.f:FMath::Clamp(MPPercent*3.f-Index,0.f,1.f));
            Segment->SetRenderOpacity(bDowned?.5f:1.f);
        }
    if(auto* Badge=GetWidgetFromName(TEXT("StatusBuffBadge")))Badge->SetVisibility(ESlateVisibility::Collapsed);
}

void UICCharacterStatusWidget::BuildSkillCooldownOverlay(UWidgetTree* Tree)
{
    if (!Tree || Tree->FindWidget(TEXT("PortraitCooldownOverlay"))) return;
    auto* Image = Cast<UImage>(Tree->FindWidget(TEXT("IMG_Portrait")));
    auto* PortraitBox = Image ? Cast<USizeBox>(Image->GetParent()) : nullptr;
    if (!PortraitBox) return;

    auto* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PortraitCooldownOverlay"));
    PortraitBox->SetContent(Overlay);
    auto Fill = [](UOverlaySlot* OverlaySlot)
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
        OverlaySlot->SetVerticalAlignment(VAlign_Fill);
    };
    Fill(Overlay->AddChildToOverlay(Image));

    auto* Remaining = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SkillCooldown"));
    Remaining->SetFont(ICFrontendLayout::Font(24, true));
    Remaining->SetJustification(ETextJustify::Center);
    Remaining->SetColorAndOpacity(FLinearColor::White);
    Remaining->SetShadowColorAndOpacity(FLinearColor::Black);
    Remaining->SetShadowOffset(FVector2D(2.f, 2.f));
    Remaining->SetVisibility(ESlateVisibility::Hidden);
    auto* RemainingSlot = Overlay->AddChildToOverlay(Remaining);
    RemainingSlot->SetHorizontalAlignment(HAlign_Fill);
    RemainingSlot->SetVerticalAlignment(VAlign_Center);

    auto* Key = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SkillKey"));
    Key->SetText(FText::FromString(TEXT("Q")));
    Key->SetFont(ICFrontendLayout::Font(16, true));
    Key->SetColorAndOpacity(FLinearColor(1.f, .77f, .3f));
    Key->SetShadowColorAndOpacity(FLinearColor::Black);
    Key->SetShadowOffset(FVector2D(1.f, 1.f));
    Key->SetVisibility(ESlateVisibility::Collapsed);
    auto* KeySlot = Overlay->AddChildToOverlay(Key);
    KeySlot->SetHorizontalAlignment(HAlign_Left);
    KeySlot->SetVerticalAlignment(VAlign_Bottom);
    KeySlot->SetPadding(FMargin(3.f));
    auto* Badge=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("StatusBuffBadge"));Badge->SetBrushColor(FLinearColor(.05f,.55f,.16f));Badge->SetPadding(FMargin(2,0));Badge->SetVisibility(ESlateVisibility::Collapsed);
    auto* Plus=Tree->ConstructWidget<UTextBlock>();Plus->SetText(FText::FromString(TEXT("+")));Plus->SetFont(ICFrontendLayout::Font(18,true));Plus->SetJustification(ETextJustify::Center);Badge->SetContent(Plus);
    auto* BadgeSlot=Overlay->AddChildToOverlay(Badge);BadgeSlot->SetHorizontalAlignment(HAlign_Right);BadgeSlot->SetVerticalAlignment(VAlign_Bottom);
}

void UICCharacterStatusWidget::UpdateSkillCooldown(bool bHasSkill, float Progress, double RemainingSeconds)
{
    const float SafeProgress = FMath::IsFinite(Progress) ? FMath::Clamp(Progress, 0.f, 1.f) : 1.f;
    const double SafeRemaining = FMath::IsFinite(RemainingSeconds) ? FMath::Max(0., RemainingSeconds) : 0.;
    const bool bCoolingDown = bHasSkill && SafeRemaining > KINDA_SMALL_NUMBER;
    const float Brightness = bDowned?.6f:bCoolingDown ? FMath::Lerp(.3f, 1.f, SafeProgress) : 1.f;

    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait"))))
    {
        Image->SetColorAndOpacity(FLinearColor(Brightness, Brightness, Brightness, 1.f));
        Image->SetRenderOpacity(Brightness);
    }
    if (auto* Remaining = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SkillCooldown"))))
    {
        Remaining->SetText(FText::AsNumber(FMath::CeilToInt(SafeRemaining)));
        Remaining->SetVisibility(bCoolingDown&&!bDowned ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (auto* Key = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SkillKey"))))
    {
        Key->SetText(ICInputPresentation::CommandLabel(Cast<AICPlayerController>(GetOwningPlayer()),EICInputCommand::Skill));
        Key->SetVisibility(bHasSkill&&(CharacterID.IsNone()||CharacterID==UICWorldStateSubsystem::PlayerId()) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UICCharacterStatusWidget::UpdateVitals(const FICCharacterState& State)
{
    bDowned=State.IsDowned()||State.HP<=0;
    const float Current[] = {State.HP, State.MP, State.SP};
    const float Maximum[] = {State.MaxHP, State.MaxMP, State.MaxSP};
    float* Percent[] = {&HPPercent, &MPPercent, &SPPercent};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Percent); ++Index)
    {
        const float Max = FMath::IsFinite(Maximum[Index]) ? FMath::Max(0.f, Maximum[Index]) : 0.f;
        const float Value = FMath::IsFinite(Current[Index]) ? FMath::Clamp(Current[Index], 0.f, Max) : 0.f;
        *Percent[Index] = Max > 0 ? Value / Max : 0.f;
    }
    RefreshPresentation();
}
void UICCharacterStatusWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);StatusRefreshAt-=DeltaSeconds;if(StatusRefreshAt>0)return;StatusRefreshAt=.25f;
    auto* GI=GetGameInstance();auto* W=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    if(W&&W->GetStoredCharacter(CharacterID))
    {
        const auto S=W->GetSnapshot(CharacterID); const auto World=W->ExportState();
        bHasBuff=ICPartyStatus::HasBuff(S,World,D); UpdateVitals(S);
        const auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
        const double Now=Time?Time->GetClock().PlaySecond:0.;
        UpdateStatusEffects(S,World,D,Now);
        if(S.Kind==EICCharacterKind::Recruitable&&D)
        {
            FICDefinitionResult Lookup;
            const auto* NPC=D->Find<FICNPCRow>(CharacterID,Lookup);
            const auto* Combat=NPC?D->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,Lookup):nullptr;
            const auto* Skill=Combat?D->Find<FICSkillRow>(Combat->UniqueSkillID,Lookup):nullptr;
            const double Remaining=Skill?FMath::Max(0.,S.Combat.SkillReadyAtPlaySecond.FindRef(Skill->ID)-Now):0.;
            const float Progress=Skill&&Skill->CooldownPlaySeconds>0?FMath::Clamp(float(1.-Remaining/Skill->CooldownPlaySeconds),0.f,1.f):1.f;
            UpdateSkillCooldown(Skill!=nullptr,Progress,Remaining);
        }
    }
}

void UICCharacterStatusWidget::UpdateStatusEffects(const FICCharacterState& State,const FICWorldState& World,UICDefinitionSubsystem* Definitions,double Now)
{
    auto* Row=Cast<UHorizontalBox>(GetWidgetFromName(TEXT("StatusEffects")));
    auto* Prototype=Cast<UImage>(GetWidgetFromName(TEXT("StatusEffectPrototype")));
    if(!Row||!Prototype||!Definitions)return;
    Row->SetFlowDirectionPreference(EFlowDirectionPreference::LeftToRight);
    struct FBadge { FName ID; FText Name; UTexture2D* Icon=nullptr; bool Harmful=false; int32 Stacks=1; double Remaining=-1; };
    TArray<FBadge> Badges;
    auto Name=[](const FICDefinitionRow& Rule){return Rule.NameKey.IsEmpty()?FText::FromName(Rule.ID):ICLocalization::Localize(FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Rule.NameKey));};
    FICDefinitionResult Lookup;
    if(!bDowned)
    {
        for(const auto& Effect:State.Combat.Effects)
            if(Effect.bInfinite||Effect.ExpiresAtPlaySecond>Now)
                if(const auto* Rule=Definitions->Find<FICStatusEffectRow>(Effect.EffectID,Lookup))
                    Badges.Add({FName(*Effect.InstanceID.ToString()),Name(*Rule),Rule->Presentation.Icon.LoadSynchronous(),Effect.bHarmful,Effect.Stacks,Effect.bInfinite?-1.:Effect.ExpiresAtPlaySecond-Now});
        for(const auto* Rule:ICPartyStatus::ActiveSynergies(World,Definitions))
            Badges.Add({Rule->ID,Name(*Rule)});
        if(const auto* Pet=World.Codex.CharacterPets.Find(State.CharacterId);Pet&&World.Codex.PetEssence.FindRef(*Pet)>=100)
            Badges.Add({FName(*FString::Printf(TEXT("PetBuff_%d"),*Pet)),NSLOCTEXT("ICStatus","PetBuff","펫 버프")});
    }
    // A compact HUD card cannot grow forever. Show dangerous effects first,
    // keep a fixed number of cells, and summarize the rest with a +N badge.
    Badges.StableSort([](const FBadge& A,const FBadge& B){return A.Harmful&&!B.Harmful;});
    const int32 Capacity=GetWidgetFromName(TEXT("BAR_SP"))?4:3;
    const int32 VisibleCount=Badges.Num()>Capacity?Capacity-1:Badges.Num();
    const int32 HiddenCount=Badges.Num()-VisibleCount;
    auto* Overflow=Cast<UBorder>(GetWidgetFromName(TEXT("StatusOverflow")));
    auto* OverflowText=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_StatusOverflow")));
    if(Overflow)Overflow->RemoveFromParent();
    TSet<FName> Active;
    for(int32 Index=0;Index<VisibleCount;++Index)Active.Add(Badges[Index].ID);
    for(auto It=EffectWidgets.CreateIterator();It;++It)
        if(!Active.Contains(It.Key())){It.Value()->RemoveFromParent();It.RemoveCurrent();}
    for(int32 Index=0;Index<VisibleCount;++Index)
    {
        const auto& Badge=Badges[Index];
        auto* Cell=EffectWidgets.FindRef(Badge.ID).Get();
        if(!Cell)
        {
            Cell=ICWidgetTemplates::Create(this,TEXT("WBP_IC_StatusEffect"));if(!Cell)continue;
            auto* Icon=CastChecked<UImage>(Cell->GetWidgetFromName(TEXT("IMG_Icon")));
            Icon->SetBrush(Prototype->GetBrush());
            if(Badge.Icon)Icon->SetBrushFromTexture(Badge.Icon);
            EffectWidgets.Add(Badge.ID,Cell);
        }
        Cell->RemoveFromParent();
        Row->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(2,0,0,0));
        CastChecked<UBorder>(Cell->GetWidgetFromName(TEXT("Frame")))->SetBrushColor(Badge.Harmful?FLinearColor(.75f,.08f,.08f,.9f):FLinearColor(.08f,.55f,.18f,.9f));
        CastChecked<UTextBlock>(Cell->GetWidgetFromName(TEXT("TXT_Count")))->SetText(Badge.Stacks>1?FText::AsNumber(Badge.Stacks):FText::GetEmpty());
        Cell->SetToolTipText(Badge.Remaining>=0?FText::Format(NSLOCTEXT("ICStatus","EffectTime","{0} · {1}초"),Badge.Name,FText::AsNumber(FMath::CeilToInt(Badge.Remaining))):Badge.Name);
    }
    if(Overflow&&OverflowText&&HiddenCount>0)
    {
        OverflowText->SetText(FText::Format(NSLOCTEXT("ICStatus","MoreEffects","+{0}"),FText::AsNumber(HiddenCount)));
        Overflow->SetToolTipText(FText::Format(NSLOCTEXT("ICStatus","HiddenEffects","숨겨진 상태 효과 {0}개"),FText::AsNumber(HiddenCount)));
        Overflow->SetVisibility(ESlateVisibility::HitTestInvisible);
        Row->AddChildToHorizontalBox(Overflow)->SetPadding(FMargin(2,0,0,0));
    }
    else if(Overflow)
    {
        Overflow->SetVisibility(ESlateVisibility::Collapsed);
        Row->AddChildToHorizontalBox(Overflow)->SetPadding(FMargin(2,0,0,0));
    }
}
