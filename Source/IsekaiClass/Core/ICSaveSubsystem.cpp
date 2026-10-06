#include "ICSaveSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "ImageUtils.h"
#include "ICSaveGame.h"
#include "ICWorldStateSubsystem.h"
#include "ICTimeSubsystem.h"
#include "../Player/PlayerSaveGame.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
    const int64 Size = IFileManager::Get().FileSize(*Path);
    return Size > 0 && Size <= MaximumSaveBytes && FFileHelper::LoadFileToArray(Bytes, *Path);
}

bool WriteVerified(const FString& Path, const TArray<uint8>& Bytes)
{
    TArray<uint8> ReadBack;
    return FFileHelper::SaveArrayToFile(Bytes, *Path) && ReadBytes(Path, ReadBack) && ReadBack == Bytes;
}

struct FWriteResult { bool bSuccess = false; FString Message; };

FWriteResult WriteSlotFile(const FString& Path, const TArray<uint8>& Bytes, bool bAuto, bool bRotateBackup)
{
    auto& Files = IFileManager::Get();
    const FString Temporary = Path + TEXT(".tmp");
    const FString Rollback = Path + TEXT(".rollback");
    const FString Backup = Path + TEXT(".bak");
    const FString BackupTemporary = Backup + TEXT(".tmp");
    if (!Files.MakeDirectory(*FPaths::GetPath(Path), true) || !WriteVerified(Temporary, Bytes))
        return { false, TEXT("임시 저장 파일 쓰기/검증에 실패했습니다. 기존 저장을 유지합니다.") };

    const bool bExisting = Files.FileExists(*Path);
    TArray<uint8> Original;
    if (bExisting)
    {
        if (!ReadBytes(Path, Original)) return { false, TEXT("기존 저장을 보존할 수 없어 교체하지 않았습니다.") };
        if ((bRotateBackup || !Files.FileExists(*Rollback)) && !WriteVerified(Rollback, Original))
            return { false, TEXT("복구용 파일 생성에 실패했습니다. 기존 저장을 유지합니다.") };
        if (bAuto && bRotateBackup)
        {
            if (!WriteVerified(BackupTemporary, Original) || !Files.Move(*Backup, *BackupTemporary, true, false, false, true))
                return { false, TEXT("오토세이브 백업 갱신에 실패했습니다. 기존 저장을 유지합니다.") };
        }
    }
    if (!Files.Move(*Path, *Temporary, true, false, false, true))
    {
        if (bExisting) WriteVerified(Path, Original);
        return { false, TEXT("저장 파일 교체에 실패했습니다. 백업/복구 파일을 보존했습니다.") };
    }
    TArray<uint8> Committed;
    if (!ReadBytes(Path, Committed) || Committed != Bytes)
    {
        if (bExisting) WriteVerified(Path, Original);
        return { false, TEXT("교체한 저장 검증에 실패했습니다. 이전 상태로 복구했습니다.") };
    }
    if (bRotateBackup || !bExisting) Files.Delete(*Rollback, false, false, true);
    return { true, TEXT("저장 완료") };
}
}

void UICSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldState = Collection.InitializeDependency<UICWorldStateSubsystem>();
    Time = Collection.InitializeDependency<UICTimeSubsystem>();
    if (FParse::Param(FCommandLine::Get(), TEXT("PlayerPrototypeSmoke"))) Prefix = TEXT("IC_Test_Smoke");
}

bool UICSaveSubsystem::ConfigureServices(UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime, const FString& InPrefix)
{
    if (bSaving || !InState || !InTime || InPrefix.IsEmpty() || InPrefix.Len() > 64) return false;
    for (TCHAR C : InPrefix) if (!FChar::IsAlnum(C) && C != TEXT('_')) return false;
    WorldState = InState; Time = InTime; Prefix = InPrefix; ResetSession();
    return true;
}

bool UICSaveSubsystem::IsTickable() const { return !IsTemplate() && GetWorld() && GetWorld()->IsGameWorld(); }
TStatId UICSaveSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UICSaveSubsystem, STATGROUP_Tickables); }
void UICSaveSubsystem::Tick(float DeltaTime) { PollAutoSave(); }

FString UICSaveSubsystem::GetSlotPath(int32 Slot) const
{
    if (Slot < 0 || Slot > 4) return FString();
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Prefix + (Slot == 0 ? TEXT("_Auto") : FString::Printf(TEXT("_Manual%d"), Slot)) + TEXT(".sav"));
}

