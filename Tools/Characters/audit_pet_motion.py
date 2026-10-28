"""Read the existing pet loops without changing animation assets."""
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'PartyFollow'
root.mkdir(parents=True,exist_ok=True)
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster')
if not table:
    table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Enemy/DT_IC_Monster')
if not table:
    matches=u.EditorAssetLibrary.list_assets('/Game/IsekaiClass/Data/DataTable',recursive=True,include_folder=False)
    table=next(u.load_asset(p) for p in matches if p.endswith('.DT_IC_Monster'))
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
report=[]
for row in rows:
    if row.get('PetCodexIndex',-1)<0 or row.get('bDeprecated'):continue
    entry={'id':row['Name'],'index':row['PetCodexIndex']}
    for key in ('IdleAnimation','MoveAnimation'):
        path=row.get(key)
        clip=u.load_asset(path) if path and path!='None' else None
        if clip:
            entry[key]={'asset':clip.get_path_name(),'root_motion':clip.get_editor_property('enable_root_motion'),
                        'root_lock':clip.get_editor_property('force_root_lock'),'length':clip.get_play_length()}
    report.append(entry)
(root/'PetMotionAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_PET_MOTION_AUDIT_DONE')
