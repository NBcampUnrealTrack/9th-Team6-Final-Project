"""Reuse the team's Warp Stone, author three endpoints and map WBPs, and fix every chest hinge."""
import unreal as u,json,pathlib,shutil,hashlib,csv
ROOT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));GAME='/Game/IsekaiClass';BASE=GAME+'/Data/DataTable/'
OUT=ROOT/'Saved/MapTravel';OUT.mkdir(parents=True,exist_ok=True)
TOOLS=u.AssetToolsHelpers.get_asset_tools();ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem);TABLES={};REPORT={}
targets=['Content/IsekaiClass/World/L_TestMap.umap','Content/IsekaiClass/World/Blueprints/BP_ICWarpStone.uasset','Content/IsekaiClass/UI/Widget/WBP_IC_Minimap.uasset','Content/IsekaiClass/Life/Home/Blueprints/BP_IC_PlayerStorage.uasset','Content/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCChest.uasset']
targets += ['Content/IsekaiClass/Data/DataTable/'+p+'.uasset' for p in ['World/DT_IC_Warp','World/DT_IC_Anchor','World/DT_IC_Map','World/DT_IC_Region','Registry/DT_IC_IDRegistry']]
targets += ['Content/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.uasset','Data/StringTables/ST_IC_Definitions.csv']
backup=OUT/'Before';manifest=[]
for rel in targets:
    src=ROOT/rel;dest=backup/rel
    if src.exists():
        if not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dest)
        manifest.append(dict(path=rel,sha256=hashlib.sha256(dest.read_bytes()).hexdigest()))
(backup/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
def save(a):
    if isinstance(a,u.Blueprint):u.BlueprintEditorLibrary.compile_blueprint(a)
    assert u.EditorAssetLibrary.save_loaded_asset(a,False),a.get_path_name()
def table(rel,struct=None):
    if rel not in TABLES:
        a=u.load_asset(BASE+rel)
        if not a:
            folder,name=(BASE+rel).rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
            f=u.DataTableFactory();f.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.'+struct));a=TOOLS.create_asset(name,folder,u.DataTable,f)
        assert a
        TABLES[rel]=(a,{r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(a))})
    return TABLES[rel][1]
def definition(id):return dict(Name=id,ID=id,NameKey=id+'.NAME',DescriptionKey='',DataVersion=1,bDeprecated=False,bTestOnly=False)
registry=table('Registry/DT_IC_IDRegistry')
def register(id,kind,rel):
    registry[id]=dict(definition(id),DefinitionType=kind,RegionID='R01',DefinitionTable=BASE+rel+'.'+rel.rsplit('/',1)[-1],Notes='Test map capture / paid portal travel')
warps=table('World/DT_IC_Warp');anchors=table('World/DT_IC_Anchor')
points=[('01','왼쪽 포탈','LEFT',-17500),('02','중앙 포탈','CENTER',1000),('03','오른쪽 포탈','RIGHT',19500)]
u.EditorLoadingAndSavingUtils.load_map(GAME+'/World/L_TestMap')
existing=ACTORS.get_all_level_actors();bylabel={a.get_actor_label():a for a in existing}
bp=u.load_asset(GAME+'/World/Blueprints/BP_ICWarpStone');assert bp
warpids=[];anchorids=[]
for code,label,english,y in points:
    wid='WRP_TEST_'+code;visit='ANC_WRP_TEST_VISIT_'+code;arrival='ANC_WRP_TEST_ARRIVAL_'+code;warpids.append(wid);anchorids += [visit,arrival]
    warps[wid]=dict(definition(wid),RegionID='R01',VisitAnchorID=visit,ArrivalAnchorID=arrival,VisitRadiusCm=350)
    register(wid,'Warp','World/DT_IC_Warp')
    for aid,x,safe in [(visit,0,False),(arrival,-250,True)]:
        tf=f'(Rotation=(X=0,Y=0,Z=0,W=1),Translation=(X={x},Y={y},Z=96),Scale3D=(X=1,Y=1,Z=1))'
        anchors[aid]=dict(definition(aid),Type='WarpArrival' if safe else 'Transit',RegionID='R01',WorldTransform=tf,bSafeForArrival=safe,ClearanceRadiusCm=50,ClearanceHalfHeightCm=96)
        register(aid,'Anchor','World/DT_IC_Anchor')
        a=bylabel.get(aid) or ACTORS.spawn_actor_from_class(u.ICWorldAnchor,u.Vector(x,y,96))
        a.set_actor_label(aid);a.set_actor_location(u.Vector(x,y,96),False,True);a.set_editor_property('AnchorID',aid)
    a=bylabel.get('IC_TestPortal_'+code) or ACTORS.spawn_actor_from_class(bp.generated_class(),u.Vector(0,y,0))
    a.set_actor_label('IC_TestPortal_'+code);a.set_actor_location(u.Vector(0,y,0),False,True)
    a.set_editor_property('WarpID',wid);a.set_editor_property('CostGold',500);a.set_editor_property('InteractionText',label)
    a.get_editor_property('Label').set_text(english+' PORTAL')
