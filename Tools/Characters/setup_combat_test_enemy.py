"""Place a normal-AI cube enemy in L_TestMap and import the revised controls label."""
import csv
import io
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
game = "/Game/IsekaiClass"
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = u.EditorLoadingAndSavingUtils.load_map(game + "/World/L_TestMap")
starts = [a for a in actors.get_all_level_actors() if isinstance(a, u.PlayerStart)]
if not starts:
    raise RuntimeError("L_TestMap has no PlayerStart")
location = starts[0].get_actor_location() + u.Vector(1200, -600, 0)
location.z = max(location.z, 96)

def upsert(table_name, row):
    table = u.load_asset(game + "/Data/DataTable/" + table_name)
    rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    rows = [r for r in rows if r["Name"] != row["Name"]]
    rows.append(row)
    old = table.get_editor_property("ignore_missing_fields")
    table.set_editor_property("ignore_missing_fields", True)
    try:
        if not u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows)):
            raise RuntimeError("Import failed: " + table_name)
    finally:
        table.set_editor_property("ignore_missing_fields", old)
    if not u.EditorAssetLibrary.save_loaded_asset(table, False):
        raise RuntimeError("Save failed: " + table_name)
    exported = list(csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table))))
    authored = next(r for r in exported if r["---"] == row["Name"])
    source = root / "Data" / (table_name + ".csv")
    reader = csv.DictReader(io.StringIO(source.read_text(encoding="utf-8-sig")))
    fields = reader.fieldnames
    original = [r for r in reader if r["---"] != row["Name"]]
    original.append({k: authored.get(k, "") for k in fields})
    output = io.StringIO()
    writer = csv.DictWriter(output, fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(original)
    source.write_text(output.getvalue(), encoding="utf-8")

monster = "MON_COMBAT_TEST_001"
anchor_id = "ANC_COMBAT_TEST_001"
spawner = "SPN_COMBAT_TEST_001"
upsert("Combat/DT_IC_Monster", dict(Name=monster, ID=monster, DataVersion=1,
    PawnClass="/Script/IsekaiClass.ICCombatTestEnemy", DetectionRangeCm=600,
    SightAngleDegrees=360, bRequiresLineOfSight=True, AttackRangeCm=150,
    AttackIntervalPlaySeconds=2, ReturnAcceptanceRadiusCm=80, RespawnPlaySeconds=5,
    CombatStats=dict(MaxHP=100, MaxMP=0, PhysicalAttack=1, MoveSpeedCmPerSecond=220,
                     AttackSpeedMultiplier=1, CriticalMultiplier=1)))
upsert("World/DT_IC_Anchor", dict(Name=anchor_id, ID=anchor_id, DataVersion=1, Type="MonsterOrigin"))
upsert("World/DT_IC_Spawner", dict(Name=spawner, ID=spawner, DataVersion=1,
    Slots=[dict(SpawnSlotIndex=0, MonsterID=monster, OriginAnchorID=anchor_id, LeashRadiusCm=1200)]))
for key, kind, path in [(monster,"Monster","Combat/DT_IC_Monster"),
                        (anchor_id,"Anchor","World/DT_IC_Anchor"),
                        (spawner,"Spawner","World/DT_IC_Spawner")]:
    asset_path=game+"/Data/DataTable/"+path
    upsert("Registry/DT_IC_IDRegistry", dict(Name=key,ID=key,DataVersion=1,
        DefinitionType=kind,DefinitionTable=asset_path+"."+path.split("/")[-1],
        Notes="Combat test enemy; spawned only by its L_TestMap anchor."))
existing = [a for a in actors.get_all_level_actors() if isinstance(a,u.ICWorldAnchor)
            and str(a.get_editor_property("anchor_id")) == anchor_id]
anchor = existing[0] if existing else actors.spawn_actor_from_class(u.ICWorldAnchor, location)
anchor.set_editor_property("anchor_id",anchor_id)
anchor.set_actor_location_and_rotation(location,u.Rotator(0,180,0),False,True)
anchor.set_actor_label("IC_Combat_Test_Enemy_Origin_6m_Detection")
if not u.ICCharacterSetupLibrary.prepare_test_navigation(world):
    raise RuntimeError("Navigation setup failed")
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.RecastNavMesh):
        actor.set_editor_property("runtime_generation",u.RuntimeGenerationType.DYNAMIC)
strings=u.load_asset(game+"/Data/DataTable/StringTables/ST_IC_UI")
if not u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(root/"Data/StringTables/ST_IC_UI.csv")):
    raise RuntimeError("UI import failed")
u.EditorAssetLibrary.save_loaded_asset(strings,False)
if not u.EditorLoadingAndSavingUtils.save_map(world,game+"/World/L_TestMap"):
    raise RuntimeError("Map save failed")
u.log("IC_COMBAT_TEST_ENEMY_PLACED " + str(location))
