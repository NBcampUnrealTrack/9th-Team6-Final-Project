"""Restore exact backed-up target files only after the owning editor has exited."""
import hashlib,json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2];out=root/'Saved/CharacterLighting/AllCharacterOutline_20261103'
scope=json.loads((out/'Scope.json').read_text(encoding='utf-8'));backup=Path(scope['backup']).resolve();assert backup.is_relative_to(root)
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
restored=[]
for name,value in scope['allowed_changes'].items():
    target=(root/name).resolve();source=(backup/name).resolve()
    assert target.is_relative_to(root) and source.is_relative_to(backup) and digest(source)==value
    if digest(target)!=value:shutil.copy2(source,target);restored.append(name)
    assert digest(target)==value
(out/'RolledBack.json').write_text(json.dumps({'original_target_files_restored':True,'restored':restored},indent=2),encoding='utf-8')
print('Restored changed target files:',len(restored))
