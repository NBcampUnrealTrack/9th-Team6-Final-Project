#include "Save/ICSaveSubsystem.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "Save/Compatibility/ICSaveDataMigration.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Save/ICSaveGame.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/Compatibility/ICLegacyPlayerSaveGame.h"
#include "Player/State/ICPlayerStateConversions.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
constexpr int64 MaximumSaveBytes = 16 * 1024 * 1024;

bool ReadBytes(const FString& Path, TArray<uint8>& Bytes)
{
    IC_TRACE_FUNCTION();
    const int64 Size = IFileManager::Get().FileSize(*Path);
    const bool bRead=Size > 0 && Size <= MaximumSaveBytes && FFileHelper::LoadFileToArray(Bytes, *Path);
    if(!bRead) UE_LOG(LogICSave, VeryVerbose, TEXT("[Save.ReadBytes.Rejected] File=%s Bytes=%lld"),*Path,Size);
    return bRead;
}

bool WriteVerified(const FString& Path, const TArray<uint8>& Bytes)
{
    IC_TRACE_FUNCTION();
    TArray<uint8> ReadBack;
    const bool bVerified=FFileHelper::SaveArrayToFile(Bytes, *Path) && ReadBytes(Path, ReadBack) && ReadBack == Bytes;
    if(!bVerified) UE_LOG(LogICSave, Warning, TEXT("[Save.WriteVerified.Failed] File=%s RequestedBytes=%d ReadBytes=%d"),*Path,Bytes.Num(),ReadBack.Num());
    return bVerified;
}

// File work runs on a worker thread. Resolve these keys to FText only on the game thread.
struct FWriteResult { bool bSuccess = false; FString MessageKey; };

FWriteResult WriteSlotFile(const FString& Path, const TArray<uint8>& Bytes, bool bAuto, bool bRotateBackup)
{
    IC_TRACE_FUNCTION();
    auto& Files = IFileManager::Get();
    const FString Temporary = Path + TEXT(".tmp");
    const FString Rollback = Path + TEXT(".rollback");
    const FString Backup = Path + TEXT(".bak");
    const FString BackupTemporary = Backup + TEXT(".tmp");
    if (!Files.MakeDirectory(*FPaths::GetPath(Path), true) || !WriteVerified(Temporary, Bytes))
        return { false, TEXT("Save_WriteTemporaryFailed") };

    const bool bExisting = Files.FileExists(*Path);
    TArray<uint8> Original;
    if (bExisting)
    {
        if (!ReadBytes(Path, Original)) return { false, TEXT("Save_PreserveExistingFailed") };
        if ((bRotateBackup || !Files.FileExists(*Rollback)) && !WriteVerified(Rollback, Original))
            return { false, TEXT("Save_WriteRollbackFailed") };
        if (bAuto && bRotateBackup)
        {
            if (!WriteVerified(BackupTemporary, Original) || !Files.Move(*Backup, *BackupTemporary, true, false, false, true))
                return { false, TEXT("Save_WriteBackupFailed") };
        }
    }
    if (!Files.Move(*Path, *Temporary, true, false, false, true))
    {
        if (bExisting) WriteVerified(Path, Original);
        return { false, TEXT("Save_ReplaceFailed") };
    }
    TArray<uint8> Committed;
    if (!ReadBytes(Path, Committed) || Committed != Bytes)
    {
        if (bExisting) WriteVerified(Path, Original);
        return { false, TEXT("Save_VerifyReplacedFailed") };
    }
    if (bRotateBackup || !bExisting) Files.Delete(*Rollback, false, false, true);
    return { true, TEXT("Save_Complete") };
}
}

void UICSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    IC_TRACE_FUNCTION();
    Super::Initialize(Collection);
    WorldState = Collection.InitializeDependency<UICWorldStateSubsystem>();
    Time = Collection.InitializeDependency<UICTimeSubsystem>();
    UE_LOG(LogICSave, Log, TEXT("[Save.Initialize] State=%s Time=%s Prefix=%s"),*GetNameSafe(WorldState),*GetNameSafe(Time),*Prefix);
}

