"""Update the approved R01 character definitions without renaming existing IDs.

Run with ordinary Python. Run setup_assets.py in Unreal afterwards to import
these CSVs and generate the corresponding Blueprint / animation assets.
"""
import csv
import io
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "Data"
GAME = "/Game/IsekaiClass"
ROLES = ["Mage", "Warrior", "Healer", "Archer", "Warrior", "Mage", "Archer", "Mage", "Healer", "Warrior", "Archer"]
CLASS = {"Warrior": "CLS_WARRIOR_001", "Archer": "CLS_ARCHER_001", "Mage": "CLS_MAGE_001", "Healer": "CLS_HEALER_001"}
FAMILY = {"Warrior": "Sword", "Archer": "Bow", "Mage": "Staff", "Healer": "Wand"}
WEAPON = {"Warrior": "SWORD", "Archer": "BOW", "Mage": "STAFF", "Healer": "WAND"}
REGISTER = {}


def load(relative):
    with (DATA / relative).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        return list(reader.fieldnames), list(reader)


def save(relative, fields, rows):
    stream=io.StringIO(newline="")
    writer=csv.DictWriter(stream,fields,lineterminator="\n")
    writer.writeheader();writer.writerows(rows)
    path=DATA/relative
    value=stream.getvalue()
    if path.exists() and path.read_text(encoding="utf-8-sig")==value:return
    temporary=path.with_suffix(".csv.tmp")
    temporary.write_text(value,encoding="utf-8")
    temporary.replace(path)


def upsert(rows, row):
    key = "---" if "---" in row else "Name"
    for index, existing in enumerate(rows):
        if existing[key] == row[key]:
            rows[index] = row
            return
    rows.append(row)


def definition(fields, identifier, **values):
    row = dict.fromkeys(fields, "")
    row.update({"---": identifier, "ID": identifier, "NameKey": identifier + ".NAME", "Tags": "(GameplayTags=)", "DataVersion": "1", "bDeprecated": "False", "bTestOnly": "False"})
    row.update({key: str(value) for key, value in values.items()})
    return row


def object_path(path):
    return path + "." + path.rsplit("/", 1)[-1]


def register(identifier, kind, table):
    REGISTER[identifier] = (kind, table)


