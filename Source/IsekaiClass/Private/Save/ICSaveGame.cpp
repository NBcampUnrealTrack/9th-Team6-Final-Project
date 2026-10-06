#include "Save/ICSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

UICSaveGame::UICSaveGame() { SaveVersion = CurrentVersion; }

namespace
{
constexpr uint32 FileMagic = 0x56534349; // ICSV
constexpr int32 ICSaveEnvelopeBytes = 16;
constexpr uint32 MaxPayload = 16 * 1024 * 1024 - ICSaveEnvelopeBytes;
}

bool UICSaveGame::EncodeForFile(UICSaveGame* Save, TArray<uint8>& Bytes)
{
    TArray<uint8> Payload;
    if (!Save || !UGameplayStatics::SaveGameToMemory(Save, Payload) || Payload.Num() <= 0 || Payload.Num() > MaxPayload) return false;
    Bytes.Reset(); FMemoryWriter Writer(Bytes);
    uint32 Magic = FileMagic, Format = CurrentVersion, Size = Payload.Num(), CRC = FCrc::MemCrc32(Payload.GetData(), Size);
    Writer << Magic << Format << Size << CRC;
    Writer.Serialize(Payload.GetData(), Size);
    return !Writer.IsError();
}

UICSaveGame* UICSaveGame::DecodeFile(const TArray<uint8>& Bytes)
{
    if (Bytes.Num() < ICSaveEnvelopeBytes) return nullptr;
    FMemoryReader Reader(Bytes); uint32 Magic = 0, Format = 0, Size = 0, CRC = 0;
    Reader << Magic << Format << Size << CRC;
    if (Reader.IsError() || Magic != FileMagic || Format != CurrentVersion || !Size || Size > MaxPayload
        || Size != static_cast<uint32>(Bytes.Num() - ICSaveEnvelopeBytes) || CRC != FCrc::MemCrc32(Bytes.GetData() + ICSaveEnvelopeBytes, Size)) return nullptr;
    TArray<uint8> Payload; Payload.Append(Bytes.GetData() + ICSaveEnvelopeBytes, Size);
    return Cast<UICSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
}

bool UICSaveGame::IsFutureFile(const TArray<uint8>& Bytes)
{
    if (Bytes.Num() < ICSaveEnvelopeBytes) return false;
    FMemoryReader Reader(Bytes); uint32 Magic = 0, Format = 0; Reader << Magic << Format;
    return Magic == FileMagic && Format > CurrentVersion;
}
