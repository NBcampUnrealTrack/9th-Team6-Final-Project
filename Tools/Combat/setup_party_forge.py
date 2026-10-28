"""Author only revival, additive party synergies, enhancement ore rules and their UI."""
import copy,csv,io,json,shutil
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass';BASE=GAME+'/Data/DataTable/'
OUT=ROOT/'Saved/PartyForge';OUT.mkdir(parents=True,exist_ok=True)
TOOLS=u.AssetToolsHelpers.get_asset_tools();TABLES={};REPORT={'tables':[],'widgets':[],'backups':[]}
def backup(path):
    relative=path.removeprefix('/Game/')+'.uasset';source=ROOT/'Content'/relative;dest=OUT/'Before/Content'/relative
    if source.exists() and not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest);REPORT['backups'].append(str(source))
def save(asset):assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset
def table(relative):
    if relative not in TABLES:
        path=BASE+relative;backup(path);a=u.load_asset(path);assert a,path
        TABLES[relative]=(a,{r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(a))})
    return TABLES[relative][1]
strings_path=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
STRINGS={r['Key']:r['SourceString'] for r in csv.DictReader(strings_path.open(encoding='utf-8-sig',newline=''))}
registry=table('Registry/DT_IC_IDRegistry');reg_template=copy.deepcopy(next(iter(registry.values())))
def meta(row,identifier,label,description):
    row.update(Name=identifier,ID=identifier,NameKey=identifier+'.NAME',DescriptionKey=identifier+'.DESCRIPTION',bDeprecated=False,bTestOnly=False)
    STRINGS[identifier+'.NAME']=label;STRINGS[identifier+'.DESCRIPTION']=description;return row
def register(identifier,kind,relative):
    r=copy.deepcopy(reg_template);r.update(Name=identifier,ID=identifier,DefinitionType=kind,RegionID='R01',ReplacementID='None',DefinitionTable=BASE+relative+'.'+relative.rsplit('/',1)[-1],NameKey=identifier+'.NAME',DescriptionKey=identifier+'.DESCRIPTION',bDeprecated=False,bTestOnly=False);registry[identifier]=r

items=table('Items/DT_IC_Item');identifier='ITM_CONS_REVIVE_SCROLL'
r=copy.deepcopy(items['ITM_CONS_001']);meta(r,identifier,'부활서','전투불능인 동료 한 명을 최대 HP의 50%, MP 최소 30%로 소생시킵니다. 살아 있는 대상에게는 사용할 수 없습니다.')
r.update(ItemID=identifier,DisplayName=f'LOCTABLE("{BASE}StringTables/ST_IC_Definitions.ST_IC_Definitions", "{identifier}.NAME")',Kind='Consumable',Category='Revival',UseEffect='Revival',UseTarget='DownedCompanion',bAllowNonCompanionRecruitUse=False,RestoreHP=0,RestoreMP=0,RestoreSP=0,RevivalHPRatio=.5,RevivalMinimumMPRatio=.3,BuyPrice=5000,SellPrice=2500,bCanGift=False,StackLimit=99)
r['Icon']=GAME+'/UI/SampleUI/MorpheusGUI/textures/Icons/64/t_Scroll_64.t_Scroll_64';items[identifier]=r;register(identifier,'Item','Items/DT_IC_Item')
shops=table('NPC/DT_IC_Shop')
for shop in shops.values():
    entries=[e for e in shop['Entries'] if e['ItemID']!=identifier];entries.append(dict(ItemID=identifier,DisplayOrder=5));shop['Entries']=entries

synergies=table('Combat/DT_IC_Synergy')
def synergy(identifier,label,description,pct,minimum,pool=None,family='None',group=None,priority=0):
    r=meta({},identifier,label,description);r.update(MemberNPCIDs=pool or [],MinimumMembers=minimum,RequiredWeaponFamily=family,bCountPlayer=family!='None',ExclusiveGroup=group or identifier,Priority=priority,bAdditiveWithOtherGroups=True,ActivationConditions={'Policy':'All','Conditions':[]},StatModifiers=[dict(Stat=s,Flat=0,Percent=pct) for s in ('PhysicalAttack','MagicAttack')],Icon='/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture',ActivationVFX='None');synergies[identifier]=r;register(identifier,'Synergy','Combat/DT_IC_Synergy')
for family,label in [('Sword','검사'),('Bow','궁수'),('Staff','마법사'),('Wand','힐러')]:
    synergy('SYN_CLASS_'+family.upper(),label+' 3인','살아 있는 같은 직업 파티원 3명 이상: 파티 전체 물리·마법 공격력 +20%. 플레이어의 현재 무기 직업을 포함합니다.',.2,3,family=family)