maps=table('World/DT_IC_Map')
maps['MAP_TEST_001']=dict(definition('MAP_TEST_001'),Type='Town',RegionID='R01',VisibleMarkers=['Player','Shop','Warp','Quest'],MinimumZoom=1,MaximumZoom=4,
    Layers=[dict(InteriorFloorID='None',DisplayNameKey='MAP_TEST_001.NAME',MapTexture=GAME+'/UI/Map/T_TestMap_Overview.T_TestMap_Overview',
        WorldOriginCm='(X=10500,Y=-20000)',WorldAxisU='(X=0,Y=1)',WorldAxisV='(X=-1,Y=0)',MapOriginUV='(X=0,Y=0)',WorldCmPerUV='(X=42000,Y=21000)',WorldBoundsMinCm='(X=-10500,Y=-20000)',WorldBoundsMaxCm='(X=10500,Y=22000)')])
register('MAP_TEST_001','Map','World/DT_IC_Map')
table('Settings/DT_IC_MapPresentation','ICMapPresentationRow').setdefault('Default',dict(Name='Default',MapID='MAP_TEST_001',TravelCostGold=500,LoadingSeconds=1.2,bUnlockPlacedPortals=True))
regions=table('World/DT_IC_Region')
r=regions['R01'];r['WarpIDs']=list(dict.fromkeys(r.get('WarpIDs',[])+warpids));r['AnchorIDs']=list(dict.fromkeys(r.get('AnchorIDs',[])+anchorids));r['TownMapID']='MAP_TEST_001'
r['BoundsCm']='(Min=(X=-10000,Y=-19000,Z=-500),Max=(X=10000,Y=21000,Z=3000),IsValid=1)'
r['BoundaryPolygonCm']=['(X=-10000,Y=-19000)','(X=10000,Y=-19000)','(X=10000,Y=21000)','(X=-10000,Y=21000)']
# Runtime and BP instances used to override the correct native -105 roll with +105.
for path in [GAME+'/Life/Home/Blueprints/BP_IC_PlayerStorage',GAME+'/NPC/Blueprints/Containers/BP_IC_NPCChest']:
    b=u.load_asset(path);u.get_default_object(b.generated_class()).get_editor_property('ChestPresentation').set_editor_property('OpenRotation',u.Rotator(roll=-105));save(b)
chests=[]
for a in ACTORS.get_all_level_actors():
    if isinstance(a,(u.ICPlayerStorage,u.ICNPCContainerActor)):
        h=a.get_editor_property('ChestPresentation')
        if h:h.set_editor_property('OpenRotation',u.Rotator(roll=-105));chests.append(a.get_actor_label())
assert u.EditorLoadingAndSavingUtils.save_current_level()
for rel,(a,rows) in TABLES.items():
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(a,json.dumps(list(rows.values()),ensure_ascii=False)),rel
    save(a);dest=OUT/'ExportedCSV'/(rel+'.csv');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(a),encoding='utf-8-sig')
for name,mini in [('WBP_IC_Map',False),('WBP_IC_Minimap',True)]:
    path=GAME+'/UI/Widget/'+name;b=u.load_asset(path)
    if not b:
        f=u.WidgetBlueprintFactory();f.set_editor_property('parent_class',u.ICMapWidget);b=TOOLS.create_asset(name,GAME+'/UI/Widget',u.WidgetBlueprint,f)
    else:u.BlueprintEditorLibrary.reparent_blueprint(b,u.ICMapWidget)
    u.get_default_object(b.generated_class()).set_editor_property('bMinimap',mini)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(b,'Minimap' if mini else 'Map');save(b)
# Keep authored definition labels localizable through the existing string table.
strings_path=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
strings={r['Key']:r['SourceString'] for r in csv.DictReader(strings_path.open(encoding='utf-8-sig'))}
strings['MAP_TEST_001.NAME']='테스트맵'
for code,label,english,y in points:strings['WRP_TEST_'+code+'.NAME']=label
st=u.load_asset(BASE+'StringTables/ST_IC_Definitions');assert u.ICTextLibrary.replace_text_sources(st,strings);save(st)
with strings_path.open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['Key','SourceString']);w.writerows(strings.items())
(OUT/'Authored.json').write_text(json.dumps(dict(portals=warpids,chests=chests,bounds=dict(center=[0,1000],width=42000,height=21000),ai_images_used=False),ensure_ascii=False,indent=2),encoding='utf-8')
imc=u.load_asset(GAME+'/Input/Contexts/IMC_Gameplay')
mapping=imc.get_editor_property('default_key_mappings')
assert any(m.action and m.action.get_name()=='IA_Map' and str(m.key.get_editor_property('key_name'))=='M' for m in mapping.get_editor_property('mappings')),'Existing M map binding is missing'
u.log('MAP_TRAVEL_SETUP_OK')
