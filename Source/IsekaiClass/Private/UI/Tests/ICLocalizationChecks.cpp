#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Font.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "UI/Framework/ICText.h"
#include "Data/Text/ICLocalization.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLanguageRoundTrip,"IC.Polish.LanguageRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FICLanguageRoundTrip::RunTest(const FString&)
{
    auto& Internationalization=FInternationalization::Get();
    auto& Localization=FTextLocalizationManager::Get();
    const FString PreviousLanguage=Internationalization.GetCurrentLanguage()->GetName();
    const FString PreviousLocale=Internationalization.GetCurrentLocale()->GetName();
#if WITH_EDITOR
    const FString PreviousPreviewLanguage=Localization.GetConfiguredGameLocalizationPreviewLanguage();
    const bool WasPreviewEnabled=Localization.IsGameLocalizationPreviewEnabled();
#endif
    ON_SCOPE_EXIT
    {
        Internationalization.SetCurrentLanguage(PreviousLanguage);
        Internationalization.SetCurrentLocale(PreviousLocale);
#if WITH_EDITOR
        if(GIsEditor)
        {
            Localization.ConfigureGameLocalizationPreviewLanguage(PreviousPreviewLanguage);
            if(WasPreviewEnabled)Localization.EnableGameLocalizationPreview(PreviousPreviewLanguage);
            else Localization.DisableGameLocalizationPreview();
        }
#endif
        Localization.WaitForAsyncTasks();
    };
#if WITH_EDITOR
    if(GIsEditor)
    {
        Internationalization.SetCurrentLanguage(TEXT("en"));
        Internationalization.SetCurrentLocale(TEXT("ko-KR"));
        Localization.WaitForAsyncTasks();
    }
#endif
    auto CheckCulture=[&](const FString& GameCulture)
    {
        FString ExpectedLanguage=GameCulture,ExpectedLocale=GameCulture;
#if WITH_EDITOR
        if(GIsEditor){ExpectedLanguage=TEXT("en");ExpectedLocale=TEXT("ko-KR");}
#endif
        TestEqual(FString::Printf(TEXT("Language after selecting %s"),*GameCulture),Internationalization.GetCurrentLanguage()->GetName(),ExpectedLanguage);
        TestEqual(FString::Printf(TEXT("Locale after selecting %s"),*GameCulture),Internationalization.GetCurrentLocale()->GetName(),ExpectedLocale);
    };
    ICLocalization::ApplyLanguage(0);
    CheckCulture(TEXT("ko"));
    FText Label=ICLocalization::Localize(FText::FromString(TEXT("체력 포션")));
    TestEqual(TEXT("Korean remains the source language"),Label.ToString(),FString(TEXT("체력 포션")));
    ICLocalization::ApplyLanguage(1);
    CheckCulture(TEXT("en"));
    TestEqual(TEXT("An existing label updates to English"),Label.ToString(),FString(TEXT("Health Potion")));
    TestEqual(TEXT("String table resolves in English"),ICText::Get(TEXT("Save_Complete")).ToString(),FString(TEXT("Save complete")));
    const FText Format=NSLOCTEXT("ICInventoryMenu","GoldBalance","보유 골드  {0} G");
    TestFalse(TEXT("Formatted native text translates before its arguments are applied"),FText::Format(ICLocalization::Localize(Format),42).ToString().Contains(TEXT("골드")));
    ICLocalization::ApplyLanguage(0);TestEqual(TEXT("Switching back restores the Korean text"),Label.ToString(),FString(TEXT("체력 포션")));
    CheckCulture(TEXT("ko"));
    return !HasAnyErrors();
}

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