heroes=[f'NPC_R01_{n:03d}' for n in (1,2,3,4,8)]
synergy('SYN_HERO_DOUBLE','용사파티 더블','용사파티 구성원 2명: 파티 전체 물리·마법 공격력 +10%. 트리플과 중복되지 않습니다.',.1,2,heroes,group='HeroParty',priority=2)
synergy('SYN_HERO_TRIPLE','용사파티 트리플','용사파티 구성원 3명 이상: 파티 전체 물리·마법 공격력 +30%. 더블을 대체합니다.',.3,3,heroes,group='HeroParty',priority=3)
synergy('SYN_CLASSMATES','반친구들','R01_09, R01_10, R01_11이 함께 생존 중: 파티 전체 물리·마법 공격력 +30%.',.3,3,[f'NPC_R01_{n:03d}' for n in (9,10,11)])
synergy('SYN_VILLAGERS','마을 주민','R01_05, R01_06, R01_07이 함께 생존 중: 파티 전체 물리·마법 공격력 +20%.',.2,3,[f'NPC_R01_{n:03d}' for n in (5,6,7)])
relative='Equipment/DT_IC_EnhancementOre';path=BASE+relative;backup(path);ore=u.load_asset(path)
if not ore:
    factory=u.DataTableFactory();factory.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.ICEnhancementOreRow'));ore=TOOLS.create_asset('DT_IC_EnhancementOre',BASE+'Equipment',u.DataTable,factory)
assert ore
TABLES[relative]=(ore,{f'ORE_{n:02d}':dict(Name=f'ORE_{n:02d}',ItemID=f'ITM_R01_ORE_{n:02d}',Count=1,SuccessBonus=(n-1)*.05) for n in range(1,7)})

backup(BASE+'StringTables/ST_IC_Definitions')
new_strings=OUT/'ST_IC_Definitions.csv'
with new_strings.open('w',encoding='utf-8',newline='') as f:
    w=csv.writer(f,lineterminator='\n');w.writerow(['Key','SourceString']);w.writerows(STRINGS.items())
strings=u.load_asset(BASE+'StringTables/ST_IC_Definitions');assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(new_strings));save(strings)
for relative,(asset,rows) in TABLES.items():
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset,json.dumps(list(rows.values()),ensure_ascii=False)),relative
    save(asset);output=OUT/'Exports'/(relative+'.csv');output.parent.mkdir(parents=True,exist_ok=True)
    exported=u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset);reader=csv.DictReader(io.StringIO(exported));buffer=io.StringIO(newline='');writer=csv.DictWriter(buffer,fieldnames=reader.fieldnames,lineterminator='\n');writer.writeheader();writer.writerows(reader);output.write_text(buffer.getvalue(),encoding='utf-8')
    REPORT['tables'].append(relative)

# UI material desaturates pixels, so colored NPC portraits become genuinely gray.
path=GAME+'/UI/Materials/M_UI_DownedPortrait';backup(path);mat=u.load_asset(path)
material_script=ROOT/'Tools/Combat/repair_party_portrait.py'
exec(compile(material_script.read_text(encoding='utf-8'),str(material_script),'exec'),{'__name__':'party_portrait_setup'})

widgets=[('UI/Widget/WBP_IC_Enhancement',u.ICEnhancementWidget,'Enhancement'),('Life/Crafting/WBP_IC_CraftingStation',u.ICCraftingStationWidget,'CraftingStation'),('Life/Cooking/WBP_IC_CookingStation',u.ICCraftingStationWidget,'CraftingStation'),('UI/Widget/WBP_IC_InventoryItemPopup',u.ICInventoryItemPopupWidget,'InventoryItemPopup'),('UI/Widget/WBP_IC_InventoryMenu',u.ICInventoryMenuWidget,'InventoryMenu'),('UI/Widget/WBP_IC_PartyMenu',u.ICPartyMenuWidget,'PartyMenu'),('UI/Widget/WBP_IC_PlayerStatus',u.ICCharacterStatusWidget,'PlayerStatus'),('UI/Widget/WBP_IC_CompanionStatus',u.ICCharacterStatusWidget,'CompanionStatus')]
for relative,parent,kind in widgets:
    path=GAME+'/'+relative;backup(path);bp=u.load_asset(path)
    if not bp:
        f=u.WidgetBlueprintFactory();f.set_editor_property('parent_class',parent);bp=TOOLS.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.WidgetBlueprint,f)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind),path;u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    REPORT['widgets'].append(dict(path=path,names=[str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)]))
(OUT/'Setup.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8');u.log('PARTY_FORGE_ASSETS_OK')
