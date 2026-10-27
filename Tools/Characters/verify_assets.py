"""Read-only structural verification of generated assets and approved R01 data."""
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass'
checks=[]


def check(value,description):
    checks.append({'pass':bool(value),'description':description})
    if not value:u.log_error('IC_CHARACTER_CHECK_FAILED '+description)


def rows(path):
    table=u.load_asset(GAME+'/Data/DataTable/'+path)
    check(table is not None,'Table loads: '+path)
    return {row['Name']:row for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))}


def run():
    npcs=rows('NPC/DT_IC_NPC');combat=rows('Character/DT_IC_RecruitCombat');presets=rows('Character/DT_IC_PlayerPreset')
    gifts=rows('NPC/DT_IC_GiftPreference');items=rows('Items/DT_IC_Item');equipment=rows('Equipment/DT_IC_Equipment')
    families=['Staff','Sword','Wand','Bow','Sword','Staff','Bow','Staff','Wand','Sword','Bow']
    classes={'Staff':'CLS_MAGE_001','Sword':'CLS_WARRIOR_001','Wand':'CLS_HEALER_001','Bow':'CLS_ARCHER_001'}
    for number,family in enumerate(families,1):
        key=f'NPC_R01_{number:03}';row=npcs[key]
        check(row['ClassID']==classes[family],key+' fixed class')
        check(row['bClassmate']==(number>=9),key+' classmate flag')
        check(row['bEndingTarget'] is False,key+' ending registration deferred')
        check(row['GiftPreferenceID']==('GIFT_R01_HERO' if number==8 else 'GIFT_R01_COMMON'),key+' approved gift preference')
        check(combat[row['RecruitCombatID']]['WeaponFamily']==family,key+' fixed weapon family')
        bp=u.load_asset(GAME+'/NPC/Blueprints/R01/BP_'+key)
        check(bp is not None,key+' Blueprint exists')
        default=u.get_default_object(bp.generated_class())
        check(str(default.get_editor_property('npc_definition_id'))==key,key+' Blueprint ID')
        profile=u.load_asset(f'{GAME}/Animation/NPC/{key}/DA_ANIM_{key}')
        mesh=u.load_asset(f'{GAME}/NPC/SampleNPC/R01/{key}/SK_R01_{number}')
        verify_profile(profile,mesh,1,key)
    for suffix,folder in [('M','Male'),('F','Female')]:
        check('PRESET_PLAYER_'+suffix in presets,'Player preset '+suffix)
        profile=u.load_asset(f'{GAME}/Animation/Player/Main{folder}/DA_ANIM_PLAYER_{suffix}')
        mesh=u.load_asset(f'{GAME}/Player/SamplePlayer/{folder}/SK_Player_{suffix}')
        verify_profile(profile,mesh,4,'PLAYER_'+suffix)
    check(gifts['GIFT_R01_COMMON']['PreferredAffinityDelta']==20,'Preferred gift +20')
    check(items['ITM_R01_FOOD_15']['bCanGift'],'Common item can be gifted')
    check(items['ITM_R01_FOOD_15']['StackLimit']==99,'Common item stack size')
    for row in equipment.values():
        if row['WeaponFamily']!='None':check(row['WeaponActorClass'] not in ['', 'None'],row['Name']+' weapon Blueprint reference')
    result={'checks':checks,'passed':sum(c['pass'] for c in checks),'failed':sum(not c['pass'] for c in checks)}
    (ROOT/'Saved/CharacterVerification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_CHARACTER_VERIFY '+json.dumps({k:v for k,v in result.items() if k!='checks'}))


def verify_profile(profile,mesh,count,key):
    check(profile is not None,key+' animation profile loads')
    check(len(profile.weapon_sets)==count,key+' family count')
    skeleton=mesh.get_editor_property('skeleton')
    check(profile.sleep.get_editor_property('skeleton')==skeleton,key+' sleep target skeleton')
    for family,entry in profile.weapon_sets.items():
        for name in ['locomotion','jump','attack','skill','hit']:
            animation=getattr(entry,name)
            check(animation is not None,key+' '+str(family)+' '+name+' assigned')
            check(animation.get_editor_property('skeleton')==skeleton,key+' '+str(family)+' '+name+' target skeleton')
    verify_springs(mesh,key)


def verify_springs(mesh,key):
    folder=mesh.get_path_name().rsplit('/',1)[0]
    name=mesh.get_name().removeprefix('SK_')
    meta=u.load_asset(folder+'/VM_'+name+'_VrmMeta')
    check(meta is not None,key+' VRM metadata loads')
    if not meta:return
    check(mesh.get_editor_property('post_process_anim_blueprint') is not None,key+' VRM post process assigned')
    data=meta.get_editor_property('vrm1_spring_bone_meta')
    groups=data.get_editor_property('collider_groups');colliders=data.get_editor_property('colliders')
    bones=[str(bone) for bone in u.ICCharacterSetupLibrary.get_character_bones(mesh)]
    valid=True;linked=True
    for spring in data.get_editor_property('springs'):
        joints=spring.get_editor_property('joints');links=spring.get_editor_property('collider_groups')
        root=str(joints[0].get_editor_property('bone_name')) if joints else ''
        if any(word in root.lower() for word in ['skirt','sleeve','hair']):linked=linked and bool(links)
        for joint in joints:
            index=joint.get_editor_property('bone_no')
            valid=valid and 0<=index<len(bones) and bones[index]==str(joint.get_editor_property('bone_name'))
        valid=valid and all(0<=index<len(groups) for index in links)
    for group in groups:valid=valid and all(0<=index<len(colliders) for index in group.get_editor_property('colliders'))
    check(valid,key+' spring bones and collider indices are valid')
    check(linked,key+' hair and clothing chains have body collision links')


run()
