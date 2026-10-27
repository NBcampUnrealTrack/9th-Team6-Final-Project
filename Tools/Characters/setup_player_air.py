"""Author player takeoff, airborne, fall and landing clips without root motion."""
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
    clip.set_editor_property('enable_root_motion',False)
    clip.set_editor_property('force_root_lock',True)
    clip.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.REF_POSE)
    setup.save(clip)

for gender,suffix in [('Male','M'),('Female','F')]:
    folder=f'{setup.GAME}/Animation/Player/Main{gender}'
    mesh=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}')
    rig=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin')
    fall_source=setup.asset(setup.GAME+'/Animation/SampleAnimation/505_Survival_Mocap_Animations/Retargeting/Characters/Mannequins/Animations/Manny/MM_Fall_Loop')
    fall=setup.retarget('505_Survival_Mocap_Animations',{'fall_loop':fall_source},mesh,rig,folder+'/Air')['fall_loop']
    lock(fall)
    clips={}
    for pack,(directory,start,loop,end) in sources.items():
        source={key:setup.asset(setup.GAME+'/Animation/SampleAnimation/'+directory+name) for key,name in [('jump_start',start),('jump',loop),('land',end)]}
        clips[pack]=setup.retarget(pack,source,mesh,rig,folder+'/'+pack)
        for clip in clips[pack].values():lock(clip)
    profile=setup.asset(folder+'/DA_ANIM_PLAYER_'+suffix)
    sets=dict(profile.get_editor_property('weapon_sets'))
    for family,pack in [('NONE','Sword_Animations'),('SWORD','Sword_Animations'),('BOW','Archer'),('STAFF','Mage'),('WAND','Mage')]:
        key=getattr(u.ICWeaponFamily,family)
        state=profile.get_editor_property('unarmed') if family=='NONE' else sets[key]
        for prop,clip in clips[pack].items():state.set_editor_property(prop,clip)
        state.set_editor_property('fall_loop',fall)
        if family=='NONE':profile.set_editor_property('unarmed',state)
        else:sets[key]=state
    profile.set_editor_property('weapon_sets',sets)
    setup.save(profile)
u.log('IC_PLAYER_AIR_SETUP_COMPLETE')
