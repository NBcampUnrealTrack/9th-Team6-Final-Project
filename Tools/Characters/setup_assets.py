"""Run in Unreal Editor Python after building IsekaiClassEditor.

Generates only the approved character/weapon groundwork. Source animations,
meshes and their import settings are preserved. Output paths are deterministic;
existing generated animation assets are reused on a repeated run.
"""
import csv
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME='/Game/IsekaiClass'
TOOLS=u.AssetToolsHelpers.get_asset_tools()
REGISTRY=u.AssetRegistryHelpers.get_asset_registry()
EAL=u.EditorAssetLibrary
REPORT={'characters':[],'retargets':[],'tables':[],'weapons':[],'warnings':[]}
ROLES=['Staff','Sword','Wand','Bow','Sword','Staff','Bow','Staff','Wand','Sword','Bow']


def asset(path):
    result=u.load_asset(path)
    if not result:raise RuntimeError('Missing asset: '+path)
    return result


def save(obj):
    if not EAL.save_loaded_asset(obj,only_if_is_dirty=False):raise RuntimeError('Save failed: '+obj.get_path_name())


def create(path,cls,factory):
    if EAL.does_asset_exist(path):return asset(path)
    folder,name=path.rsplit('/',1)
    result=TOOLS.create_asset(name,folder,cls,factory)
    if not result:raise RuntimeError('Create failed: '+path)
    return result


def blueprint(path,parent):
    factory=u.BlueprintFactory();factory.set_editor_property('parent_class',parent)
    return create(path,u.Blueprint,factory)


def cdo(bp):
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    return u.get_default_object(bp.generated_class())


def candidates(pack,kind=u.AnimSequence):
    folder=ROOT/'Content/IsekaiClass/Animation/SampleAnimation'/pack
    paths=[]
    for file in folder.rglob('*.uasset'):
        if 'Sequence1' in file.parts:continue
        paths.append('/Game/'+file.relative_to(ROOT/'Content').with_suffix('').as_posix())
    return paths


def sequence(pack,name):
    found=[p for p in candidates(pack) if p.rsplit('/',1)[-1]==name]
    if len(found)!=1:raise RuntimeError(f'Ambiguous sequence {pack}/{name}: {found}')
    return asset(found[0])


def preview_mesh(animation,pack):
    skeleton=animation.get_editor_property('skeleton')
    for file in (ROOT/'Content/IsekaiClass/Animation/SampleAnimation'/pack/'Demo').rglob('*.uasset'):
        if not file.stem.startswith(('SK_','SKM_')):continue
        path='/Game/'+file.relative_to(ROOT/'Content').with_suffix('').as_posix()
        item=u.load_asset(path)
        if isinstance(item,u.SkeletalMesh) and item.get_editor_property('skeleton')==skeleton:return item
    raise RuntimeError('No matching source mesh for '+animation.get_path_name())


def source_rig(pack,mesh):
    path=GAME+'/Animation/Common/Retarget/IK_SRC_'+pack
    rig=create(path,u.IKRigDefinition,u.IKRigDefinitionFactory())
    ctl=u.IKRigController.get_controller(rig)
    if not ctl.set_skeletal_mesh(mesh):raise RuntimeError('Source mesh rejected')
    if not ctl.apply_auto_generated_retarget_definition():raise RuntimeError('No source humanoid template: '+pack)
    save(rig);return rig


def retarget(pack,animations,target,target_rig,folder):
    prefix='RT_'+pack+'_'
    expected={name:folder+'/'+prefix+seq.get_name() for name,seq in animations.items()}
    if all(EAL.does_asset_exist(p) for p in expected.values()):return {k:asset(p) for k,p in expected.items()}
    source=preview_mesh(next(iter(animations.values())),pack)
    rig=source_rig(pack,source)
    rtg_path=folder+'/RTG_'+pack
    existing_retargeter=EAL.does_asset_exist(rtg_path)
    rtg=create(rtg_path,u.IKRetargeter,u.IKRetargetFactory())
    ctl=u.IKRetargeterController.get_controller(rtg)
    ctl.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,rig)
    ctl.set_ik_rig(u.RetargetSourceOrTarget.TARGET,target_rig)
    ctl.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE,source)
    ctl.set_preview_mesh(u.RetargetSourceOrTarget.TARGET,target)
    if not existing_retargeter:
        ctl.add_default_ops();ctl.auto_map_chains(u.AutoMapChainType.FUZZY,True)
        ctl.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
    save(rtg)
    inputs=u.IKRetargetBatchOperationInputs()
    inputs.set_editor_property('assets_to_retarget',[REGISTRY.get_asset_by_object_path(seq.get_path_name()) for seq in dict.fromkeys(animations.values())])
    for key,value in {'source_mesh':source,'target_mesh':target,'ik_retarget_asset':rtg,'target_path':folder,'prefix':prefix,'include_referenced_assets':False,'overwrite_existing_files':False,'use_source_path':False}.items():inputs.set_editor_property(key,value)
    results=u.IKRetargetBatchOperation.run_batch_retarget(inputs)
    REPORT['retargets'].append({'target':target.get_name(),'pack':pack,'created':len(results)})
    for result in results:save(result.get_asset())
    return {key:asset(path) for key,path in expected.items()}


