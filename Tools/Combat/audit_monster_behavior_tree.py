"""Read-only audit of the authored monster BT, Blackboard, and live DT bindings."""
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/MonsterBehaviorReview';out.mkdir(parents=True,exist_ok=True)
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster');assert table
tree=u.load_asset('/Game/IsekaiClass/Enemy/AI/BehaviorTrees/BT_ICMonsterBase');assert tree
blackboard=u.load_asset('/Game/IsekaiClass/Enemy/AI/Blackboards/BB_ICMonsterBase');assert blackboard
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
report=dict(rows=[{k:r.get(k) for k in ('Name','ID','BehaviorTree','Blackboard','PawnClass','ChaseLimitCm','InvestigationAdvanceDistanceCm')} for r in rows])

def prop(obj,name):
    try:return obj.get_editor_property(name)
    except Exception as error:return str(error)

def node(value):
    if not isinstance(value,u.Object):return str(value)
    result=dict(name=value.get_name(),type=value.get_class().get_path_name())
    for name in ('node_name','blackboard_key','home_location_key','wander_location_key'):
        field=prop(value,name)
        if not isinstance(field,str) or not field.startswith('Property'):result[name]=str(field)
    services=prop(value,'services')
    if not isinstance(services,str):result['services']=[node(s) for s in services]
    children=prop(value,'children')
    if not isinstance(children,str):
        result['children']=[]
        for child in children:
            branch={}
            for name in ('child_composite','child_task'):
                obj=prop(child,name)
                if isinstance(obj,u.Object):branch[name]=node(obj)
            decorators=prop(child,'decorators')
            if not isinstance(decorators,str):branch['decorators']=[node(d) for d in decorators]
            result['children'].append(branch)
    return result

report['tree_blackboard']=str(prop(tree,'blackboard_asset'))
report['root']=node(prop(tree,'root_node'))
report['blackboard_keys']=str(prop(blackboard,'keys'))
(out/'Audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_MONSTER_BT_AUDIT_READY')
