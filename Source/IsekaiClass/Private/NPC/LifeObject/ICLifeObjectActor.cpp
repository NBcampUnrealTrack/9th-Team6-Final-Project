#include "NPC/LifeObject/ICLifeObjectActor.h"

#include "NPC/LifeObject/ICLifeObjectSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "SmartObjectComponent.h"
#include "SmartObjectDefinition.h"
#include "SmartObjectSubsystem.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "NPC/Character/ICNPCBase.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"

AICLifeObjectActor::AICLifeObjectActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 부모의 Shape는 루트라서 자식 BP에서 상대 트랜스폼을 조정할 수 없다.
    // 별도 루트를 두고 Shape를 자식으로 만들어 메시 정렬을 가능하게 한다.
    PlacementRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlacementRoot"));
    RootComponent = PlacementRoot;
    Shape->SetupAttachment(PlacementRoot);
    Shape->SetMobility(EComponentMobility::Movable);

    SmartObject = CreateDefaultSubobject<USmartObjectComponent>(TEXT("SmartObject"));
    SmartObject->SetupAttachment(RootComponent);

    DebugLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugLabel"));
    DebugLabel->SetupAttachment(RootComponent);
    DebugLabel->SetRelativeLocation(FVector(0.0, 0.0, 170.0));
    DebugLabel->SetHorizontalAlignment(EHTA_Center);
    DebugLabel->SetVerticalAlignment(EVRTA_TextBottom);
    DebugLabel->SetWorldSize(18.f);
    DebugLabel->SetTextRenderColor(FColor(255, 210, 64));
    DebugLabel->SetHiddenInGame(true);
    DebugLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AICLifeObjectActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if(ObjectType==EICLifeObjectType::Bed&&Shape)
    {Shape->SetCollisionProfileName(TEXT("BlockAllDynamic"));Shape->SetCanEverAffectNavigation(false);}
    // 에디터 뷰포트에서 ID와 종류를 바로 확인할 수 있게 표시한다(게임에서는 디버그 CVar가 켜질 때만 보인다).
    SetDebugText(FString::Printf(TEXT("%s\n[%s]"), *GetDebugID().ToString(), ICLife::LexType(ObjectType)), false);
}

void AICLifeObjectActor::BeginPlay()
{
    Super::BeginPlay();
    if(ObjectType==EICLifeObjectType::Bed&&Shape)
    {
        Shape->SetCollisionProfileName(TEXT("BlockAllDynamic"));Shape->SetCanEverAffectNavigation(false);
        // NPC schedules still reach their bed slots; the player's capsule remains blocked.
        for(TActorIterator<AICNPCBase> It(GetWorld());It;++It)It->GetCapsuleComponent()->IgnoreActorWhenMoving(this,true);
    }
    if (ObjectID.IsNone())
        UE_LOG(LogICLifeObject, Warning, TEXT("[LifeObject.MissingID] Actor=%s 고유 ID(ObjectID)를 입력하세요."), *GetName());
    if (!SmartObject || !SmartObject->GetDefinition())
        UE_LOG(LogICLifeObject, Warning, TEXT("[LifeObject.MissingDefinition] Object=%s SmartObject 컴포넌트에 Definition Asset이 없습니다. 슬롯이 0개라 NPC가 사용할 수 없습니다."), *GetDebugID().ToString());
    if (UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this))
        Life->RegisterObject(this);
}

void AICLifeObjectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // SmartObject 컴포넌트가 등록 해제되기 전에(=Super 전에) 사용자에게 먼저 알리고 예약을 푼다.
    if (UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this))
        Life->UnregisterObject(this);
    Super::EndPlay(EndPlayReason);
}

bool AICLifeObjectActor::TryInteract_Implementation(AICPlayerController* Controller)
{
    if (ObjectType != EICLifeObjectType::Bed || !bPlayerCanSleep)
        return Super::TryInteract_Implementation(Controller);
    if (!Controller || !Controller->GetPawn()) return false;

    if (!OwnerNPCID.IsNone())
    {
        const UICWorldStateSubsystem* State = Controller->GetWorldState();
        const int32 Affinity = State ? State->GetSnapshot(OwnerNPCID).Affinity : 0;
        if (Affinity <= PlayerRequiredAffinity)
        {
            Controller->ShowLocalized(NSLOCTEXT("ICLifeObject", "BedAffinity", "아직 이 침대를 사용할 수 없습니다."));
            UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.PlayerBedRejected] Object=%s Owner=%s Affinity=%d Required=%d"),
                *GetDebugID().ToString(), *OwnerNPCID.ToString(), Affinity, PlayerRequiredAffinity);
            return false;
        }
    }

    UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    FText Failure;
    if (!Life || !Life->TryLockForPlayer(*this, Controller->GetPawn(), Failure))
    {
        if (!Failure.IsEmpty()) Controller->ShowLocalized(Failure);
        return false;
    }
    auto* Character=Cast<AICPlayerCharacter>(Controller->GetPawn());
    const bool bSlept = Character&&Character->LifeActions&&Character->LifeActions->BeginSleep(this);
    if (!bSlept) Life->UnlockForPlayer(*this);
    UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.PlayerSleep] Object=%s Success=%d"), *GetDebugID().ToString(), bSlept ? 1 : 0);
    return bSlept;
}

FName AICLifeObjectActor::GetDebugID() const
{
    return ObjectID.IsNone() ? FName(*GetName()) : ObjectID;
}

FGameplayTagContainer AICLifeObjectActor::GetEffectiveActivityTags() const
{
    return ActivityTags.IsEmpty() ? ICLife::GetDefaultActivityTags(ObjectType) : ActivityTags;
}

