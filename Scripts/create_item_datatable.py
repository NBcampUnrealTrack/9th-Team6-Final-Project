import csv
from pathlib import Path
import unreal

project_dir = Path(unreal.Paths.project_dir())
csv_path = project_dir / "Data" / "Items" / "DT_IC_Item.csv"
csv_path.parent.mkdir(parents=True, exist_ok=True)

fields = [
    "Name", "ItemID", "DisplayName", "Tab", "Category", "Kind", "EquipSlot",
    "StackLimit", "bCanDiscard", "bCanSell", "bCanGift", "bDeprecated",
    "BuyPrice", "SellPrice", "EffectID", "RequiredClass", "GrowthStat",
    "GrowthAmount", "RestoreHP", "RestoreSP", "DefaultGrade", "MaxEnhanceLevel",
    "MaxRandomOptions", "OptionPoolID", "BaseAttack", "BaseDefense",
    "EnhanceAttackPerLevel", "EnhanceDefensePerLevel", "EnhanceBaseCost",
    "EnhanceBaseSuccessRate", "EquipmentMesh", "AttachSocket"
]

rows = []

def add(item_id, display_name, category, kind, slot="None", stack=99, effect="None",
        required_class=-1, growth_stat="None", growth_amount=0, restore_hp=0,
        restore_sp=0, can_discard=True, can_sell=True, can_gift=False,
        max_enhance=0, max_options=0, socket="None"):
    rows.append({
        "Name": item_id,
        "ItemID": item_id,
        "DisplayName": display_name,
        "Tab": "Quest" if kind == "Quest" else "Normal",
        "Category": category,
        "Kind": kind,
        "EquipSlot": slot,
        "StackLimit": stack,
        "bCanDiscard": str(can_discard).lower(),
        "bCanSell": str(can_sell).lower(),
        "bCanGift": str(can_gift).lower(),
        "bDeprecated": "false",
        "BuyPrice": 0,
        "SellPrice": 0,
        "EffectID": effect,
        "RequiredClass": required_class,
        "GrowthStat": growth_stat,
        "GrowthAmount": growth_amount,
        "RestoreHP": restore_hp,
        "RestoreSP": restore_sp,
        "DefaultGrade": "Common",
        "MaxEnhanceLevel": max_enhance,
        "MaxRandomOptions": max_options,
        "OptionPoolID": "None",
        "BaseAttack": 0,
        "BaseDefense": 0,
        "EnhanceAttackPerLevel": 0,
        "EnhanceDefensePerLevel": 0,
        "EnhanceBaseCost": 0,
        "EnhanceBaseSuccessRate": 1,
        "EquipmentMesh": "None",
        "AttachSocket": socket,
    })

add("ITM_CONS_001", "체력 포션", "Consumable", "Consumable", effect="RestoreHP", restore_hp=25, can_gift=True)
add("ITM_FOOD_001", "채소 수프", "Food", "Consumable", effect="RestoreSP", restore_sp=25, can_gift=True)
add("ITM_FOOD_002", "빵", "Food", "Consumable", effect="RestoreSP", restore_sp=15, can_gift=True)

book_specs = [
    ("Strength", "근력"), ("Dexterity", "민첩"), ("Intelligence", "지력"),
    ("Constitution", "체력"), ("Charisma", "매력"), ("Luck", "운")
]
book_grades = [("초급", 1), ("중급", 3), ("고급", 5)]
serial = 1
for stat, stat_label in book_specs:
    for grade_label, amount in book_grades:
        add(f"ITM_BOOK_{serial:03d}", f"{grade_label} {stat_label}의 서", "Book", "Consumable",
            effect="GrowMainStat", growth_stat=stat, growth_amount=amount, can_gift=True)
        serial += 1

add("ITM_MAT_001", "목재", "Material", "Material")
add("ITM_MAT_002", "밀가루", "Material", "Material")
add("ITM_MAT_003", "철 주괴", "Material", "Material")
add("ITM_TKT_001", "가방 확장권", "Ticket", "Expansion")
add("ITM_QST_001", "퀘스트 반지", "Quest", "Quest", can_discard=False, can_sell=False)

weapons = [
    ("EQP_WPN_SWORD_001", "초보자 검", 0),
    ("EQP_WPN_BOW_001", "초보자 활", 1),
    ("EQP_WPN_STAFF_001", "초보자 마법스태프", 2),
    ("EQP_WPN_WAND_001", "초보자 치유완드", 3),
]
for item_id, display_name, required_class in weapons:
    add(item_id, display_name, "Equipment", "Equipment", "Weapon", 1,
        required_class=required_class, max_enhance=10, max_options=4, socket="weapon_r")
    rows[-1].update({"OptionPoolID":"OPT_WPN","BaseAttack":10,"EnhanceAttackPerLevel":2,"EnhanceBaseCost":100,"EnhanceBaseSuccessRate":1})

add("EQP_ARM_001", "여행자의 옷", "Equipment", "Equipment", "Clothes", 1, max_enhance=10, max_options=4)
rows[-1].update({"OptionPoolID":"OPT_ARM","BaseDefense":6,"EnhanceDefensePerLevel":1.5,"EnhanceBaseCost":100,"EnhanceBaseSuccessRate":1})
add("EQP_RNG_001", "구리 반지", "Equipment", "Equipment", "Ring", 1, max_enhance=10, max_options=4)
rows[-1].update({"OptionPoolID":"OPT_ACC","BaseAttack":3,"EnhanceAttackPerLevel":1,"EnhanceBaseCost":100,"EnhanceBaseSuccessRate":1})
add("EQP_NCK_001", "구리 목걸이", "Equipment", "Equipment", "Necklace", 1, max_enhance=10, max_options=4)
rows[-1].update({"OptionPoolID":"OPT_ACC","BaseDefense":2,"EnhanceDefensePerLevel":1,"EnhanceBaseCost":100,"EnhanceBaseSuccessRate":1})

with csv_path.open("w", encoding="utf-8-sig", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)

asset_path = "/Game/IsekaiClass/Data/Items/DT_IC_Item"
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    data_table = unreal.EditorAssetLibrary.load_asset(asset_path)
else:
    row_struct = unreal.find_object(None, "/Script/IsekaiClass.PlayerItemDefinition")
    if not row_struct:
        raise RuntimeError("FPlayerItemDefinition ScriptStruct was not found")
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    data_table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DT_IC_Item", "/Game/IsekaiClass/Data/Items", unreal.DataTable, factory
    )

if not data_table:
    raise RuntimeError("DT_IC_Item could not be created")

if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(data_table, str(csv_path)):
    raise RuntimeError("DT_IC_Item CSV import failed")

unreal.EditorAssetLibrary.save_loaded_asset(data_table, only_if_is_dirty=False)
row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(data_table)
if len(row_names) != len(rows):
    raise RuntimeError(f"Expected {len(rows)} rows, got {len(row_names)}")

unreal.log(f"DT_IC_Item created and verified with {len(row_names)} rows")
