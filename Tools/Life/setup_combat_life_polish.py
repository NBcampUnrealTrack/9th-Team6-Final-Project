"""Targeted, repeatable authoring for the combat/life/controller polish."""
import unreal as u,json,pathlib,copy,csv
ROOT=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));GAME='/Game/IsekaiClass';BASE=GAME+'/Data/DataTable/'
TOOLS=u.AssetToolsHelpers.get_asset_tools();ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem)
REPORT={'animations':[],'chests':[],'furniture':[],'widgets':[]};TABLES={}
def save(a):
    if isinstance(a,u.Blueprint):u.BlueprintEditorLibrary.compile_blueprint(a)
    assert u.EditorAssetLibrary.save_loaded_asset(a,False),a.get_path_name()
def table(relative,struct=None):
    if relative not in TABLES:
        a=u.load_asset(BASE+relative)
        if not a:
            folder,name=(BASE+relative).rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
            f=u.DataTableFactory();f.set_editor_property('struct',u.load_object(None,'/Script/IsekaiClass.'+struct));a=TOOLS.create_asset(name,folder,u.DataTable,f)
        assert a,relative
        TABLES[relative]=(a,{r['Name']:r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(a))})
    return TABLES[relative][1]
chicken=table('Life/DT_Ranching')['RAN_R01_01']
chicken.update(DeathAnimation=GAME+'/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Animations/Chicken/AS_Chicken_Death.AS_Chicken_Death',SlaughterSeconds=1.5)
table('Life/DT_IC_SleepPresentation','ICSleepPresentationRow').setdefault('Default',dict(Name='Default',EnterSeconds=.8,FadeOutSeconds=.25,BlackHoldSeconds=.08,FadeInSeconds=.25,CameraOffset='(X=420,Y=450,Z=300)',CameraFocusOffset='(X=0,Y=0,Z=70)',FieldOfView=70))

# Keep the original body/material; SM_chest_03 is the original pack's separate lid.
lid=u.load_asset(GAME+'/World/SampleLevel/Medieval_House/meshes/props/SM_chest_03');assert lid
def chest(actor):
    hinge=actor.get_editor_property('ChestPresentation');assert hinge
    hinge.set_editor_property('HingeOffset',u.Vector(0,-40,50));hinge.set_editor_property('LidClosedOffset',u.Vector(0,-40,50))
    hinge.set_editor_property('OpenRotation',u.Rotator(roll=-105));hinge.set_editor_property('AnimationSeconds',.7)
    visual=hinge.get_editor_property('Lid');visual.set_editor_property('static_mesh',lid)
    visual.set_material(0,u.load_asset(GAME+'/World/SampleLevel/Medieval_House/materials/props_materials/MI_chest_Inst'))
    REPORT['chests'].append(actor.get_path_name())
for path in [GAME+'/Life/Home/Blueprints/BP_IC_PlayerStorage',GAME+'/NPC/Blueprints/Containers/BP_IC_NPCChest']:
    bp=u.load_asset(path);chest(u.get_default_object(bp.generated_class()));save(bp)

