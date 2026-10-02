#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"
#include "PlayerSaveGame.h"
#include "Containers/Ticker.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void APlayerPrototypeController::StartSmokeTest()
{
    TWeakObjectPtr<APlayerPrototypeController> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis,Step=0](float)mutable
    {
        auto* Controller=WeakThis.Get();if(!Controller)return false;
        auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("PlayerSmoke")/Name,true,false);};bool Passed=true;
        if(Step==0)Capture(TEXT("01_Creation.png"));
        if(Step==1){Controller->PlayerName=TEXT("테스트플레이어");Controller->ExecuteUI(TEXT("Create"),0);Passed=Controller->bCreated&&Controller->GetInventory()->Equipment[0].ItemID==TEXT("Sword");}
        if(Step==2)Capture(TEXT("02_Play.png"));if(Step==3)Controller->SetMenu(TEXT("Inventory"));if(Step==4)Capture(TEXT("03_Inventory.png"));
        if(Step==5){Controller->OpenContainer(TEXT("HomeA"),true,true);Passed=Controller->GetInventory()->Transfer(TEXT("PlayerHome"),0,true);}
        if(Step==6){Controller->OpenContainer(TEXT("HomeB"),true,true);Passed=Controller->GetInventory()->Storage[TEXT("PlayerHome")].Slots[0].Count==5;Capture(TEXT("04_Storage.png"));}
        if(Step==7){Passed=Controller->SavePlayer();Controller->PlayerDied();Passed=Passed&&Controller->Menu==TEXT("Load")&&Controller->LoadPlayer();}
        if(Step==8){Controller->Preset=1;Cast<APlayerPrototypeCharacter>(Controller->GetPawn())->ApplyPreset(1);for(int32 Count=0;Count<8;++Count)Controller->GetInventory()->ExpandUsingTicket();Controller->SetMenu(TEXT("Inventory"));Passed=Controller->GetInventory()->Bag.Slots.Num()==100;}
        if(Step==9)Capture(TEXT("05_FemaleExpanded.png"));
        if(!Passed){UE_LOG(LogTemp,Error,TEXT("PLAYER_SMOKE_FAILED step=%d"),Step);FPlatformMisc::RequestExitWithStatus(false,1);return false;}
        if(++Step>10){UGameplayStatics::DeleteGameInSlot(GetPlayerSaveSlot(),0);UE_LOG(LogTemp,Display,TEXT("PLAYER_SMOKE_PASSED creation, shared storage, save, death, load, inventory layouts"));FPlatformMisc::RequestExit(false);return false;}
        return true;
    }),3.f);
}