def add_sockets(mesh):
    if not u.ICCharacterSetupLibrary.prepare_character_mesh(mesh):
        raise RuntimeError('Required hand bones missing: '+mesh.get_path_name())
    save(mesh)


def setup_weapons():
    base=blueprint(GAME+'/Items/Weapons/Blueprints/BP_ICWeaponBase',u.ICWeaponActor)
    save(base)
    specifications=[('ArmingSword','Arming_Sword','Swords',True),('Bow2','Bow_2','Bows',False),('BoneSpear2','Bone_Spear_2','MagicStaves',True),('AncietStaff','Anciet_Staff','HealingWands',True)]
    for name,mesh_name,kind,auto_muzzle in specifications:
        matches=list((ROOT/'Content/IsekaiClass/Items/Weapons'/kind).rglob(mesh_name+'.uasset'))
        matches=[p for p in matches if p.parent.name=='StaticMeshes']
        if len(matches)!=1:raise RuntimeError('Weapon mesh selection: '+mesh_name)
        mesh=asset('/Game/'+matches[0].relative_to(ROOT/'Content').with_suffix('').as_posix())
        path=GAME+'/Items/Weapons/Blueprints/BP_WPN_'+name
        existing=EAL.does_asset_exist(path)
        bp=blueprint(path,base.generated_class())
        default=cdo(bp);visual=default.get_editor_property('visual')
        initialized=existing and bool(visual.get_editor_property('static_mesh'))
        visual.set_editor_property('static_mesh',mesh)
        bounds=mesh.get_bounding_box();extent=bounds.max-bounds.min
        index=max(range(3),key=lambda i:[extent.x,extent.y,extent.z][i])
        axis=[u.Vector(1,0,0),u.Vector(0,1,0),u.Vector(0,0,1)][index]
        default.set_editor_property('auto_muzzle_at_tip',auto_muzzle)
        center=(bounds.min+bounds.max)*.5
        length=[extent.x,extent.y,extent.z][index]
        # A repeated setup must preserve the reviewed grip, axis and scale.
        if not initialized:
            default.set_editor_property('length_axis',axis)
            visual.set_editor_property('relative_location',-(center-axis*(length*.3)))
            if not auto_muzzle:default.get_editor_property('muzzle').set_editor_property('relative_location',u.Vector(45,0,15))
        u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
        REPORT['weapons'].append({'bp':bp.get_path_name(),'mesh':mesh.get_path_name(),'axis':index,'source_length_cm':length})
    gift=blueprint(GAME+'/Items/Blueprints/BP_Item_P0_01',u.ICDroppedItem)
    default=cdo(gift);default.get_editor_property('mesh').set_editor_property('static_mesh',asset('/Engine/BasicShapes/Cube'))
    default.get_editor_property('mesh').set_editor_property('relative_scale3d',u.Vector(.25,.25,.25));save(gift)


def source_sets():
    out={}
    for family,pack in [('Sword','Sword_Animations'),('Bow','Archer'),('Staff','Mage')]:
        sword=family=='Sword'
        names={'idle':'Idle_Combat_Seq' if sword else 'AS_Idle_Combat_Seq','walk':'Walk_Loop_F_0_Seq' if sword else 'AS_Walk_F_0_Loop_Seq','run':'Run_Loop_F_0_Seq' if sword else 'AS_Run_F_0_Loop_Seq','jump':'Jump_Loop_0_Seq' if sword else 'AS_Jump_Loop_0_Seq','attack':'Combo_Attack_01_01_Seq' if sword else 'AS_Combo_Attack_01_01_Seq' if family=='Bow' else 'AS_Attack_01_01_Seq','skill':'Combo_Attack_02_01_Seq' if sword else 'AS_Skill_03_Seq','hit':'Hit_F_Seq' if sword else 'AS_Hit_F_Seq'}
        if family=='Staff':names['heal']='AS_Skill_04_Seq'
        out[family]=(pack,{key:sequence(pack,value) for key,value in names.items()})
    return out


