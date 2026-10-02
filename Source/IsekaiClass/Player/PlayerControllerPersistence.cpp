#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"
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
    SyncSaveSafety();auto* Saves=GetSaves();
    if(!Saves||!bCreated||SelectedSaveSlot<1||SelectedSaveSlot>4){Show(TEXT("수동 저장 슬롯 1~4를 선택하세요."));return false;}
    const bool Accepted=Saves->SaveSlot(SelectedSaveSlot);
    if(!Accepted)Show(Saves->GetLastMessage());
    return Accepted;
}
bool APlayerPrototypeController::LoadPlayer()
{
    SyncSaveSafety();auto* Saves=GetSaves();if(!Saves)return false;
    const bool WasPending=bInitialSavePending;bInitialSavePending=false;
    const bool Loaded=Saves->LoadSlot(SelectedLoadSlot);
    if(Loaded)RefreshSaveSlots();else bInitialSavePending=WasPending;
    Show(Saves->GetLastMessage());return Loaded;
}