bool AICLifeObjectActor::IsLinkedToAnchor(FName AnchorID) const
{
    return !AnchorID.IsNone() && (LinkedAnchorID == AnchorID || LinkedAnchorIDs.Contains(AnchorID));
}

int32 AICLifeObjectActor::GetSlotCount() const
{
    const USmartObjectDefinition* Definition = SmartObject ? SmartObject->GetDefinition() : nullptr;
    return Definition ? Definition->GetSlots().Num() : 0;
}

bool AICLifeObjectActor::IsSmartObjectReady() const
{
    const USmartObjectSubsystem* SO = USmartObjectSubsystem::GetCurrent(GetWorld());
    return SO && SmartObject && SmartObject->GetRegisteredHandle().IsValid() && SO->IsSmartObjectValid(SmartObject->GetRegisteredHandle());
}

bool AICLifeObjectActor::GetSlotHandle(int32 SlotIndex, FSmartObjectSlotHandle& OutSlot) const
{
    OutSlot = FSmartObjectSlotHandle();
    if (!IsSmartObjectReady() || SlotIndex < 0) return false;
    const USmartObjectSubsystem* SO = USmartObjectSubsystem::GetCurrent(GetWorld());
    TArray<FSmartObjectSlotHandle> Slots;
    SO->GetAllSlots(SmartObject->GetRegisteredHandle(), Slots);
    for (const FSmartObjectSlotHandle& Slot : Slots)
    {
        if (Slot.GetSlotIndex() == SlotIndex)
        {
            OutSlot = Slot;
            return true;
        }
    }
    return false;
}

bool AICLifeObjectActor::GetSlotTransform(int32 SlotIndex, FTransform& OutTransform) const
{
    FSmartObjectSlotHandle Slot;
    if (!GetSlotHandle(SlotIndex, Slot)) return false;
    const USmartObjectSubsystem* SO = USmartObjectSubsystem::GetCurrent(GetWorld());
    const TOptional<FTransform> Transform = SO ? SO->GetSlotTransform(Slot) : TOptional<FTransform>();
    if (!Transform.IsSet()) return false;
    OutTransform = Transform.GetValue();
    return true;
}

const FICLifeSlotPresentation* AICLifeObjectActor::FindActorPresentation(int32 SlotIndex) const
{
    if (SlotPresentations.IsValidIndex(SlotIndex) && SlotPresentations[SlotIndex].IsSet()) return &SlotPresentations[SlotIndex];
    if (DefaultPresentation.IsSet()) return &DefaultPresentation;
    return nullptr;
}

bool AICLifeObjectActor::CanBeUsedBy(FName NPCID, const FGameplayTag& OccupationTag, FName NPCRegionID, FString* OutReason) const
{
    auto Reject = [OutReason](const TCHAR* Reason)
    {
        if (OutReason) *OutReason = Reason;
        return false;
    };
    if (!OwnerNPCID.IsNone() && OwnerNPCID != NPCID) return Reject(TEXT("OwnedByOther"));
    if (AllowedNPCIDs.Num() > 0 && !AllowedNPCIDs.Contains(NPCID)) return Reject(TEXT("NPCNotAllowed"));
    if (!AllowedOccupationTags.IsEmpty() && (!OccupationTag.IsValid() || !OccupationTag.MatchesAny(AllowedOccupationTags)))
        return Reject(TEXT("OccupationNotAllowed"));
    if (!RegionID.IsNone() && !NPCRegionID.IsNone() && RegionID != NPCRegionID) return Reject(TEXT("OtherRegion"));
    return true;
}

bool AICLifeObjectActor::IsOccupiedByAnyNPC() const
{
    if(GetUserIDs().Num()>0)return true;
    if(ObjectType==EICLifeObjectType::Bed&&GetWorld())for(TActorIterator<AICNPCBase> It(GetWorld());It;++It)
        if((OwnerNPCID.IsNone()||It->GetNPCDefinition().ID==OwnerNPCID)&&It->IsSleepingForInteraction()
            &&FVector::DistSquared2D(It->GetActorLocation(),GetActorLocation())<FMath::Square(150.f))return true;
    return false;
}
FText AICLifeObjectActor::GetInteractionText() const
{
    if(ObjectType!=EICLifeObjectType::Bed)return Super::GetInteractionText();
    if(IsOccupiedByAnyNPC()||IsPlayerLocked())return NSLOCTEXT("ICLifeObject","BedBusyHint","침대 · 사용 중");
    return OwnerNPCID.IsNone()?NSLOCTEXT("ICLifeObject","BedFreeHint","빈 침대 · 잠자기")
        :FText::Format(NSLOCTEXT("ICLifeObject","OwnedBedHint","NPC 침대 · 잠자기 (호감도 {0} 초과)"),FText::AsNumber(PlayerRequiredAffinity));
}

bool AICLifeObjectActor::IsUsedByNPC(FName NPCID) const
{
    return GetUserIDs().Contains(NPCID);
}

TArray<FName> AICLifeObjectActor::GetUserIDs() const
{
    TArray<FName> IDs;
    if (const UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this))
    {
        TArray<const FICLifeSlotOccupant*> Occupants;
        Life->GetOccupantsOf(*this, Occupants);
        for (const FICLifeSlotOccupant* Occupant : Occupants) IDs.Add(Occupant->UserID);
    }
    return IDs;
}

void AICLifeObjectActor::SetPlayerLockedInternal(bool bLocked, AActor* Player)
{
    bPlayerLocked = bLocked;
    LockingPlayer = bLocked ? Player : nullptr;
}

void AICLifeObjectActor::SetDebugText(const FString& Text, bool bVisible)
{
    if (!DebugLabel) return;
    DebugLabel->SetText(FText::FromString(Text));
    DebugLabel->SetHiddenInGame(!bVisible);
}
