#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"
#include "PlayerGameState.h"
#include "PlayerSaveGame.h"
#include "PlayerTimeSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"

void APlayerPrototypeController::CreatePlayer()
{
    auto* State=GetWorldState();auto* Saves=GetSaves();auto* Time=GetTime();auto* Inventory=GetInventory();
    if(!State||!Saves||!Time||!Inventory||bCreated||Saves->bSaving||Job<0||Job>3||Preset<0||Preset>1)return;
    const FString Clean=PlayerName.TrimStartAndEnd();bool Invalid=Clean.IsEmpty()||Clean.Len()>12;
    for(TCHAR Ch:PlayerName)if(Ch<TEXT(' ')||Ch==0x2028||Ch==0x2029)Invalid=true;
    if(Invalid){Show(TEXT("이름은 공백을 제외한 1~12자이며 줄바꿈을 사용할 수 없습니다."));return;}
    if(!bInitialSavePending)
    {
        PlayerName=Clean;Survival=FPlayerSurvivalState();MainStats=FPlayerMainStats();Gold=1000;auto* Inventory=GetInventory();Inventory->ResetInventory();Inventory->CurrentClass=Job;
        const FName Weapons[]={TEXT("EQP_WPN_SWORD_001"),TEXT("EQP_WPN_BOW_001"),TEXT("EQP_WPN_STAFF_001"),TEXT("EQP_WPN_WAND_001")};
        const FName Jobs[]={TEXT("Warrior"),TEXT("Archer"),TEXT("Mage"),TEXT("Healer")};
        const FName Skills[]={TEXT("SKL_WAR_001"),TEXT("SKL_ARC_001"),TEXT("SKL_MAG_001"),TEXT("SKL_HEAL_001")};
        Inventory->AddItem(Weapons[Job],1,false);Inventory->Equip(0);FirstSkill=Skills[Job];
        Inventory->AddItem(TEXT("ITM_CONS_001"),5,false);Inventory->AddItem(TEXT("ITM_FOOD_001"),3,false);Inventory->AddItem(TEXT("ITM_BOOK_001"),2,false);Inventory->AddItem(TEXT("ITM_TKT_001"),10,false);
        Inventory->AddItem(TEXT("ITM_MAT_001"),98,false);Inventory->AddItem(TEXT("EQP_ARM_001"),1,false);Inventory->AddItem(TEXT("EQP_RNG_001"),1,false);Inventory->AddItem(TEXT("EQP_NCK_001"),1,false);
        Inventory->QuickSlots={TEXT("ITM_CONS_001"),TEXT("ITM_FOOD_001"),TEXT("ITM_BOOK_001"),TEXT("ITM_TKT_001")};
        Cast<APlayerPrototypeCharacter>(GetPawn())->ApplyPreset(Preset);OnPlayerInitialized.Broadcast(Jobs[Job],FirstSkill);bInitialSavePending=true;bCreated=true;
    }
    if(!AutoSavePlayer()){bCreated=false;Show(TEXT("초기 저장 실패. 생성 버튼으로 저장을 재시도하세요."));UpdateGamePhase();return;}
    bCreated=true;bInitialSavePending=false;SetMenu(TEXT(""));Show(TEXT("생성 완료. I: 인벤토리 / F: 상호작용 / F5: 저장 / F9: 불러오기"));
        const FName Weapons[]={TEXT("Sword"),TEXT("Bow"),TEXT("Staff"),TEXT("HealBook")};
        const FName Skills[]={TEXT("SKL_WAR_001"),TEXT("SKL_ARC_001"),TEXT("SKL_MAG_001"),TEXT("SKL_HEAL_001")};
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=Clean;Initial.Preset=Preset;
        Initial.Job=Job;Initial.FirstSkill=Skills[Job];Initial.PlayerPosition=GetPawn()->GetActorLocation();Initial.PlayerRotation=GetControlRotation();
        FICCharacterState InitialCharacterState;InitialCharacterState.CharacterId=UICWorldStateSubsystem::PlayerId();InitialCharacterState.Kind=EICCharacterKind::Player;
        Initial.Characters.Add(InitialCharacterState.CharacterId,InitialCharacterState);bInitialSavePending=true;
        if(!State->RestoreState(Initial)){bInitialSavePending=false;Show(TEXT("초기 상태 검증 실패"));return;}
        Time->ResetClock();Saves->ResetSession();
        Inventory->AddItem(Weapons[Job],1,false);Inventory->Equip(0);
        const TPair<FName,int32> Items[]={
            {TEXT("PotionHP"),5},{TEXT("Food"),3},{TEXT("BookSTR"),2},{TEXT("Ticket"),10},
            {TEXT("Wood"),98},{TEXT("Clothes"),1},{TEXT("Ring"),1},{TEXT("Necklace"),1}
        };
        for(const auto& Item:Items)Inventory->AddItem(Item.Key,Item.Value,false);
        auto Data=Inventory->GetSnapshot();Data.QuickSlots={TEXT("PotionHP"),TEXT("Food"),TEXT("BookSTR"),TEXT("Ticket")};
        if(!Inventory->ValidateSnapshot(Data)||!State->CommitInventoryState(Data)){Show(TEXT("초기 인벤토리 검증 실패"));return;}
    }
    SyncSaveSafety();
    if(!Saves->SaveSlot(0)){Show(Saves->GetLastMessage()+TEXT(" 생성 버튼으로 재시도하세요."));UpdateGamePhase();}
}