def setup_character(identifier,mesh_path,rig_path,folder,families,sets):
    mesh=asset(mesh_path);add_sockets(mesh)
    rig=asset(rig_path)
    profile_factory=u.DataAssetFactory();profile_factory.set_editor_property('data_asset_class',u.ICCharacterAnimationProfile)
    profile=create(folder+'/DA_ANIM_'+identifier,u.ICCharacterAnimationProfile,profile_factory)
    previous=profile.get_editor_property('weapon_sets')
    mapped={}
    for family in families:
        key='Staff' if family=='Wand' else family
        pack,source=sets[key]
        clips=retarget(pack,source,mesh,rig,folder+'/'+pack)
        for clip in dict.fromkeys(clips.values()):
            clip.set_editor_property('force_root_lock',True)
            clip.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.REF_POSE)
            save(clip)
        blend=u.ICCharacterSetupLibrary.create_locomotion(folder+'/BS_'+family,clips['idle'],clips['walk'],clips['run'])
        if not blend:raise RuntimeError('Blend space failed: '+identifier)
        save(blend)
        actions={}
        for action in ['attack','skill','hit']:
            clip=clips['heal'] if action=='skill' and family=='Wand' else clips[action]
            montage=u.ICCharacterSetupLibrary.create_action_montage(folder+'/AM_'+family+'_'+action,clip,action!='hit')
            if not montage:raise RuntimeError('Montage failed')
            save(montage);actions[action]=montage
        family_key=getattr(u.ICWeaponFamily,family.upper())
        offset=previous[family_key].get_editor_property('ground_offset_cm') if family_key in previous else 0.0
        mapped[family_key]=u.ICCharacterAnimationSet(locomotion=blend,jump=clips['jump'],attack=actions['attack'],skill=actions['skill'],hit=actions['hit'],ground_offset_cm=offset)
    sleep=retarget('Sleep_Animation',{'sleep':sequence('Sleep_Animation','anim_Sleep_06_L')},mesh,rig,folder+'/Sleep')['sleep']
    profile.set_editor_property('weapon_sets',mapped);profile.set_editor_property('sleep',sleep)
    profile.set_editor_property('default_family',getattr(u.ICWeaponFamily,families[0].upper()));save(profile)
    if identifier.startswith('NPC_'):
        parent=EAL.load_blueprint_class(GAME+'/NPC/Blueprints/BP_ICNPCBase')
        bp=blueprint(GAME+'/NPC/Blueprints/R01/BP_'+identifier,parent)
        default=cdo(bp);default.set_editor_property('npc_definition_id',identifier);default.set_editor_property('animation_profile',profile)
        component=default.get_editor_property('mesh');component.set_editor_property('skeletal_mesh',mesh)
        component.set_editor_property('relative_location',u.Vector(0,0,-88));component.set_editor_property('relative_rotation',u.Rotator(pitch=0,yaw=-90,roll=0))
        component.set_editor_property('anim_class',u.ICCharacterAnimInstance)
        u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    REPORT['characters'].append({'id':identifier,'families':families,'profile':profile.get_path_name(),'mesh':mesh.get_path_name(),'classmate':identifier in ['NPC_R01_009','NPC_R01_010','NPC_R01_011']})
    checkpoint()


def import_tables():
    modified=['NPC/DT_IC_NPC','Character/DT_IC_RecruitCombat','NPC/DT_IC_Dialogue','NPC/DT_IC_Schedule','World/DT_IC_Anchor','World/DT_IC_Home','Combat/DT_IC_Skill','NPC/DT_IC_GiftPreference','Items/DT_IC_Item','Character/DT_IC_PlayerPreset','Equipment/DT_IC_Equipment','Settings/DT_IC_PartySettings','World/DT_IC_Region','Presentation/DT_IC_ResourceReference','Registry/DT_IC_IDRegistry']
    for name in modified:
        table=asset(GAME+'/Data/DataTable/'+name)
        source=(ROOT/'Data'/(name+'.csv')).read_text(encoding='utf-8-sig')
        if not u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,source):raise RuntimeError('CSV import failed: '+name)
        save(table);REPORT['tables'].append(name)
    for name in ['ST_IC_Definitions','ST_IC_Dialogue']:
        table=asset(GAME+'/Data/DataTable/StringTables/'+name)
        if not u.ICBlueprintMaintenanceLibrary.import_ui_strings(table,str(ROOT/'Data/StringTables'/(name+'.csv'))):raise RuntimeError('String table import failed')
        save(table)


def checkpoint():
    (ROOT/'Saved/CharacterSetupReport.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')


def main():
    setup_weapons();sets=source_sets()
    for suffix,folder in [('M','Male'),('F','Female')]:
        setup_character('PLAYER_'+suffix,f'{GAME}/Player/SamplePlayer/{folder}/SK_Player_{suffix}',f'{GAME}/Player/SamplePlayer/{folder}/IK_Player_{suffix}_Mannequin',f'{GAME}/Animation/Player/Main{folder}',['Sword','Bow','Staff','Wand'],sets)
    for number,family in enumerate(ROLES,1):
        identifier=f'NPC_R01_{number:03}'
        setup_character(identifier,f'{GAME}/NPC/SampleNPC/R01/{identifier}/SK_R01_{number}',f'{GAME}/NPC/SampleNPC/R01/{identifier}/IK_R01_{number}_Mannequin',f'{GAME}/Animation/NPC/{identifier}',[family],sets)
    import_tables();REPORT['complete']=True;checkpoint();u.log('IC_CHARACTER_SETUP_COMPLETE')


try:main()
except Exception as error:
    REPORT['error']=str(error);checkpoint();raise