FString UICSaveSubsystem::GetSaveBlockReason(bool bAutoSave) const
{
    if (!WorldState || !Time) return TEXT("저장 시스템이 준비되지 않았습니다.");
    if (bSaving) return TEXT("다른 저장을 처리 중입니다.");
    if (!WorldState->ExportState().bPlayerCreated) return TEXT("캐릭터를 먼저 생성하세요.");
    if (Safety.bDead) return TEXT("사망 상태에서는 저장할 수 없습니다.");
    if (Safety.bInCombat) return TEXT("전투 중에는 저장할 수 없습니다.");
    if (Safety.bInDialogue) return TEXT("대화 중에는 저장할 수 없습니다.");
    if (Safety.bInEvent) return TEXT("이벤트 중에는 저장할 수 없습니다.");
    if (Safety.bEffectPending) return TEXT("아이템 효과 처리 완료 후 저장하세요.");
    const auto Player = WorldState->GetSnapshot(UICWorldStateSubsystem::PlayerId());
    if (Player.CharacterId.IsNone() || Player.HP <= 0 || Player.MaxHP <= 0) return TEXT("플레이어 생존 상태를 확인할 수 없습니다.");
    if (bAutoSave)
    {
        if (Player.HP / Player.MaxHP <= FMath::Clamp(AutoSaveHealthThreshold, 0.f, 1.f)) return TEXT("체력이 낮아 자동 저장을 연기합니다.");
        for (const auto& D : Player.Debuffs) if (D.RemainingSeconds > 0) return TEXT("디버프가 있어 자동 저장을 연기합니다.");
    }
    return FString();
}
bool UICSaveSubsystem::CanSave(bool bAutoSave) const { return GetSaveBlockReason(bAutoSave).IsEmpty(); }

FString UICSaveSubsystem::GetLegacySavePath() const
{
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / ((Prefix.StartsWith(TEXT("IC_Test_")) ? Prefix + TEXT("_LegacyV1") : TEXT("PlayerPrototype_Auto")) + TEXT(".sav"));
}
bool UICSaveSubsystem::HasLegacySave() const { return IFileManager::Get().FileExists(*GetLegacySavePath()); }

bool UICSaveSubsystem::DeleteSlot(int32 Slot)
{
    if (bSaving) { LastMessage = TEXT("저장 처리 중에는 삭제할 수 없습니다."); return false; }
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty()) { LastMessage = TEXT("잘못된 저장 슬롯입니다."); return false; }
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
    if (!Failed.IsEmpty()) { LastMessage = TEXT("삭제하지 못한 파일: ") + FString::Join(Failed, TEXT(", ")); return false; }
    if (Slot == 0) ResetSession();
    LastMessage = Slot == 0 ? TEXT("자동 저장과 복구 파일을 삭제했습니다. 이후 자동 저장 조건을 만족하면 새로 생성됩니다.") : FString::Printf(TEXT("수동 슬롯 %d와 복구 파일을 삭제했습니다."), Slot);
    return true;
}
bool UICSaveSubsystem::DeleteLegacySave()
{
    if (bSaving) { LastMessage = TEXT("저장 처리 중에는 삭제할 수 없습니다."); return false; }
    const FString Path = GetLegacySavePath();
    IFileManager::Get().Delete(*Path, false, false, true);
    if (IFileManager::Get().FileExists(*Path)) { LastMessage = TEXT("기존 Version 1 원본 삭제에 실패했습니다."); return false; }
    LastMessage = TEXT("기존 Version 1 원본을 삭제했습니다. 가져온 수동 저장은 유지됩니다."); return true;
}

bool UICSaveSubsystem::ValidateSave(const UICSaveGame* Save, int32 Slot) const
{
    return Save && Save->SaveVersion == UICSaveGame::CurrentVersion && Save->Slot == Slot && Save->Clock.IsValid()
        && Save->WorldState.bPlayerCreated && WorldState && WorldState->CanRestoreState(Save->WorldState);
}

UICSaveGame* UICSaveSubsystem::ReadSlot(int32 Slot, bool bAllowBackup, bool& bUsedBackup) const
{
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
            if (ValidateSave(Save, Slot)) { bUsedBackup = true; return Save; }
        }
    }
    return nullptr;
}

