#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInteractionObject.h"
#include "PlayerInventory.h"
#include "PlayerDroppedItem.h"
#include "../Core/ICSaveSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

void APlayerPrototypeController::SetupInputComponent()
{
    Super::SetupInputComponent();
    const TArray<FKey> Keys={EKeys::SpaceBar,EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::Tab,EKeys::LeftShift,EKeys::F,
        EKeys::Q,EKeys::E,EKeys::R,EKeys::T,EKeys::Z,EKeys::X,EKeys::C,EKeys::V,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,
        EKeys::I,EKeys::O,EKeys::M,EKeys::P,MenuCancelKey(),EKeys::F5,EKeys::F9,EKeys::Enter};
    for(FKey Key:Keys)
    {
        FInputKeyBinding Binding(FInputChord(Key),IE_Pressed);Binding.bExecuteWhenPaused=true;
        Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this,Key](){HandleKey(Key);});InputComponent->KeyBindings.Add(Binding);
    }
    FInputKeyBinding BlockRelease(FInputChord(EKeys::RightMouseButton),IE_Released);BlockRelease.bExecuteWhenPaused=true;
    BlockRelease.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){if(bBlocking){bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0);}});
    InputComponent->KeyBindings.Add(BlockRelease);
    FInputKeyBinding JumpRelease(FInputChord(EKeys::SpaceBar),IE_Released);
    JumpRelease.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){if(auto* TargetCharacter=Cast<ACharacter>(GetPawn()))TargetCharacter->StopJumping();});
    InputComponent->KeyBindings.Add(JumpRelease);
}
void APlayerPrototypeController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);SyncSaveSafety();
    const auto* Target=bCreated&&!bDead&&Menu.IsEmpty()&&!IsPaused()?FindInteractionTarget():nullptr;
    const FString Hint=Target?FString(TEXT("F: "))+Target->Label->Text.ToString()+TEXT(" · ")+(Target->Action==TEXT("Storage")?TEXT("창고 열기"):Target->Action==TEXT("Loot")?TEXT("목재 획득"):Target->Action==TEXT("Quest")?TEXT("퀘스트 아이템 획득"):TEXT("미정 행동")):FString();
    if(Hint!=InteractionHint){InteractionHint=Hint;bDirty=true;}
    if(!bCreated||bDead||!Menu.IsEmpty()||IsPaused())return;
    auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());if(!PlayerPawn)return;
    PlayerPawn->GetCharacterMovement()->MaxWalkSpeed=PlayerPawn->FinalMoveSpeed*(Survival.IsExhausted()?ExhaustedMoveSpeedMultiplier:1.f);
    const FRotator Yaw(0,GetControlRotation().Yaw,0);
    const float Forward=(IsInputKeyDown(EKeys::W)?1.f:0.f)-(IsInputKeyDown(EKeys::S)?1.f:0.f);
    const float Right=(IsInputKeyDown(EKeys::D)?1.f:0.f)-(IsInputKeyDown(EKeys::A)?1.f:0.f);
    PlayerPawn->AddMovementInput((Yaw.Vector()*Forward+FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y)*Right).GetSafeNormal());
    float MouseX,MouseY;GetInputMouseDelta(MouseX,MouseY);FRotator Rotation=GetControlRotation();
    Rotation.Yaw+=MouseX*PlayerPawn->CameraSensitivity;Rotation.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch)+MouseY*PlayerPawn->CameraSensitivity,-65.f,65.f);SetControlRotation(Rotation);
    if(PlayerPawn->GetActorLocation().Z<-1500)PlayerPawn->SetActorLocation(FVector(0,0,130));
}
void APlayerPrototypeController::HandleKey(FKey Key)
{
    if(Menu==TEXT("Confirm")){if(Key==EKeys::Enter)ConfirmModal(true);else if(Key==MenuCancelKey())ConfirmModal(false);return;}
    if(Menu==TEXT("Quit")){if(Key==MenuCancelKey())CloseSessionMenu();return;}
    if(Menu==TEXT("Save"))
    {
        const TArray<FKey> Numbers={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
        const int32 Number=Numbers.IndexOfByKey(Key);
        if(Number!=INDEX_NONE)ExecuteUI(TEXT("SaveSlot"),Number+1);
        else if(Key==EKeys::Enter)RequestManualSave();
        else if(Key==MenuCancelKey())CloseSessionMenu();
        return;
    }
    if(Menu==TEXT("Load")&&Key==MenuCancelKey()){CloseSessionMenu();return;}
    if(Key==EKeys::F9){LoadPlayer();return;}
    if(Key==MenuCancelKey())
    {
        if(!PendingItem.IsNone())CancelTarget();
        else if(bCreated&&!bDead)Menu.IsEmpty()?SetMenu(TEXT("Pause")):CloseSessionMenu();
        return;
    }
    if(Key==EKeys::F5){OpenSaveMenu();return;}
    if(!bCreated||bDead)return;
    if(Key==EKeys::I||Key==EKeys::O||Key==EKeys::M||Key==EKeys::P)
    {
        const FString Target=Key==EKeys::I?TEXT("Inventory"):Key==EKeys::O?TEXT("Quest"):Key==EKeys::M?TEXT("Map"):TEXT("Party");
        SetMenu(Menu==Target?TEXT(""):Target);return;
    }
    const TArray<FKey> Party={EKeys::Z,EKeys::X,EKeys::C,EKeys::V};const int32 PartyIndex=Party.IndexOfByKey(Key);
    const TArray<FKey> Quick={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};const int32 QuickIndex=Quick.IndexOfByKey(Key);
    if(!PendingItem.IsNone())
    {
        if(QuickIndex!=INDEX_NONE&&Menu.IsEmpty())
        {if(QuickIndex==PendingQuick)ConfirmTarget(TEXT("Player"));else BeginItem(GetInventory()->GetSnapshot().QuickSlots[QuickIndex],QuickIndex);}
        if(PartyIndex!=INDEX_NONE)
        {if(PendingPartyKey==PartyIndex)ConfirmTarget(TEXT("Player"));else{PendingPartyKey=PartyIndex;ConfirmTarget(FName(*FString::Printf(TEXT("Companion%d"),PartyIndex+1)));}}
        return;
    }
    if(!Menu.IsEmpty())return;
    if(QuickIndex!=INDEX_NONE){BeginItem(GetInventory()->GetSnapshot().QuickSlots[QuickIndex],QuickIndex);return;}
    if(PartyIndex!=INDEX_NONE){OnCombatRequest.Broadcast(TEXT("CompanionSkill"),PartyIndex);Show(TEXT("동료 스킬 요청 전송 (동료 담당 연결 대기)"));return;}
    if(Key==EKeys::SpaceBar){if(auto* TargetCharacter=Cast<ACharacter>(GetPawn()))TargetCharacter->Jump();return;}
    if(Key==EKeys::F){Interact();return;}
    FName Action=NAME_None;int32 Slot=0;
    if(Key==EKeys::LeftMouseButton)Action=TEXT("Attack");
    else if(Key==EKeys::RightMouseButton){Action=TEXT("BlockStart");bBlocking=true;}
    else if(Key==EKeys::LeftShift)Action=TEXT("Dodge");
    else if(Key==EKeys::Tab)Action=TEXT("NextTarget");
    else if(Key==EKeys::Q||Key==EKeys::E||Key==EKeys::R||Key==EKeys::T)
    {Action=TEXT("Skill");Slot=Key==EKeys::E?1:Key==EKeys::R?2:Key==EKeys::T?3:0;}
    if(!Action.IsNone()){OnCombatRequest.Broadcast(Action,Slot);Show(FString::Printf(TEXT("%s 요청 전송 - 전투 담당 연결 대기"),*Action.ToString()));}
}
APlayerPrototypeObject* APlayerPrototypeController::FindInteractionTarget() const
{
    const auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());if(!PlayerPawn||!GetWorld())return nullptr;
    const FVector Origin=PlayerPawn->GetActorLocation();
    const FVector Forward=FRotator(0,GetControlRotation().Yaw,0).Vector();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerInteraction),false);Params.AddIgnoredActor(PlayerPawn);
    auto Eligible=[&](APlayerPrototypeObject* Object)
    {
        if(!IsValid(Object))return false;
        const FVector Offset=Object->GetActorLocation()-Origin;
        if(Offset.Size()>250.f||FVector::DotProduct(Forward,Offset.GetSafeNormal2D())<0.35f)return false;
        FHitResult Occlusion;
        GetWorld()->LineTraceSingleByChannel(Occlusion,PlayerPawn->GetPawnViewLocation(),Object->GetActorLocation(),ECC_Visibility,Params);
        return !Occlusion.bBlockingHit||Occlusion.GetActor()==Object;
    };
    FHitResult Hit;
    const FVector Camera=PlayerPawn->Camera->GetComponentLocation();
    GetWorld()->LineTraceSingleByChannel(Hit,Camera,Camera+PlayerPawn->Camera->GetForwardVector()*1000.f,ECC_Visibility,Params);
    if(auto* Exact=Cast<APlayerPrototypeObject>(Hit.GetActor());Eligible(Exact))return Exact;
    APlayerPrototypeObject* Best=nullptr;float BestScore=MAX_flt;
    for(TActorIterator<APlayerPrototypeObject> It(GetWorld());It;++It)
    {
        auto* Object=*It;if(!Eligible(Object))continue;
        const FVector Offset=Object->GetActorLocation()-Origin;
        const float Score=Offset.Size()-100.f*FVector::DotProduct(Forward,Offset.GetSafeNormal2D());
        if(Score<BestScore){Best=Object;BestScore=Score;}
    }
    return Best;
}
void APlayerPrototypeController::Interact()
{
    if(auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn()))
    {
        FHitResult Hit;FCollisionQueryParams Params;Params.AddIgnoredActor(PlayerPawn);
        const FVector Start=PlayerPawn->Camera->GetComponentLocation();
        GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+PlayerPawn->Camera->GetForwardVector()*1000.f,ECC_Visibility,Params);
        if(auto* Dropped=Cast<APlayerDroppedItem>(Hit.GetActor());Dropped&&FVector::Dist(PlayerPawn->GetActorLocation(),Dropped->GetActorLocation())<=250.f)
        {Dropped->TryPickup(this);return;}
    }

    auto* Object=FindInteractionTarget();
    if(!Object)
    {
        bool bAny=false,bNear=false,bFront=false;
        if(GetPawn())for(TActorIterator<APlayerPrototypeObject> It(GetWorld());It;++It)
        {
            bAny=true;const FVector Offset=It->GetActorLocation()-GetPawn()->GetActorLocation();
            if(Offset.Size()<=250.f){bNear=true;bFront|=FVector::DotProduct(FRotator(0,GetControlRotation().Yaw,0).Vector(),Offset.GetSafeNormal2D())>=0.35f;}
        }
        Show(!bAny?TEXT("이 맵에 상호작용 상자가 없습니다."):!bNear?TEXT("상자에 250cm 이내로 접근하세요."):!bFront?TEXT("상자가 있는 방향을 바라보세요."):TEXT("상자가 가려져 있습니다. 벽 너머에서는 사용할 수 없습니다."));return;
    }
    if(Object->Action==TEXT("Storage"))
    {
        if(!Object->bHome&&!Object->bAccessGranted){Show(TEXT("이 창고에 접근할 권한이 없습니다."));return;}
        if(!Object->bHome&&Object->ContainerID.IsNone()){Show(TEXT("창고 ID가 설정되지 않았습니다."));return;}
        OpenContainer(Object->ContainerID,Object->bHome,Object->bAccessGranted);
    }
    else if(Object->Action==TEXT("Sleep"))Sleep();
    else if(Object->Action==TEXT("Kitchen")||Object->Action==TEXT("Forge")){ActiveStation=Object->Action;SetMenu(TEXT("Craft"));}
    else if(Object->Action==TEXT("Supply"))
    {GetInventory()->AddItem(TEXT("ITM_MAT_001"),5,true);GetInventory()->AddItem(TEXT("ITM_MAT_002"),6,true);GetInventory()->AddItem(TEXT("ITM_MAT_003"),6,true);Show(TEXT("제작 재료를 지급했습니다."));}
    else if(Object->Action==TEXT("Loot")){GetInventory()->AddItem(TEXT("ITM_MAT_001"),3,true);Show(TEXT("목재 획득 완료. 초과분은 바닥에 생성됩니다."));}
    else if(Object->Action==TEXT("Quest"))GetInventory()->TryAddItem(TEXT("ITM_QST_001"),1);
    else Show(TEXT("이 상자의 상호작용 종류가 설정되지 않았습니다."));
}
