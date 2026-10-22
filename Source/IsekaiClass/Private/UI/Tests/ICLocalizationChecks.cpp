#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Font.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "UI/Framework/ICText.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLocalizationAssetsCheck,"IC.UI.LocalizationAssets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)

bool FICLocalizationAssetsCheck::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI.ST_IC_UI"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/IsekaiClass/UI/Fonts/FONT_IC_UI.FONT_IC_UI"));
    if (!TestNotNull(TEXT("UI String Table"),Table) || !TestNotNull(TEXT("UI Composite Font"),Font)) return false;

    for (const TCHAR* Key : {TEXT("Continue_NoSave"),TEXT("Save_InvalidSlot"),TEXT("Save_Region1"),TEXT("Interaction_NoBox")})
    {
        FString Source;
        TestTrue(FString::Printf(TEXT("String Table key %s"),Key),Table->GetStringTable()->GetSourceString(FTextKey(Key),Source));
        TestFalse(FString::Printf(TEXT("Source text for %s"),Key),Source.IsEmpty());
    }
    TestTrue(TEXT("Formatted save notice resolves"),!ICText::FormatText(TEXT("Save_ManualResult"),1,ICText::Get(TEXT("Save_Complete"))).IsEmpty());

    const auto& Composite=Font->GetInternalCompositeFont();
    TestTrue(TEXT("CJK fallback face is packaged"),Composite.FallbackTypeface.Typeface.Fonts.Num()>0);
    TestTrue(TEXT("Japanese culture face exists"),Composite.SubTypefaces.ContainsByPredicate([](const FCompositeSubFont& Face)
    { return Face.Cultures == TEXT("ja") && Face.Typeface.Fonts.Num()>0; }));
    return true;
}
#endif
