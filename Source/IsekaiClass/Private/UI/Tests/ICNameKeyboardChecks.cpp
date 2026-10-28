#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/CharacterCreate/ICNameComposer.h"
#include "UI/Framework/ICPrologueWidget.h"
#include "Settings/ICGameUserSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNameKeyboardChecks,"IC.Frontend.NameKeyboard",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNameKeyboardChecks::RunTest(const FString& Parameters)
{
    FICNameComposer Name;
    auto Type=[&](const TCHAR* Keys){Name.Clear();for(TCHAR C:FString(Keys))Name.Append(C);return Name.Text;};
    TestEqual(TEXT("Hangul syllables"),Type(TEXT("ㅎㅏㄴㄱㅡㄹ")),FString(TEXT("한글")));
    TestEqual(TEXT("Final moves to next syllable"),Type(TEXT("ㄱㅏㄴㅏ")),FString(TEXT("가나")));
    TestEqual(TEXT("Compound final splits before vowel"),Type(TEXT("ㄷㅏㄹㄱㅏ")),FString(TEXT("달가")));
    TestEqual(TEXT("Compound vowel"),Type(TEXT("ㄱㅗㅏㄱㅗㅐ")),FString(TEXT("과괘")));
    TestEqual(TEXT("Double consonant and vowel"),Type(TEXT("ㄲㅑㅆㅐ")),FString(TEXT("꺄쌔")));
    TestEqual(TEXT("Mixed alphabet digits and Hangul"),Type(TEXT("Aㅎㅏㄴ 89z")),FString(TEXT("A한 89z")));
    Type(TEXT("ㅎㅏㄴ"));Name.Backspace();TestEqual(TEXT("Backspace removes the final"),Name.Text,FString(TEXT("하")));
    Name.Backspace();TestEqual(TEXT("Backspace removes the vowel"),Name.Text,FString(TEXT("ㅎ")));
    Name.Backspace();Name.Backspace();TestTrue(TEXT("Empty deletion is safe"),Name.Text.IsEmpty());
    Type(TEXT("abcdefghijkl"));TestFalse(TEXT("Thirteenth character rejected"),Name.Append(TEXT('m')));
    TestEqual(TEXT("Limit preserves existing name"),Name.Text,FString(TEXT("abcdefghijkl")));
    TestEqual(TEXT("G is menu confirm"),UICGameUserSettings::MenuKey(EKeys::G),EKeys::Enter);
    TestEqual(TEXT("F is menu confirm"),UICGameUserSettings::MenuKey(EKeys::F),EKeys::Enter);
    TestEqual(TEXT("All prologue lines retained"),UICPrologueWidget::Lines().Num(),55);
    TestEqual(TEXT("Opening sentence retained"),UICPrologueWidget::Lines()[0],FString(TEXT("나는 어느 날 갑자기 이세계에 떨어졌다.")));
    TestEqual(TEXT("Last sentence retained"),UICPrologueWidget::Lines().Last(),FString(TEXT("우선 친구들부터 만나러 가자.")));
    return !HasAnyErrors();
}
#endif