bool UICSaveSubsystem::ConfigureServices(UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime, const FString& InPrefix)
{
    IC_TRACE_FUNCTION();
    if (bSaving || !InState || !InTime || InPrefix.IsEmpty() || InPrefix.Len() > 64) return false;
    for (TCHAR C : InPrefix) if (!FChar::IsAlnum(C) && C != TEXT('_')) return false;
    WorldState = InState; Time = InTime; Prefix = InPrefix; ResetSession();
    return true;
}

bool UICSaveSubsystem::IsTickable() const { IC_TRACE_FUNCTION(); return !IsTemplate() && GetWorld() && GetWorld()->IsGameWorld(); }
TStatId UICSaveSubsystem::GetStatId() const { IC_TRACE_FUNCTION(); RETURN_QUICK_DECLARE_CYCLE_STAT(UICSaveSubsystem, STATGROUP_Tickables); }
void UICSaveSubsystem::Tick(float DeltaTime) { IC_TRACE_FUNCTION(); PollAutoSave(); }

FString UICSaveSubsystem::GetSlotPath(int32 Slot) const
{
    IC_TRACE_FUNCTION();
    if (Slot < 0 || Slot > 4) return FString();
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Prefix + (Slot == 0 ? TEXT("_Auto") : FString::Printf(TEXT("_Manual%d"), Slot)) + TEXT(".sav"));
}

FString UICSaveSubsystem::GetSaveBlockReason(bool bAutoSave) const
{
    IC_TRACE_FUNCTION();return GetSaveBlockReasonText(bAutoSave).ToString();
}
FText UICSaveSubsystem::GetSaveBlockReasonText(bool bAutoSave) const
{
    IC_TRACE_FUNCTION();
    if (!WorldState || !Time) return ICText::Get(TEXT("Save_NotReady"));
    if (bSaving) return ICText::Get(TEXT("Save_Busy"));
    if (!WorldState->ExportState().bPlayerCreated) return ICText::Get(TEXT("Save_CreatePlayerFirst"));
    if (Safety.bDead) return ICText::Get(TEXT("Save_BlockedDead"));
    if (Safety.bInCombat) return ICText::Get(TEXT("Save_BlockedCombat"));
    if (Safety.bInDialogue) return ICText::Get(TEXT("Save_BlockedDialogue"));
    if (Safety.bInEvent) return ICText::Get(TEXT("Save_BlockedEvent"));
    if (Safety.bEffectPending) return ICText::Get(TEXT("Save_BlockedEffect"));
    const auto Player = WorldState->GetSnapshot(UICWorldStateSubsystem::PlayerId());
    if (Player.CharacterId.IsNone() || Player.HP <= 0 || Player.MaxHP <= 0) return ICText::Get(TEXT("Save_InvalidPlayer"));
    if (bAutoSave)
    {
        if (Player.HP / Player.MaxHP <= FMath::Clamp(AutoSaveHealthThreshold, 0.f, 1.f)) return ICText::Get(TEXT("Save_AutoLowHealth"));
        for (const auto& D : Player.Debuffs) if (D.RemainingSeconds > 0) return ICText::Get(TEXT("Save_AutoDebuff"));
        for (const auto& Effect : Player.Combat.Effects)
            if (Effect.bHarmful && (Effect.bInfinite || Effect.ExpiresAtPlaySecond > Time->GetClock().PlaySecond)) return ICText::Get(TEXT("Save_AutoDebuff"));
    }
    return FText::GetEmpty();
}
bool UICSaveSubsystem::CanSave(bool bAutoSave) const { IC_TRACE_FUNCTION(); return GetSaveBlockReasonText(bAutoSave).IsEmpty(); }

FString UICSaveSubsystem::GetLegacySavePath() const
{
    IC_TRACE_FUNCTION();
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / ((Prefix.StartsWith(TEXT("IC_Test_")) ? Prefix + TEXT("_LegacyV1") : TEXT("PlayerPrototype_Auto")) + TEXT(".sav"));
}
bool UICSaveSubsystem::HasLegacySave() const { IC_TRACE_FUNCTION(); return IFileManager::Get().FileExists(*GetLegacySavePath()); }

