"""Connect the existing test monsters to the team's BT/BB without editing either AI asset."""
from pathlib import Path
import csv,hashlib,json
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
game='/Game/IsekaiClass'
bt_path=game+'/Enemy/AI/BehaviorTrees/BT_ICMonsterBase.BT_ICMonsterBase'
bb_path=game+'/Enemy/AI/Blackboards/BB_ICMonsterBase.BB_ICMonsterBase'
tree=u.load_asset(bt_path);blackboard=u.load_asset(bb_path);assert tree and blackboard
assert tree.get_editor_property('blackboard_asset')==blackboard
files=[root/'Content/IsekaiClass/Enemy/AI/BehaviorTrees/BT_ICMonsterBase.uasset',root/'Content/IsekaiClass/Enemy/AI/Blackboards/BB_ICMonsterBase.uasset']
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
table=u.load_asset(game+'/Data/DataTable/Combat/DT_IC_Monster');assert table
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
ids={f'MON_R01_{i:03}' for i in range(1,11)}|{'MON_R01_BOSS_001'}
changed=[]
for row in rows:
    if row['Name'] not in ids:continue
    assert row['BehaviorTree'] in ('None','',bt_path),row['Name']
    assert row['Blackboard'] in ('None','',bb_path),row['Name']
    row.update(BehaviorTree=bt_path,Blackboard=bb_path);changed.append(row['Name'])
assert set(changed)==ids,changed
csv_path=root/'Data/Combat/DT_IC_Monster.csv'
with csv_path.open(encoding='utf-8-sig',newline='') as source:
    reader=csv.DictReader(source);fields=reader.fieldnames;csv_rows=list(reader)
assert {row['---'] for row in csv_rows if row['---'] in ids}==ids
for row in csv_rows:
    if row['---'] in ids:row.update(BehaviorTree=bt_path,Blackboard=bb_path)
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table,False)
with csv_path.open('w',encoding='utf-8-sig',newline='') as target:
    writer=csv.DictWriter(target,fieldnames=fields,lineterminator='\n');writer.writeheader();writer.writerows(csv_rows)
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==before[str(p)] for p in files),'The team BT/BB must remain unchanged'
out=root/'Saved/MonsterBehaviorReview';out.mkdir(parents=True,exist_ok=True)
(out/'Connected.json').write_text(json.dumps(dict(monsters=sorted(changed),behavior_tree=bt_path,blackboard=bb_path,team_asset_hashes=before),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_TEST_MONSTER_BT_CONNECTED')