def main():
    tables = {}
    for relative in ["NPC/DT_IC_NPC.csv", "Character/DT_IC_RecruitCombat.csv", "NPC/DT_IC_Dialogue.csv", "NPC/DT_IC_Schedule.csv", "World/DT_IC_Anchor.csv", "World/DT_IC_Home.csv", "Combat/DT_IC_Skill.csv"]:
        tables[relative] = load(relative)
    for home_row in tables["World/DT_IC_Home.csv"][1]:
        if home_row["Type"] == "NPC":
            home_row["ContainerAccessAffinity"] = "200"
    fields, npcs = tables["NPC/DT_IC_NPC.csv"]
    for extra in ["bClassmate", "AnimationProfile", "MeshTransform"]:
        if extra not in fields:
            fields.append(extra)
            for row in npcs:
                row[extra] = "False" if extra == "bClassmate" else "None" if extra == "AnimationProfile" else "(Rotation=(X=0,Y=0,Z=-0.707107,W=0.707107),Translation=(X=0,Y=0,Z=-88),Scale3D=(X=1,Y=1,Z=1))"
    text_fields, texts = load("StringTables/ST_IC_Definitions.csv")
    dlg_fields, dlg_texts = load("StringTables/ST_IC_Dialogue.csv")
    skills = tables["Combat/DT_IC_Skill.csv"][1]
    source_skills = {family: next(row.copy() for row in skills if row["ID"] == f"SKL_START_{family}_001") for family in WEAPON.values()}
    for number, role in enumerate(ROLES, 1):
        npc = f"NPC_R01_{number:03}"
        display = f"R01_{number:02}"
        home = "HOME_RECRUIT_001" if number == 1 else f"HOME_R01_{number:03}"
        schedule = "SCH_RECRUIT_001" if number == 1 else f"SCH_R01_{number:03}"
        anchors = {kind: f"ANC_RECRUIT_{kind}" if number == 1 else f"ANC_R01_{number:03}_{kind}" for kind in ["HOME", "BED", "WORK"]}
        combat = "RCB_RECRUIT_001" if number == 1 else f"RCB_R01_{number:03}"
        dialogue = "DLG_RECRUIT_001" if number == 1 else f"DLG_R01_{number:03}"
        skill = f"SKL_NPC_R01_{number:03}"
        mesh = f"{GAME}/NPC/SampleNPC/R01/{npc}/SK_R01_{number}"
        profile = f"{GAME}/Animation/NPC/{npc}/DA_ANIM_{npc}"
        upsert(npcs, definition(fields, npc, Type="Recruitable", Race="Human", RegionID="R01", HomeID=home, InitialAnchorID=anchors["HOME"], ScheduleID=schedule, DialogueIDs=f'(\"{dialogue}\")', GiftPreferenceID="GIFT_R01_COMMON", ServiceIDs="()", RecruitCombatID=combat, ClassID=CLASS[role], OccupationTag="None", bRecruitable="True", bEndingTarget="False", bClassmate=str(number>=9), InitialAffinity=0, AppearanceResourceID=f"RES_{npc}", Mesh=object_path(mesh), Portrait=object_path(f"{GAME}/NPC/SampleNPC/R01/R01_portrait/R01_{number}"), AnimationClass="/Script/IsekaiClass.ICCharacterAnimInstance", AnimationProfile=object_path(profile), ActorClass=f"{GAME}/NPC/Blueprints/R01/BP_{npc}.BP_{npc}_C", ScheduleStateTree="None", MeshTransform="(Rotation=(X=0,Y=0,Z=-0.707107,W=0.707107),Translation=(X=0,Y=0,Z=-88),Scale3D=(X=1,Y=1,Z=1))"))
        cf, cr = tables["Character/DT_IC_RecruitCombat.csv"]
        upsert(cr, definition(cf, combat, NPCID=npc, ClassID=CLASS[role], WeaponFamily=FAMILY[role], Role={"Healer":"Healer","Warrior":"Frontline","Archer":"RangedDamage","Mage":"MagicDamage"}[role], InitialBaseStats="(STR=1,DEX=1,INT=1,CON=1,CHA=1,LUK=1)", InitialEquipment=f"(WeaponID=EQU_WPN_{WEAPON[role]}_001)", UniqueSkillID=skill, InitialAttackDisposition="Defensive", InitialLowHPDisposition="Wait", InitialTargetPriority="PlayerTargetFirst", bInitiallyAutoSkillEnabled="False"))
        source = source_skills[WEAPON[role]].copy()
        source.update({"---":skill,"ID":skill,"NameKey":skill+".NAME","DescriptionKey":"","OwnerType":"RecruitUnique","ClassID":"None","NPCID":npc,"bInitiallyUnlocked":"True"})
        upsert(skills, source)
        df, dr = tables["NPC/DT_IC_Dialogue.csv"]
        upsert(dr, definition(df, dialogue, NPCID=npc, EntryNodeID="GREETING", Availability="(Policy=All,Conditions=())", SelectionPriority=0, Nodes=f'((NodeID="GREETING",SpeakerNPCID="{npc}",TextKey="{dialogue}.GREETING",NextNodeID=None,Choices=(),GestureMontage=None))'))
        sf, sr = tables["NPC/DT_IC_Schedule.csv"]
        segments = [(0,360,"Sleep","BED"),(360,390,"Move","WORK"),(390,1200,"Work","WORK"),(1200,1230,"Move","HOME"),(1230,1440,"Sleep","BED")]
        parts=[]
        for index,(start,end,activity,destination) in enumerate(segments):
            departure=anchors["HOME"] if index==1 else anchors["WORK"] if index==3 else "None"
            parts.append(f'(SegmentID="S{index:02}",StartGameMinute={start},EndGameMinute={end},Activity={activity},DepartureAnchorID="{departure}",DestinationAnchorID="{anchors[destination]}",ActivityTag=(TagName="None"),ServiceIDs=(),ActivityResourceID=None)')
        upsert(sr, definition(sf,schedule,Segments="("+",".join(parts)+")",PathFailureRecoveryPlaySeconds=5))
        af, ar = tables["World/DT_IC_Anchor.csv"]
        for kind, anchor in anchors.items():
            # Match BP_R01TestLayout at (2000,0,0), yaw 180: 5 / 10 test steps of 70 cm.
            x,y=2700 if kind=="BED" else 2000,(6-number)*350
            upsert(ar,definition(af,anchor,Type={"HOME":"Home","BED":"Bed","WORK":"Work"}[kind],RegionID="R01",HomeID=home,OwnerNPCID=npc,WorldTransform=f"(Rotation=(X=0,Y=0,Z=1,W=0),Translation=(X={x},Y={y},Z=88),Scale3D=(X=1,Y=1,Z=1))",InteriorFloorID="None",bSafeForArrival="True",ClearanceRadiusCm=42,ClearanceHalfHeightCm=96,SmartObjectDefinition="None",SmartObjectSlotIndex=0))
            register(anchor,"Anchor","World/DT_IC_Anchor")
        hf, hr = tables["World/DT_IC_Home.csv"]
        upsert(hr,definition(hf,home,Type="NPC",RegionID="R01",OwnerNPCID=npc,EntranceAnchorID=anchors["HOME"],BedAnchorIDs=f'(\"{anchors["BED"]}\")',KitchenAnchorIDs="()",CraftingAnchorIDs="()",InnServiceID="None",ContainerAccessAffinity=200,NPCContainers="()",StorageInstallAnchorIDs="()",FarmPlotAnchorIDs="()",StorageSlotsPerInstallation=40,MaximumStorageInstallations=3,SharedStorageGroupID="None"))
        for identifier,kind,table in [(npc,"NPC","NPC/DT_IC_NPC"),(combat,"RecruitCombat","Character/DT_IC_RecruitCombat"),(skill,"Skill","Combat/DT_IC_Skill"),(dialogue,"Dialogue","NPC/DT_IC_Dialogue"),(schedule,"Schedule","NPC/DT_IC_Schedule"),(home,"Home","World/DT_IC_Home")]:
            register(identifier,kind,table)
        upsert_text(texts,npc+".NAME",display)
        upsert_text(texts,skill+".NAME",display+" 기술")
        upsert_text(dlg_texts,dialogue+".GREETING",display+"입니다.")
    for relative,(columns,rows) in tables.items():
        save(relative,columns,rows)
    save("StringTables/ST_IC_Definitions.csv",text_fields,texts)
    save("StringTables/ST_IC_Dialogue.csv",dlg_fields,dlg_texts)

    f,r=load("NPC/DT_IC_GiftPreference.csv")
    upsert(r,definition(f,"GIFT_R01_COMMON",PreferredAffinityDelta=20,NeutralAffinityDelta=0,DislikedAffinityDelta=0,Items='((ItemID="ITEM_P0_01",Preference=Preferred))'))
    save("NPC/DT_IC_GiftPreference.csv",f,r);register("GIFT_R01_COMMON","GiftPreference","NPC/DT_IC_GiftPreference")
    f,r=load("Items/DT_IC_Item.csv")
    for extra in ["WorldMesh","WorldMeshScale"]:
        if extra not in f:
            f.append(extra)
            for row in r:row[extra]="None" if extra=="WorldMesh" else "(X=0.25,Y=0.25,Z=0.25)"
    row=definition(f,"ITEM_P0_01",ItemID="ITEM_P0_01",DisplayName='LOCTABLE("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions", "ITEM_P0_01.NAME")',Tab="Normal",Category="Material",Kind="Material",EquipSlot="None",StackLimit=99,bCanDiscard="True",bCanSell="True",bCanGift="True",RequiredClass=-1,GrowthStat="None",DefaultGrade="Common",UseTarget="None",UseEffect="None",bRejectWhenNoEffect="True",WorldMesh="/Engine/BasicShapes/Cube.Cube",WorldMeshScale="(X=0.25,Y=0.25,Z=0.25)")
    # Numeric CSV properties must contain valid import values, rather than empty strings.
    for key in ["BuyPrice","SellPrice","GrowthAmount","RestoreHP","RestoreSP","MaxEnhanceLevel","MaxRandomOptions","BaseAttack","BaseDefense","EnhanceAttackPerLevel","EnhanceDefensePerLevel","EnhanceBaseCost","RestoreMP","RestoreHPRatio","RestoreMPRatio","RestoreSPRatio","RevivalHPRatio","RevivalMinimumMPRatio"]:
        if key in row:row[key]="0"
    row["EnhanceBaseSuccessRate"]="1"
    upsert(r,row);save("Items/DT_IC_Item.csv",f,r);register("ITEM_P0_01","Item","Items/DT_IC_Item")
    f,t=load("StringTables/ST_IC_Definitions.csv");upsert_text(t,"ITEM_P0_01.NAME","Item_P0_01");save("StringTables/ST_IC_Definitions.csv",f,t)

    f,r=load("Character/DT_IC_PlayerPreset.csv")
    for extra in ["AnimationProfile","MeshTransform"]:
        if extra not in f:f.append(extra)
    for suffix,gender,folder in [("M","Male","Male"),("F","Female","Female")]:
        identifier="PRESET_PLAYER_"+suffix;mesh=f"{GAME}/Player/SamplePlayer/{folder}/SK_Player_{suffix}"
        upsert(r,definition(f,identifier,Gender=gender,Mesh=object_path(mesh),AnimationClass="/Script/IsekaiClass.ICCharacterAnimInstance",CharacterClass=f"{GAME}/Player/Blueprints/BP_ICPlayerCharacter.BP_ICPlayerCharacter_C",Portrait=object_path(f"{GAME}/Player/SamplePlayer/{folder}/T_Thumbnail"),AppearanceResourceID="None",AnimationProfile=object_path(f"{GAME}/Animation/Player/Main{folder}/DA_ANIM_PLAYER_{suffix}"),MeshTransform="(Rotation=(X=0,Y=0,Z=-0.707107,W=0.707107),Translation=(X=0,Y=0,Z=-88),Scale3D=(X=1,Y=1,Z=1))"));register(identifier,"PlayerPreset","Character/DT_IC_PlayerPreset")
    save("Character/DT_IC_PlayerPreset.csv",f,r)

    f,r=load("Presentation/DT_IC_ResourceReference.csv")
    for number in range(1,12):
        npc=f"NPC_R01_{number:03}";identifier="RES_"+npc
        folder=f"{GAME}/NPC/SampleNPC/R01/{npc}"
        upsert(r,definition(f,identifier,Kind="SkeletalMesh",Asset=object_path(folder+f"/SK_R01_{number}"),UsedBy=f"((Type=NPC,ID={npc}))",Skeleton=object_path(folder+f"/SKEL_R01_{number}"),SkeletonGroup="VROID_R01",RequiredSockets='("weapon_r","weapon_l")',RequiredNotifyNames="()",DimensionsCm="(X=0,Y=0,Z=0)",ImportScale="(X=1,Y=1,Z=1)",AxisAndGripNotes="Original VRM import retained. Per-weapon BP configures grip, length axis and muzzle. Visual calibration pending.",bVerifiedInRepresentativeScene="False",bVerifiedForCooking="False"))
        register(identifier,"ResourceReference","Presentation/DT_IC_ResourceReference")
    save("Presentation/DT_IC_ResourceReference.csv",f,r)

    f,r=load("Equipment/DT_IC_Equipment.csv")
    if "WeaponActorClass" not in f:
        f.append("WeaponActorClass")
        for row in r:row["WeaponActorClass"]="None"
    for row in r:
        if row["WeaponFamily"] in ["Sword","Bow","Staff","Wand"]:
            name={"Sword":"ArmingSword","Bow":"Bow2","Staff":"BoneSpear2","Wand":"AncietStaff"}[row["WeaponFamily"]]
            row["WeaponActorClass"]=f"{GAME}/Items/Weapons/Blueprints/BP_WPN_{name}.BP_WPN_{name}_C"
            row["AttachSocket"]="weapon_l" if row["WeaponFamily"]=="Bow" else "weapon_r"
    save("Equipment/DT_IC_Equipment.csv",f,r)
    f,r=load("Settings/DT_IC_PartySettings.csv")
    for row in r:row.update(MaximumCompanions="3",RequiredRecruitAffinity="300",FormationOffsetsCm="((X=-180,Y=-120,Z=0),(X=-220,Y=0,Z=0),(X=-180,Y=120,Z=0))")
    save("Settings/DT_IC_PartySettings.csv",f,r)
    f,r=load("World/DT_IC_Region.csv")
    for row in r:
        if row["ID"]=="R01":
            row["World"]=f"{GAME}/World/L_TestMap.L_TestMap"
            row["NPCIDs"]="("+",".join('"'+f"NPC_R01_{i:03}"+'"' for i in range(1,12))+',"NPC_R01_101")'
    save("World/DT_IC_Region.csv",f,r)
    f,r=load("Registry/DT_IC_IDRegistry.csv")
    for identifier,(kind,table) in REGISTER.items():
        upsert(r,definition(f,identifier,DefinitionType=kind,RegionID="R01" if "R01" in identifier else "None",ReplacementID="None",DefinitionTable=f"{GAME}/Data/DataTable/{table}.{table.rsplit('/',1)[-1]}",Notes="R01 approved character groundwork. Animation and placement require editor validation."))
    save("Registry/DT_IC_IDRegistry.csv",f,r)
    f,t=load("StringTables/ST_IC_Definitions.csv")
    known={row["Key"] for row in t}
    for identifier in REGISTER:
        if identifier+".NAME" not in known:upsert_text(t,identifier+".NAME",identifier)
    save("StringTables/ST_IC_Definitions.csv",f,t)
    print(f"Updated {len(ROLES)} NPCs; classmates R01_09/10/11; common gift +20; {len(REGISTER)} registry entries.")


def upsert_text(rows,key,value):
    for row in rows:
        if row["Key"]==key:row["SourceString"]=value;return
    rows.append({"Key":key,"SourceString":value})


if __name__=="__main__":main()
