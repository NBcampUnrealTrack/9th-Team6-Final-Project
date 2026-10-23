"""Repair missing cloth collision links on the 13 approved VRM characters.

Run in Unreal Python. Dry-run by default; IC_APPLY_VRM_COLLISIONS=1 saves only
changed VrmMeta assets, after backing their original packages up under Saved.
Existing hair, face, bust and authored collider settings are preserved.
"""
import json
import os
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
APPLY = os.environ.get("IC_APPLY_VRM_COLLISIONS") == "1"
REPORT = {"apply": APPLY, "characters": []}


def asset(path):
    value = u.load_asset(path)
    if not value:
        raise RuntimeError("Missing asset: " + path)
    return value


def repair(folder, name, profile_path):
    mesh = asset(folder + "/SK_" + name)
    meta_path = folder + "/VM_" + name + "_VrmMeta"
    meta = asset(meta_path)
    data = meta.get_editor_property("vrm1_spring_bone_meta")
    springs = list(data.get_editor_property("springs"))
    colliders = list(data.get_editor_property("colliders"))
    groups = list(data.get_editor_property("collider_groups"))
    bones = [str(bone) for bone in u.ICCharacterSetupLibrary.get_character_bones(mesh)]
    row = {"name": name, "asset": meta_path, "links": [], "added_colliders": []}
    targets = []
    for index, spring in enumerate(springs):
        joints = list(spring.get_editor_property("joints"))
        root = str(joints[0].get_editor_property("bone_name")) if joints else ""
        if not spring.get_editor_property("collider_groups") and any(word in root for word in ("CoatSkirt", "Sleeve")):
            targets.append((index, root))
    if not targets:
        REPORT["characters"].append(row)
        return

    # Reuse the original body collider groups wherever possible.
    by_bone = {}
    for index, group in enumerate(groups):
        for collider_index in group.get_editor_property("colliders"):
            if not 0 <= collider_index < len(colliders):
                raise RuntimeError("Invalid original collider index: " + name)
            bone = str(colliders[collider_index].get_editor_property("bone_name"))
            by_bone.setdefault(bone, []).append(index)

    # Male imports have no thigh colliders. Build capsules from their actual
    # reference leg proportions, converting UE centimetres to VRM local metres.
    if any("CoatSkirt" in root for _, root in targets):
        profile = asset(profile_path)
        sets = profile.get_editor_property("weapon_sets")
        locomotion = next(iter(sets.values())).get_editor_property("locomotion")
        sequence = locomotion.get_editor_property("sample_data")[0].get_editor_property("animation")
        options = u.AnimPoseEvaluationOptions()
        options.set_editor_property("optional_skeletal_mesh", mesh)
        pose = u.AnimPoseExtensions.get_anim_pose_at_time(sequence, 0, options)
        for side in ("L", "R"):
            thigh = "J_Bip_" + side + "_UpperLeg"
            knee = "J_Bip_" + side + "_LowerLeg"
            if thigh in by_bone:
                continue
            leg = u.AnimPoseExtensions.get_ref_bone_pose(pose, knee, u.AnimPoseSpaces.LOCAL).translation
            length = leg.length()
            radius_cm = max(3.0, min(7.0, length * .16))
            offset = leg * .12
            tail = leg * .78
            collider = type(colliders[0])()
            for key, value in {
                "bone_no": bones.index(thigh), "bone_name": thigh, "shape_type": "capsule",
                "offset": u.Vector(offset.x / 100, offset.z / 100, -offset.y / 100),
                "tail": u.Vector(tail.x / 100, tail.z / 100, -tail.y / 100), "radius": radius_cm / 100,
            }.items():
                collider.set_editor_property(key, value)
            group = type(groups[0])()
            group.set_editor_property("name", "IC_Cloth_" + side + "_Thigh")
            group.set_editor_property("colliders", [len(colliders)])
            by_bone[thigh] = [len(groups)]
            colliders.append(collider)
            groups.append(group)
            row["added_colliders"].append({"bone": thigh, "radius_cm": radius_cm})

    for index, root in targets:
        if "CoatSkirt" in root:
            wanted = ["J_Bip_L_UpperLeg", "J_Bip_R_UpperLeg"]
        else:
            side = "L" if "_L_" in root else "R"
            wanted = ["J_Bip_" + side + "_" + part for part in ("UpperArm", "LowerArm", "Hand")]
        links = sorted({group for bone in wanted for group in by_bone.get(bone, [])})
        if not links:
            raise RuntimeError("No suitable cloth colliders: " + name + "/" + root)
        springs[index].set_editor_property("collider_groups", links)
        row["links"].append({"root": root, "groups": links})

    if APPLY:
        source = ROOT / "Content" / (meta_path.removeprefix("/Game/") + ".uasset")
        backup = ROOT / "Saved/SecondaryMotionReview/OriginalAssets" / source.relative_to(ROOT / "Content")
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            shutil.copy2(source, backup)
        data.set_editor_property("springs", springs)
        data.set_editor_property("colliders", colliders)
        data.set_editor_property("collider_groups", groups)
        meta.set_editor_property("vrm1_spring_bone_meta", data)
        if not u.EditorAssetLibrary.save_loaded_asset(meta, only_if_is_dirty=False):
            raise RuntimeError("Could not save: " + meta_path)
    REPORT["characters"].append(row)


for suffix, gender in [("M", "Male"), ("F", "Female")]:
    repair("/Game/IsekaiClass/Player/SamplePlayer/" + gender, "Player_" + suffix,
           f"/Game/IsekaiClass/Animation/Player/Main{gender}/DA_ANIM_PLAYER_{suffix}")
for number in range(1, 12):
    identifier = f"NPC_R01_{number:03}"
    repair("/Game/IsekaiClass/NPC/SampleNPC/R01/" + identifier, f"R01_{number}",
           f"/Game/IsekaiClass/Animation/NPC/{identifier}/DA_ANIM_{identifier}")

destination = ROOT / "Saved/SecondaryMotionReview"
destination.mkdir(parents=True, exist_ok=True)
(destination / ("CollisionRepairApplied.json" if APPLY else "CollisionRepairPlan.json")).write_text(
    json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
u.log("IC_VRM_COLLISION_REPAIR_COMPLETE apply=" + str(APPLY))
