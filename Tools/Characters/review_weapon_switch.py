"""PIE review of actual right-click weapon switching, with isolated test saves.

Launch a rendering editor with -RenderOffscreen and -ExecCmds="py <this file>".
Creates no content/map assets. Captures go to Saved/LiveCharacterReview and a
machine-readable outcome goes to Saved/WeaponSwitchLiveReview.json.
"""
import json
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
UI_ONLY = "-ICWeaponUIOnly" in u.SystemLibrary.get_command_line()
OUT = ROOT / ("Saved/WeaponSwitchUIReview.json" if UI_ONLY else "Saved/WeaponSwitchLiveReview.json")
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
STATE = {"phase": "starting", "started": time.monotonic(), "samples": [], "complete": False}


def write():
    OUT.write_text(json.dumps({key: value for key, value in STATE.items()
                              if key in {"phase", "samples", "complete", "error"}},
                             ensure_ascii=False, indent=2), encoding="utf-8")


def check(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def is_empty(item):
    return str(item.item_id) == "None" or item.count <= 0


def sample(label, capture=True):
    player, inventory = STATE["player"], STATE["inventory"]
    slots = inventory.get_weapon_loadout()
    weapon = player.get_editor_property("equipped_weapon_actor")
    item = {"case": label, "preset": STATE["preset"],
            "menu": str(STATE["pc"].get_editor_property("menu")),
            "family": str(inventory.get_equipped_weapon_family()),
            "put_away": inventory.is_weapon_sheathed(), "switching": player.is_switching_weapon(),
            "weapon_hidden": weapon.get_editor_property("hidden") if weapon else True,
            "slot_ids": [slot.instance_id.to_string() for slot in slots]}
    check(item["weapon_hidden"] == item["put_away"], "Weapon visibility does not match the stored draw state")
    if STATE.get("expected_ids"):
        check(item["slot_ids"] == STATE["expected_ids"], "Cycling moved a weapon out of its family equipment slot")
    STATE["samples"].append(item)
    if capture:
        filename = (ROOT / "Saved/LiveCharacterReview" / ("weapon_" + label + ".png")).as_posix()
        command = f'shot showui filename="{filename}" -nosuffix' if "equipment" in label else f'HighResShot 1280x720 filename="{filename}"'
        u.SystemLibrary.execute_console_command(STATE["world"], command)
    write()


def camera_to_player():
    player = STATE["player"]
    focus = player.get_actor_location() + u.Vector(0, 0, -8)
    location = focus + player.get_actor_forward_vector() * 480 + player.get_actor_right_vector() * 280 + u.Vector(0, 0, 80)
    STATE["camera"].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)


def grant_loadout():
    inventory = STATE["inventory"]
    for index, identifier in enumerate(["EQP_WPN_SWORD_001", "EQP_WPN_BOW_001", "EQP_WPN_STAFF_001", "EQP_WPN_WAND_001"]):
        if not is_empty(inventory.get_weapon_loadout()[index]):
            continue
        check(inventory.add_item(identifier, 1, False), "Grant failed: " + identifier)
        snapshot = inventory.get_snapshot()
        slot = next(i for i, value in enumerate(snapshot.bag.slots) if str(value.item_id) == identifier)
        check(inventory.equip(slot), "Equip failed: " + identifier)


def begin_click():
    STATE["pc"].handle_input_command(u.ICInputCommand.SWITCH_WEAPON, 0)
    check(STATE["player"].is_switching_weapon(), "Right click did not start a weapon montage")
    check(not STATE["player"].request_cycle_weapon(), "Repeated right click was not blocked")
    STATE.update(phase="switching", click_started=time.monotonic(),
                 previous_away=STATE["inventory"].is_weapon_sheathed(), events=0)
    sample(f'{STATE["preset"]}_{STATE["count"]}_{STATE["click"]}_start')


