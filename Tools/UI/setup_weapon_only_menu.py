"""Retire non-weapon gear, author ten NPC talk quests, and rebuild the changed menu WBPs."""
import csv
import io
import re
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
STAGING=ROOT/'Saved/WeaponOnlyMenu/Data'
BASE='/Game/IsekaiClass/Data/DataTable/'
RETIRED=('EQP_ARM_','EQP_RNG_','EQP_NCK_','EQU_ARM_','EQU_RNG_','EQU_NCK_','ENH_ARM_','ENH_RNG_','ENH_NCK_')
def retired(id):return str(id).startswith(RETIRED) or str(id) in ('OPT_ARM','OPT_ACC')
def save(asset):assert u.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
def read_table(relative):
    asset=u.load_asset(BASE+relative);assert asset,relative
    reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)))
    return asset,reader.fieldnames,list(reader)
def write_table(relative,asset,fields,rows):
    output=io.StringIO(newline='');writer=csv.DictWriter(output,fieldnames=fields,lineterminator='\n')
    writer.writeheader();writer.writerows(rows);text=output.getvalue()
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset,text),relative
    save(asset);destination=STAGING/(relative+'.csv');destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(text,encoding='utf-8',newline='')
def meta(fields,id):
    row={key:'' for key in fields};row.update({fields[0]:id,'ID':id,'NameKey':id+'.NAME','DescriptionKey':'','Tags':'(GameplayTags=)','DataVersion':'1','bDeprecated':'False','bTestOnly':'False'});return row

for relative in ('Items/DT_IC_Item','Equipment/DT_IC_Equipment','Equipment/DT_IC_Enhancement','Equipment/DT_IC_OptionPool','Equipment/DT_IC_Option','Character/DT_IC_Class','Character/DT_IC_RecruitCombat','Settings/DT_IC_NewGame','Registry/DT_IC_IDRegistry'):
    asset,fields,rows=read_table(relative);rows=[r for r in rows if not retired(r[fields[0]])]
    for row in rows:
        if relative=='Equipment/DT_IC_Option':row['AllowedSlots']='(Weapon)'
        if relative=='Character/DT_IC_Class' and row['StartingWeaponID'].startswith('EQP_'):row['StartingWeaponID']=row['StartingWeaponID'].replace('EQP_','EQU_',1)
        if relative=='Equipment/DT_IC_Equipment':assert row['Slot']=='Weapon'
        if relative=='Items/DT_IC_Item' and row['Kind']=='Equipment':assert row['EquipSlot']=='Weapon'
        if relative=='Settings/DT_IC_NewGame':row['StartingItems']='('+','.join(part for part in re.findall(r'\([^()]*\)',row['StartingItems']) if not any(prefix in part for prefix in RETIRED))+')'
    write_table(relative,asset,fields,rows)

_,_,npcs=read_table('NPC/DT_IC_NPC');anchors={r['ID']:r['InitialAnchorID'] for r in npcs}
reward_id='RWD_NPC_TALK_100'
asset,fields,rewards=read_table('Rewards/DT_IC_Reward');rewards=[r for r in rewards if r['ID']!=reward_id]
reward=meta(fields,reward_id);reward.update({'Gold':'100','Items':'','Affinity':'','UnlockRecipeIDs':'','Overflow':'RejectAll'});rewards.append(reward)
write_table('Rewards/DT_IC_Reward',asset,fields,rewards)

