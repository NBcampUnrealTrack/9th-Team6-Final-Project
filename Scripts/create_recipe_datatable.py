import csv
from pathlib import Path
import unreal

project_dir = Path(unreal.Paths.project_dir())
csv_path = project_dir / "Data" / "Recipes" / "DT_IC_Recipe.csv"
csv_path.parent.mkdir(parents=True, exist_ok=True)
fields = ["Name", "RecipeID", "DisplayName", "Type", "ResultItemID", "ResultCount", "IngredientID1", "IngredientCount1", "IngredientID2", "IngredientCount2", "SPCost", "bUnlockedByDefault"]
rows = [
    {"Name":"RCP_COOK_001","RecipeID":"RCP_COOK_001","DisplayName":"빵","Type":"Cook","ResultItemID":"ITM_FOOD_002","ResultCount":1,"IngredientID1":"ITM_MAT_002","IngredientCount1":2,"IngredientID2":"None","IngredientCount2":0,"SPCost":5,"bUnlockedByDefault":"true"},
    {"Name":"RCP_CRAFT_001","RecipeID":"RCP_CRAFT_001","DisplayName":"초보자 검","Type":"Craft","ResultItemID":"EQP_WPN_SWORD_001","ResultCount":1,"IngredientID1":"ITM_MAT_003","IngredientCount1":2,"IngredientID2":"ITM_MAT_001","IngredientCount2":1,"SPCost":10,"bUnlockedByDefault":"true"},
]
with csv_path.open("w",encoding="utf-8-sig",newline="") as stream:
    writer=csv.DictWriter(stream,fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)
asset_path="/Game/IsekaiClass/Data/Recipes/DT_IC_Recipe"
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    table=unreal.EditorAssetLibrary.load_asset(asset_path)
else:
    row_struct=unreal.find_object(None,"/Script/IsekaiClass.PlayerRecipeDefinition")
    if not row_struct: raise RuntimeError("FPlayerRecipeDefinition was not found")
    factory=unreal.DataTableFactory()
    factory.set_editor_property("struct",row_struct)
    table=unreal.AssetToolsHelpers.get_asset_tools().create_asset("DT_IC_Recipe","/Game/IsekaiClass/Data/Recipes",unreal.DataTable,factory)
if not table or not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table,str(csv_path)):
    raise RuntimeError("DT_IC_Recipe import failed")
unreal.EditorAssetLibrary.save_loaded_asset(table,only_if_is_dirty=False)
names=unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
if len(names)!=len(rows): raise RuntimeError(f"Expected {len(rows)} rows, got {len(names)}")
unreal.log(f"DT_IC_Recipe created with {len(names)} rows")
