#include "Save/ICSaveGame.h"
#include "Debug/ICLog.h"
#include "Save/Compatibility/ICSaveDataMigration.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "World/State/ICWorldStateSubsystem.h"

UICSaveGame::UICSaveGame() { IC_TRACE_FUNCTION(); SaveVersion = CurrentVersion; }

namespace
{
constexpr uint32 FileMagic = 0x56534349; // ICSV
constexpr int32 ICSaveEnvelopeBytes = 16;
constexpr uint32 MaxPayload = 16 * 1024 * 1024 - ICSaveEnvelopeBytes;
}

bool UICSaveGame::EncodeForFile(UICSaveGame* Save, TArray<uint8>& Bytes)
{
    IC_TRACE_FUNCTION();
    TArray<uint8> Payload;
    if (!Save || Save->SaveVersion != CurrentVersion || !UICWorldStateSubsystem::ValidateState(Save->WorldState)
        || !UICWorldStateSubsystem::ValidateStateClock(Save->WorldState, Save->Clock)
        || !UGameplayStatics::SaveGameToMemory(Save, Payload) || Payload.Num() <= 0 || Payload.Num() > MaxPayload){UE_LOG(LogICSave, Verbose, TEXT("[Save.Encode.Rejected] Version=%d PayloadBytes=%d: 상태/시간/직렬화/크기 검사 실패"),Save?Save->SaveVersion:-1,Payload.Num());return false;}
    Bytes.Reset(); FMemoryWriter Writer(Bytes);
    uint32 Magic = FileMagic, Format = CurrentVersion, Size = Payload.Num(), CRC = FCrc::MemCrc32(Payload.GetData(), Size);
    Writer << Magic << Format << Size << CRC;
    Writer.Serialize(Payload.GetData(), Size);
    UE_LOG(LogICSave, Verbose, TEXT("[Save.Encoded] Version=%d Bytes=%d WriterError=%d"),Save->SaveVersion,Bytes.Num(),Writer.IsError());
    return !Writer.IsError();
}

UICSaveGame* UICSaveGame::DecodeFile(const TArray<uint8>& Bytes)
{
    IC_TRACE_FUNCTION();
    if (Bytes.Num() < ICSaveEnvelopeBytes){UE_LOG(LogICSave, VeryVerbose, TEXT("[Save.Decode.Rejected] Bytes=%d: 헤더 길이 부족"),Bytes.Num());return nullptr;}
    FMemoryReader Reader(Bytes); uint32 Magic = 0, Format = 0, Size = 0, CRC = 0;
    Reader << Magic << Format << Size << CRC;
    if (Reader.IsError() || Magic != FileMagic || (Format != 2 && Format != CurrentVersion) || !Size || Size > MaxPayload
        || Size != static_cast<uint32>(Bytes.Num() - ICSaveEnvelopeBytes) || CRC != FCrc::MemCrc32(Bytes.GetData() + ICSaveEnvelopeBytes, Size)){UE_LOG(LogICSave, VeryVerbose, TEXT("[Save.Decode.Rejected] Bytes=%d Magic=%08x Format=%u PayloadSize=%u: 헤더/버전/길이/CRC 검사 실패"),Bytes.Num(),Magic,Format,Size);return nullptr;}
    TArray<uint8> Payload; Payload.Append(Bytes.GetData() + ICSaveEnvelopeBytes, Size);
    UICSaveGame* Save = Cast<UICSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    if (!Save){UE_LOG(LogICSave, VeryVerbose, TEXT("[Save.Decode.Rejected] Format=%u: SaveGame 자료형 복원 실패"),Format);return nullptr;}
    // Tagged serialization may omit a property equal to its CDO default. A v2
    // payload can therefore inherit today's constructor version (3) on load.
    // The envelope is the schema authority; reject any unrelated payload version.
    if (Format == 2)
    {
        if (Save->SaveVersion != 2 && Save->SaveVersion != CurrentVersion) return nullptr;
        Save->SaveVersion = 2;
        if (!Save->UpgradeFromVersion2()) return nullptr;
    }
    else if (Save->SaveVersion != CurrentVersion) return nullptr;
    ICSaveDataMigration::NormalizeInventoryIDs(Save->WorldState);
    const bool bValid=UICWorldStateSubsystem::ValidateState(Save->WorldState) && UICWorldStateSubsystem::ValidateStateClock(Save->WorldState, Save->Clock);
    UE_LOG(LogICSave, VeryVerbose, TEXT("[Save.Decoded] EnvelopeVersion=%u CurrentVersion=%d Bytes=%d StateValid=%d"),Format,Save->SaveVersion,Bytes.Num(),bValid);
    return bValid?Save:nullptr;
}

bool UICSaveGame::UpgradeFromVersion2()
{
    IC_TRACE_FUNCTION();
    if (SaveVersion != 2 || !Clock.IsValid()) return false;
    for (auto& Pair : WorldState.Characters)
    {
        auto& C = Pair.Value;
        if (C.Mode == EICCharacterMode::Downed)
        {
            C.Mode = EICCharacterMode::Companion;
            C.LifeState = EICCharacterLifeState::Downed;
            C.HP = 0;
        }
        C.LastSimGameMinute = FMath::Min(C.LastSimGameMinute, Clock.GameMinute);
        C.LastSimPlaySecond = Clock.PlaySecond;
        C.Combat.LastCapturedPlaySecond = Clock.PlaySecond;
        if (C.Kind == EICCharacterKind::Player) { C.Position = WorldState.PlayerPosition; C.Rotation = WorldState.PlayerRotation; }
        for (const auto& Debuff : C.Debuffs)
        {
            if (Debuff.ID.IsNone() || !FMath::IsFinite(Debuff.RemainingSeconds) || Debuff.RemainingSeconds < 0) return false;
            if (Debuff.RemainingSeconds == 0) continue;
            FICActiveEffectState Effect;
            Effect.InstanceID = FGuid::NewGuid(); Effect.EffectID = Debuff.ID; Effect.bHarmful = true;
            Effect.ExpiresAtPlaySecond = Clock.PlaySecond + Debuff.RemainingSeconds;
            C.Combat.Effects.Add(Effect);
        }
        C.Debuffs.Reset();
    }
    for (auto& Pair : WorldState.Quests)
    {
        auto& Quest = Pair.Value;
        if (Quest.bCompleted && !Quest.bRewardClaimed)
        {
            // v2 completed quests are treated as already paid; migration never grants rewards.
            Quest.bRewardClaimed = true; Quest.CompletionCount = 1;
            Quest.RewardTransactionID = FGuid::NewGuid();
            WorldState.CompletedTransactionIDs.Add(Quest.RewardTransactionID);
        }
    }
    SaveVersion = CurrentVersion;
    UE_LOG(LogICSave, Log, TEXT("[Save.Upgraded] FromVersion=2 ToVersion=%d Characters=%d Quests=%d"),SaveVersion,WorldState.Characters.Num(),WorldState.Quests.Num());
    return UICWorldStateSubsystem::ValidateState(WorldState) && UICWorldStateSubsystem::ValidateStateClock(WorldState, Clock);
}

bool UICSaveGame::IsFutureFile(const TArray<uint8>& Bytes)
{
    IC_TRACE_FUNCTION();
    if (Bytes.Num() < ICSaveEnvelopeBytes) return false;
    FMemoryReader Reader(Bytes); uint32 Magic = 0, Format = 0; Reader << Magic << Format;
    return Magic == FileMagic && Format > CurrentVersion;
}