def tick(delta):
    try:
        now = time.monotonic()
        check(now - STATE["started"] < 180, "Live review timed out")
        if STATE["phase"] == "starting":
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            world = worlds[0]
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc or str(pc.get_editor_property("menu")) != "Create":
                return
            check(u.ICCharacterSetupLibrary.prepare_live_character_review(world), "Could not create isolated review player")
            STATE.update(world=world, pc=pc, phase="creating")
            return
        if STATE["phase"] == "creating":
            pc = STATE["pc"]
            if not pc.get_editor_property("created"):
                return
            player = u.GameplayStatics.get_player_pawn(STATE["world"], 0)
            inventory = pc.get_inventory()
            check(sum(not is_empty(slot) for slot in inventory.get_weapon_loadout()) == 1, "New game must grant exactly one weapon")
            check(inventory.is_weapon_sheathed(), "New game must start with empty hands")
            player.apply_preset(0)
            player.set_actor_location_and_rotation(u.Vector(-1000, -2500, 90), u.Rotator(), False, True)
            camera = player.get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property("field_of_view", 42)
            STATE.update(player=player, inventory=inventory, camera=camera, preset="M", count=1, click=0,
                         phase="settling", ready_at=now + 2)
            camera_to_player()
            if UI_ONLY:
                grant_loadout()
                STATE["count"] = 4
                pc.handle_input_command(u.ICInputCommand.EQUIPMENT, 0)
                STATE.update(phase="equipment_capture", ready_at=now + 1)
            return
        camera_to_player()
        if STATE["phase"] == "settling" and now >= STATE["ready_at"]:
            sample(f'{STATE["preset"]}_{STATE["count"]}_empty')
            STATE["click"] += 1
            begin_click()
        elif STATE["phase"] == "switching":
            away = STATE["inventory"].is_weapon_sheathed()
            if away != STATE["previous_away"]:
                STATE["events"] += 1
                sample(f'{STATE["preset"]}_{STATE["count"]}_{STATE["click"]}_notify{STATE["events"]}')
                STATE["previous_away"] = away
            check(now - STATE["click_started"] < 10, "A montage left weapon switching stuck")
            if not STATE["player"].is_switching_weapon():
                sample(f'{STATE["preset"]}_{STATE["count"]}_{STATE["click"]}_done')
                if STATE["click"] < STATE["count"] + 1:
                    STATE["click"] += 1
                    begin_click()
                else:
                    check(away, "One full cycle did not return to empty hands")
                    if STATE["count"] == 1:
                        grant_loadout()
                        STATE["expected_ids"] = [slot.instance_id.to_string() for slot in STATE["inventory"].get_weapon_loadout()]
                        STATE.update(count=4, click=0, phase="settling", ready_at=now + .7)
                    elif STATE["preset"] == "M":
                        STATE["pc"].handle_input_command(u.ICInputCommand.EQUIPMENT, 0)
                        sample("M_equipment_four_slots")
                        STATE.update(phase="equipment", ready_at=now + 1)
                    else:
                        STATE.update(phase="finishing", ready_at=now + 1)
        elif STATE["phase"] == "equipment" and now >= STATE["ready_at"]:
            STATE["pc"].handle_input_command(u.ICInputCommand.MENU, 0)
            STATE["player"].apply_preset(1)
            STATE.update(preset="F", click=0, phase="settling", ready_at=now + 1)
        elif STATE["phase"] == "equipment_capture" and now >= STATE["ready_at"]:
            sample("equipment_four_slots")
            STATE.update(phase="finishing", ready_at=now + 1)
        elif STATE["phase"] == "finishing" and now >= STATE["ready_at"]:
            saves = STATE["pc"].get_saves()
            if saves.get_editor_property("saving"):
                return
            saves.delete_slot(0)
            STATE.update(complete=True, phase="complete")
            write()
            LEVELS.editor_request_end_play()
            u.unregister_slate_post_tick_callback(STATE["handle"])
            u.SystemLibrary.quit_editor()
    except Exception:
        STATE.update(phase="error", error=traceback.format_exc())
        write()
        u.log_error(STATE["error"])
        u.unregister_slate_post_tick_callback(STATE["handle"])
        LEVELS.editor_request_end_play()
        u.SystemLibrary.quit_editor()


try:
    check(not u.EditorLevelLibrary.get_pie_worlds(False), "An unrelated PIE session is already running")
    u.EditorLoadingAndSavingUtils.load_map("/Game/IsekaiClass/World/L_TestMap")
    STATE["handle"] = u.register_slate_post_tick_callback(tick)
    LEVELS.editor_request_begin_play()
    write()
except Exception:
    STATE.update(phase="error", error=traceback.format_exc())
    write()
    u.log_error(STATE["error"])
    u.SystemLibrary.quit_editor()