definitions=ROOT/'Data/StringTables/ST_IC_Definitions.csv'
strings={r['Key']:r['SourceString'] for r in csv.DictReader(definitions.open(encoding='utf-8-sig',newline=''))}
registry=table('Registry/DT_IC_IDRegistry');drops=table('Rewards/DT_IC_DropTable')
for row_name,dropid,label,items in [
    ('Bookshelf','DRP_FURNITURE_BOOKSHELF','책장 보상',[f'ITM_ELIXIR_{n:03d}' for n in [1,2,3,4,5,6,7,8,9,10,11,12,16,17,18]]),
    ('Wardrobe','DRP_FURNITURE_WARDROBE','옷장 보상',['ITM_CONS_001']+[f'ITM_R01_SEED_{n:02d}' for n in range(1,6)])]:
    table('Life/DT_IC_FurnitureLoot','ICFurnitureLootRow').setdefault(row_name,dict(Name=row_name,DisplayName=label,DropTableID=dropid,RefillGameMinutes=0))
    if dropid not in drops:
        row=copy.deepcopy(drops['DRP_R01_NORMAL_001']);entry=copy.deepcopy(row['Entries'][0])
        row.update(Name=dropid,ID=dropid,NameKey=dropid+'.NAME',Selection='WeightedChoice',Rolls=1,MinimumGold=0,MaximumGold=0,bAutoCollectItems=True,
            Entries=[dict(entry,ItemID=item,MinimumCount=1,MaximumCount=1,Weight=1,Probability=1) for item in items]);drops[dropid]=row
    if dropid not in registry:
        row=copy.deepcopy(registry['DRP_R01_CHICKEN']);row.update(Name=dropid,ID=dropid,NameKey=dropid+'.NAME');registry[dropid]=row
    strings[dropid+'.NAME']=label
    bp=u.load_asset(GAME+'/NPC/Blueprints/Containers/BP_IC_NPC'+row_name)
    cdo=u.get_default_object(bp.generated_class());cdo.set_editor_property('bPickupOnly',True);cdo.set_editor_property('FurnitureLootRow',row_name);save(bp)

# Empty icons previously collapsed valid quickslots. Supply editable defaults only where absent.
icons=GAME+'/UI/SampleUI/MorpheusGUI/textures/Icons/64/'
for row in table('Items/DT_IC_Item').values():
    if row.get('Icon') not in ('None','',None):continue
    item=row['ID'];icon='t_PotionStrength_64' if item.startswith('ITM_ELIXIR') else 't_Backpack_64' if item=='ITM_TKT_001' else 't_PotionHeart_64' if item.startswith('ITM_CONS') else 't_Apple_64' if 'FOOD' in item or 'RANCH' in item or 'CHICKEN_MEAT' in item else None
    if icon:
        tex=u.load_asset(icons+icon);assert tex;row['Icon']=tex.get_path_name()

# Source-rig identity includes the mesh, since this pack includes UE4 and UE5 skeletons.
helper={};path=ROOT/'Tools/Characters/setup_assets.py'
exec(compile(path.read_text(encoding='utf-8').split('\nif __name__ ==')[0],str(path),'exec'),helper)
def source_rig(pack,mesh):
    path=GAME+'/Animation/Common/Retarget/IK_SRC_PlayerLife_'+mesh.get_name();a=u.load_asset(path)
    if not a:a=TOOLS.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],u.IKRigDefinition,u.IKRigDefinitionFactory())
    c=u.IKRigController.get_controller(a);assert c.set_skeletal_mesh(mesh);assert c.apply_auto_generated_retarget_definition();save(a);return a
helper['source_rig']=source_rig;pack='505_Survival_Mocap_Animations'
sleep={name:u.load_asset(GAME+'/Animation/SampleAnimation/'+pack+'/Animations_UE4/IP/Sleep/IP_sleep_bed_'+suffix) for name,suffix in [('Enter','start'),('Loop','loop'),('Exit','end')]}
air={name:u.load_asset(GAME+'/Animation/SampleAnimation/'+pack+'/Retargeting/Characters/Mannequins/Animations/Manny/'+name) for name in ['MM_Jump','MM_Fall_Loop','MM_Land']}
assert all(sleep.values()) and all(air.values())
for gender,code in [('Male','M'),('Female','F')]:
    folder=GAME+'/Animation/Player/Main'+gender;mesh=u.load_asset(GAME+'/Player/SamplePlayer/'+gender+'/SK_Player_'+code);rig=u.load_asset(GAME+'/Player/SamplePlayer/'+gender+'/IK_Player_'+code+'_Mannequin')
    sleeping=helper['retarget'](pack,sleep,mesh,rig,folder+'/SleepCinematic')
    airborne=helper['retarget'](pack,air,mesh,rig,folder+'/UnarmedAir')
    for clip in list(sleeping.values())+list(airborne.values()):
        clip.set_editor_property('force_root_lock',True);clip.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.REF_POSE);save(clip)
    profile=u.load_asset(folder+'/DA_ANIM_PLAYER_'+code)
    profile.set_editor_property('SleepEnter',sleeping['Enter']);profile.set_editor_property('Sleep',sleeping['Loop']);profile.set_editor_property('SleepExit',sleeping['Exit'])
    profile.set_editor_property('SleepMeshTransform',u.Transform(location=u.Vector(-55,-36,-31 if code=='M' else -25),rotation=u.Rotator(yaw=0)))
    unarmed=profile.get_editor_property('Unarmed');unarmed.set_editor_property('JumpStart',airborne['MM_Jump']);unarmed.set_editor_property('Jump',airborne['MM_Fall_Loop']);unarmed.set_editor_property('FallLoop',airborne['MM_Fall_Loop']);unarmed.set_editor_property('Land',airborne['MM_Land']);profile.set_editor_property('Unarmed',unarmed);save(profile)
    REPORT['animations'].append({'gender':gender,'sleep':{k:v.get_path_name() for k,v in sleeping.items()},'jump':{k:v.get_path_name() for k,v in airborne.items()}})