bool UICSaveSubsystem::PrepareSave(int32 Slot, TArray<uint8>& Bytes, double& CapturedPlaySecond, bool& bRotateBackup)
{
    if (GetSlotPath(Slot).IsEmpty()) { LastMessage = TEXT("잘못된 저장 슬롯입니다."); return false; }
    LastMessage = GetSaveBlockReason(Slot == 0); if (!LastMessage.IsEmpty()) return false;
    TArray<uint8> ExistingBytes;
    if (ReadBytes(GetSlotPath(Slot), ExistingBytes))
    {
        const auto* Existing = UICSaveGame::DecodeFile(ExistingBytes);
        if (UICSaveGame::IsFutureFile(ExistingBytes) || (Existing && Existing->SaveVersion > UICSaveGame::CurrentVersion))
        { LastMessage = TEXT("더 높은 버전의 저장 파일을 덮어쓰지 않습니다."); return false; }
    }
    if (!WorldState->FlushParticipants() || !WorldState->CanRestoreState(WorldState->ExportState()))
    { LastMessage = TEXT("저장 전 상태 검증/반영에 실패했습니다."); return false; }
    auto* Save = NewObject<UICSaveGame>();
    Save->WorldState = WorldState->ExportState(); Save->Clock = Time->GetClock();
    Save->Slot = Slot; Save->SavedAt = FDateTime::UtcNow();
    Save->Preview.Location = PreviewLocation.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(this, true) : PreviewLocation;
    Save->Preview.PartyIds.Add(UICWorldStateSubsystem::PlayerId());
    TArray<FICCharacterState> Companions;
    for (const auto& Entry : Save->WorldState.Characters)
        if (Entry.Key != UICWorldStateSubsystem::PlayerId() && Entry.Value.Mode == EICCharacterMode::Companion && Entry.Value.PartySlot >= 0)
            Companions.Add(Entry.Value);
    Companions.Sort([](const FICCharacterState& A, const FICCharacterState& B)
    { return A.PartySlot == B.PartySlot ? A.CharacterId.LexicalLess(B.CharacterId) : A.PartySlot < B.PartySlot; });
    for (const FICCharacterState& Companion : Companions)
    { if (Save->Preview.PartyIds.Num() >= 5) break; Save->Preview.PartyIds.AddUnique(Companion.CharacterId); }
    if (bCaptureSaveThumbnail) CaptureThumbnail(Save->Preview.ThumbnailPNG);
    if (!ValidateSave(Save, Slot) || !UICSaveGame::EncodeForFile(Save, Bytes))
    { LastMessage = TEXT("저장 직렬화에 실패했습니다."); return false; }
    bool bUnused = false;
    bRotateBackup = ReadSlot(Slot, false, bUnused) != nullptr;
    CapturedPlaySecond = Save->Clock.PlaySecond;
    return true;
}

bool UICSaveSubsystem::SaveSlot(int32 Slot)
{
    TArray<uint8> Bytes; double Captured = 0; bool bRotate = false;
    if (!PrepareSave(Slot, Bytes, Captured, bRotate)) return false;
    const uint64 Generation = AutoRequestGeneration;
    bSaving = true; LastMessage = TEXT("저장 중..."); OnSaveStarted.Broadcast(Slot);
    const FString Path = GetSlotPath(Slot);
    const TWeakObjectPtr<UICSaveSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, Path, Bytes = MoveTemp(Bytes), Slot, Captured, Generation, bRotate]()
    {
        const FWriteResult Result = WriteSlotFile(Path, Bytes, Slot == 0, bRotate);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Slot, Captured, Generation, Result]()
        {
            if (WeakThis.IsValid()) WeakThis->FinishSave(Slot, Result.bSuccess, Captured, Generation, Result.Message);
        });
    });
    return true;
}

bool UICSaveSubsystem::SaveSlotSynchronously(int32 Slot)
{
    TArray<uint8> Bytes; double Captured = 0; bool bRotate = false;
    if (!PrepareSave(Slot, Bytes, Captured, bRotate)) return false;
    const uint64 Generation = AutoRequestGeneration;
    bSaving = true; OnSaveStarted.Broadcast(Slot);
    const FWriteResult Result = WriteSlotFile(GetSlotPath(Slot), Bytes, Slot == 0, bRotate);
    FinishSave(Slot, Result.bSuccess, Captured, Generation, Result.Message);
    return Result.bSuccess;
}

