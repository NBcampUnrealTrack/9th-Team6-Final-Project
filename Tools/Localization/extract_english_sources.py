"""Collect player-facing string tables and native text for the English catalogue."""
import csv,json,re,pathlib,sys
root=pathlib.Path(__file__).resolve().parents[2]
sources={}
def add(value,origin):
    if re.search('[가-힣]',value):sources.setdefault(value,[]).append(origin)
for path in (root/'Data/StringTables').glob('*.csv'):
    for row in csv.DictReader(path.open(encoding='utf-8-sig')):add(row['SourceString'],path.stem+':'+row['Key'])
literal=r'"((?:\\.|[^"\\])*)"'
for path in (root/'Source/IsekaiClass').rglob('*.cpp'):
    if '/Tests/' in path.as_posix():continue
    code=path.read_text(encoding='utf-8-sig')
    for match in re.finditer(r'(?:NSLOCTEXT|LOCTEXT)\s*\((.*?)\)',code,re.S):
        values=re.findall(literal,match.group(1))
        if len(values)>=2:
            try:add(json.loads('"'+values[-1]+'"'),path.name)
            except ValueError:pass
    for match in re.finditer(r'(?:FromString|Printf)\(TEXT\('+literal+r'\)',code):
        try:add(json.loads('"'+match.group(1)+'"'),path.name)
        except ValueError:pass
out=root/'Saved/Polish';out.mkdir(exist_ok=True)
(out/'EnglishSources.json').write_text(json.dumps(sources,ensure_ascii=False,indent=2),encoding='utf-8')
(out/'EnglishSources.txt').write_text('\n'.join(f'{i}|{v}' for i,v in enumerate(sources)),encoding='utf-8')
print(len(sources))
if '--append' in sys.argv:
    path=root/'Data/Localization/SourceCatalogue.json'
    catalogue=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
    added=0
    for source,origins in sources.items():
        if source not in catalogue:catalogue[source]=origins;added+=1
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(catalogue,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Appended new sources without changing existing translation numbers:',added)