bool UICSaveSubsystem::DeleteSlot(int32 Slot)
{
    IC_TRACE_FUNCTION();
    if (bSaving) { SetLastMessage(ICText::Get(TEXT("Save_DeleteBusy"))); return false; }
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty()) { SetLastMessage(ICText::Get(TEXT("Save_InvalidSlot"))); return false; }
    TArray<FString> Failed;
    for (const FString& Suffix : {FString(), FString(TEXT(".bak")), FString(TEXT(".tmp")), FString(TEXT(".rollback")), FString(TEXT(".bak.tmp"))})
    {
        const FString File = Path + Suffix;
        if (IFileManager::Get().FileExists(*File))
        {
            IFileManager::Get().Delete(*File, false, false, true);
            if (IFileManager::Get().FileExists(*File)) Failed.Add(FPaths::GetCleanFilename(File));
        }
    }
    if (!Failed.IsEmpty()) { SetLastMessage(ICText::FormatText(TEXT("Save_DeleteFilesFailed"),FString::Join(Failed,TEXT(", "))));UE_LOG(LogICSave, Warning, TEXT("[Save.Delete.Failed] Slot=%d Reason=%s"),Slot,*LastMessage); return false; }
    UE_LOG(LogICSave, Log, TEXT("[Save.Deleted] Slot=%d File=%s: 원본/백업/임시/복구 파일 정리"),Slot,*Path);
    if (Slot == 0) ResetSession();
    SetLastMessage(Slot == 0 ? ICText::Get(TEXT("Save_AutoDeleted")) : ICText::FormatText(TEXT("Native_ea5f792f4df6"), Slot));
    return true;
}
bool UICSaveSubsystem::DeleteLegacySave()
{
    IC_TRACE_FUNCTION();
    if (bSaving) { SetLastMessage(ICText::Get(TEXT("Save_DeleteBusy"))); return false; }
    const FString Path = GetLegacySavePath();
    IFileManager::Get().Delete(*Path, false, false, true);
    if (IFileManager::Get().FileExists(*Path)) { SetLastMessage(ICText::Get(TEXT("Save_DeleteLegacyFailed"))); return false; }
    SetLastMessage(ICText::Get(TEXT("Save_LegacyDeleted")));UE_LOG(LogICSave, Log, TEXT("[Save.Legacy.Deleted] File=%s"),*Path); return true;
}

bool UICSaveSubsystem::ValidateSave(const UICSaveGame* Save, int32 Slot) const
{
    IC_TRACE_FUNCTION();
    return Save && Save->SaveVersion == UICSaveGame::CurrentVersion && Save->Slot == Slot && Save->Clock.IsValid()
        && Save->WorldState.bPlayerCreated && UICWorldStateSubsystem::ValidateStateClock(Save->WorldState, Save->Clock)
        && WorldState && WorldState->CanRestoreState(Save->WorldState);
}

UICSaveGame* UICSaveSubsystem::ReadSlot(int32 Slot, bool bAllowBackup, bool& bUsedBackup) const
{
    IC_TRACE_FUNCTION();
    bUsedBackup = false;
    const FString Path = GetSlotPath(Slot); if (Path.IsEmpty()) return nullptr;
    TArray<uint8> Bytes;
    if (ReadBytes(Path, Bytes))
    {
        auto* Save = UICSaveGame::DecodeFile(Bytes);
        if (UICSaveGame::IsFutureFile(Bytes) || (Save && Save->SaveVersion > UICSaveGame::CurrentVersion)) return nullptr;
        if (ValidateSave(Save, Slot)) return Save;
    }
    if (bAllowBackup)
    {
        const TArray<FString> Candidates = Slot == 0
            ? TArray<FString>{ Path + TEXT(".bak"), Path + TEXT(".rollback"), Path + TEXT(".bak.tmp") }
            : TArray<FString>{ Path + TEXT(".rollback") };
        for (const auto& Candidate : Candidates)
        {
            Bytes.Reset();
            if (!ReadBytes(Candidate, Bytes)) continue;
            auto* Save = UICSaveGame::DecodeFile(Bytes);
            if (ValidateSave(Save, Slot)) { UE_LOG(LogICSave, Verbose, TEXT("[Save.BackupAvailable] Slot=%d File=%s"),Slot,*Candidate);bUsedBackup = true; return Save; }
        }
    }
    return nullptr;
}

