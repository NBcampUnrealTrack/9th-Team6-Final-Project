#include "Life/Gacha/ICGachaMachine.h"
#include "Inventory/Rewards/ICGachaService.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Debug/ICLog.h"

AICGachaMachine::AICGachaMachine()
{
    Action=TEXT("Gacha");InteractionText=NSLOCTEXT("ICGacha","MachineName","뽑기 기계");
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetRelativeScale3D(FVector(1.2,1.,1.5));Label->SetText(FText::FromString(TEXT("GACHA [F]")));
}
FText AICGachaMachine::GetInteractionText() const
{
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* Rule=D?D->Find<FICGachaMachineRow>(GachaMachineID,R):nullptr;
    if(!Rule)return InteractionText;
    const FText Name=Rule->NameKey.IsEmpty()?InteractionText:FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),Rule->NameKey);
    return FText::Format(NSLOCTEXT("ICGacha","NamedPrompt","{0} · {1}G / 1회"),Name,FText::AsNumber(Rule->CostGold));
}
bool AICGachaMachine::TryInteract_Implementation(AICPlayerController* PC)
{
    if(bDrawing||!PC||!PC->bCreated||!PC->GetPawn()||PC->IsDead()||PC->bInCombat||PC->bInDialogue||PC->bInEvent||!PC->Menu.IsEmpty()
        ||FVector::DistSquared(PC->GetPawn()->GetActorLocation(),GetActorLocation())>FMath::Square(300.))return false;
    TGuardValue<bool> Guard(bDrawing,true);auto* State=PC->GetWorldState();auto* Time=PC->GetTime();
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* Rule=D?D->Find<FICGachaMachineRow>(GachaMachineID,R):nullptr;
    if(!State||!Time||!Rule||!State->FlushParticipants())return false;
    auto Next=State->ExportState();const FGuid Receipt=FGuid::NewGuid();FRandomStream Random(GetTypeHash(Receipt));FICGachaDrawResult Result;
    const FVector DropPosition=GetActorLocation()+(PC->GetPawn()->GetActorLocation()-GetActorLocation()).GetSafeNormal2D()*150.;
    if(!FICGachaService::PrepareDraw(Next,*Rule,D,Random,DropPosition,Time->GetClock().PlaySecond,Result))
    {PC->ShowLocalized(Result.Failure);return false;}
    Next.CompletedTransactionIDs.Add(Receipt);
    if(!State->CommitGameplayTransaction(Next,Receipt))return false;
    PC->SetGold(Next.Gold);
    if(Result.ReceivedCount>0){auto Received=Result.Prize;Received.Count=Result.ReceivedCount;State->OnItemsReceived.Broadcast({Received});}
    const auto* Item=D->Find<FICItemRow>(Result.Prize.ItemID,R);const FText Name=Item?Item->DisplayName:FText::FromName(Result.Prize.ItemID);
    PC->ShowLocalized(FText::Format(Result.DroppedCount>0?NSLOCTEXT("ICGacha","DroppedResult","{0} ×{1} 당첨! 가방이 가득 차 바닥에 놓였습니다."):NSLOCTEXT("ICGacha","Result","{0} ×{1} 당첨!"),Name,FText::AsNumber(Result.Prize.Count)));
    UE_LOG(LogICInventory,Log,TEXT("[Gacha.Draw] Machine=%s Cost=%d Item=%s Count=%d Received=%d Dropped=%d"),*GachaMachineID.ToString(),Rule->CostGold,*Result.Prize.ItemID.ToString(),Result.Prize.Count,Result.ReceivedCount,Result.DroppedCount);
    return true;
}
