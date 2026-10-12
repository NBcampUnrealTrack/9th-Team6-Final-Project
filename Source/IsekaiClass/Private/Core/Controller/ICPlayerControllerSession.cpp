#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "Save/ICSaveSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

FString AICPlayerController::MenuCancelLabel() const
{
    IC_TRACE_FUNCTION();
    if(DefaultMappingContext)
        for(const FICInputCommandBinding& Binding:CommandActions)
            if(Binding.Command==EICInputCommand::Menu&&Binding.Action)
                for(const FEnhancedActionKeyMapping& Mapping:DefaultMappingContext->GetMappings())
                    if(Mapping.Action==Binding.Action)return Mapping.Key.GetDisplayName().ToString();
    return TEXT("메뉴 / 취소");
}

void AICPlayerController::ShowModal(const FString& Action,const FString& Text,int32 Slot)
{
    IC_TRACE_FUNCTION();
    ModalReturnMenu=Menu;ModalAction=Action;ModalText=Text;ModalSlot=Slot;SetMenu(TEXT("Confirm"));
}
FString AICPlayerController::GetSessionMenu() const
{
    IC_TRACE_FUNCTION();return Menu==TEXT("Confirm")?ModalReturnMenu:Menu;
}
int32 AICPlayerController::GetSelectedSessionSlot() const
{
    IC_TRACE_FUNCTION();return GetSessionMenu()==TEXT("Save")?SelectedSaveSlot:SelectedLoadSlot;
}
void AICPlayerController::SelectSessionSlot(int32 Slot)
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves||Saves->bSaving||Menu==TEXT("Confirm"))return;
    if(Menu==TEXT("Save")&&Slot>=1&&Slot<=4)SelectedSaveSlot=Slot;
    else if(Menu==TEXT("Load")&&Slot>=0&&Slot<=4)SelectedLoadSlot=Slot;
    else return;
    bDirty=true;OnSaveSessionChanged.Broadcast();
}
bool AICPlayerController::HandleSessionKey(const FKey& Key,bool bRepeat)
{
    IC_TRACE_FUNCTION();
    const FString SessionMenu=GetSessionMenu();
    if(SessionMenu!=TEXT("Save")&&SessionMenu!=TEXT("Load"))return false;
    if(DefaultMappingContext)
        for(const FICInputCommandBinding& Binding:CommandActions)
            if(Binding.Command==EICInputCommand::QuickSlot||Binding.Command==EICInputCommand::Confirm||Binding.Command==EICInputCommand::Menu)
                for(const FEnhancedActionKeyMapping& Mapping:DefaultMappingContext->GetMappings())
                    if(Mapping.Action==Binding.Action&&Mapping.Key==Key)
                    {if(!bRepeat)HandleInputCommand(Binding.Command,Binding.Slot);return true;}
    const FKey NumberKeys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};
    for(int32 Index=0;Index<4;++Index)if(Key==NumberKeys[Index])
    {if(!bRepeat)HandleInputCommand(EICInputCommand::QuickSlot,Index);return true;}
    if(Key==EKeys::Enter||Key==EKeys::Escape)
    {if(!bRepeat)HandleInputCommand(Key==EKeys::Enter?EICInputCommand::Confirm:EICInputCommand::Menu);return true;}
    if(Key==EKeys::Delete&&Menu!=TEXT("Confirm"))
    {if(!bRepeat)RequestDelete();return true;}
    return false;
}
void AICPlayerController::RequestLoadPlayer()
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves||Menu!=TEXT("Load"))return;
    if(Saves->bSaving){Show(TEXT("저장 완료 후 불러오세요."));return;}
    if(IsMainMenuLevel()&&SessionGameplayMap.IsNull()){Show(TEXT("메인 메뉴의 Start Map을 지정하세요."));return;}
    RefreshSaveSlots();
    if(!SaveSlots.IsValidIndex(SelectedLoadSlot)||!SaveSlots[SelectedLoadSlot].bValid){Show(TEXT("불러올 수 있는 슬롯을 선택하세요."));return;}
    ShowModal(TEXT("Load"),FString::Printf(TEXT("%s / %s 저장을 불러올까요? 현재 저장하지 않은 진행은 교체됩니다."),SelectedLoadSlot==0?TEXT("자동 저장"):*FString::Printf(TEXT("수동 슬롯 %d"),SelectedLoadSlot),*SaveSlots[SelectedLoadSlot].PlayerName),SelectedLoadSlot);
}
void AICPlayerController::LoadSelectedSessionSlot()
{
    IC_TRACE_FUNCTION();
    if(!IsMainMenuLevel())
    {
        if(LoadPlayer())SetMenu(TEXT(""));
        return;
    }
    auto* Saves=GetSaves();if(!Saves||SessionGameplayMap.IsNull())return;
    if(!Saves->LoadSlot(SelectedLoadSlot)){Show(Saves->GetLastMessage());RefreshSaveSlots();return;}
    Saves->SetAutoSaveSuspended(true);
    if(auto* Time=GetTime())Time->PrepareForTravel();
    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->CloseAllScreens();
    UGameplayStatics::SetGamePaused(this,false);
    UE_LOG(LogICSave,Log,TEXT("[Session.Load.Travel] Slot=%d Map=%s"),SelectedLoadSlot,*SessionGameplayMap.ToSoftObjectPath().ToString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(this,SessionGameplayMap);
}
void AICPlayerController::OpenSaveMenu(bool bQuitAfter)
{
    IC_TRACE_FUNCTION();
    if(!bCreated){Show(TEXT("캐릭터 생성 후 수동 저장할 수 있습니다."));return;}
    if(Menu==TEXT("Confirm"))return;
    if(bItemTransactionInProgress){Show(TEXT("아이템 효과 처리 완료 후 저장하세요."));return;}
    bExitAfterSave=bQuitAfter;
    if(Menu!=TEXT("Save"))SaveReturnMenu=Menu;
    if(SelectedSaveSlot<1||SelectedSaveSlot>4)SelectedSaveSlot=1;
    RefreshSaveSlots();SetMenu(TEXT("Save"));
    Show(TEXT("버튼 또는 키보드 1~4로 슬롯 선택 → Enter 또는 저장 실행"));
}
void AICPlayerController::OpenLoadMenu()
{
    IC_TRACE_FUNCTION();
    if(Menu==TEXT("Confirm"))return;
    if(Menu!=TEXT("Load"))LoadReturnMenu=Menu;
    RefreshSaveSlots();SetMenu(TEXT("Load"));
}
void AICPlayerController::CloseSessionMenu()
{
    IC_TRACE_FUNCTION();
    if(auto* Saves=GetSaves();Saves&&Saves->bSaving&&(GetSessionMenu()==TEXT("Save")||GetSessionMenu()==TEXT("Load")))
    {Show(TEXT("저장이 완료된 뒤 돌아갈 수 있습니다."));return;}
    if(Menu==TEXT("Confirm")){ConfirmModal(false);return;}
    if(Menu==TEXT("Save")){bExitAfterSave=false;SetMenu(SaveReturnMenu);return;}
    if(Menu==TEXT("Load"))
    {
        SetMenu(IsMainMenuLevel()?LoadReturnMenu:bDead?TEXT("Pause"):!bCreated?TEXT("Create"):LoadReturnMenu);
        if(bDead)Show(TEXT("사망 상태입니다. 저장을 불러오거나 게임을 종료하세요."));
        return;
    }
    if(Menu==TEXT("Quit")){bQuitAfterWrite=false;bExitAfterSave=false;SetMenu(QuitReturnMenu);return;}
    SetMenu(bDead?TEXT("Load"):bCreated?TEXT(""):TEXT("Create"));
}
void AICPlayerController::RequestManualSave()
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves||Menu!=TEXT("Save"))return;
    if(SelectedSaveSlot<1||SelectedSaveSlot>4){Show(TEXT("수동 저장 슬롯 1~4를 선택하세요."));return;}
    SyncSaveSafety();
    const FString Reason=Saves->GetSaveBlockReason(false);
    if(!Reason.IsEmpty()){Show(Reason);return;}
    RefreshSaveSlots();
    if(SaveSlots[SelectedSaveSlot].bExists)
    {
        ShowModal(TEXT("Overwrite"),FString::Printf(TEXT("수동 슬롯 %d / %s의 기존 저장을 덮어쓸까요?"),SelectedSaveSlot,SaveSlots[SelectedSaveSlot].bValid?*SaveSlots[SelectedSaveSlot].PlayerName:TEXT("읽을 수 없는 저장")),SelectedSaveSlot);return;
    }
    SavePlayer();
}
void AICPlayerController::RequestCreatePlayer()
{
    IC_TRACE_FUNCTION();
    FString Reason;
    if(!ValidateCreationRequest(Reason)){ReportCreationFailure(Reason);return;}
    auto* Saves=GetSaves();
    RefreshSaveSlots();
    if(!bInitialSavePending&&SaveSlots[0].bExists)
    {ShowModal(TEXT("CreateOverwrite"),FString::Printf(TEXT("새 캐릭터를 생성하면 자동 저장 / %s를 교체합니다. 계속할까요?"),SaveSlots[0].bValid?*SaveSlots[0].PlayerName:TEXT("읽을 수 없는 저장")),0);return;}
    CreatePlayer();
}
void AICPlayerController::RequestQuit()
{
    IC_TRACE_FUNCTION();
    if(Menu==TEXT("Confirm"))return;
    if(Menu!=TEXT("Quit"))QuitReturnMenu=Menu;
    bExitAfterSave=false;bQuitAfterWrite=false;SetMenu(TEXT("Quit"));
    Show(TEXT("저장 후 종료 / 저장하지 않고 종료 / 취소 중 선택하세요."));
}
void AICPlayerController::RequestDelete(bool bLegacy)
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves)return;
    if(Saves->bSaving){Show(TEXT("저장 완료 후 삭제하세요."));return;}
    RefreshSaveSlots();
    const int32 Slot=Menu==TEXT("Save")?SelectedSaveSlot:SelectedLoadSlot;
    if(bLegacy)
    {
        if(!Saves->HasLegacySave()){Show(TEXT("기존 Version 1/8 원본이 없습니다."));return;}
        ShowModal(TEXT("DeleteLegacy"),TEXT("기존 Version 1/8 원본을 삭제할까요? 이미 가져온 수동 저장은 유지됩니다."));return;
    }
    if(!SaveSlots.IsValidIndex(Slot)||!SaveSlots[Slot].bExists){Show(TEXT("선택 슬롯이 비어 있습니다."));return;}
    const FString Label=Slot==0?TEXT("자동 저장"):FString::Printf(TEXT("수동 슬롯 %d"),Slot);
    ShowModal(TEXT("Delete"),Label+TEXT(" / ")+(SaveSlots[Slot].bValid?SaveSlots[Slot].PlayerName:TEXT("읽을 수 없는 저장"))+TEXT("과 백업·임시·복구 파일을 삭제할까요? 되돌릴 수 없습니다. 현재 플레이는 계속됩니다.")+(Slot==0?TEXT(" 이후 자동 저장이 새로 생성될 수 있습니다."):TEXT("")),Slot);
}
void AICPlayerController::ConfirmModal(bool bAccept)
{
    IC_TRACE_FUNCTION();
    if(Menu!=TEXT("Confirm"))return;
    auto* Saves=GetSaves();if(!Saves)return;
    if(Saves->bSaving){Show(TEXT("저장 완료 후 확인하세요."));return;}
    const FString Action=ModalAction;const int32 Slot=ModalSlot;const FString Return=ModalReturnMenu;
    UE_LOG(LogICUI, Log, TEXT("[UI.Modal.Result] Action=%s Slot=%d Accepted=%d"),*Action,Slot,bAccept);
    ModalAction.Empty();ModalText.Empty();ModalSlot=INDEX_NONE;SetMenu(Return);
    if(!bAccept){Show(TEXT("취소했습니다."));return;}
    if(Action==TEXT("Overwrite")){SelectedSaveSlot=Slot;SavePlayer();}
    else if(Action==TEXT("Load")){SelectedLoadSlot=Slot;LoadSelectedSessionSlot();}
    else if(Action==TEXT("CreateOverwrite"))CreatePlayer();
    else if(Action==TEXT("Delete")||Action==TEXT("DeleteLegacy"))
    {
        if(Action==TEXT("Delete"))Saves->DeleteSlot(Slot);else Saves->DeleteLegacySave();
        RefreshSaveSlots();Show(Saves->GetLastMessage());
    }
}
void AICPlayerController::QuitWithoutSave()
{
    IC_TRACE_FUNCTION();
    if(Menu!=TEXT("Quit"))return;
    if(auto* Saves=GetSaves();Saves&&Saves->bSaving)
    {bQuitAfterWrite=true;Show(TEXT("진행 중인 저장이 완료되면 종료합니다. 실패하면 이 화면을 유지합니다."));return;}
    CompleteQuit();
}
void AICPlayerController::FinishSessionSave(int32 Slot,bool bSuccess)
{
    IC_TRACE_FUNCTION();
    if(bQuitAfterWrite||(bExitAfterSave&&Slot==SelectedSaveSlot&&Slot>=1))
    {
        if(bSuccess){CompleteQuit();return;}
        bQuitAfterWrite=false;bExitAfterSave=false;SetMenu(TEXT("Quit"));
        Show(TEXT("저장에 실패하여 종료하지 않았습니다. 저장 상태를 확인하세요."));
    }
}
void AICPlayerController::CompleteQuit()
{
    IC_TRACE_FUNCTION();
    if(bSessionClosing)return;
    bSessionClosing=true;
    if(auto* Saves=GetSaves())Saves->SetAutoSaveSuspended(true);
    UE_LOG(LogICCore,Log,TEXT("[Session.Quit] Controller=%s: 게임 세션 종료"),*GetNameSafe(this));
#if WITH_EDITOR
    if(GetWorld()&&GetWorld()->WorldType==EWorldType::PIE)
    {
        if(auto* Editor=Cast<UEditorEngine>(GEngine))Editor->RequestEndPlayMap();
        return;
    }
#endif
    UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
}