bool UICSaveSubsystem::PrepareSave(int32 Slot, TArray<uint8>& Bytes, double& CapturedPlaySecond, bool& bRotateBackup)
{
    IC_TRACE_FUNCTION();
    if (GetSlotPath(Slot).IsEmpty()) { SetLastMessage(ICText::Get(TEXT("Save_InvalidSlot"))); return false; }
    SetLastMessage(GetSaveBlockReasonText(Slot == 0)); if (!LastMessage.IsEmpty()) return false;
    TArray<uint8> ExistingBytes;
    if (ReadBytes(GetSlotPath(Slot), ExistingBytes))
    {
        const auto* Existing = UICSaveGame::DecodeFile(ExistingBytes);
        if (UICSaveGame::IsFutureFile(ExistingBytes) || (Existing && Existing->SaveVersion > UICSaveGame::CurrentVersion))
        { SetLastMessage(ICText::Get(TEXT("Save_FutureVersion"))); return false; }
    }
    if (!WorldState->FlushParticipants() || !WorldState->CanRestoreState(WorldState->ExportState()))
    { SetLastMessage(ICText::Get(TEXT("Save_StateInvalid"))); return false; }
    auto* Save = NewObject<UICSaveGame>();
    Save->WorldState = WorldState->ExportState(); Save->Clock = Time->GetClock();
    Save->Slot = Slot; Save->SavedAt = FDateTime::UtcNow();
    Save->Preview.Location = TEXT("R01");
    Save->Preview.PartyIds.Add(UICWorldStateSubsystem::PlayerId());
    TArray<FICCharacterState> Companions;
    for (const auto& Entry : Save->WorldState.Characters)
        if (Entry.Key != UICWorldStateSubsystem::PlayerId() && Entry.Value.Mode == EICCharacterMode::Companion && Entry.Value.PartySlot >= 0)
            Companions.Add(Entry.Value);
    Companions.Sort([](const FICCharacterState& A, const FICCharacterState& B)
    { IC_TRACE_FUNCTION(); return A.PartySlot == B.PartySlot ? A.CharacterId.LexicalLess(B.CharacterId) : A.PartySlot < B.PartySlot; });
    for (const FICCharacterState& Companion : Companions)
    { if (Save->Preview.PartyIds.Num() >= 5) break; Save->Preview.PartyIds.AddUnique(Companion.CharacterId); }
    if (!ValidateSave(Save, Slot) || !UICSaveGame::EncodeForFile(Save, Bytes))
    { SetLastMessage(ICText::Get(TEXT("Save_EncodeFailed"))); return false; }
    bool bUnused = false;
    bRotateBackup = ReadSlot(Slot, false, bUnused) != nullptr;
    CapturedPlaySecond = Save->Clock.PlaySecond;
    return true;
}

bool UICSaveSubsystem::SaveSlot(int32 Slot)
{
    IC_TRACE_FUNCTION();
    TArray<uint8> Bytes; double Captured = 0; bool bRotate = false;
    if (!PrepareSave(Slot, Bytes, Captured, bRotate)){UE_LOG(LogICSave, Log, TEXT("[Save.Request.Rejected] Slot=%d Reason=%s"),Slot,*LastMessage);return false;}
    const uint64 Generation = AutoRequestGeneration;
    UE_LOG(LogICSave, Log, TEXT("[Save.Started] Slot=%d Mode=Async File=%s Bytes=%d PlaySecond=%.2f Generation=%llu"),Slot,*GetSlotPath(Slot),Bytes.Num(),Captured,Generation);
    bSaving = true; SetLastMessage(ICText::Get(TEXT("Save_InProgress"))); OnSaveStarted.Broadcast(Slot);
    const FString Path = GetSlotPath(Slot);
    const TWeakObjectPtr<UICSaveSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, Path, Bytes = MoveTemp(Bytes), Slot, Captured, Generation, bRotate]()
    {
        IC_TRACE_FUNCTION();
        const FWriteResult Result = WriteSlotFile(Path, Bytes, Slot == 0, bRotate);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Slot, Captured, Generation, Result]()
        {
            IC_TRACE_FUNCTION();
            if (WeakThis.IsValid()) WeakThis->FinishSave(Slot, Result.bSuccess, Captured, Generation, Result.MessageKey);
            else UE_LOG(LogICSave, Verbose, TEXT("[Save.CallbackIgnored] Slot=%d Success=%d: 서비스 제거 후 콜백"),Slot,Result.bSuccess);
        });
    });
    return true;
}

