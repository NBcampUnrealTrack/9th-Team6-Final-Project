"""Copy the approved TIL and unmodified images into the user-specified vault folders."""
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'Docs/Characters/TIL/VRoidLighting_2026-11-03'
DEST=Path(r'K:\Tia-blog\content\TIL').resolve()
ATTACHMENTS=(DEST/'첨부파일모음').resolve()
NOTE=DEST/'261103(화)_TIL.md'
assert DEST.is_dir() and ATTACHMENTS.is_dir()
assert NOTE.resolve().parent==DEST and ATTACHMENTS.parent==DEST

text=(SOURCE/'TIL.md').read_text(encoding='utf-8')
references=re.findall(r'!\[([^\]]*)\]\((images/[^)]+)\)',text)
assert len(references)==8
images={}
digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
for _,relative in references:
    source=(SOURCE/relative).resolve()
    assert source.is_relative_to(SOURCE.resolve()) and source.is_file()
    destination=ATTACHMENTS/('20261103-vroid-lighting-'+source.name)
    assert destination.resolve().parent==ATTACHMENTS
    if destination.exists():assert digest(source)==digest(destination),'Different attachment already exists: '+str(destination)
    images[relative]=(source,destination)

def embed(match):
    return '![['+images[match.group(2)][1].name+']]'

text=re.sub(r'!\[([^\]]*)\]\((images/[^)]+)\)',embed,text)
frontmatter='''---
date: 2026-11-03
date&time: 2026-11-03T16:18:12+09:00
tags:
  - TIL
  - UnrealEngine
  - VRoid
  - VRM4U
---

'''
text=frontmatter+text
assert len(re.findall(r'!\[\[([^\]]+)\]\]',text))==8
assert 'C:/' not in text and 'K:/' not in text
assert all(label in text for label in ['밤 외부','밤 내부','낮 외부','낮 내부'])

staging=ROOT/'Saved/CharacterLighting/ObsidianTIL'
staging.mkdir(parents=True,exist_ok=True)
(staging/'261103_TIL.md').write_text(text,encoding='utf-8')
if NOTE.exists():assert NOTE.read_text(encoding='utf-8')==text,'An existing daily note would be overwritten: '+str(NOTE)

for source,destination in images.values():
    if not destination.exists():
        with source.open('rb') as incoming,destination.open('xb') as outgoing:
            shutil.copyfileobj(incoming,outgoing)
    assert digest(source)==digest(destination)
if not NOTE.exists():
    with NOTE.open('x',encoding='utf-8',newline='\n') as file:file.write(text)
assert NOTE.read_text(encoding='utf-8')==text
for name in re.findall(r'!\[\[([^\]]+)\]\]',NOTE.read_text(encoding='utf-8')):
    assert (ATTACHMENTS/name).is_file(),name

receipt={'note':str(NOTE),'attachments_directory':str(ATTACHMENTS),'unique_images':len(images),
         'embeds':len(references),'images_unchanged':True,'link_style':'Obsidian wikilink, unique basename; existing Quartz shortest resolution',
         'files':[{'path':str(destination),'sha256':digest(destination)} for _,destination in images.values()]}
(staging/'Placement.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({key:value for key,value in receipt.items() if key!='files'},ensure_ascii=False))
