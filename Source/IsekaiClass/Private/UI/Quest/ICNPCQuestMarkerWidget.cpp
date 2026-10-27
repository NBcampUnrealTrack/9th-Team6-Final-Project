#include "UI/Quest/ICNPCQuestMarkerWidget.h"
#include "NPC/Character/ICNPCBase.h"
#include "Quest/ICQuestSubsystem.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UICNPCQuestMarkerWidget::SetNPC(AICNPCBase* Value) { NPC=Value; RefreshMarker(); }
void UICNPCQuestMarkerWidget::RefreshMarker()
{
    auto* Quests=GetGameInstance()?GetGameInstance()->GetSubsystem<UICQuestSubsystem>():nullptr;
    const int32 Marker=NPC.IsValid()&&Quests?Quests->GetNPCQuestMarker(NPC->NPCDefinitionID):0;
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("QuestMarkerText"))))
    {
        Text->SetText(FText::FromString(Marker==2?TEXT("?"):Marker==1?TEXT("!"):TEXT("")));
        Text->SetColorAndOpacity(Marker==2?FLinearColor(1,.38f,.035f):FLinearColor(.08f,1,.22f));
        // Keep the owner widget ticking even when the glyph is absent.
        Text->SetVisibility(Marker?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    }
}
void UICNPCQuestMarkerWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    RefreshRemaining-=DeltaSeconds;
    if(RefreshRemaining<=0){RefreshRemaining=.3f;RefreshMarker();}
}
