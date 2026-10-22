"""Place one configurable layout origin; NPCs remain runtime-spawned."""
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
game='/Game/IsekaiClass'
world=u.EditorLoadingAndSavingUtils.load_map(game+'/World/L_TestMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
path=game+'/World/Blueprints/BP_R01TestLayout'
bp=u.load_asset(path)
if not bp:
    factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.ICNPCTestLayout)
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_R01TestLayout',game+'/World/Blueprints',u.Blueprint,factory)
if not bp:raise RuntimeError('Cannot create test layout Blueprint')
u.BlueprintEditorLibrary.compile_blueprint(bp)
u.EditorAssetLibrary.save_loaded_asset(bp)
existing=[actor for actor in actors.get_all_level_actors() if isinstance(actor,u.ICNPCTestLayout)]
if len(existing)>1:raise RuntimeError('More than one test layout exists; refusing to duplicate it')
layout=existing[0] if existing else actors.spawn_actor_from_class(bp.generated_class(),u.Vector(2000,0,0),u.Rotator(pitch=0,yaw=180,roll=0))
layout.set_actor_label('IC_R01_NPC_TestLayout')
layout.set_actor_location_and_rotation(u.Vector(2000,0,0),u.Rotator(pitch=0,yaw=180,roll=0),False,True)
layout.set_editor_property('row_spacing_cm',350)
layout.set_editor_property('bed_distance_cm',700)
layout.set_editor_property('anchor_height_cm',88)
if not u.ICCharacterSetupLibrary.prepare_test_navigation(world):raise RuntimeError('Navigation bounds setup failed')
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.RecastNavMesh):actor.set_editor_property('runtime_generation',u.RuntimeGenerationType.DYNAMIC)
for name in ['World/DT_IC_Anchor','Registry/DT_IC_IDRegistry']:
    table=u.load_asset(game+'/Data/DataTable/'+name)
    csv=(root/'Data'/(name+'.csv')).read_text(encoding='utf-8-sig')
    if not u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,csv):raise RuntimeError('DT import failed: '+name)
    u.EditorAssetLibrary.save_loaded_asset(table)
strings=u.load_asset(game+'/Data/DataTable/StringTables/ST_IC_Definitions')
if not u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(root/'Data/StringTables/ST_IC_Definitions.csv')):raise RuntimeError('Definition string import failed')
u.EditorAssetLibrary.save_loaded_asset(strings)
if not u.EditorLoadingAndSavingUtils.save_map(world,game+'/World/L_TestMap'):raise RuntimeError('Map save failed')
report={'map':world.get_path_name(),'layout':layout.get_path_name(),'spacing_cm':350,'bed_distance_cm':700,'generated_runtime_anchors':33,'generated_runtime_beds':11,'directly_placed_npcs':[actor.get_path_name() for actor in actors.get_all_level_actors() if isinstance(actor,u.ICNPCBase)],'nav_actors':[actor.get_class().get_name() for actor in actors.get_all_level_actors() if isinstance(actor,(u.NavMeshBoundsVolume,u.RecastNavMesh))]}
(root/'Saved/NPCTestLayoutSetup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_NPC_TEST_LAYOUT_SAVED '+json.dumps(report))