void UICSaveSubsystem::FinishSave(int32 Slot, bool bSuccess, double CapturedPlaySecond, uint64 CapturedGeneration, const FString& Message)
{
    bSaving = false; LastMessage = Message;
    if (bSuccess)
    {
        LastSavePlaySecond = CapturedPlaySecond;
        RetryAfterPlaySecond = 0;
        if (Slot == 0 && CapturedGeneration == AutoRequestGeneration) { bAutoSavePending = false; PendingReason = NAME_None; }
    }
    else if (Time) RetryAfterPlaySecond = Time->GetClock().PlaySecond + 30;
    OnSaveFinished.Broadcast(Slot, bSuccess, Message);
}

bool UICSaveSubsystem::LoadSlot(int32 Slot)
{
    if (bSaving || !WorldState || !Time || Safety.bEffectPending) { LastMessage = TEXT("저장/효과 처리 중에는 불러올 수 없습니다."); return false; }
    bool bBackup = false;
    auto* Save = ReadSlot(Slot, true, bBackup);
    if (!Save) { LastMessage = TEXT("저장 파일이 없거나 손상/버전/데이터 검증에 실패했습니다."); return false; }
    // Both parts are validated before either is applied.
    if (!Save->Clock.IsValid() || !WorldState->CanRestoreState(Save->WorldState)) return false;
    const auto PreviousClock = Time->GetClock();
    Time->RestoreClock(Save->Clock); // Participants restore schedules against the loaded clock.
    if (!WorldState->RestoreState(Save->WorldState)) { Time->RestoreClock(PreviousClock); return false; }
    PreviewLocation = Save->Preview.Location;
    ResetSession();
    LastMessage = bBackup ? TEXT("백업에서 불러왔습니다. 원본 파일은 보존했습니다.") : TEXT("불러오기 완료");
    return true;
}

TArray<FICSaveSlotInfo> UICSaveSubsystem::GetSlotList() const
{
    TArray<FICSaveSlotInfo> Result;
    for (int32 Slot = 0; Slot <= 4; ++Slot)
    {
        FICSaveSlotInfo Info; Info.Slot = Slot;
        for (const FString& Suffix : {FString(), FString(TEXT(".bak")), FString(TEXT(".tmp")), FString(TEXT(".rollback")), FString(TEXT(".bak.tmp"))})
            Info.bExists |= IFileManager::Get().FileExists(*(GetSlotPath(Slot) + Suffix));
        bool bBackup = false;
        if (const auto* Save = ReadSlot(Slot, true, bBackup))
        { Info.bExists = true; Info.bValid = true; Info.bUsesBackup = bBackup; Info.PlayerName = Save->WorldState.PlayerName; Info.SavedAt = Save->SavedAt; Info.Preset = Save->WorldState.Preset; Info.Preview = Save->Preview; }
        Result.Add(Info);
    }
    return Result;
}

void UICSaveSubsystem::RequestAutoSave(FName Reason)
{
    ++AutoRequestGeneration;
    bAutoSavePending = true; PendingReason = Reason;
    PollAutoSave();
}

void UICSaveSubsystem::PollAutoSave()
{
    if (!Time || !WorldState || !WorldState->ExportState().bPlayerCreated) return;
    const double Now = Time->GetClock().PlaySecond;
    if (FMath::IsFinite(AutoSaveInterval) && AutoSaveInterval >= 1 && Now - LastSavePlaySecond >= AutoSaveInterval)
    { bAutoSavePending = true; if (PendingReason.IsNone()) PendingReason = TEXT("Interval"); }
    if (!bAutoSaveSuspended && bAutoSavePending && Now >= RetryAfterPlaySecond && CanSave(true)) SaveSlot(0);
}

void UICSaveSubsystem::ResetSession()
{
    LastSavePlaySecond = Time ? Time->GetClock().PlaySecond : 0;
    RetryAfterPlaySecond = 0; bAutoSavePending = false; PendingReason = NAME_None;
}

