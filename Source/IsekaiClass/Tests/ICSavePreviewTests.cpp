#include "../Core/ICSaveGame.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSavePreviewRoundTrip, "IC.Save.PreviewRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICSavePreviewRoundTrip::RunTest(const FString& /*Parameters*/)
{
    TStrongObjectPtr<UICSaveGame> Save(NewObject<UICSaveGame>());
    Save->WorldState.PlayerName = TEXT("PreviewTest");
    Save->Preview.Location = TEXT("해변마을 · 선착장");
    Save->Preview.PartyIds = { FName(TEXT("CHR_PLAYER")), FName(TEXT("NPC_R01_001")) };
    // Tests byte preservation, not PNG decoding. Actual capture requires a rendered world.
    Save->Preview.ThumbnailPNG = { 137, 80, 78, 71, 0, 255, 42 };
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Encode preview"), UICSaveGame::EncodeForFile(Save.Get(), Bytes))) return false;
    TStrongObjectPtr<UICSaveGame> Loaded(UICSaveGame::DecodeFile(Bytes));
    if (!TestNotNull(TEXT("Decode preview"), Loaded.Get())) return false;
    TestEqual(TEXT("Location survives"), Loaded->Preview.Location, Save->Preview.Location);
    TestTrue(TEXT("Party order survives"), Loaded->Preview.PartyIds == Save->Preview.PartyIds);
    TestTrue(TEXT("PNG bytes survive"), Loaded->Preview.ThumbnailPNG == Save->Preview.ThumbnailPNG);
    Bytes.Last() ^= 1;
    TestNull(TEXT("Modified payload fails CRC"), UICSaveGame::DecodeFile(Bytes));
    Save->Preview = FICSavePreview();
    TestTrue(TEXT("Empty preview encodes"), UICSaveGame::EncodeForFile(Save.Get(), Bytes));
    TStrongObjectPtr<UICSaveGame> Empty(UICSaveGame::DecodeFile(Bytes));
    if (!TestNotNull(TEXT("Empty preview decodes"), Empty.Get())) return false;
    TestTrue(TEXT("Missing image remains empty"), Empty->Preview.ThumbnailPNG.IsEmpty());
    return true;
}
#endif
