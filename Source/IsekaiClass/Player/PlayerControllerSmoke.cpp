#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInteractionObject.h"
#include "PlayerInventory.h"
#include "PlayerPrototypeWidget.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Core/ICSaveSubsystem.h"
#include "../Core/ICSaveGame.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Components/TextRenderComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void APlayerPrototypeController::StartSmokeTest()
{
    FString Stage;FParse::Value(FCommandLine::Get(),TEXT("SmokeStage="),Stage);
    const bool bReload=Stage==TEXT("Reload");
    const FString Manifest=FPaths::ProjectSavedDir()/TEXT("PlayerSmoke")/TEXT("IC_Test_Smoke_Expected.txt");
    if(!bReload)
    {
        for(int32 Slot=0;Slot<=4;++Slot)GetSaves()->DeleteSlot(Slot);
        IFileManager::Get().Delete(*Manifest,false,false,true);
    }
    TWeakObjectPtr<APlayerPrototypeController> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis,bReload,Manifest,Step=0,Waits=0](float) mutable
    {
        auto* C=WeakThis.Get();if(!C)return false;
        auto* Saves=C->GetSaves();auto* State=C->GetWorldState();auto* Time=C->GetTime();auto* Inv=C->GetInventory();
        auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("PlayerSmoke")/Name,true,false);};
        auto UIKey=[](FKey Key){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));};
        auto GameKey=[C](FKey Key){C->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));C->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0.f));};
        auto Approach=[C,GameKey](const FString& Label)
        {
            C->SetMenu(TEXT(""));
            for(TActorIterator<APlayerPrototypeObject> It(C->GetWorld());It;++It)
            {
                if(It->Label->Text.ToString()==Label)
                {
                    C->GetPawn()->SetActorLocation(It->GetActorLocation()+FVector(-150,0,100));
                    C->SetControlRotation(FRotator(-10,0,0));GameKey(EKeys::F);return true;
                }
            }
            return false;
        };
        bool Passed=true;
        if(!bReload&&(Step==2||Step==10)&&Saves->bSaving)
        {if(++Waits<15)return true;UE_LOG(LogTemp,Error,TEXT("PLAYER_SMOKE_FAILED save timeout"));FPlatformMisc::RequestExitWithStatus(false,1);return false;}
        Waits=0;
        if(bReload)
        {
            if(Step==0)
            {
                FString Expected;TArray<FString> Fields;
                Passed=FFileHelper::LoadFileToString(Expected,*Manifest);
                Expected.ParseIntoArray(Fields,TEXT("|"));
                C->SelectedLoadSlot=2;Passed=Passed&&C->LoadPlayer();
                if(Fields.Num()!=4)Passed=false;
                if(Passed)
                {
                    const auto Data=Inv->GetSnapshot();
                    Passed=Data.Equipment[0].InstanceID.ToString()==Fields[0]&&Data.Bag.Slots.Num()==100&&Data.Storage[TEXT("PlayerHome")].Slots[0].Count==5;
                    Passed=Passed&&FMath::IsNearlyEqual(Time->GetClock().GameMinute,FCString::Atod(*Fields[2]),0.0001)&&FMath::IsNearlyEqual(Time->GetClock().PlaySecond,FCString::Atod(*Fields[3]),0.0001);
                }
                C->SetMenu(TEXT("Inventory"));Capture(TEXT("08_Reload.png"));
            }
            if(Step==1){C->OpenSaveMenu();UIKey(EKeys::Two);C->RequestDelete();Capture(TEXT("09_DeleteConfirm.png"));}
            if(Step==2)
            {
                UIKey(C->MenuCancelKey());Passed=C->Menu==TEXT("Save")&&Saves->GetSlotList()[2].bValid;
                C->RequestDelete();UIKey(EKeys::Enter);
                Passed=Passed&&!Saves->GetSlotList()[2].bExists&&C->bCreated;
                C->SetMenu(TEXT("Inventory"));C->ExecuteUI(TEXT("Quit"),0);Capture(TEXT("10_Quit.png"));
            }
            if(Step==3)
            {
                C->CloseSessionMenu();Passed=C->Menu==TEXT("Inventory");
                auto* Future=NewObject<UICSaveGame>();Future->WorldState=State->ExportState();Future->Clock=Time->GetClock();Future->Slot=3;Future->SaveVersion=999;
                TArray<uint8> Bytes;Passed=UICSaveGame::EncodeForFile(Future,Bytes)&&FFileHelper::SaveArrayToFile(Bytes,*Saves->GetSlotPath(3))&&Passed;
                C->OpenSaveMenu(true);UIKey(EKeys::Three);C->RequestManualSave();UIKey(EKeys::Enter);
                Passed=Passed&&C->Menu==TEXT("Save")&&!Saves->bSaving&&C->bCreated;
                C->RequestQuit();
                for(int32 Slot=0;Slot<=4;++Slot)Passed=Saves->DeleteSlot(Slot)&&Passed;
                IFileManager::Get().Delete(*Manifest,false,false,true);
                if(Passed){UE_LOG(LogTemp,Display,TEXT("PLAYER_SMOKE_RELOAD_PASSED process restart, identity, clock, deletion, quit cancel"));C->QuitWithoutSave();return false;}
            }
        }
        else
        {
            if(Step==0)Capture(TEXT("01_Creation.png"));
            if(Step==1){C->PlayerName=TEXT("테스트플레이어");C->ExecuteUI(TEXT("Create"),0);}
            if(Step==2){Passed=C->bCreated&&Inv->GetSnapshot().Equipment[0].ItemID==TEXT("Sword");Capture(TEXT("02_Play.png"));}
            if(Step==3){C->SelectedLoadSlot=0;Passed=C->LoadPlayer()&&C->SelectedSaveSlot==1;C->SetMenu(TEXT("Inventory"));}
            if(Step==4)Capture(TEXT("03_Inventory.png"));
            if(Step==5)Passed=Approach(TEXT("HOME A"));
            if(Step==6){Passed=C->Menu==TEXT("Storage")&&Inv->Transfer(TEXT("PlayerHome"),0,true);Passed=Approach(TEXT("HOME B (SHARED)"))&&Passed;}
            if(Step==7){Passed=C->Menu==TEXT("Storage")&&Inv->GetSnapshot().Storage[TEXT("PlayerHome")].Slots[0].Count==5;Capture(TEXT("04_Storage.png"));}
            if(Step==8)
            {
                const auto Before=Time->GetClock();
                for(const auto& Entry:TArray<TPair<int32,double>>{{3,3980.0},{4,3260.0}})
                {
                    auto SavedClock=Before;SavedClock.GameMinute=Entry.Value;
                    Passed=Time->RestoreClock(SavedClock)&&Saves->SaveSlotSynchronously(Entry.Key)&&Passed;
                }
                Passed=Time->RestoreClock(Before)&&Passed;
                C->SetMenu(TEXT("Inventory"));C->ExecuteUI(TEXT("Save"),0);UIKey(EKeys::Four);
                Passed=Passed&&C->Menu==TEXT("Save")&&C->SelectedSaveSlot==4;
                Capture(TEXT("06_Save.png"));
            }
            if(Step==9)
            {
                bool bMorning=false,bAfternoon=false;
                if(C->Screen&&C->Screen->WidgetTree)C->Screen->WidgetTree->ForEachWidget([&](UWidget* Widget)
                {
                    if(const auto* Text=Cast<UTextBlock>(Widget))
                    {
                        const auto Label=Text->GetText().ToString();
                        bMorning|=Label.Contains(TEXT("이세계3일차 오전6시20분"));
                        bAfternoon|=Label.Contains(TEXT("이세계3일차 오후6시20분"));
                    }
                });
                Passed=bMorning&&bAfternoon;
                UIKey(EKeys::One);UIKey(EKeys::Enter);Passed=Passed&&C->SelectedSaveSlot==1&&Saves->bSaving;
            }
            if(Step==10)
            {
                C->PlayerDied();C->SelectedLoadSlot=1;Passed=C->Menu==TEXT("Load")&&C->LoadPlayer();
                Passed=Passed&&Inv->GetSnapshot().Storage[TEXT("PlayerHome")].Slots[0].Count==5&&C->SelectedSaveSlot==1;
            }
            if(Step==11)
            {
                auto Data=State->ExportState();Data.Preset=1;Passed=State->RestoreState(Data);
                for(int32 N=0;N<8;++N)Passed=Inv->ExpandUsingTicket()&&Passed;
                C->SetMenu(TEXT("Inventory"));Passed=Passed&&Inv->GetSnapshot().Bag.Slots.Num()==100;
            }
            if(Step==12)
            {
                Capture(TEXT("05_FemaleExpanded.png"));const auto Before=Time->GetClock();Time->AdvanceClock(10,true);
                Passed=Time->GetClock().PlaySecond==Before.PlaySecond&&Time->GetClock().GameMinute==Before.GameMinute;
                Passed=Time->JumpGameMinutes(180)&&Passed&&Time->GetClock().PlaySecond==Before.PlaySecond;
                Time->RestoreClock(Before);
            }
            if(Step==13){C->SetMenu(TEXT(""));GameKey(EKeys::RightMouseButton);GameKey(C->MenuCancelKey());}
            if(Step==14)
            {
                Passed=C->Menu==TEXT("Pause")&&!C->bBlocking;
                UIKey(C->MenuCancelKey());C->BeginItem(TEXT("Food"));C->HandleKey(C->MenuCancelKey());Passed=Passed&&C->PendingItem.IsNone();
            }
            if(Step==15)
            {
                const auto Before=Time->GetClock();C->ConsoleCommand(TEXT("ICAddGameMinutes 60"),true);
                Passed=Time->GetClock().GameMinute==Before.GameMinute+60&&Time->GetClock().PlaySecond==Before.PlaySecond;
                const int32 FoodBefore=Inv->CountItem(TEXT("Food"));C->ConsoleCommand(TEXT("ICGiveItem Food 1"),true);
                C->ConsoleCommand(TEXT("ICSetAffinity NPC_R01_001 100"),true);C->ConsoleCommand(TEXT("ICTeleport 10 20 130"),true);
                Passed=Passed&&Inv->CountItem(TEXT("Food"))==FoodBefore+1&&State->GetSnapshot(TEXT("NPC_R01_001")).Affinity==100&&C->GetPawn()->GetActorLocation().Equals(FVector(10,20,130));
                Time->RestoreClock(Before);
            }
            if(Step==16){C->RequestQuit();Capture(TEXT("07_Quit.png"));}
            if(Step==17)
            {
                C->ExecuteUI(TEXT("QuitSave"),0);
                UIKey(EKeys::Two);
                const FString Expected=Inv->GetSnapshot().Equipment[0].InstanceID.ToString()+FString::Printf(TEXT("|%d|%.9f|%.9f"),Inv->GetSnapshot().Bag.Slots.Num(),Time->GetClock().GameMinute,Time->GetClock().PlaySecond);
                Passed=FFileHelper::SaveStringToFile(Expected,*Manifest);
                UIKey(EKeys::Enter);Passed=Passed&&C->SelectedSaveSlot==2&&Saves->bSaving;
                if(Passed){UE_LOG(LogTemp,Display,TEXT("PLAYER_SMOKE_SAVE_READY actual F input, UI number/Enter input, async save then quit"));return false;}
            }
        }
        if(!Passed){UE_LOG(LogTemp,Error,TEXT("PLAYER_SMOKE_FAILED stage=%s step=%d notice=%s"),bReload?TEXT("Reload"):TEXT("Save"),Step,*C->Notice);FPlatformMisc::RequestExitWithStatus(false,1);return false;}
        ++Step;return true;
    }),1.f);
}
