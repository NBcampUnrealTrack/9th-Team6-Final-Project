"""Move native labels and user-facing notice literals into ST_IC_UI."""
import csv
import hashlib
import io
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / 'Data/StringTables/ST_IC_UI.csv'
with path.open(encoding='utf-8-sig', newline='') as file:
    rows = list(csv.DictReader(file))
strings = {row['Key']: row['SourceString'] for row in rows}

def localized(value):
    key = 'Native_' + hashlib.sha1(value.encode()).hexdigest()[:12]
    strings[key] = value.replace('\\n', '\n').replace('\\"', '"')
    return 'ICText::Get(TEXT("' + key + '"))'

sources = list((ROOT / 'Source/IsekaiClass/Private').rglob('*.cpp')) + list((ROOT / 'Source/IsekaiClass/Public/UI').rglob('*.h'))
for source in sources:
    if 'Tests' in source.parts or 'Debug' in source.parts:
        continue
    original = source.read_text(encoding='utf-8-sig')
    text = original
    if text.startswith('#include "UI/Framework/ICText.h"\n'):
        text = text.split('\n', 1)[1]
    if 'UI' in source.parts:
        text = re.sub(r'NSLOCTEXT\("[^"]*",\s*"[^"]*",\s*"((?:\\.|[^"\\])*)"\)', lambda match: localized(match[1]), text)
    # These call sites consume strings immediately; temporary localized FString lifetime is sufficient.
    text = re.sub(r'(\b(?:Show|Fail|ReportCreationFailure)\()TEXT\("([^"\n]*[가-힣][^"\n]*)"\)', lambda match: match[1] + '*' + localized(match[2]) + '.ToString()', text)
    text = re.sub(r'FString::Printf\(TEXT\("([^"\n]*[가-힣][^"\n]*)"\)', lambda match: 'FString::Printf(*' + localized(match[1]) + '.ToString()', text)
    def format_call(match):
        key = match[1]
        position = 0
        def placeholder(specifier):
            nonlocal position
            result = '{' + str(position) + '}'
            position += 1
            return result
        strings[key] = re.sub(r'%[ds]', placeholder, strings[key])
        return 'ICText::Format(TEXT("' + key + '"),'
    text = re.sub(r'FString::Printf\(\*ICText::Get\(TEXT\("([^"]+)"\)\)\.ToString\(\),', format_call, text)
    if text != original or ('ICText::' in text and '#include "UI/Framework/ICText.h"' not in text):
        if '#include "UI/Framework/ICText.h"' not in text:
            first, separator, rest = text.partition('\n')
            text = first + separator + '#include "UI/Framework/ICText.h"\n' + rest
        source.write_text(text, encoding='utf-8')

settings = ROOT / 'Source/IsekaiClass/Private/UI/Settings/ICSettingsWidget.cpp'
text = settings.read_text(encoding='utf-8')
old = 'const TCHAR* Labels[]={TEXT("낮음"),TEXT("중간"),TEXT("높음"),TEXT("매우 높음")};\n    for(const TCHAR* Text:Labels)Quality->AddOption(Text);'
new = 'const FText Labels[]={' + ','.join(localized(value) for value in ('낮음','중간','높음','매우 높음')) + '};\n    for(const FText& Text:Labels)Quality->AddOption(Text.ToString());'
settings.write_text(text.replace(old, new), encoding='utf-8')
stream = io.StringIO(newline='')
writer = csv.DictWriter(stream, ['Key', 'SourceString'], lineterminator='\n')
writer.writeheader()
writer.writerows({'Key': key, 'SourceString': value} for key, value in strings.items())
temporary = path.with_suffix('.csv.tmp')
temporary.write_text(stream.getvalue().replace('\r\n', '\n').replace('\r', ''), encoding='utf-8', newline='')
temporary.replace(path)
print('NATIVE_UI_LOCALIZED', len(strings))
