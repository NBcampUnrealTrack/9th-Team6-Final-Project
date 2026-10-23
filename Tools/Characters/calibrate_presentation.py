"""Retarget the bed-compatible sleep pose, then calibrate and render all characters.

Run in a rendering editor with -ExecutePythonScript, not a Python commandlet.
Generated animations, profiles, weapon BPs and target mesh grip sockets are saved.
The source animation and weapon mesh packs remain intact.
"""
from pathlib import Path
import unreal as u

try:
    path=Path(u.Paths.project_dir())/'Tools/Characters/setup_assets.py'
    setup={}
    exec(compile(path.read_text(encoding='utf-8').split('\nif __name__ ==')[0],str(path),'exec'),setup)
    for index in range(13):
        player=index<2
        identifier=('PLAYER_M' if index==0 else 'PLAYER_F') if player else f'NPC_R01_{index-1:03}'
        folder=f'/Game/IsekaiClass/Animation/Player/Main{"Male" if index==0 else "Female"}' if player else '/Game/IsekaiClass/Animation/NPC/'+identifier
        source=f'/Game/IsekaiClass/Player/SamplePlayer/{"Male" if index==0 else "Female"}' if player else f'/Game/IsekaiClass/NPC/SampleNPC/R01/{identifier}'
        mesh=setup['asset'](source+('/SK_Player_M' if index==0 else '/SK_Player_F') if player else source+f'/SK_R01_{index-1}')
        rig=setup['asset'](source+('/IK_Player_M_Mannequin' if index==0 else '/IK_Player_F_Mannequin') if player else source+f'/IK_R01_{index-1}_Mannequin')
        clip=setup['retarget']('Sleep_Animation',{'sleep':setup['sequence']('Sleep_Animation','anim_Sleep_06_L')},mesh,rig,folder+'/Sleep')['sleep']
        profile=setup['asset'](folder+'/DA_ANIM_'+identifier)
        profile.set_editor_property('sleep',clip);setup['save'](profile)
        if u.ICWeaponFamily.SWORD in profile.get_editor_property('weapon_sets'):
            skill=setup['retarget']('Sword_Animations',{'skill':setup['sequence']('Sword_Animations','Combo_Attack_02_01_Seq')},mesh,rig,folder+'/Sword_Animations')['skill']
            montage=u.ICCharacterSetupLibrary.create_action_montage(folder+'/AM_Sword_skill',skill,True)
            if not montage:raise RuntimeError('Sword skill update failed: '+identifier)
            setup['save'](montage)
    report=u.ICCharacterSetupLibrary.inspect_character_presentation(True)
    if not report:raise RuntimeError('Calibration requires a rendering editor')
    u.log('IC_CHARACTER_CALIBRATION_COMPLETE '+report)
finally:
    u.SystemLibrary.quit_editor()
