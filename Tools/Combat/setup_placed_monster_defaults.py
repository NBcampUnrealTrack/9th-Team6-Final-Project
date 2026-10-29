"""Preset individual monster BPs for direct level placement without changing the team AI."""
import hashlib,json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/PlacedMonsterReview';out.mkdir(parents=True,exist_ok=True)
team=[root/'Content/IsekaiClass/Enemy/AI/BehaviorTrees/BT_ICMonsterBase.uasset',root/'Content/IsekaiClass/Enemy/AI/Blackboards/BB_ICMonsterBase.uasset']
hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in team}
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster');assert table
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
ids={f'MON_R01_{i:03d}' for i in range(1,11)}|{'MON_R01_BOSS_001'}
report=[]
for row in rows:
    if row['ID'] not in ids:continue
    bp=u.load_asset(row['PawnClass'].split('.')[0]);assert bp
    defaults=u.get_default_object(bp.generated_class());assert isinstance(defaults,u.ICEnemyBase)
    defaults.set_editor_property('monster_id',row['ID'])
    defaults.set_editor_property('initialize_when_placed',True)
    defaults.set_editor_property('placed_leash_radius_cm',1600. if row['ID']=='MON_R01_BOSS_001' else 1000.)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    defaults=u.get_default_object(bp.generated_class())
    assert str(defaults.get_editor_property('monster_id'))==row['ID']
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    report.append({'monster':row['ID'],'blueprint':bp.get_path_name(),'leash_cm':defaults.get_editor_property('placed_leash_radius_cm')})
assert len(report)==11
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==hashes[str(p)] for p in team)
(out/'Setup.json').write_text(json.dumps({'blueprints':report,'unchanged_team_assets':hashes},ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_PLACED_MONSTER_DEFAULTS_READY')
