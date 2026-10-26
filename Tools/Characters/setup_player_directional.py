"""Create eight-way combat locomotion and dodges for both player skeletons."""
import sys
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0,str(root/'Tools/Characters'))
import setup_assets as setup

directions=['F_0','F_R_45','F_R_90','B_R_45','B_180','B_L_45','F_L_90','F_L_45']
sword_dodges=['F','F_R_45','R','B_R_45','B','B_L_45','L','F_L_45']
other_dodges=['F_0','F_R_45','R_90','B_R_45','B_180','B_L_45','L_90','F_L_45']

def find(directory,name):
    matches=list(directory.rglob(name+'.uasset'))
    if len(matches)!=1:raise RuntimeError(f'Ambiguous source {name}: {matches}')
    return setup.asset('/Game/'+matches[0].relative_to(root/'Content').with_suffix('').as_posix())

for gender,suffix in [('Male','M'),('Female','F')]:
    folder=f'{setup.GAME}/Animation/Player/Main{gender}'
    mesh=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}')
    rig=setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin')
    profile=setup.asset(folder+'/DA_ANIM_PLAYER_'+suffix)
    authored={}
    for pack in ['Sword_Animations','Archer','Mage']:
        sword=pack=='Sword_Animations'
        source=root/'Content/IsekaiClass/Animation/SampleAnimation'/pack/('Animation/Sequence' if pack=='Archer' else 'Animations/Sequence2')
        clips={}
        for index,direction in enumerate(directions):
            name=f'Run_Combat_Loop_{direction}_Seq' if sword else f'AS_Run_Combat_{direction}_Loop_Seq'
            clips[f'run{index}']=find(source/'04_Run/02_Run_Combat',name)
            dodge=(sword_dodges if sword else other_dodges)[index]
            clips[f'dodge{index}']=find(source/'06_Dodge/02_Dodge_Combat',('Dodge_Combat_' if sword else 'AS_Dodge_Combat_')+dodge+'_Seq')
        idle_name='Idle_Combat_Seq' if sword else 'AS_Idle_Combat_Seq'
        idle=setup.asset(folder+'/'+pack+'/RT_'+pack+'_'+idle_name)
        output=setup.retarget(pack,clips,mesh,rig,folder+'/'+pack)
        for clip in output.values():
            clip.set_editor_property('enable_root_motion',False)
            clip.set_editor_property('force_root_lock',True)
            clip.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.REF_POSE)
            setup.save(clip)
        blend=u.ICCharacterSetupLibrary.create_combat_locomotion(folder+'/BS_Combat8_'+pack,idle,[output[f'run{i}'] for i in range(8)])
        if not blend:raise RuntimeError('Combat BlendSpace creation failed: '+pack)
        setup.save(blend)
        montages=[]
        for i in range(8):
            montage=u.ICCharacterSetupLibrary.create_action_montage(folder+f'/AM_Dodge8_{pack}_{i}',output[f'dodge{i}'],False)
            for prop in ['blend_in','blend_out']:
                settings=montage.get_editor_property(prop)
                settings.set_editor_property('blend_time',.04)
                montage.set_editor_property(prop,settings)
            setup.save(montage);montages.append(montage)
        authored[pack]=(blend,montages)
        u.log(f'IC_DIRECTIONAL_PACK_SAVED {gender} {pack}')
    sets=dict(profile.get_editor_property('weapon_sets'))
    for family,pack in [('NONE','Sword_Animations'),('SWORD','Sword_Animations'),('BOW','Archer'),('STAFF','Mage'),('WAND','Mage')]:
        key=getattr(u.ICWeaponFamily,family)
        state=profile.get_editor_property('unarmed') if family=='NONE' else sets[key]
        blend,montages=authored[pack]
        state.set_editor_property('combat_locomotion',blend)
        state.set_editor_property('directional_dodges',montages)
        state.set_editor_property('dodge',montages[0])
        if family=='NONE':profile.set_editor_property('unarmed',state)
        else:sets[key]=state
    profile.set_editor_property('weapon_sets',sets)
    setup.save(profile)
u.log('IC_PLAYER_DIRECTIONAL_SETUP_COMPLETE')
