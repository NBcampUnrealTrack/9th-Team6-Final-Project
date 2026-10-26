#include "NPC/Character/ICNPCBase.h"
#include "UI/Framework/ICText.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "StateTree.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Rows/NPC/ICGiftPreferenceRow.h"
#include "NPC/Schedule/ICNPCScheduleLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "UI/Quest/ICNPCQuestMarkerWidget.h"
AICNPCBase::AICNPCBase()
{
    IC_TRACE_FUNCTION();
    AIControllerClass=AICAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::Disabled;
    QuestMarker=CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestMarker"));
    QuestMarker->SetupAttachment(GetCapsuleComponent());
    QuestMarker->SetWidgetSpace(EWidgetSpace::Screen);
    QuestMarker->SetDrawSize(FVector2D(70,80));
    QuestMarker->SetRelativeLocation(FVector(0,0,125));
    QuestMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    auto* Movement=GetCharacterMovement();
    Movement->MaxWalkSpeed=ScheduleMoveSpeed;

    Movement->bUseRVOAvoidance=true;
    Movement->AvoidanceConsiderationRadius=300.f;
    Movement->AvoidanceWeight=.5f;
}
void AICNPCBase::BeginPlay() { IC_TRACE_FUNCTION();Super::BeginPlay();if(!bInitialized&&!NPCDefinitionID.IsNone())InitializeNPC(NPCDefinitionID); }
void AICNPCBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    IC_TRACE_FUNCTION();
    if (auto* Time = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICTimeSubsystem>() : nullptr)
        Time->OnTimeJumped.RemoveDynamic(this, &ThisClass::HandleTimeJumped);
    Super::EndPlay(EndPlayReason);
}
bool AICNPCBase::InitializeNPC(FName ID)
{
    if (bInitialized) return NPCDefinitionID == ID;
    IC_TRACE_FUNCTION();auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* NPC=D?D->Find<FICNPCRow>(ID,R):nullptr;if(!NPC||!WorldState())return false;
    const auto Kind=static_cast<EICCharacterKind>(static_cast<int32>(NPC->Type)+1);
    if(!WorldState()->EnsureCharacter(ID,Kind))return false;


    auto State=WorldState()->GetSnapshot(ID);const FName AnchorID=!State.CurrentAnchorID.IsNone()?State.CurrentAnchorID:NPC->InitialAnchorID;
    if(State.Mode==EICCharacterMode::Scheduled&&!AnchorID.IsNone())if(auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>())
    {
        FTransform AnchorTransform;
        if(Spawns->ResolveLoadedAnchor(AnchorID,AnchorTransform))
        {
            State.CurrentAnchorID=AnchorID;State.Position=AnchorTransform.GetLocation();State.Rotation=AnchorTransform.Rotator();
            WorldState()->SetCharacterState(State);SetActorLocationAndRotation(State.Position,State.Rotation);
        }
    }
    Definition=*NPC;NPCDefinitionID=ID;GetCharacterMovement()->MaxWalkSpeed=ScheduleMoveSpeed;if(Kind==EICCharacterKind::Recruitable)EnableCombat(true,false);
    if(!InitializeState(ID))return false;
    AnimationProfile = Definition.AnimationProfile;
    if(auto* CharacterMesh=Definition.Mesh.LoadSynchronous()){GetMesh()->SetSkeletalMesh(CharacterMesh);GetMesh()->SetRelativeTransform(Definition.MeshTransform);}
    if(auto* Anim=Definition.AnimationClass.LoadSynchronous())GetMesh()->SetAnimInstanceClass(Anim);
    RefreshWeaponAppearance();
    bInitialized=true;
    if(auto* MarkerClass=LoadClass<UICNPCQuestMarkerWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCQuestMarker.WBP_IC_NPCQuestMarker_C")))
    {
        QuestMarker->SetRelativeLocation(FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+40));
        QuestMarker->SetWidgetClass(MarkerClass);QuestMarker->InitWidget();
        if(auto* Marker=Cast<UICNPCQuestMarkerWidget>(QuestMarker->GetWidget()))Marker->SetNPC(this);
    }
    if (auto* Time = GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
        Time->OnTimeJumped.AddUniqueDynamic(this, &ThisClass::HandleTimeJumped);
    RefreshExecution();return true;
}
void AICNPCBase::HandleTimeJumped(double FromGameMinute, double ToGameMinute)
{
    IC_TRACE_FUNCTION();
    if (!bInitialized || !WorldState()) return;
    const FICCharacterState Previous = WorldState()->GetSnapshot(CharacterID);
    if (Previous.IsDowned() || Previous.Mode != EICCharacterMode::Scheduled) return;

    auto* Definitions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    auto* Spawns = GetWorld() ? GetWorld()->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    if (!Definitions || !Spawns) return;

    FICDefinitionResult Result;
    const FICScheduleRow* Schedule = Definitions->Find<FICScheduleRow>(Definition.ScheduleID, Result);
    if (!Schedule) return;
    const FICNPCScheduleResolution Current = UICNPCScheduleLibrary::ResolveSchedule(*Schedule, ToGameMinute);
    if (!Current.bFound || Current.DestinationAnchorID.IsNone()) return;

    FTransform Destination;
    if (!Spawns->ResolveLoadedAnchor(Current.DestinationAnchorID, Destination))
    {
        UE_LOG(LogICWorldState, Warning, TEXT("[NPC.TimeJumpAnchorUnavailable] NPC=%s Anchor=%s From=%.2f To=%.2f"),
            *CharacterID.ToString(), *Current.DestinationAnchorID.ToString(), FromGameMinute, ToGameMinute);
        return;
    }

    if (Previous.CurrentAnchorID != Current.DestinationAnchorID)
    {
        FICCharacterState Updated = Previous;
        Updated.CurrentAnchorID = Current.DestinationAnchorID;
        Updated.Position = Destination.GetLocation();
        Updated.Rotation = Destination.Rotator();
        if (!WorldState()->SetCharacterState(Updated)) return;
    }
    SetActorLocationAndRotation(Destination.GetLocation(), Destination.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
    if (auto* AI = Cast<AICAIController>(GetController())) AI->RestoreScheduleFacing();
    UE_LOG(LogICWorldState, Verbose, TEXT("[NPC.TimeJumpRelocated] NPC=%s Anchor=%s From=%.2f To=%.2f"),
        *CharacterID.ToString(), *Current.DestinationAnchorID.ToString(), FromGameMinute, ToGameMinute);
}
void AICNPCBase::RefreshExecution()
{
    IC_TRACE_FUNCTION();if(!bInitialized||!WorldState())return;const auto S=WorldState()->GetSnapshot(CharacterID);
    if(S.IsDowned()){if(auto* AI=Cast<AICAIController>(GetController()))AI->StopExecution();return;}
    GetWorldTimerManager().ClearTimer(DefeatTimer);SetActorEnableCollision(true);
    if(!GetController())SpawnDefaultController();if(auto* AI=Cast<AICAIController>(GetController()))AI->StartExecution(
        S.Mode==EICCharacterMode::Companion?nullptr:Cast<UStateTree>(Definition.ScheduleStateTree.LoadSynchronous()),nullptr);
}
void AICNPCBase::HandleZeroHP()
{
    IC_TRACE_FUNCTION();if(bZeroHPHandled)return;EndInteraction();if(auto* AI=Cast<AICAIController>(GetController()))AI->StopExecution();

    SetActorEnableCollision(false);GetWorldTimerManager().SetTimer(DefeatTimer,this,&AActor::K2_DestroyActor,1.f,false);
    Super::HandleZeroHP();
}
void AICNPCBase::HoldDownedActor()
{ IC_TRACE_FUNCTION();GetWorldTimerManager().ClearTimer(DefeatTimer);SetActorEnableCollision(false); }
void AICNPCBase::StoredCharacterChanged(FName ID)
{
    IC_TRACE_FUNCTION();const auto Previous=AbilitySystem?AbilitySystem->Capture():FICCharacterState();
    Super::StoredCharacterChanged(ID);
    if(ID==CharacterID&&AbilitySystem){const auto Current=AbilitySystem->Capture();if(Previous.Mode!=Current.Mode||Previous.LifeState!=Current.LifeState)RefreshExecution();}
}
void AICNPCBase::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{ IC_TRACE_FUNCTION();Super::RestoreFromWorldState_Implementation(Candidate);RefreshExecution(); }
TArray<FName> AICNPCBase::GetAvailableServices() const
{
    IC_TRACE_FUNCTION();
    if (!WorldState()) return {};
    const FICCharacterState State = WorldState()->GetSnapshot(CharacterID);
    if (State.Mode == EICCharacterMode::Companion && !State.IsDowned()) return Definition.ServiceIDs;
    if (State.Mode != EICCharacterMode::Scheduled || State.IsDowned()) return {};
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;auto* T=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;FICDefinitionResult R;
    const auto* Schedule=D?D->Find<FICScheduleRow>(Definition.ScheduleID,R):nullptr;if(!Schedule||!T)return {};
    const int32 Minute=int32(FMath::FloorToDouble(T->GetClock().GameMinute))%1440;
    for(const auto& Segment:Schedule->Segments)if(Minute>=Segment.StartGameMinute&&Minute<Segment.EndGameMinute&&Segment.Activity==EICScheduleActivity::Work)return Segment.ServiceIDs;
    return {};
}
bool AICNPCBase::TryInteract(AICPlayerController* Requester)
{
    IC_TRACE_FUNCTION();
    if(!bInitialized||!Requester||!Requester->GetPawn()||bInteracting||!WorldState()
        || FVector::DistSquared(Requester->GetPawn()->GetActorLocation(),GetActorLocation())>FMath::Square(350.f))return false;
    const auto State=WorldState()->GetSnapshot(CharacterID);
    if(State.IsDowned()||(State.Mode!=EICCharacterMode::Scheduled&&!CanDismiss()))
    {UE_LOG(LogICInteraction,Verbose,TEXT("[NPC.InteractRejected] NPC=%s Mode=%d Downed=%d"),*CharacterID.ToString(),static_cast<int32>(State.Mode),State.IsDowned()?1:0);return false;}
    const TArray<FName> Services=GetAvailableServices();
    const FName ServiceID=Services.Num()>0?Services[0]:NAME_None;
    bInteracting=true;
    if(auto* AI=Cast<AICAIController>(GetController()))AI->SetInteractionHold(true,Requester->GetPawn());
    UE_LOG(LogICInteraction,Log,TEXT("[NPC.Interact] NPC=%s Service=%s"),*CharacterID.ToString(),*ServiceID.ToString());
    OnInteractionStarted.Broadcast(this,Requester,ServiceID);
    return true;
}
void AICNPCBase::EndInteraction()
{
    IC_TRACE_FUNCTION();
    if(!bInteracting)return;
    bInteracting=false;
    if(auto* AI=Cast<AICAIController>(GetController()))
    {
        AI->SetInteractionHold(false,nullptr);
        AI->RestoreScheduleFacing();
    }
    UE_LOG(LogICInteraction,Log,TEXT("[NPC.InteractEnd] NPC=%s"),*CharacterID.ToString());
}
bool AICNPCBase::TryGiveGift(AICPlayerController* Requester, FName ItemID)
{
    auto Fail=[Requester](const FText& Message){if(Requester)Requester->ShowLocalized(Message);return false;};
    if (!Requester || Requester->GetWorld()!=GetWorld() || !WorldState() || !Requester->GetPawn()
        || FVector::DistSquared(Requester->GetPawn()->GetActorLocation(),GetActorLocation())>FMath::Square(350.f))
        return Fail(NSLOCTEXT("ICGift","TooFar","NPC 가까이에서 선물해 주세요."));
    if(ItemID.IsNone())return Fail(NSLOCTEXT("ICGift","ChooseItem","선물할 아이템을 선택해 주세요."));
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult Result;
    const auto* Preference=D->Find<FICGiftPreferenceRow>(Definition.GiftPreferenceID,Result);
    if (!Preference)return Fail(NSLOCTEXT("ICGift","Unavailable","이 NPC에게는 선물할 수 없습니다."));
    const auto* Item=D->Find<FICItemRow>(ItemID,Result);
    if(!Item||!Item->bCanGift||Item->Kind==EPlayerItemKind::Quest||Item->Tab==EPlayerItemTab::Quest)
        return Fail(NSLOCTEXT("ICGift","ItemUnavailable","선물할 수 없는 아이템입니다."));
    if(!WorldState()->FlushParticipants())return Fail(NSLOCTEXT("ICGift","StateChanged","아이템 상태가 변경되었습니다. 다시 선택해 주세요."));
    auto Next=WorldState()->ExportState();auto* NPC=Next.Characters.Find(CharacterID);
    if(!NPC||(NPC->Mode!=EICCharacterMode::Scheduled&&NPC->Mode!=EICCharacterMode::Companion)||NPC->IsDowned())
        return Fail(NSLOCTEXT("ICGift","NPCUnavailable","지금은 선물을 받을 수 없습니다."));
    const auto* Entry=Preference->Items.FindByPredicate([ItemID](const auto& E){return E.ItemID==ItemID;});
    const auto Kind=Entry?Entry->Preference:EICGiftPreference::Neutral;
    const int32 Delta=Kind==EICGiftPreference::Preferred?Preference->PreferredAffinityDelta
        :Kind==EICGiftPreference::Disliked?Preference->DislikedAffinityDelta:Preference->NeutralAffinityDelta;
    if(Delta>0&&NPC->Affinity>=500)return Fail(NSLOCTEXT("ICGift","MaxAffinity","호감도가 이미 최대입니다."));
    auto* Slot=Next.Inventory.Bag.Slots.FindByPredicate([ItemID](const auto& S){return S.ItemID==ItemID&&!S.IsEmpty()&&!S.bLocked;});
    if(!Slot)return Fail(NSLOCTEXT("ICGift","MissingItem","선물할 아이템이 없거나 잠겨 있습니다."));
    if(--Slot->Count==0)*Slot=FPlayerItemInstance();
    const int32 PreviousAffinity=NPC->Affinity;
    NPC->Affinity=static_cast<int32>(FMath::Clamp<int64>(int64(NPC->Affinity)+Delta,0,500));
    const int32 ActualDelta=NPC->Affinity-PreviousAffinity;
    if(Delta>0&&NPC->Affinity>=100)if(const auto* Combat=D->Find<FICRecruitCombatRow>(Definition.RecruitCombatID,Result))
        if(auto* Player=Next.Characters.Find(UICWorldStateSubsystem::PlayerId()))Player->UnlockedSkillIDs.Add(Combat->UniqueSkillID);
    if(!WorldState()->CommitGameplayTransaction(Next,FGuid::NewGuid()))return Fail(NSLOCTEXT("ICGift","CommitFailed","선물을 전달하지 못했습니다. 다시 시도해 주세요."));
    const FText Name=Item->DisplayName.IsEmpty()?FText::FromName(ItemID):Item->DisplayName;
    FNumberFormattingOptions Signed;Signed.SetAlwaysSign(true);
    Requester->ShowLocalized(ActualDelta==0?FText::Format(NSLOCTEXT("ICGift","Given","{0} 1개를 선물했습니다."),Name)
        :FText::Format(NSLOCTEXT("ICGift","GivenAffinity","{0} 1개를 선물했습니다. 호감도 {1}"),Name,FText::AsNumber(ActualDelta,&Signed)));
    return true;
}
bool AICNPCBase::CanRecruit() const
{
    const auto* State = WorldState();
    const auto* Character = State ? State->GetStoredCharacter(CharacterID) : nullptr;
    return bInitialized && Definition.Type == EICNPCType::Recruitable && Definition.bRecruitable && Character
        && Character->Kind == EICCharacterKind::Recruitable && Character->Affinity >= 300
        && Character->Mode == EICCharacterMode::Scheduled && Character->PartySlot == INDEX_NONE
        && !Character->IsDowned();
}

bool AICNPCBase::TryRecruit(AICPlayerController* Requester)
{
    if (!Requester || Requester->GetWorld() != GetWorld() || !bInteracting) return false;
    if (!CanRecruit())
    {
        Requester->ShowLocalized(NSLOCTEXT("ICNPC", "RecruitUnavailable", "현재는 동료로 영입할 수 없습니다."));
        return false;
    }
    auto* State = WorldState();
    bool Occupied[3] = {false, false, false};
    for (const auto& Pair : State->ExportState().Characters)
        if (Pair.Value.PartySlot >= 0 && Pair.Value.PartySlot < UE_ARRAY_COUNT(Occupied))
            Occupied[Pair.Value.PartySlot] = true;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Occupied); ++Index)
        if (!Occupied[Index] && State->SetPartySlot(CharacterID, Index))
        {
            EndInteraction();
            Requester->ShowLocalized(NSLOCTEXT("ICNPC", "Recruited", "동료가 되었습니다."));
            return true;
        }
    Requester->ShowLocalized(NSLOCTEXT("ICNPC", "PartyFull", "동료는 최대 3명까지 함께할 수 있습니다."));
    return false;
}

