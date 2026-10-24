"""Create quest journal WBPs and enable abandoning the existing sample quests."""
import csv
import io
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
TOOLS=u.AssetToolsHelpers.get_asset_tools()
SAMPLES={'QST_RING_001','QST_TEST_TALK_001','QST_TEST_DELIVER_001','QST_TEST_COLLECT_001','QST_TEST_AFFINITY_001','QST_TEST_STAT_001','QST_TEST_VISIT_001','QST_TEST_POSSESS_001'}
def allow_sample_abandon(text):
    reader=csv.DictReader(io.StringIO(text));rows=list(reader)
    for row in rows:
        if row[reader.fieldnames[0]] in SAMPLES:row['bAllowAbandon']='True'
    output=io.StringIO(newline='');writer=csv.DictWriter(output,reader.fieldnames,lineterminator='\n')
    writer.writeheader();writer.writerows(rows);return output.getvalue()
def save(asset):assert u.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)

table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Quest/DT_IC_Quest')
assert table
current=u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,allow_sample_abandon(current))
save(table)
source=ROOT/'Data/Quest/DT_IC_Quest.csv'
source.write_text(allow_sample_abandon(source.read_text(encoding='utf-8-sig')),encoding='utf-8',newline='')

folder='/Game/IsekaiClass/UI/Widget'
for name,parent,kind in (
    ('WBP_IC_QuestMenuEntry',u.ICQuestMenuEntryWidget,'QuestMenuEntry'),
    ('WBP_IC_QuestDetail',u.ICQuestDetailWidget,'QuestDetail'),
    ('WBP_IC_QuestMenu',u.ICQuestMenuWidget,'QuestMenu'),
    ('WBP_IC_GameMenu',u.ICGameMenuWidget,'GameMenu'),
):
    bp=u.load_asset(folder+'/'+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=TOOLS.create_asset(name,folder,u.WidgetBlueprint,factory)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
    if kind=='QuestDetail':assert {'TXT_Name','TXT_Description','TXT_Objectives','TXT_Rewards','BTN_Abandon','BTN_Start','BTN_Back'}<=names
    if kind=='QuestMenu':assert {'QuestEntries','QuestDetail','QuestPages'}<=names
    if kind=='GameMenu':assert {'QuestContent','PartyContent','InventoryContent'}<=names
for name in ('WBP_GameHUD','WBP_IC_QuestTracker'):
    bp=u.load_asset(folder+'/'+name);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
u.log('IC_QUEST_MENU_SETUP_OK')
