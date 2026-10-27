"""Move existing retargeted Run_Fast loops from dash to held sprint locomotion."""
import unreal as u

for gender, suffix in [("Male", "M"), ("Female", "F")]:
    folder = f"/Game/IsekaiClass/Animation/Player/Main{gender}"
    profile = u.load_asset(f"{folder}/DA_ANIM_PLAYER_{suffix}")
    sets = dict(profile.get_editor_property("weapon_sets"))
    for family, pack, name in [
        (u.ICWeaponFamily.NONE, "Sword_Animations", "Run_Fast_Loop_Seq"),
        (u.ICWeaponFamily.SWORD, "Sword_Animations", "Run_Fast_Loop_Seq"),
        (u.ICWeaponFamily.BOW, "Archer", "AS_Run_Fast_Loop_Seq"),
        (u.ICWeaponFamily.STAFF, "Mage", "AS_Run_Fast_Loop_Seq"),
        (u.ICWeaponFamily.WAND, "Mage", "AS_Run_Fast_Loop_Seq"),
    ]:
        clip = u.load_asset(f"{folder}/{pack}/RT_{pack}_{name}")
        if not clip:
            raise RuntimeError(f"Missing Run_Fast clip: {gender}/{pack}")
        current = profile.get_editor_property("unarmed") if family == u.ICWeaponFamily.NONE else sets[family]
        current.set_editor_property("sprint", clip)
        previous = current.get_editor_property("dodge")
        if previous and previous.get_path_name().startswith(folder + "/AM_") and "_dash." in previous.get_path_name():
            current.set_editor_property("dodge", None)
        if family == u.ICWeaponFamily.NONE:
            profile.set_editor_property("unarmed", current)
        else:
            sets[family] = current
    profile.set_editor_property("weapon_sets", sets)
    if not u.EditorAssetLibrary.save_loaded_asset(profile, False):
        raise RuntimeError(f"Failed saving sprint profile: {gender}")
u.log("IC_PLAYER_SPRINT_SETUP_COMPLETE")