imc=u.load_asset(GAME+'/Input/Contexts/IMC_Gameplay');data=imc.get_editor_property('default_key_mappings');mappings=list(data.get_editor_property('mappings'))
def key(name):
    value=u.Key();value.set_editor_property('key_name',name);return value
for name,axis in [('IA_Move','Gamepad_Left2D'),('IA_Look','Gamepad_Right2D')]:
    action=u.load_asset(GAME+'/Input/Actions/'+name);assert action
    mappings=[m for m in mappings if not(m.action==action and str(m.key.get_editor_property('key_name')).startswith('Gamepad_'))]
    dead=u.new_object(u.InputModifierDeadZone,outer=imc);dead.set_editor_property('lower_threshold',.15)
    mappings.append(u.EnhancedActionKeyMapping(action=action,key=key(axis),modifiers=[dead]))
data.set_editor_property('mappings',mappings);imc.set_editor_property('default_key_mappings',data);save(imc)
for name,kind in [('WBP_IC_QuickSlot','QuickSlot'),('WBP_IC_QuickSlots','QuickSlots'),('WBP_IC_Settings','Settings')]:
    bp=u.load_asset(GAME+'/UI/Widget/'+name);assert bp and u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind);save(bp);REPORT['widgets'].append(name)

u.EditorLoadingAndSavingUtils.load_map(GAME+'/World/L_TestMap')
for actor in ACTORS.get_all_level_actors():
    if isinstance(actor,u.ICPlayerStorage):chest(actor)
    elif isinstance(actor,u.ICNPCContainerActor):
        class_name=actor.get_class().get_name()
        if 'Bookshelf' in class_name or 'Wardrobe' in class_name:
            row='Bookshelf' if 'Bookshelf' in class_name else 'Wardrobe';actor.set_editor_property('bPickupOnly',True);actor.set_editor_property('FurnitureLootRow',row);REPORT['furniture'].append(actor.get_path_name())
        else:chest(actor)
assert u.EditorLoadingAndSavingUtils.save_current_level()
for relative,(asset,rows) in TABLES.items():
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset,json.dumps(list(rows.values()),ensure_ascii=False)),relative
    save(asset);dest=ROOT/'Saved/Polish/ExportedCSV'/(relative+'.csv');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset),encoding='utf-8-sig')
with definitions.open('w',encoding='utf-8-sig',newline='') as stream:
    w=csv.writer(stream);w.writerow(['Key','SourceString']);w.writerows(strings.items())
st=u.load_asset(BASE+'StringTables/ST_IC_Definitions');assert u.ICTextLibrary.replace_text_sources(st,strings);save(st)
out=ROOT/'Saved/Polish';out.mkdir(exist_ok=True);(out/'Authored.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8');u.log('COMBAT_LIFE_POLISH_SETUP_OK')