bool UICSaveSubsystem::SaveSlotSynchronously(int32 Slot)
{
    IC_TRACE_FUNCTION();
    TArray<uint8> Bytes; double Captured = 0; bool bRotate = false;
    if (!PrepareSave(Slot, Bytes, Captured, bRotate)){UE_LOG(LogICSave, Log, TEXT("[Save.Request.Rejected] Slot=%d Reason=%s"),Slot,*LastMessage);return false;}
    const uint64 Generation = AutoRequestGeneration;
    UE_LOG(LogICSave, Log, TEXT("[Save.Started] Slot=%d Mode=Sync File=%s Bytes=%d PlaySecond=%.2f Generation=%llu"),Slot,*GetSlotPath(Slot),Bytes.Num(),Captured,Generation);
    bSaving = true; SetLastMessage(ICText::Get(TEXT("Save_InProgress"))); OnSaveStarted.Broadcast(Slot);
    const FWriteResult Result = WriteSlotFile(GetSlotPath(Slot), Bytes, Slot == 0, bRotate);
    FinishSave(Slot, Result.bSuccess, Captured, Generation, Result.MessageKey);
    return Result.bSuccess;
}

void UICSaveSubsystem::FinishSave(int32 Slot, bool bSuccess, double CapturedPlaySecond, uint64 CapturedGeneration, const FString& Message)
{
    IC_TRACE_FUNCTION();
    bSaving = false; SetLastMessage(ICText::Get(*Message));
    if(bSuccess)
    {
        UE_LOG(LogICSave, Log, TEXT("[Save.Completed] Slot=%d PlaySecond=%.2f Generation=%llu"),Slot,CapturedPlaySecond,CapturedGeneration);
    }
    else
    {
        UE_LOG(LogICSave, Warning, TEXT("[Save.Failed] Slot=%d File=%s Reason=%s"),Slot,*GetSlotPath(Slot),*Message);
    }
    if (bSuccess)
    {
        LastSavePlaySecond = CapturedPlaySecond;
        RetryAfterPlaySecond = 0;
        if (Slot == 0 && CapturedGeneration == AutoRequestGeneration) { bAutoSavePending = false; PendingReason = NAME_None; }
    }
    else if (Time) RetryAfterPlaySecond = Time->GetClock().PlaySecond + 30;
    OnSaveFinished.Broadcast(Slot, bSuccess, LastMessage);
}

bool UICSaveSubsystem::LoadSlot(int32 Slot)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICSave, Verbose, TEXT("[Save.Load.Request] Slot=%d File=%s"),Slot,*GetSlotPath(Slot));
    if (bSaving || !WorldState || !Time || Safety.bEffectPending) { SetLastMessage(ICText::Get(TEXT("Save_LoadBusy")));UE_LOG(LogICSave, Log, TEXT("[Save.Load.Rejected] Slot=%d Saving=%d EffectPending=%d Reason=%s"),Slot,bSaving,Safety.bEffectPending,*LastMessage); return false; }
    bool bBackup = false;
    auto* Save = ReadSlot(Slot, true, bBackup);
    if (!Save) { SetLastMessage(ICText::Get(TEXT("Save_LoadInvalid")));UE_LOG(LogICSave, Log, TEXT("[Save.Load.Rejected] Slot=%d File=%s Reason=%s"),Slot,*GetSlotPath(Slot),*LastMessage); return false; }

    if (!Save->Clock.IsValid() || !WorldState->CanRestoreState(Save->WorldState)) return false;
    const auto PreviousClock = Time->GetClock();
    Time->RestoreClock(Save->Clock);
    if (!WorldState->RestoreState(Save->WorldState)) { UE_LOG(LogICSave, Warning, TEXT("[Save.Load.Rollback] Slot=%d: 상태 복원 실패, 이전 시간 복원"),Slot);Time->RestoreClock(PreviousClock); return false; }
    PreviewLocation = Save->Preview.Location == TEXT("지역 1") ? TEXT("R01") : Save->Preview.Location;
    ResetSession();
    SetLastMessage(ICText::Get(bBackup ? TEXT("Save_LoadedBackup") : TEXT("Save_Loaded")));
    UE_LOG(LogICSave, Log, TEXT("[Save.Loaded] Slot=%d Backup=%d Version=%d GameMinute=%.2f PlaySecond=%.2f"),Slot,bBackup,Save->SaveVersion,Save->Clock.GameMinute,Save->Clock.PlaySecond);
    return true;
}

