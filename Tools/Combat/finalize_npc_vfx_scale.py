"""Targeted migration for imported NPC Niagara component scaling; preserves balance."""
import csv
import io
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
count=0
for row in rows:
    if not row['Name'].startswith('NPC_R01_'):continue
    for role in ('Slash','Range','Projectile','Trail','Cast','Impact'):
        row[role]['bUseComponentScale']=True
    for role in ('CastLayers','ImpactLayers'):
        for style in row[role]:style['bUseComponentScale']=True
    count+=1
assert count==11
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table,False)
reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
out=io.StringIO(newline='');writer=csv.DictWriter(out,fieldnames=reader.fieldnames,lineterminator='\n')
writer.writeheader();writer.writerows(reader)
(root/'Data/Combat/DT_IC_CombatVFX.csv').write_text(out.getvalue(),encoding='utf-8',newline='')
u.log('NPC_IMPORTED_SCALE_OK')
