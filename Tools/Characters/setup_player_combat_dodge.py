"""Author actual Dodge clips while preserving held Run_Fast sprint."""
import sys
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(root / "Tools/Characters"))
import setup_assets as setup
from dodge_timing import configure_dodge_blends

sources = {
    "Sword_Animations": "Sword_Animations/Animations/Sequence2/06_Dodge/01_Dodge/Dodge_F_Seq",
    "Archer": "Archer/Animation/Sequence/06_Dodge/01_Dodge/AS_Dodge_F_0_Seq",
    "Mage": "Mage/Animations/Sequence2/06_Dodge/01_Dodge/AS_Dodge_F_0_Seq",
}
for gender, suffix in [("Male", "M"), ("Female", "F")]:
    folder = f"{setup.GAME}/Animation/Player/Main{gender}"
    mesh = setup.asset(f"{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}")
    rig = setup.asset(f"{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin")
    profile = setup.asset(f"{folder}/DA_ANIM_PLAYER_{suffix}")
    montages = {}
    for pack, path in sources.items():
        clip = setup.retarget(pack, {"dodge": setup.asset(f"{setup.GAME}/Animation/SampleAnimation/{path}")}, mesh, rig, folder + "/" + pack)["dodge"]
        clip.set_editor_property("enable_root_motion", False)
        clip.set_editor_property("force_root_lock", True)
        clip.set_editor_property("root_motion_root_lock", u.RootMotionRootLock.REF_POSE)
        setup.save(clip)
        montage = u.ICCharacterSetupLibrary.create_action_montage(f"{folder}/AM_{pack}_combat_dodge", clip, False)
        configure_dodge_blends(montage)
        setup.save(montage)
        montages[pack] = montage
    sets = dict(profile.get_editor_property("weapon_sets"))
    for family, pack in [("SWORD", "Sword_Animations"), ("BOW", "Archer"), ("STAFF", "Mage"), ("WAND", "Mage")]:
        key = getattr(u.ICWeaponFamily, family)
        current = sets[key]
        current.set_editor_property("dodge", montages[pack])
        sets[key] = current
    unarmed = profile.get_editor_property("unarmed")
    unarmed.set_editor_property("dodge", montages["Sword_Animations"])
    profile.set_editor_property("unarmed", unarmed)
    profile.set_editor_property("weapon_sets", sets)
    setup.save(profile)
u.log("IC_PLAYER_COMBAT_DODGE_SETUP_COMPLETE")