bool AICNPCBase::CanDismiss() const
{
    const auto* State = WorldState();
    const auto* Character = State ? State->GetStoredCharacter(CharacterID) : nullptr;
    return bInitialized && Character && Character->Kind == EICCharacterKind::Recruitable
        && Character->Mode == EICCharacterMode::Companion && Character->PartySlot >= 0 && !Character->IsDowned();
}

bool AICNPCBase::TryDismiss(AICPlayerController* Requester)
{
    if (!Requester || Requester->GetWorld() != GetWorld() || !bInteracting || !CanDismiss()) return false;
    if (!WorldState()->SetPartySlot(CharacterID, INDEX_NONE)) return false;
    EndInteraction();
    Requester->ShowLocalized(NSLOCTEXT("ICNPC", "Dismissed", "동료를 해제했습니다."));
    return true;
}

FText AICNPCBase::GetInteractionText() const
{
    IC_TRACE_FUNCTION();
    return ICText::Get(TEXT("NPC_TalkHint"));
}
TArray<FICNPCInteractionOption> AICNPCBase::GetInteractionOptions() const
{
    IC_TRACE_FUNCTION();
    TArray<FICNPCInteractionOption> Options;
    auto Add=[&Options](EICNPCInteractionOption Kind,const FText& Label,FName ServiceID,bool bEnabled)
    { FICNPCInteractionOption Option;Option.Option=Kind;Option.Label=Label;Option.ServiceID=ServiceID;Option.bEnabled=bEnabled;Options.Add(Option); };
    if (CanDismiss()) Add(EICNPCInteractionOption::Dismiss, NSLOCTEXT("ICNPC", "Dismiss", "동료 해제하기"), NAME_None, true);
    else if (CanRecruit()) Add(EICNPCInteractionOption::Recruit, NSLOCTEXT("ICNPC", "Recruit", "동료 영입하기"), NAME_None, true);
    Add(EICNPCInteractionOption::Talk,ICText::Get(TEXT("NPC_Talk")),NAME_None,true);
    Add(EICNPCInteractionOption::Gift,ICText::Get(TEXT("NPC_Gift")),NAME_None,true);

    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    if(D)
    {
        const TArray<FName> Working=GetAvailableServices();FICDefinitionResult R;
        for(const FName ID:Definition.ServiceIDs)
        {
            const auto* Service=D->Find<FICServiceRow>(ID,R);
            if(Service&&Service->Type==EICServiceType::Shop){Add(EICNPCInteractionOption::Trade,ICText::Get(TEXT("NPC_Trade")),ID,Working.Contains(ID));break;}
        }
    }
    Add(EICNPCInteractionOption::End,ICText::Get(TEXT("NPC_EndTalk")),NAME_None,true);
    return Options;
}
