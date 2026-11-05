#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICText.h"
#include "UI/Framework/ICInputPresentation.h"
#include "Settings/ICGameUserSettings.h"
#include "Debug/ICLog.h"
#include "Save/ICSaveSubsystem.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Core/Flow/ICLoadingScreenSubsystem.h"
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
    if(IsUsingGamepad())return ICInputPresentation::KeyLabel(UICGameUserSettings::GamepadFaceKey(2)).ToString();
    if(DefaultMappingContext)
        for(const FICInputCommandBinding& Binding:CommandActions)
            if(Binding.Command==EICInputCommand::Menu&&Binding.Action)
                for(const FEnhancedActionKeyMapping& Mapping:DefaultMappingContext->GetMappings())
                    if(Mapping.Action==Binding.Action)return Mapping.Key.GetDisplayName().ToString();
    return ICText::Get(TEXT("Input_MenuCancel")).ToString();
}

void AICPlayerController::ShowModal(const FString& Action,const FString& Text,int32 Slot)
{
    IC_TRACE_FUNCTION();ShowModal(Action,FText::FromString(Text),Slot);
}
void AICPlayerController::ShowModal(const FString& Action,const FText& Text,int32 Slot)
{
    IC_TRACE_FUNCTION();
    ModalReturnMenu=Menu;ModalAction=Action;ModalDisplayText=Text;ModalText=Text.ToString();ModalSlot=Slot;SetMenu(TEXT("Confirm"));
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
    if(Key==EKeys::G||Key==EKeys::F)
    {if(!bRepeat)HandleInputCommand(EICInputCommand::Confirm);return true;}
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
    if(Saves->bSaving){ShowLocalized(ICText::Get(TEXT("Native_519c9c884838")));return;}
    if(IsMainMenuLevel()&&SessionGameplayMap.IsNull()){ShowLocalized(ICText::Get(TEXT("Native_649fe9810a66")));return;}
    RefreshSaveSlots();
    if(!SaveSlots.IsValidIndex(SelectedLoadSlot)||!SaveSlots[SelectedLoadSlot].bValid){ShowLocalized(ICText::Get(TEXT("Native_12080ba88469")));return;}
    ShowModal(TEXT("Load"),ICText::FormatText(TEXT("Native_85f281a56d17"),SelectedLoadSlot==0?ICText::Get(TEXT("Native_f79024b509aa")):ICText::FormatText(TEXT("Native_8ace6aff0651"),SelectedLoadSlot),FText::FromString(SaveSlots[SelectedLoadSlot].PlayerName)),SelectedLoadSlot);
}
bool AICPlayerController::ContinueLatestSave()
{
    IC_TRACE_FUNCTION();
    if (!IsMainMenuLevel()) return false;
    auto* Saves = GetSaves();
    if (!Saves || Saves->bSaving)
    {
        ShowLocalized(ICText::Get(TEXT("Continue_SaveBusy")));
        return false;
    }
    RefreshSaveSlots();
    const FICSaveSlotInfo* Latest = nullptr;
    for (const FICSaveSlotInfo& Slot : SaveSlots)
        if (Slot.bValid && (!Latest || Slot.SavedAt > Latest->SavedAt)) Latest = &Slot;
    if (!Latest)
    {
        ShowLocalized(ICText::Get(TEXT("Continue_NoSave")));
        return false;
    }
    SelectedLoadSlot = Latest->Slot;
    SessionGameplayMap = GetDefault<UICGameFlowSettings>()->PlayMap;
    if (SessionGameplayMap.IsNull())
    {
        ShowLocalized(ICText::Get(TEXT("Continue_MapMissing")));
        return false;
    }
    return LoadSelectedSessionSlot();
}
bool AICPlayerController::LoadSelectedSessionSlot()
{
    IC_TRACE_FUNCTION();
    if(!IsMainMenuLevel())
    {
        if(LoadPlayer()){SetMenu(TEXT(""));return true;}
        return false;
    }
    auto* Saves=GetSaves();if(!Saves||SessionGameplayMap.IsNull())return false;
    if(!Saves->LoadSlot(SelectedLoadSlot)){ShowLocalized(Saves->GetLastMessageText());RefreshSaveSlots();return false;}
    Saves->SetAutoSaveSuspended(true);
    if(GetLocalPlayer())if(auto* UI=GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>())UI->CloseAllScreens();
    SetMenu(TEXT("Loading"));
    if(auto* Time=GetTime())Time->PrepareForTravel();
    if(auto* Loading=GetGameInstance()->GetSubsystem<UICLoadingScreenSubsystem>())Loading->Arm();
    UGameplayStatics::SetGamePaused(this,false);
    UE_LOG(LogICSave,Log,TEXT("[Session.Load.Travel] Slot=%d Map=%s"),SelectedLoadSlot,*SessionGameplayMap.ToSoftObjectPath().ToString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(this,SessionGameplayMap);
    return true;
}
void AICPlayerController::OpenSaveMenu(bool bQuitAfter)
{
    IC_TRACE_FUNCTION();
    if(bDead){SetMenu(TEXT("Death"));return;}
    if(!bCreated){ShowLocalized(ICText::Get(TEXT("Native_1d492b506be0")));return;}
    if(Menu==TEXT("Confirm"))return;
    if(bItemTransactionInProgress){ShowLocalized(ICText::Get(TEXT("Native_b8c492a66d83")));return;}
    bExitAfterSave=bQuitAfter;
    if(Menu!=TEXT("Save"))SaveReturnMenu=Menu;
    if(SelectedSaveSlot<1||SelectedSaveSlot>4)SelectedSaveSlot=1;
    RefreshSaveSlots();SetMenu(TEXT("Save"));
    ShowLocalized(ICText::Get(TEXT("Native_8094d9d48e6d")));
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
    {ShowLocalized(ICText::Get(TEXT("Native_890a429e13bf")));return;}
    if(Menu==TEXT("Confirm")){ConfirmModal(false);return;}
    if(Menu==TEXT("Save")){bExitAfterSave=false;SetMenu(SaveReturnMenu);return;}
    if(Menu==TEXT("Load"))
    {
        SetMenu(IsMainMenuLevel()?LoadReturnMenu:bDead?TEXT("Death"):!bCreated?TEXT("Create"):LoadReturnMenu);
        return;
    }
    if(Menu==TEXT("Quit")){bQuitAfterWrite=false;bExitAfterSave=false;SetMenu(QuitReturnMenu);return;}
    if(Menu==TEXT("Settings")){SetMenu(SettingsReturnMenu);return;}
    if(Menu==TEXT("MenuSettings")){SetMenu(TEXT("Pause"));return;}
    if(Menu==TEXT("Death"))return;
    SetMenu(bDead?TEXT("Death"):bCreated?TEXT(""):TEXT("Create"));
}
void AICPlayerController::RequestManualSave()
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves||Menu!=TEXT("Save"))return;
    if(SelectedSaveSlot<1||SelectedSaveSlot>4){ShowLocalized(ICText::Get(TEXT("Native_a443b14e9863")));return;}
    SyncSaveSafety();
    const FText Reason=Saves->GetSaveBlockReasonText(false);
    if(!Reason.IsEmpty()){ShowLocalized(Reason);return;}
    RefreshSaveSlots();
    if(SaveSlots[SelectedSaveSlot].bExists)
    {
        ShowModal(TEXT("Overwrite"),ICText::FormatText(TEXT("Native_f7b4049ed41c"),SelectedSaveSlot,SaveSlots[SelectedSaveSlot].bValid?FText::FromString(SaveSlots[SelectedSaveSlot].PlayerName):ICText::Get(TEXT("Save_Unreadable"))),SelectedSaveSlot);return;
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
    {ShowModal(TEXT("CreateOverwrite"),ICText::FormatText(TEXT("Native_654e45c909eb"),SaveSlots[0].bValid?FText::FromString(SaveSlots[0].PlayerName):ICText::Get(TEXT("Save_Unreadable"))),0);return;}
    CreatePlayer();
}
void AICPlayerController::RequestQuit()
{
    IC_TRACE_FUNCTION();
    if(Menu==TEXT("Confirm")||bSessionClosing)return;
    bExitAfterSave=false;
    if(auto* Saves=GetSaves();Saves&&Saves->bSaving)
    {bQuitAfterWrite=true;ShowLocalized(ICText::Get(TEXT("Native_fc02a78eddf6")));return;}
    CompleteQuit();
}
void AICPlayerController::RequestDelete(bool bLegacy)
{
    IC_TRACE_FUNCTION();
    auto* Saves=GetSaves();if(!Saves)return;
    if(Saves->bSaving){ShowLocalized(ICText::Get(TEXT("Native_bd04992c6f93")));return;}
    RefreshSaveSlots();
    const int32 Slot=Menu==TEXT("Save")?SelectedSaveSlot:SelectedLoadSlot;
    if(bLegacy)
    {
        if(!Saves->HasLegacySave()){ShowLocalized(ICText::Get(TEXT("Native_4f52cbacd7d9")));return;}
        ShowModal(TEXT("DeleteLegacy"),ICText::Get(TEXT("Save_DeleteLegacyConfirm")));return;
    }
    if(!SaveSlots.IsValidIndex(Slot)||!SaveSlots[Slot].bExists){ShowLocalized(ICText::Get(TEXT("Native_f369d0e2a9f4")));return;}
    const FText Label=Slot==0?ICText::Get(TEXT("Native_f79024b509aa")):ICText::FormatText(TEXT("Native_8ace6aff0651"),Slot);
    const FText PlayerLabel=SaveSlots[Slot].bValid?FText::FromString(SaveSlots[Slot].PlayerName):ICText::Get(TEXT("Save_Unreadable"));
    ShowModal(TEXT("Delete"),ICText::FormatText(Slot==0?TEXT("Save_DeleteAutoConfirm"):TEXT("Save_DeleteManualConfirm"),Label,PlayerLabel),Slot);
}
void AICPlayerController::ConfirmModal(bool bAccept)
{
    IC_TRACE_FUNCTION();
    if(Menu!=TEXT("Confirm"))return;
    auto* Saves=GetSaves();if(!Saves)return;
    if(Saves->bSaving){ShowLocalized(ICText::Get(TEXT("Native_d2fcf886a269")));return;}
    const FString Action=ModalAction;const int32 Slot=ModalSlot;const FString Return=ModalReturnMenu;
    UE_LOG(LogICUI, Log, TEXT("[UI.Modal.Result] Action=%s Slot=%d Accepted=%d"),*Action,Slot,bAccept);
    ModalAction.Empty();ModalText.Empty();ModalDisplayText=FText::GetEmpty();ModalSlot=INDEX_NONE;SetMenu(Return);
    if(!bAccept){ShowLocalized(ICText::Get(TEXT("Native_72a564eab4ea")));return;}
    if(Action==TEXT("Overwrite")){SelectedSaveSlot=Slot;SavePlayer();}
    else if(Action==TEXT("Load")){SelectedLoadSlot=Slot;LoadSelectedSessionSlot();}
    else if(Action==TEXT("CreateOverwrite"))CreatePlayer();
    else if(Action==TEXT("Delete")||Action==TEXT("DeleteLegacy"))
    {
        if(Action==TEXT("Delete"))Saves->DeleteSlot(Slot);else Saves->DeleteLegacySave();
        RefreshSaveSlots();ShowLocalized(Saves->GetLastMessageText());
    }
}
void AICPlayerController::QuitWithoutSave()
{
    IC_TRACE_FUNCTION();
    if(Menu!=TEXT("Quit"))return;
    RequestQuit();
}
void AICPlayerController::FinishSessionSave(int32 Slot,bool bSuccess)
{
    IC_TRACE_FUNCTION();
    // A direct quit finishes after any existing write, without opening another choice screen.
    if(bQuitAfterWrite){CompleteQuit();return;}
    if(bExitAfterSave&&Slot==SelectedSaveSlot&&Slot>=1)
    {
        if(bSuccess){CompleteQuit();return;}
        bQuitAfterWrite=false;bExitAfterSave=false;SetMenu(TEXT("Quit"));
        ShowLocalized(ICText::Get(TEXT("Native_c9b04467ce1c")));
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
