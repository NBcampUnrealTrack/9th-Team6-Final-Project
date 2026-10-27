"""Author player-only empty-hand locomotion and weapon draw/put-away montages.

Run in Unreal Python after building IsekaiClassEditor. Reuses the reviewed IK
retargeters and preserves existing combat, grip, physics and NPC assets.
"""
import json
import sys
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT / "Tools/Characters"))
import setup_assets as setup

REPORT = {"players": [], "complete": False}


def prepare_clip(clip):
    clip.set_editor_property("force_root_lock", True)
    clip.set_editor_property("root_motion_root_lock", u.RootMotionRootLock.REF_POSE)
    setup.save(clip)


def configure_player(gender, suffix):
    folder = f"/Game/IsekaiClass/Animation/Player/Main{gender}"
    mesh = setup.asset(f"/Game/IsekaiClass/Player/SamplePlayer/{gender}/SK_Player_{suffix}")
    rig = setup.asset(f"/Game/IsekaiClass/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin")
    profile = setup.asset(f"{folder}/DA_ANIM_PLAYER_{suffix}")
    sets = dict(profile.get_editor_property("weapon_sets"))
    sources = {
        "Sword": ("Sword_Animations", "Idle_To_Idle_Combat_Seq", "Idle_Combat_To_Idle_Seq"),
        "Bow": ("Archer", "AS_Idle_to_Idle_Combat_Seq", "AS_Idle_Combat_to_Idle_Seq"),
        "Staff": ("Mage", "AS_Idle_to_Idle_Combat_Seq", "AS_Idle_Combat_to_Idle_Seq"),
        "Wand": ("Mage", "AS_Idle_to_Idle_Combat_Seq", "AS_Idle_Combat_to_Idle_Seq"),
    }
    result = {"preset": suffix, "families": {}}
    for family, (pack, draw_name, sheathe_name) in sources.items():
        clips = setup.retarget(pack, {
            "draw": setup.sequence(pack, draw_name),
            "sheathe": setup.sequence(pack, sheathe_name),
        }, mesh, rig, folder + "/" + pack)
        for clip in clips.values():
            prepare_clip(clip)
        current = sets[getattr(u.ICWeaponFamily, family.upper())]
        details = {}
        for action in ["draw", "sheathe"]:
            montage = u.ICCharacterSetupLibrary.create_weapon_transition_montage(
                f"{folder}/AM_{family}_{action}", clips[action], action == "draw", .55)
            if not montage:
                raise RuntimeError(f"Montage creation failed: {suffix}/{family}/{action}")
            setup.save(montage)
            current.set_editor_property(action, montage)
            details[action] = {"asset": montage.get_path_name(), "seconds": montage.get_play_length()}
        sets[getattr(u.ICWeaponFamily, family.upper())] = current
        result["families"][family] = details

    neutral = setup.retarget("Sword_Animations", {
        key: setup.sequence("Sword_Animations", name) for key, name in {
            "idle": "Idle_Seq", "walk": "Walk_Loop_F_0_Seq",
            "run": "Run_Loop_F_0_Seq", "jump": "Jump_Loop_0_Seq",
        }.items()
    }, mesh, rig, folder + "/Sword_Animations")
    for clip in neutral.values():
        prepare_clip(clip)
    locomotion = u.ICCharacterSetupLibrary.create_locomotion(
        folder + "/BS_Unarmed", neutral["idle"], neutral["walk"], neutral["run"])
    if not locomotion:
        raise RuntimeError("Empty-hand locomotion creation failed: " + suffix)
    setup.save(locomotion)
    sword = sets[u.ICWeaponFamily.SWORD]
    profile.set_editor_property("unarmed", u.ICCharacterAnimationSet(
        locomotion=locomotion, jump=neutral["jump"], hit=sword.get_editor_property("hit"),
        dodge=profile.get_editor_property("unarmed").get_editor_property("dodge"),
        sprint=profile.get_editor_property("unarmed").get_editor_property("sprint"),
        jump_start=profile.get_editor_property("unarmed").get_editor_property("jump_start"),
        fall_loop=profile.get_editor_property("unarmed").get_editor_property("fall_loop"),
        land=profile.get_editor_property("unarmed").get_editor_property("land"),
        combat_locomotion=profile.get_editor_property("unarmed").get_editor_property("combat_locomotion"),
        directional_dodges=profile.get_editor_property("unarmed").get_editor_property("directional_dodges"),
        ground_offset_cm=sword.get_editor_property("ground_offset_cm")))
    profile.set_editor_property("weapon_sets", sets)
    setup.save(profile)
    result["unarmed"] = locomotion.get_path_name()
    REPORT["players"].append(result)


def main():
    for gender, suffix in [("Male", "M"), ("Female", "F")]:
        configure_player(gender, suffix)
    strings = setup.asset("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI")
    if not u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings, str(ROOT / "Data/StringTables/ST_IC_UI.csv")):
        raise RuntimeError("Weapon equipment labels could not be imported")
    setup.save(strings)
    REPORT["complete"] = True
    u.log("IC_WEAPON_SWITCH_ASSETS_COMPLETE")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        REPORT["error"] = str(error)
        raise
    finally:
        (ROOT / "Saved/WeaponSwitchAssets.json").write_text(
            json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
