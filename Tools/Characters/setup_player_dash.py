"""Retarget Run_Fast loops and connect player dash montages. Run in Unreal Python."""
import json
import sys
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT / "Tools/Characters"))
import setup_assets as setup

REPORT = {"complete": False, "players": []}


def source(pack):
    # Explicit non-root-motion source: some packs contain duplicate names in RM folders.
    paths = {
        "Sword_Animations": "Sword_Animations/Animations/Sequence2/04_Run/01_Run/11_Run_Fast/Run_Fast_Loop_Seq",
        "Archer": "Archer/Animation/Sequence/04_Run/01_Run/11_Run_Fast/AS_Run_Fast_Loop_Seq",
        "Mage": "Mage/Animations/Sequence2/04_Run/01_Run/11_Run_Fast/AS_Run_Fast_Loop_Seq",
    }
    return setup.asset(setup.GAME + "/Animation/SampleAnimation/" + paths[pack])


def configure(gender, suffix):
    folder = f"{setup.GAME}/Animation/Player/Main{gender}"
    mesh = setup.asset(f"{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}")
    rig = setup.asset(f"{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin")
    profile = setup.asset(f"{folder}/DA_ANIM_PLAYER_{suffix}")
    sets = dict(profile.get_editor_property("weapon_sets"))
    montages = {}
    for pack in ["Sword_Animations", "Archer", "Mage"]:
        clip = setup.retarget(pack, {"dash": source(pack)}, mesh, rig, folder + "/" + pack)["dash"]
        clip.set_editor_property("enable_root_motion", False)
        clip.set_editor_property("force_root_lock", True)
        clip.set_editor_property("root_motion_root_lock", u.RootMotionRootLock.REF_POSE)
        setup.save(clip)
        montage = u.ICCharacterSetupLibrary.create_action_montage(
            f"{folder}/AM_{pack}_dash", clip, False)
        if not montage or montage.get_play_length() <= 0:
            raise RuntimeError(f"Invalid dash montage: {gender}/{pack}")
        for prop in ["blend_in", "blend_out"]:
            blend = montage.get_editor_property(prop)
            blend.set_editor_property("blend_time", .04)
            montage.set_editor_property(prop, blend)
        if montage.get_editor_property("skeleton") != mesh.get_editor_property("skeleton"):
            raise RuntimeError(f"Dash skeleton mismatch: {gender}/{pack}")
        setup.save(montage)
        montages[pack] = montage
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
    # Read the assigned properties back before declaring the authoring step complete.
    assigned = profile.get_editor_property("weapon_sets")
    for key in sets:
        if not assigned[key].get_editor_property("dodge"):
            raise RuntimeError(f"Missing dash assignment: {gender}/{key}")
    REPORT["players"].append({"gender": gender, "profile": profile.get_path_name(),
        "montages": {p: m.get_path_name() for p, m in montages.items()},
        "unarmed": unarmed.get_editor_property("dodge").get_path_name()})


if __name__ == "__main__":
    try:
        configure("Male", "M")
        configure("Female", "F")
        REPORT["complete"] = True
        u.log("IC_PLAYER_DASH_SETUP_COMPLETE")
    except Exception as error:
        REPORT["error"] = str(error)
        raise
    finally:
        (ROOT / "Saved/PlayerDashSetup.json").write_text(
            json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
