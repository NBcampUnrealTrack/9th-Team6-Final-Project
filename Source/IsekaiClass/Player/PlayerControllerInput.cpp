#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInteractionObject.h"
#include "PlayerDroppedItem.h"
#include "PlayerInventory.h"
#include "PlayerWorldStateSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void APlayerPrototypeController::SetupInputComponent()
{
    Super::SetupInputComponent();
    const TArray<FKey> Keys={EKeys::SpaceBar,EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::Tab,EKeys::LeftShift,EKeys::F,EKeys::Q,EKeys::E,EKeys::R,EKeys::T,EKeys::Z,EKeys::X,EKeys::C,EKeys::V,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::I,EKeys::O,EKeys::M,EKeys::P,EKeys::Escape,EKeys::F5,EKeys::F9};
    for(FKey Key:Keys) { FInputKeyBinding Binding(FInputChord(Key),IE_Pressed);Binding.bExecuteWhenPaused=true;Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this,Key](){HandleKey(Key);});InputComponent->KeyBindings.Add(Binding); }
    FInputKeyBinding BlockRelease(FInputChord(EKeys::RightMouseButton),IE_Released);BlockRelease.bExecuteWhenPaused=true;BlockRelease.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){bBlocking=false;OnCombatRequest.Broadcast(TEXT("BlockEnd"),0);});InputComponent->KeyBindings.Add(BlockRelease);
    FInputKeyBinding JumpRelease(FInputChord(EKeys::SpaceBar),IE_Released);JumpRelease.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){if(auto* Character=Cast<ACharacter>(GetPawn()))Character->StopJumping();});InputComponent->KeyBindings.Add(JumpRelease);
}

void APlayerPrototypeController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);if(!bCreated||bDead||!Menu.IsEmpty()||IsPaused())return;
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
    if(!bCreated)return;if(Key==EKeys::F9){LoadPlayer();return;}if(bDead)return;
    if(Key==EKeys::Escape){if(!PendingItem.IsNone())CancelTarget();else SetMenu(Menu.IsEmpty()?TEXT("Pause"):TEXT(""));return;}if(Key==EKeys::F5){SavePlayer();return;}
    if(Key==EKeys::I||Key==EKeys::O||Key==EKeys::M||Key==EKeys::P){const FString Target=Key==EKeys::I?TEXT("Inventory"):Key==EKeys::O?TEXT("Quest"):Key==EKeys::M?TEXT("Map"):TEXT("Party");SetMenu(Menu==Target?TEXT(""):Target);return;}
    const TArray<FKey> Party={EKeys::Z,EKeys::X,EKeys::C,EKeys::V};const int32 PartyIndex=Party.IndexOfByKey(Key);
    if(!PendingItem.IsNone())
    {
        const TArray<FKey> Quick={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};const int32 QuickIndex=Quick.IndexOfByKey(Key);
        if(QuickIndex!=INDEX_NONE&&Menu.IsEmpty()){if(QuickIndex==PendingQuick)ConfirmTarget(TEXT("Player"));else BeginItem(GetInventory()->QuickSlots[QuickIndex],QuickIndex);}
        if(PartyIndex!=INDEX_NONE)
        {
            if(PendingPartyKey==PartyIndex){ConfirmTarget(TEXT("CHR_PLAYER"));return;}
            PendingPartyKey=PartyIndex;
            auto* WorldState=GetGameInstance()->GetSubsystem<UPlayerWorldStateSubsystem>();const FName Target=WorldState?WorldState->GetPartyTarget(PartyIndex):NAME_None;
            if(Target.IsNone()){Show(TEXT("해당 파티 칸에 동료가 없습니다. 같은 키를 한 번 더 누르면 플레이어를 선택합니다."));return;}
            ConfirmTarget(Target);return;
        }
        return;
    }
    if(!Menu.IsEmpty())return;
    if(Key==EKeys::One||Key==EKeys::Two||Key==EKeys::Three||Key==EKeys::Four){const int32 QuickIndex=Key==EKeys::One?0:Key==EKeys::Two?1:Key==EKeys::Three?2:3;BeginItem(GetInventory()->QuickSlots[QuickIndex],QuickIndex);return;}
    if(PartyIndex!=INDEX_NONE){OnCombatRequest.Broadcast(TEXT("CompanionSkill"),PartyIndex);Show(TEXT("동료 스킬 요청 전송 (동료 시스템 연결 지점)"));return;}
    if(Key==EKeys::SpaceBar)Cast<ACharacter>(GetPawn())->Jump();else if(Key==EKeys::F)Interact();else
    {const FName Action=Key==EKeys::LeftMouseButton?TEXT("Attack"):Key==EKeys::Tab?TEXT("NextTarget"):Key==EKeys::LeftShift?TEXT("Dodge"):Key==EKeys::RightMouseButton?TEXT("BlockStart"):TEXT("Skill");const int32 Slot=Key==EKeys::E?1:Key==EKeys::R?2:Key==EKeys::T?3:0;if(Key==EKeys::RightMouseButton)bBlocking=true;OnCombatRequest.Broadcast(Action,Slot);Show(FString::Printf(TEXT("%s 요청 전송 - 판정은 전투 파트에서 연결합니다."),*Action.ToString()));}
}

void APlayerPrototypeController::Interact()
{
    auto* PlayerPawn=Cast<APlayerPrototypeCharacter>(GetPawn());FHitResult Hit;const FVector Start=PlayerPawn->Camera->GetComponentLocation();const FVector End=Start+PlayerPawn->Camera->GetForwardVector()*1000;
    FCollisionQueryParams Params;Params.AddIgnoredActor(GetPawn());GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params);
    if(!Hit.GetActor()||FVector::Dist(PlayerPawn->GetActorLocation(),Hit.GetActor()->GetActorLocation())>250){Show(TEXT("화면 중앙의 대상에 250cm 이내로 접근하세요."));return;}
    if(auto* Dropped=Cast<APlayerDroppedItem>(Hit.GetActor())){Dropped->TryPickup(this);return;}auto* Object=Cast<APlayerPrototypeObject>(Hit.GetActor());if(!Object)return;
    if(Object->Action==TEXT("Storage"))OpenContainer(Object->ContainerID,Object->bHome,Object->bAccessGranted);else if(Object->Action==TEXT("Sleep"))Sleep();
    else if(Object->Action==TEXT("Kitchen")||Object->Action==TEXT("Forge")){ActiveStation=Object->Action;SetMenu(TEXT("Craft"));}
    else if(Object->Action==TEXT("Supply")){GetInventory()->AddItem(TEXT("ITM_MAT_001"),5,true);GetInventory()->AddItem(TEXT("ITM_MAT_002"),6,true);GetInventory()->AddItem(TEXT("ITM_MAT_003"),6,true);Show(TEXT("프로토타입 제작 재료를 지급했습니다."));}
    else if(Object->Action==TEXT("Loot")){GetInventory()->AddItem(TEXT("ITM_MAT_001"),3,true);Show(TEXT("목재 획득 요청 완료. 초과분은 바닥에 생성됩니다."));}else if(Object->Action==TEXT("Quest")){GetInventory()->AddItem(TEXT("ITM_QST_001"),1,true);Show(TEXT("퀘스트 가방에 반지를 보관했습니다."));}
}