bool APlayerPrototypeController::SavePlayer()
{
    return SavePlayerToSlot(GetPlayerManualSaveSlot(0),TEXT("수동 저장 1번 완료"));
}

bool APlayerPrototypeController::AutoSavePlayer()
{
    return SavePlayerToSlot(GetPlayerSaveSlot(),TEXT("오토세이브 완료"));
}

bool APlayerPrototypeController::SavePlayerToSlot(const TCHAR* SlotName,const TCHAR* SuccessMessage)
{
    if((!bCreated&&!bInitialSavePending)||bInCombat||bInDialogue||bInEvent||bDead||bEffectWaiting){Show(TEXT("현재 상태에서는 저장할 수 없습니다."));return false;}
    auto* Inventory=GetInventory();if(!Inventory||!Inventory->ValidateState()){Show(TEXT("인벤토리 검증 실패. 저장하지 않았습니다."));return false;}
    auto* Save=Cast<UPlayerPrototypeSave>(UGameplayStatics::CreateSaveGameObject(UPlayerPrototypeSave::StaticClass()));
    Save->PlayerName=PlayerName;Save->Preset=Preset;Save->Job=Job;Save->FirstSkill=FirstSkill;Save->Gold=Gold;Save->Bag=Inventory->Bag;Save->QuestItems=Inventory->QuestItems;Save->Equipment=Inventory->Equipment;Save->QuickSlots=Inventory->QuickSlots;Save->Storage=Inventory->Storage;
    Save->Position=GetPawn()->GetActorLocation();Save->Rotation=GetControlRotation();Save->Survival=Survival;Save->MainStats=MainStats;
    if(auto* Time=GetWorld()?GetWorld()->GetSubsystem<UPlayerTimeSubsystem>():nullptr){Save->GameMinute=Time->GameMinute;Save->PlaySeconds=Time->PlaySeconds;}
    FlushWorldState();const bool Saved=UGameplayStatics::SaveGameToSlot(Save,SlotName,0);Show(Saved?SuccessMessage:TEXT("저장 실패. 기존 상태를 유지합니다."));return Saved;
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||!bCreated||SelectedSaveSlot<1||SelectedSaveSlot>4){Show(TEXT("수동 저장 슬롯 1~4를 선택하세요."));return false;}
    const bool Accepted=Saves->SaveSlot(SelectedSaveSlot);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
bool APlayerPrototypeController::LoadPlayer()
{
    if(bEffectWaiting)return false;
    auto* Save=Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromSlot(GetPlayerSaveSlot(),0));
    if(!Save)Save=Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromSlot(GetPlayerManualSaveSlot(0),0));
    if(!Save||Save->Version!=8||Save->Gold<0||Save->Job<0||Save->Job>3||Save->Preset<0||Save->Preset>1||Save->PlayerName.TrimStartAndEnd().IsEmpty()||Save->PlayerName.Len()>12||Save->Position.ContainsNaN()||Save->Rotation.ContainsNaN()||!Save->MainStats.IsValid()||!Save->Survival.IsValid()){Show(TEXT("사용 가능한 저장 파일이 없습니다."));return false;}
    auto* Inventory=GetInventory();auto* Check=NewObject<UPlayerInventory>(this);Check->Definitions=Inventory->Definitions;Check->ItemTable=Inventory->ItemTable;
    Check->Bag=Save->Bag;Check->QuestItems=Save->QuestItems;Check->Equipment=Save->Equipment;Check->QuickSlots=Save->QuickSlots;Check->Storage=Save->Storage;
    if(!Check->ValidateState()){Show(TEXT("저장 데이터가 올바르지 않습니다. 현재 상태를 유지합니다."));return false;}
    Inventory->Bag=Save->Bag;Inventory->QuestItems=Save->QuestItems;Inventory->Equipment=Save->Equipment;Inventory->QuickSlots=Save->QuickSlots;Inventory->Storage=Save->Storage;
    PlayerName=Save->PlayerName;Preset=Save->Preset;Job=Save->Job;FirstSkill=Save->FirstSkill;Gold=Save->Gold;Survival=Save->Survival;MainStats=Save->MainStats;Inventory->CurrentClass=Job;bLastExhausted=Survival.IsExhausted();if(auto* Time=GetWorld()?GetWorld()->GetSubsystem<UPlayerTimeSubsystem>():nullptr)Time->Restore(Save->GameMinute,Save->PlaySeconds);
    bCreated=true;bDead=false;bInitialSavePending=false;bInCombat=bInDialogue=bInEvent=false;SelectedBag=SelectedStorage=MoveFrom=INDEX_NONE;
    Cast<APlayerPrototypeCharacter>(GetPawn())->ApplyPreset(Preset);GetPawn()->SetActorLocation(Save->Position);Cast<ACharacter>(GetPawn())->GetCharacterMovement()->StopMovementImmediately();SetControlRotation(Save->Rotation);
    FlushWorldState();Inventory->OnChanged.Broadcast();SetMenu(TEXT(""));Show(TEXT("불러오기 완료"));return true;
    SyncSaveSafety();auto* Saves=GetSaves();if(!Saves)return false;
    const bool WasPending=bInitialSavePending;bInitialSavePending=false;
    const bool Loaded=Saves->LoadSlot(SelectedLoadSlot);
    if(Loaded)RefreshSaveSlots();else bInitialSavePending=WasPending;
    Show(Saves->GetLastMessage());return Loaded;
}