TArray<FICSaveSlotInfo> UICSaveSubsystem::GetSlotList() const
{
    IC_TRACE_FUNCTION();
    TArray<FICSaveSlotInfo> Result;
    for (int32 Slot = 0; Slot <= 4; ++Slot)
    {
        FICSaveSlotInfo Info; Info.Slot = Slot;
        for (const FString& Suffix : {FString(), FString(TEXT(".bak")), FString(TEXT(".tmp")), FString(TEXT(".rollback")), FString(TEXT(".bak.tmp"))})
            Info.bExists |= IFileManager::Get().FileExists(*(GetSlotPath(Slot) + Suffix));
        bool bBackup = false;
        if (const auto* Save = ReadSlot(Slot, true, bBackup))
        {
            Info.bExists = true;
            Info.bValid = true;
            Info.bUsesBackup = bBackup;
            Info.PlayerName = Save->WorldState.PlayerName;
            Info.SavedAt = Save->SavedAt;
            Info.Preset = Save->WorldState.Preset;
            Info.Preview = Save->Preview;
            if (Info.Preview.Location == TEXT("지역 1")) Info.Preview.Location = TEXT("R01");
            Info.Clock = Save->Clock;
        }
        Result.Add(Info);
    }
    return Result;
}

void UICSaveSubsystem::RequestAutoSave(FName Reason)
{
    IC_TRACE_FUNCTION();
    ++AutoRequestGeneration;
    bAutoSavePending = true; PendingReason = Reason;
    UE_LOG(LogICSave, Verbose, TEXT("[Save.Auto.Request] Reason=%s Generation=%llu"),*Reason.ToString(),AutoRequestGeneration);
    PollAutoSave();
}

void UICSaveSubsystem::PollAutoSave()
{
    IC_TRACE_FUNCTION();
    if (!Time || !WorldState || !WorldState->ExportState().bPlayerCreated) return;
    const double Now = Time->GetClock().PlaySecond;
    if (FMath::IsFinite(AutoSaveInterval) && AutoSaveInterval >= 1 && Now - LastSavePlaySecond >= AutoSaveInterval)
    { bAutoSavePending = true; if (PendingReason.IsNone()) PendingReason = TEXT("Interval"); }
    if (!bAutoSaveSuspended && bAutoSavePending && Now >= RetryAfterPlaySecond && CanSave(true)) SaveSlot(0);
}

void UICSaveSubsystem::ResetSession()
{
    IC_TRACE_FUNCTION();
    LastSavePlaySecond = Time ? Time->GetClock().PlaySecond : 0;
    RetryAfterPlaySecond = 0; bAutoSavePending = false; PendingReason = NAME_None;
}

bool UICSaveSubsystem::ConvertLegacySave(const UICLegacyPlayerSaveGame* Legacy, FICWorldState& OutState) const
{
    IC_TRACE_FUNCTION();
    if (!Legacy || (Legacy->Version != 1 && Legacy->Version != 8)) return false;
    FICWorldState Candidate;
    Candidate.bPlayerCreated = true; Candidate.PlayerName = Legacy->PlayerName;
    Candidate.Preset = Legacy->Preset; Candidate.Job = Legacy->Job; Candidate.FirstSkill = Legacy->FirstSkill;
    Candidate.PlayerPosition = Legacy->Position; Candidate.PlayerRotation = Legacy->Rotation;
    Candidate.Inventory.Bag = Legacy->Bag; Candidate.Inventory.QuestItems = Legacy->QuestItems;
    Candidate.Inventory.Equipment = Legacy->Equipment; Candidate.Inventory.QuickSlots = Legacy->QuickSlots; Candidate.Inventory.Storage = Legacy->Storage;
    FICCharacterState Player; Player.CharacterId = UICWorldStateSubsystem::PlayerId(); Player.Kind = EICCharacterKind::Player;
    if(Legacy->Version==8)
    {
        if(!Legacy->Survival.IsValid()||!Legacy->MainStats.IsValid()||Legacy->Gold<0||Legacy->GameMinute<0||Legacy->GameMinute>=1440||!FMath::IsFinite(Legacy->PlaySeconds)||Legacy->PlaySeconds<0.f)return false;
        ICPlayerStateConversions::Apply(Legacy->MainStats,Legacy->Survival,Player);Candidate.Gold=Legacy->Gold;
    }
    else {Player.HP = Legacy->HP; Player.SP = Legacy->SP; Player.MainStats.STR = Legacy->Strength;}
    Candidate.Characters.Add(Player.CharacterId, Player);
    ICSaveDataMigration::NormalizeInventoryIDs(Candidate);
    if (!WorldState || !WorldState->CanRestoreState(Candidate)) return false;
    OutState = MoveTemp(Candidate); return true;
}

