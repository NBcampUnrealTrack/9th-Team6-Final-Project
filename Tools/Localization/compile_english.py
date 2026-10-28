"""Build the runtime Korean/English FText catalogue and a reviewable source CSV."""
import csv,json,pathlib,re,sys
sys.stdout.reconfigure(encoding='utf-8')
root=pathlib.Path(__file__).resolve().parents[2]
sources=json.loads((root/'Data/Localization/SourceCatalogue.json').read_text(encoding='utf-8'))
glossary={}
for file in (root/'Data/Localization').glob('*.tsv'):
    for row in csv.reader(file.open(encoding='utf-8-sig'),delimiter='\t'):
        if len(row)==2 and row[0] and not row[0].startswith('#'):glossary[row[0].replace('\\n','\n')]=row[1].replace('\\n','\n')
catalogue=dict(glossary)
for file in (root/'Data/Localization').glob('English_*.txt'):
    for line in file.read_text(encoding='utf-8-sig').splitlines():
        if '|' not in line:continue
        number,value=line.split('|',1)
        if number.isdigit():catalogue[list(sources)[int(number)]]=value.replace('\\n','\n')
terms=sorted(catalogue,key=len,reverse=True)
for source in sources:
    if source in catalogue:continue
    result=source
    for term in terms:
        if len(term)>=2:result=result.replace(term,catalogue[term])
        else:result=re.sub(r'(?<![가-힣])'+re.escape(term)+r'(?![가-힣])',lambda _:catalogue[term],result)
    result=re.sub(r'(\d+)개',r'\1',result)
    result=re.sub(r'(\d+)초',r'\1 sec',result)
    result=re.sub(r'(\d+)분',r'\1 min',result)
    result=re.sub(r'(\d+)시간',r'\1 hr',result)
    if not re.search('[가-힣]',result):catalogue[source]=result
missing=[(i,s) for i,s in enumerate(sources) if s not in catalogue]
out=root/'Content/IsekaiClass/Localization';out.mkdir(parents=True,exist_ok=True)
(out/'English.json').write_text(json.dumps(catalogue,ensure_ascii=False,indent=2),encoding='utf-8')
keys=[]
literal=r'"((?:\\.|[^"\\])*)"'
for path in (root/'Source/IsekaiClass').rglob('*.cpp'):
    code=path.read_text(encoding='utf-8-sig');ns=re.search(r'#define\s+LOCTEXT_NAMESPACE\s+'+literal,code)
    for match in re.finditer(r'(NSLOCTEXT|LOCTEXT)\s*\((.*?)\)',code,re.S):
        values=re.findall(literal,match.group(2))
        try:values=[json.loads('"'+s+'"') for s in values]
        except ValueError:continue
        if match.group(1)=='LOCTEXT' and ns:values.insert(0,ns.group(1))
        if len(values)==3:keys.append(dict(zip(['namespace','key','source'],values)))
(out/'NativeKeys.json').write_text(json.dumps(keys,ensure_ascii=False,indent=2),encoding='utf-8')
data=root/'Data/Localization';data.mkdir(parents=True,exist_ok=True)
with (data/'English.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['Source','English']);w.writerows((s,catalogue.get(s,'')) for s in sources)
(root/'Saved/Polish/EnglishMissing.txt').write_text('\n'.join(str(i)+'|'+s.replace('\n','\\n') for i,s in missing),encoding='utf-8')
print(json.dumps(dict(sources=len(sources),translated=len(sources)-len(missing),missing=len(missing))))
if len(sys.argv)>1:
    start,end=map(int,sys.argv[1:3]);print('\n'.join(f'{i}|'+s.replace('\n','\\n') for i,s in enumerate(sources) if start<=i<end))
