#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"
#include "PlayerSaveGame.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void APlayerPrototypeController::CreatePlayer()
{
    if(bCreated||Job<0||Job>3||Preset<0||Preset>1)return;
    FString Clean=PlayerName.TrimStartAndEnd();bool Invalid=Clean.IsEmpty()||Clean.Len()>12;
    for(TCHAR Codepoint:PlayerName)if(Codepoint<TEXT(' ')||Codepoint==0x2028||Codepoint==0x2029)Invalid=true;
    if(Invalid){Show(TEXT("이름은 공백을 제외한 1~12자이며 줄바꿈을 사용할 수 없습니다."));return;}
    if(!bInitialSavePending)
    {
        PlayerName=Clean;auto* Inventory=GetInventory();Inventory->ResetInventory();
        const FName Weapons[]={TEXT("Sword"),TEXT("Bow"),TEXT("Staff"),TEXT("HealBook")};
        const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
        const FName Skills[]={TEXT("SKL_WAR_001"),TEXT("SKL_ARC_001"),TEXT("SKL_MAG_001"),TEXT("SKL_HEAL_001")};
        Inventory->AddItem(Weapons[Job],1,false);Inventory->Equip(0);FirstSkill=Skills[Job];
        Inventory->AddItem(TEXT("PotionHP"),5,false);Inventory->AddItem(TEXT("Food"),3,false);Inventory->AddItem(TEXT("BookSTR"),2,false);Inventory->AddItem(TEXT("Ticket"),10,false);
        Inventory->AddItem(TEXT("Wood"),98,false);Inventory->AddItem(TEXT("Clothes"),1,false);Inventory->AddItem(TEXT("Ring"),1,false);Inventory->AddItem(TEXT("Necklace"),1,false);
        Inventory->QuickSlots={TEXT("PotionHP"),TEXT("Food"),TEXT("BookSTR"),TEXT("Ticket")};
        Cast<APlayerPrototypeCharacter>(GetPawn())->ApplyPreset(Preset);OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);bInitialSavePending=true;bCreated=true;
    }
    if(!SavePlayer()){bCreated=false;Show(TEXT("초기 저장 실패. 생성 버튼으로 저장을 재시도하세요."));UpdateGamePhase();return;}
    bCreated=true;bInitialSavePending=false;SetMenu(TEXT(""));Show(TEXT("생성 완료. I: 인벤토리 / F: 상호작용 / F5: 저장 / F9: 불러오기"));
}

bool APlayerPrototypeController::SavePlayer()
{
    if((!bCreated&&!bInitialSavePending)||bInCombat||bInDialogue||bInEvent||bDead||bEffectWaiting){Show(TEXT("현재 상태에서는 저장할 수 없습니다."));return false;}
    auto* Inventory=GetInventory();if(!Inventory||!Inventory->ValidateState()){Show(TEXT("인벤토리 검증 실패. 저장하지 않았습니다."));return false;}
    auto* Save=Cast<UPlayerPrototypeSave>(UGameplayStatics::CreateSaveGameObject(UPlayerPrototypeSave::StaticClass()));
    Save->PlayerName=PlayerName;Save->Preset=Preset;Save->Job=Job;Save->FirstSkill=FirstSkill;Save->Bag=Inventory->Bag;Save->QuestItems=Inventory->QuestItems;Save->Equipment=Inventory->Equipment;Save->QuickSlots=Inventory->QuickSlots;Save->Storage=Inventory->Storage;
    Save->Position=GetPawn()->GetActorLocation();Save->Rotation=GetControlRotation();Save->HP=TestHP;Save->SP=TestSP;Save->Strength=TestStrength;
    const bool Saved=UGameplayStatics::SaveGameToSlot(Save,GetPlayerSaveSlot(),0);Show(Saved?TEXT("프로토타입 저장 완료"):TEXT("저장 실패. 기존 상태를 유지합니다."));return Saved;
}

bool APlayerPrototypeController::LoadPlayer()
{
    if(bEffectWaiting)return false;
    auto* Save=Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromSlot(GetPlayerSaveSlot(),0));
    if(!Save||Save->Version!=1||Save->Job<0||Save->Job>3||Save->Preset<0||Save->Preset>1||Save->PlayerName.TrimStartAndEnd().IsEmpty()||Save->PlayerName.Len()>12||Save->Position.ContainsNaN()||Save->Rotation.ContainsNaN()){Show(TEXT("사용 가능한 저장 파일이 없습니다."));return false;}
    auto* Inventory=GetInventory();auto* Check=NewObject<UPlayerInventory>(this);Check->Definitions=Inventory->Definitions;Check->ItemTable=Inventory->ItemTable;
    Check->Bag=Save->Bag;Check->QuestItems=Save->QuestItems;Check->Equipment=Save->Equipment;Check->QuickSlots=Save->QuickSlots;Check->Storage=Save->Storage;
    if(!Check->ValidateState()){Show(TEXT("저장 데이터가 올바르지 않습니다. 현재 상태를 유지합니다."));return false;}
    Inventory->Bag=Save->Bag;Inventory->QuestItems=Save->QuestItems;Inventory->Equipment=Save->Equipment;Inventory->QuickSlots=Save->QuickSlots;Inventory->Storage=Save->Storage;
    PlayerName=Save->PlayerName;Preset=Save->Preset;Job=Save->Job;FirstSkill=Save->FirstSkill;TestHP=Save->HP;TestSP=Save->SP;TestStrength=Save->Strength;
    bCreated=true;bDead=false;bInitialSavePending=false;bInCombat=bInDialogue=bInEvent=false;SelectedBag=SelectedStorage=MoveFrom=INDEX_NONE;
    Cast<APlayerPrototypeCharacter>(GetPawn())->ApplyPreset(Preset);GetPawn()->SetActorLocation(Save->Position);Cast<ACharacter>(GetPawn())->GetCharacterMovement()->StopMovementImmediately();SetControlRotation(Save->Rotation);
    Inventory->OnChanged.Broadcast();SetMenu(TEXT(""));Show(TEXT("불러오기 완료"));return true;
}