bool UICSaveSubsystem::ConvertLegacySave(const UPlayerPrototypeSave* Legacy, FICWorldState& OutState) const
{
    if (!Legacy || Legacy->Version != 1) return false;
    FICWorldState Candidate;
    Candidate.bPlayerCreated = true; Candidate.PlayerName = Legacy->PlayerName;
    Candidate.Preset = Legacy->Preset; Candidate.Job = Legacy->Job; Candidate.FirstSkill = Legacy->FirstSkill;
    Candidate.PlayerPosition = Legacy->Position; Candidate.PlayerRotation = Legacy->Rotation;
    Candidate.Inventory.Bag = Legacy->Bag; Candidate.Inventory.QuestItems = Legacy->QuestItems;
    Candidate.Inventory.Equipment = Legacy->Equipment; Candidate.Inventory.QuickSlots = Legacy->QuickSlots; Candidate.Inventory.Storage = Legacy->Storage;
    FICCharacterState Player; Player.CharacterId = UICWorldStateSubsystem::PlayerId(); Player.Kind = EICCharacterKind::Player;
    Player.HP = Legacy->HP; Player.SP = Legacy->SP; Player.MainStats.STR = Legacy->Strength;
    Candidate.Characters.Add(Player.CharacterId, Player);
    if (!WorldState || !WorldState->CanRestoreState(Candidate)) return false;
    OutState = MoveTemp(Candidate); return true;
}

bool UICSaveSubsystem::ImportLegacySave()
{
    if (!WorldState || !Time || bSaving || Safety.bEffectPending || GetSlotList()[1].bExists)
    { LastMessage = TEXT("수동 슬롯 1이 비어 있고 처리 중이 아닐 때만 기존 저장을 가져옵니다."); return false; }
    const FString LegacyName = Prefix.StartsWith(TEXT("IC_Test_")) ? Prefix + TEXT("_LegacyV1") : TEXT("PlayerPrototype_Auto");
    const auto* Legacy = Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromSlot(LegacyName, 0));
    FICWorldState Candidate;
    if (!ConvertLegacySave(Legacy, Candidate)) { LastMessage = TEXT("기존 Version 1 저장을 검증할 수 없습니다."); return false; }
    auto* Save = NewObject<UICSaveGame>(); Save->WorldState = Candidate; Save->Clock = FICClockState(); Save->Slot = 1; Save->SavedAt = FDateTime::UtcNow();
    TArray<uint8> Bytes;
    if (!UICSaveGame::EncodeForFile(Save, Bytes)) return false;
    const FWriteResult Result = WriteSlotFile(GetSlotPath(1), Bytes, false, false);
    if (!Result.bSuccess) { LastMessage = Result.Message; return false; }
    return LoadSlot(1);
}

bool UICSaveSubsystem::CaptureThumbnail(TArray<uint8>& OutPNG)
{
    OutPNG.Reset();
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    if (!PC || !PC->PlayerCameraManager || !World->IsGameWorld()) return false;
    FActorSpawnParameters SpawnParams; SpawnParams.ObjectFlags |= RF_Transient;
    AActor* CaptureActor = World->SpawnActor<AActor>(AActor::StaticClass(), PC->PlayerCameraManager->GetCameraLocation(), PC->PlayerCameraManager->GetCameraRotation(), SpawnParams);
    if (!CaptureActor) return false;
    UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(CaptureActor);
    Target->RenderTargetFormat = RTF_RGBA8;
    Target->InitAutoFormat(512, 288); Target->UpdateResourceImmediate(true);
    USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor);
    CaptureActor->AddInstanceComponent(Capture); CaptureActor->SetRootComponent(Capture);
    Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
    Capture->TextureTarget = Target; Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->FOVAngle = PC->PlayerCameraManager->GetFOVAngle();
    Capture->RegisterComponent();
    Capture->SetWorldLocationAndRotation(PC->PlayerCameraManager->GetCameraLocation(), PC->PlayerCameraManager->GetCameraRotation());
    Capture->CaptureScene();
    TArray<FColor> Pixels;
    FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
    const bool bRead = Resource && Resource->ReadPixels(Pixels) && Pixels.Num() == 512 * 288;
    if (bRead)
    {
        for (FColor& Pixel : Pixels) Pixel.A = 255;
        TArray64<uint8> Encoded;
        FImageUtils::PNGCompressImageArray(512, 288, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Encoded);
        if (Encoded.Num() > 0 && Encoded.Num() <= 2 * 1024 * 1024) OutPNG.Append(Encoded.GetData(), static_cast<int32>(Encoded.Num()));
    }
    CaptureActor->Destroy();
    return !OutPNG.IsEmpty();
}
