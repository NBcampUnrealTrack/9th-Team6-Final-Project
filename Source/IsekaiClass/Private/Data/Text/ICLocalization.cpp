#include "Data/Text/ICLocalization.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

// Keep translation state out of the shared anonymous namespace used by Unity builds.
namespace ICLocalizationState
{
bool Loaded=false;
TMap<FString,FString> English;
TMap<FString,FText> Cache;
TArray<FString> Phrases;
void Load()
{
    if(Loaded)return;Loaded=true;
    FString Source;TSharedPtr<FJsonObject> Object;
    if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("IsekaiClass/Localization/English.json")))
        ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Object)||!Object)return;
    for(const auto& Pair:Object->Values)
    {FString Value;if(Pair.Value->TryGetString(Value)&&!Value.IsEmpty())English.Add(FString(*Pair.Key),Value);}
    English.GetKeys(Phrases);Phrases.Sort([](const FString& A,const FString& B){return A.Len()>B.Len();});
    TArray<FPolyglotTextData> Entries;
    auto Register=[&](const FString& Namespace,const FString& Key,const FString& Native)
    {
        if(const auto* Translation=English.Find(Native))
        {FPolyglotTextData Entry(ELocalizedTextSourceCategory::Game,Namespace,Key,Native,TEXT("ko"));Entry.AddLocalizedString(TEXT("en"),*Translation);Entries.Add(MoveTemp(Entry));}
    };
    for(const TCHAR* Name:{TEXT("ST_IC_UI"),TEXT("ST_IC_Definitions"),TEXT("ST_IC_Quest"),TEXT("ST_IC_Dialogue")})
    {
        const FString Path=FString::Printf(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/%s.%s"),Name,Name);
        if(auto* Table=LoadObject<UStringTable>(nullptr,*Path))
        {const auto Strings=Table->GetStringTable();Strings->EnumerateSourceStrings([&](const FString& Key,const FString& Native){Register(Strings->GetNamespace(),Key,Native);return true;});}
    }
    FString KeySource;TArray<TSharedPtr<FJsonValue>> Keys;
    if(FFileHelper::LoadFileToString(KeySource,*(FPaths::ProjectContentDir()/TEXT("IsekaiClass/Localization/NativeKeys.json")))
        &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(KeySource),Keys))
        for(const auto& Value:Keys)if(const auto Row=Value->AsObject())Register(Row->GetStringField(TEXT("namespace")),Row->GetStringField(TEXT("key")),Row->GetStringField(TEXT("source")));
    FTextLocalizationManager::Get().RegisterPolyglotTextData(Entries,true);
}
FString Translate(const FString& Source)
{
    if(const auto* Exact=English.Find(Source))return *Exact;
    FString Result=Source;
    // Longest phrases first handles runtime item names, quantities and HUD notices.
    for(const auto& Phrase:Phrases)if(Phrase.Len()>=2&&Result.Contains(Phrase))Result.ReplaceInline(*Phrase,**English.Find(Phrase),ESearchCase::CaseSensitive);
    return Result;
}
bool HasKorean(const FString& Value)
{for(TCHAR C:Value)if(C>=0xAC00&&C<=0xD7A3)return true;return false;}
}
void ICLocalization::ApplyLanguage(int32 Language)
{
    ICLocalizationState::Load();const FString Culture=Language==1?TEXT("en"):TEXT("ko");
    FInternationalization::Get().SetCurrentCulture(Culture);
#if WITH_EDITOR
    if(GIsEditor)
    {FTextLocalizationManager::Get().ConfigureGameLocalizationPreviewLanguage(Culture);FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);}
#endif
    FTextLocalizationManager::Get().WaitForAsyncTasks();
}
FText ICLocalization::Localize(const FText& Text)
{
    ICLocalizationState::Load();
    const FString* Native=FTextInspector::GetSourceString(Text);
    const FString Source=Native?*Native:Text.ToString();
    if(!ICLocalizationState::HasKorean(Source))return Text;
    if(const auto* Existing=ICLocalizationState::Cache.Find(Source))return *Existing;
    const FString Translation=ICLocalizationState::Translate(Source);if(Translation==Source){ICLocalizationState::Cache.Add(Source,Text);return Text;}
    FTCHARToUTF8 Bytes(*Source);const FString Key=FMD5::HashBytes(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    FPolyglotTextData Data(ELocalizedTextSourceCategory::Game,TEXT("IC.Runtime"),Key,Source,TEXT("ko"));
    Data.AddLocalizedString(TEXT("en"),Translation);
    FTextLocalizationManager::Get().RegisterPolyglotTextData(Data,true);
    // GetText is available in cooked builds, unlike the editor-only ChangeKey API.
    FText Result=Data.GetText();ICLocalizationState::Cache.Add(Source,Result);return Result;
}
