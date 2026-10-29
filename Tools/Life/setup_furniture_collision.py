"""Retain original meshes/art; enable the chest lid and NPC bed player blockers."""
from pathlib import Path
import json
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
game='/Game/IsekaiClass'
report={}
mesh_tools=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
lid=u.load_asset(game+'/World/SampleLevel/Medieval_House/meshes/props/SM_chest_03');assert lid
count=mesh_tools.get_simple_collision_count(lid)
if count==0:assert mesh_tools.add_simple_collisions(lid,u.ScriptingCollisionShapeType.BOX)>=0
assert mesh_tools.get_simple_collision_count(lid)>0
assert u.EditorAssetLibrary.save_loaded_asset(lid,False)
report['lid_simple_shapes']=mesh_tools.get_simple_collision_count(lid)
for path in [game+'/Life/Home/Blueprints/BP_IC_PlayerStorage',game+'/NPC/Blueprints/Containers/BP_IC_NPCChest']:
    bp=u.load_asset(path);assert bp,path
    default=u.get_default_object(bp.generated_class());hinge=default.get_editor_property('ChestPresentation')
    hinge.get_editor_property('Lid').set_collision_profile_name('BlockAllDynamic')
    u.BlueprintEditorLibrary.compile_blueprint(bp);assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    report[path]='BlockAllDynamic'
bed=u.load_asset(game+'/NPC/Blueprints/Interaction/BP_IC_NPCBed');assert bed
shape=u.get_default_object(bed.generated_class()).get_editor_property('Shape')
shape.set_collision_profile_name('BlockAllDynamic')
u.BlueprintEditorLibrary.compile_blueprint(bed);assert u.EditorAssetLibrary.save_loaded_asset(bed,False)
report['npc_bed_interaction']='BlockAllDynamic'
table=u.load_asset(game+'/Data/DataTable/NPC/DT_IC_DialoguePresentation');assert table
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
for row in rows:row['bAllowPlayerMovement']=False
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
assert u.EditorAssetLibrary.save_loaded_asset(table,False)
(root/'Data/NPC/DT_IC_DialoguePresentation.csv').write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table),encoding='utf-8-sig')
out=root/'Saved/ManagedUIReview';out.mkdir(parents=True,exist_ok=True)
(out/'FurnitureCollision.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_FURNITURE_COLLISION_READY')