bool UICSaveSubsystem::ImportLegacySave()
{
    IC_TRACE_FUNCTION();
    if (!WorldState || !Time || bSaving || Safety.bEffectPending || GetSlotList()[1].bExists)
    { SetLastMessage(ICText::Get(TEXT("Save_LegacySlotBusy"))); return false; }
    const FString LegacyName = Prefix.StartsWith(TEXT("IC_Test_")) ? Prefix + TEXT("_LegacyV1") : TEXT("PlayerPrototype_Auto");
    const auto* Legacy = Cast<UICLegacyPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(LegacyName, 0));
    FICWorldState Candidate;
    if (!ConvertLegacySave(Legacy, Candidate)) { SetLastMessage(ICText::Get(TEXT("Save_LegacyInvalid")));UE_LOG(LogICSave, Log, TEXT("[Save.Legacy.ImportRejected] File=%s Reason=%s"),*GetLegacySavePath(),*LastMessage); return false; }
    auto* Save = NewObject<UICSaveGame>(); Save->WorldState = Candidate; Save->Clock = FICClockState(); Save->Slot = 1; Save->SavedAt = FDateTime::UtcNow();
    if(Legacy->Version==8){Save->Clock.GameMinute=Legacy->GameMinute;Save->Clock.PlaySecond=Legacy->PlaySeconds;}
    if (auto* Player = Save->WorldState.Characters.Find(UICWorldStateSubsystem::PlayerId()))
    {
        Player->LastSimGameMinute = FMath::Min(Player->LastSimGameMinute, Save->Clock.GameMinute);
        Player->LastSimPlaySecond = Save->Clock.PlaySecond;
        Player->Position = Save->WorldState.PlayerPosition; Player->Rotation = Save->WorldState.PlayerRotation;
    }
    TArray<uint8> Bytes;
    if (!UICSaveGame::EncodeForFile(Save, Bytes)) return false;
    const FWriteResult Result = WriteSlotFile(GetSlotPath(1), Bytes, false, false);
    if (!Result.bSuccess) { SetLastMessage(ICText::Get(*Result.MessageKey));UE_LOG(LogICSave, Warning, TEXT("[Save.Legacy.WriteFailed] %s"),*LastMessage); return false; }
    UE_LOG(LogICSave, Log, TEXT("[Save.Legacy.Imported] SourceVersion=%d TargetSlot=1 TargetVersion=%d"),Legacy->Version,UICSaveGame::CurrentVersion);
    return LoadSlot(1);
}

void UICSaveSubsystem::SetAutoSaveSuspended(bool bSuspended)
{
    IC_TRACE_FUNCTION();
    if(bAutoSaveSuspended!=bSuspended)UE_LOG(LogICSave, Verbose, TEXT("[Save.Auto.Suspended] Before=%d After=%d Pending=%d"),bAutoSaveSuspended,bSuspended,bAutoSavePending);
    bAutoSaveSuspended=bSuspended;
}
void UICSaveSubsystem::SetSafetyState(const FICSaveSafety& NewSafety)
{
    IC_TRACE_FUNCTION();
    if(Safety.bDead!=NewSafety.bDead || Safety.bInCombat!=NewSafety.bInCombat
        || Safety.bInDialogue!=NewSafety.bInDialogue || Safety.bInEvent!=NewSafety.bInEvent
        || Safety.bEffectPending!=NewSafety.bEffectPending)
        UE_LOG(LogICSave, Verbose, TEXT("[Save.Safety] Dead=%d Combat=%d Dialogue=%d Event=%d EffectPending=%d"),
            NewSafety.bDead,NewSafety.bInCombat,NewSafety.bInDialogue,NewSafety.bInEvent,NewSafety.bEffectPending);
    Safety=NewSafety;
}
