"""Configure weapon jump arcs and timing while preserving each player's unarmed set."""
import sys
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(root/'Tools/Characters'))
import setup_assets as setup

sources={
    'Sword_Animations':('Sword_Animations/Animations/Sequence2/05_Jump/01_Jump/01_Jump_0/','Jump_Start_0_Seq','Jump_Loop_0_Seq','Jump_Stop_0_Seq'),
    'Archer':('Archer/Animation/Sequence/05_Jump/01_Jump/01_Jump_0/','AS_Jump_Start_0_Seq','AS_Jump_Loop_0_Seq','AS_Jump_End_0_Seq'),
    'Mage':('Mage/Animations/Sequence2/05_Jump/01_Jump/01_Jump_0/','AS_Jump_Start_0_Seq','AS_Jump_Loop_0_Seq','AS_Jump_Stop_0_Seq'),
}
def lock(clip):
    changed=False
    for prop,value in dict(enable_root_motion=False,force_root_lock=True,root_motion_root_lock=u.RootMotionRootLock.REF_POSE,additive_anim_type=u.AdditiveAnimationType.AAT_NONE).items():
        if clip.get_editor_property(prop)!=value:clip.set_editor_property(prop,value);changed=True
    if changed:setup.save(clip)

for gender,suffix in [('Male','M'),('Female','F')]:
    folder=f'{setup.GAME}/Animation/Player/Main{gender}'
    mesh=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}')
    rig=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin')
    clips={}
    for pack,(directory,start,loop,end) in sources.items():
        source={key:setup.asset(setup.GAME+'/Animation/SampleAnimation/'+directory+name) for key,name in [('jump_start',start),('jump',loop),('land',end)]}
        clips[pack]=setup.retarget(pack,source,mesh,rig,folder+'/'+pack)
        for clip in clips[pack].values():lock(clip)
    profile=setup.asset(folder+'/DA_ANIM_PLAYER_'+suffix)
    sets=dict(profile.get_editor_property('weapon_sets'))
    for family,pack in [('SWORD','Sword_Animations'),('BOW','Archer'),('STAFF','Mage'),('WAND','Mage')]:
        key=getattr(u.ICWeaponFamily,family)
        state=sets[key]
        for prop,clip in clips[pack].items():state.set_editor_property(prop,clip)
        state.set_editor_property('fall_loop',clips[pack]['jump'])
        state.set_editor_property('jump_height_cm',115.0)
        state.set_editor_property('jump_air_time_seconds',.75)
        state.set_editor_property('jump_start_duration_seconds',.20)
        state.set_editor_property('landing_duration_seconds',.65)
        sets[key]=state
    profile.set_editor_property('weapon_sets',sets)
    setup.save(profile)
u.log('IC_PLAYER_AIR_SETUP_COMPLETE')