asset,fields,quests=read_table('Quest/DT_IC_Quest');quests=[r for r in quests if not r['ID'].startswith('QST_NPC_TALK_')]
names={reward_id+'.NAME':'대화 퀘스트 보상'};quest_text={}
for index in range(1,11):
    id='QST_NPC_TALK_%03d'%index;npc='NPC_R01_%03d'%index;target='NPC_R01_%03d'%(index+1)
    row=meta(fields,id);row.update({'QuestType':'General','bTutorial':'False','IsAutoAccept':'False','bListBeforeAccept':'False','RegionID':'R01','OfferNPCID':npc,'OfferAnchorID':anchors[npc],
        'ObjectiveGroupPolicy':'All','ObjectiveGroups':'((Policy=All,Objectives=((ObjectiveID="OBJ_TALK",DescriptionKey="%s.OBJECTIVE",Type=Talk,Target=(Type=NPC,ID="%s"),RequiredCount=1,GuidanceAnchorID="%s"))))'%(id,target,anchors[target]),
        'CompletionMode':'SystemEvent','TurnInNPCID':'None','TurnInAnchorID':'None','RewardID':reward_id,'bEndingQuest':'False','bAllowAbandon':'True','bRepeatable':'False',
        'OfferDialogueID':'None','ActiveDialogueID':'None','ReadyDialogueID':'None','CompletedDialogueID':'None','DescriptionKey':id+'.DESCRIPTION'})
    quests.append(row)
    quest_text.update({id+'.NAME':'퀘스트 %d'%index,id+'.DESCRIPTION':'%d번 NPC의 부탁입니다. %d번 NPC와 대화하세요.'%(index,index+1),id+'.OBJECTIVE':'%d번 NPC와 대화하기'%(index+1)})
write_table('Quest/DT_IC_Quest',asset,fields,quests)

asset,fields,registry=read_table('Registry/DT_IC_IDRegistry')
for id,kind,relative in [(q['ID'],'Quest','Quest/DT_IC_Quest') for q in quests if q['ID'].startswith('QST_NPC_TALK_')]+[(reward_id,'Reward','Rewards/DT_IC_Reward')]:
    registry=[r for r in registry if r['ID']!=id];row=meta(fields,id)
    row.update({'DefinitionType':kind,'RegionID':'R01','ReplacementID':'None','DefinitionTable':BASE+relative+'.'+relative.rsplit('/',1)[1],'Notes':'NPC dialogue quest prototype.'});registry.append(row)
write_table('Registry/DT_IC_IDRegistry',asset,fields,registry)

for name,entries in (('ST_IC_Quest',quest_text),('ST_IC_Definitions',names)):
    path=ROOT/'Data/StringTables'/(name+'.csv')
    reader=csv.DictReader(io.StringIO(path.read_text(encoding='utf-8-sig')));fields=reader.fieldnames;rows=[r for r in reader if not retired(r['Key'].split('.')[0])]
    by_key={r['Key']:r for r in rows}
    for key,value in entries.items():
        if key in by_key:by_key[key]['SourceString']=value
        else:rows.append({'Key':key,'SourceString':value})
    destination=STAGING/'StringTables'/(name+'.csv');destination.parent.mkdir(parents=True,exist_ok=True)
    with destination.open('w',encoding='utf-8',newline='') as file:
        writer=csv.DictWriter(file,fieldnames=fields,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    strings=u.load_asset(BASE+'StringTables/'+name);assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(destination));save(strings)

folder='/Game/IsekaiClass/UI/Widget';tools=u.AssetToolsHelpers.get_asset_tools()
for name,parent,kind in (
    ('WBP_IC_InventoryItemPopup',u.ICInventoryItemPopupWidget,'InventoryItemPopup'),
    ('WBP_IC_PartyMenu',u.ICPartyMenuWidget,'PartyMenu'),
    ('WBP_IC_InventoryMenu',u.ICInventoryMenuWidget,'InventoryMenu'),
    ('WBP_IC_GameMenu',u.ICGameMenuWidget,'GameMenu'),
):
    bp=u.load_asset(folder+'/'+name)
    if not bp:
        factory=u.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent);bp=tools.create_asset(name,folder,u.WidgetBlueprint,factory)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    widgets={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
    if kind=='InventoryMenu':assert {'ItemPopup','QuickSlotsRow','QuickSlot0','QuickSlot3'}<=widgets and 'ArmorHeading' not in widgets and 'BTN_Equip' not in widgets
    if kind=='PartyMenu':assert {'ResourcesGroup','AttributesGroup','CombatGroup'}<=widgets
for name in ('WBP_GameHUD','WBP_IC_CodexMenu','WBP_IC_QuestMenu','WBP_IC_QuestDetail'):
    bp=u.load_asset(folder+'/'+name);u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
u.log('IC_WEAPON_ONLY_MENU_SETUP_OK')
