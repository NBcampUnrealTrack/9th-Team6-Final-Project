"""Interactive visual review in the actual L_TestMap PIE session.

Run with -ExecCmds="py <absolute path to this script>" or the editor console.
The asynchronous capture loop must remain in an interactive editor. Saves use an isolated
namespace. Requests in Saved/LiveCharacterReview/control.json select an actor
and movement; PNGs are screenshots of the running game viewport, not previews.
No content assets or map packages are saved.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / "Saved/LiveCharacterReview"
OUT.mkdir(parents=True, exist_ok=True)
CONTROL = OUT / "control.json"
STATUS = OUT / "status.json"
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
SAMPLES = OUT / "samples.json"
STATE = {"phase": "starting", "started": time.monotonic(), "seen": None, "command": {}, "frames": [],
         "samples": json.loads(SAMPLES.read_text(encoding="utf-8")) if SAMPLES.exists() else []}


def write_status(**extra):
    data = {key: STATE[key] for key in ["phase", "command", "frames"]}
    data.update(extra)
    STATUS.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def camera_to(actor, sleeping, back=False):
    mesh = actor.get_editor_property("mesh")
    if sleeping:
        points = [mesh.get_socket_location(name) for name in ["J_Bip_C_Head", "J_Bip_C_Hips", "J_Bip_L_Foot", "J_Bip_R_Foot"]]
        focus = sum(points, u.Vector()) / len(points)
        offset = u.Vector(300, 300 if not back else -300, 330)
    else:
        focus = actor.get_actor_location() + u.Vector(0, 0, 3)
        offset = actor.get_actor_forward_vector() * (360 if not back else -360) + actor.get_actor_right_vector() * 280 + u.Vector(0, 0, 65)
    location = focus + offset
    STATE["camera"].set_world_location_and_rotation(location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)


def restore_target():
    target = STATE.get("target")
    if target and isinstance(target, u.ICNPCBase):
        target.get_component_by_class(u.CharacterMovementComponent).set_editor_property("max_walk_speed", STATE.get("original_speed", 180))
        if not STATE["command"].get("mode", "").endswith("sleep"):
            target.set_actor_transform(STATE["original_transform"], False, True)
            target.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()


def begin_command(command):
    if command.get("mode") not in {"player_walk", "player_run", "player_sleep", "npc_walk", "npc_run", "npc_sleep"}:
        raise ValueError(f"Unknown review mode: {command.get('mode')}")
    restore_target()
    STATE["command"] = command
    STATE["frames"] = []
    STATE["capture_index"] = 0
    STATE["case_started"] = time.monotonic()
    STATE["phase"] = "settling"
    STATE["sleep_result"] = None
    mode = command["mode"]
    pc, world = STATE["pc"], STATE["world"]
    if mode.startswith("player"):
        target = u.GameplayStatics.get_player_pawn(world, 0)
        target.apply_preset(int(command.get("preset", 0)))
        target.set_actor_location_and_rotation(u.Vector(-1000, -2500, 90), u.Rotator(0, 0, 0), False, True)
        target.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
        STATE["scale"] = .30 if mode.endswith("walk") else 1.0
        if mode == "player_sleep":
            # Exercise the real gameplay action. Do not force a lying pose that
            # the player controller/animation graph does not currently support.
            before = STATE["time"].get_clock().game_minute
            succeeded = pc.sleep()
            STATE["sleep_result"] = {"succeeded": succeeded, "minutes_advanced": STATE["time"].get_clock().game_minute - before}
    else:
        identifier = command.get("npc", "NPC_R01_001")
        target = next(actor for actor in STATE["npcs"] if str(actor.get_character_id()) == identifier)
        STATE["original_transform"] = target.get_actor_transform()
        STATE["original_speed"] = target.get_component_by_class(u.CharacterMovementComponent).get_editor_property("max_walk_speed")
        if mode.endswith("sleep"):
            clock = STATE["time"].get_clock()
            minute = clock.game_minute % 1440
            if not 1230 <= minute < 1400:
                STATE["time"].jump_game_minutes((1230 - minute) % 1440)
        else:
            # AI still uses its normal schedule/path and the native animation graph.
            # A temporary speed of 600 exercises the run blend, since schedules
            # normally use walking speed. All changes live only in this PIE world.
            target.get_component_by_class(u.CharacterMovementComponent).set_editor_property("max_walk_speed", 180 if mode.endswith("walk") else 600)
            position = STATE["row_positions"][identifier]
            target.set_actor_location(position - u.Vector(2200, 0, 0), False, True)
            target.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
    STATE["target"] = target
    pc.set_view_target_with_blend(u.GameplayStatics.get_player_pawn(world, 0), 0)
    camera_to(target, mode.endswith("sleep"))
    write_status(target=target.get_name())


def tick(delta):
    try:
        now = time.monotonic()
        if STATE["phase"] == "starting":
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            world = worlds[0]
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc or str(pc.get_editor_property("menu")) != "Create":
                return
            if not u.ICCharacterSetupLibrary.prepare_live_character_review(world):
                raise RuntimeError("Could not isolate and create the PIE review player")
            STATE.update(world=world, pc=pc, phase="creating")
            write_status()
            return
        if STATE["phase"] == "creating":
            if not STATE["pc"].get_editor_property("created"):
                return
            npcs = [actor for actor in u.GameplayStatics.get_all_actors_of_class(STATE["world"], u.ICNPCBase)
                    if str(actor.get_character_id()).startswith("NPC_R01_0")]
            if len(npcs) != 11:
                return
            camera = u.GameplayStatics.get_player_pawn(STATE["world"], 0).get_component_by_class(u.CameraComponent)
            camera.detach_from_component(u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD, u.DetachmentRule.KEEP_WORLD)
            camera.set_editor_property("field_of_view", 45)
            STATE.update(npcs=npcs, camera=camera,
                         time=STATE["pc"].get_time(),
                         row_positions={str(actor.get_character_id()): actor.get_actor_location() for actor in npcs},
                         phase="ready")
            STATE["pc"].set_menu("")
            write_status(npcs=len(npcs))
        if CONTROL.exists():
            text = CONTROL.read_text(encoding="utf-8-sig")
            if text != STATE["seen"]:
                # An external writer may briefly truncate the control file.
                # Keep the current review alive and read the completed request
                # on the next tick instead of aborting the capture session.
                try:
                    command = json.loads(text)
                except json.JSONDecodeError:
                    return
                STATE["seen"] = text
                if command.get("mode") == "stop":
                    restore_target()
                    u.unregister_slate_post_tick_callback(STATE["handle"])
                    STATE["phase"] = "stopped"
                    write_status()
                    return
                begin_command(command)
        target = STATE.get("target")
        if not target:
            return
        command = STATE["command"]
        mode = command["mode"]
        if mode.startswith("player") and mode != "player_sleep" and STATE["phase"] != "captured":
            target.add_movement_input(u.Vector(1, 0, 0), STATE["scale"], False)
        anim = target.get_editor_property("mesh").get_anim_instance()
        sleeping = bool(anim.get_editor_property("sleeping"))
        elapsed = now - STATE["case_started"]
        velocity = target.get_velocity()
        speed = math.hypot(velocity.x, velocity.y)
        ready = True if mode == "player_sleep" else (sleeping if mode.endswith("sleep") else speed > (350 if mode.endswith("run") else 80))
        camera_to(target, sleeping, command.get("view") == "back" or (command.get("view") != "front" and STATE["capture_index"] >= 4))
        if STATE["phase"] != "captured" and ready and elapsed > float(command.get("settle_seconds", .8)):
            STATE["phase"] = "recording"
            if now - STATE.get("last_capture", 0) > float(command.get("frame_interval", .32)):
                index = STATE["capture_index"]
                case = command.get("id", mode)
                filename = f"{case}_{index:02}"
                u.ICCharacterSetupLibrary.capture_live_character_review(STATE["world"], filename)
                STATE["frames"].append(filename + ".png")
                STATE["samples"].append({"case": case, "frame": index, "speed": round(speed, 3), "sleeping": sleeping,
                                         "delta": delta, "mesh": target.get_editor_property("mesh").get_editor_property("skeletal_mesh").get_path_name(),
                                         "anim": anim.get_class().get_name(), "sleep_action": STATE["sleep_result"]})
                STATE["capture_index"] += 1
                STATE["last_capture"] = now
                if STATE["capture_index"] >= int(command.get("frame_count", 8)):
                    STATE["phase"] = "captured"
                    SAMPLES.write_text(json.dumps(STATE["samples"], ensure_ascii=False, indent=2), encoding="utf-8")
        if now - STATE.get("last_status", 0) > .5:
            STATE["last_status"] = now
            write_status(speed=round(speed, 2), sleeping=sleeping, elapsed=round(elapsed, 2))
    except Exception:
        STATE["phase"] = "error"
        write_status(error=traceback.format_exc())
        u.log_error(traceback.format_exc())
        u.unregister_slate_post_tick_callback(STATE["handle"])


existing = u.EditorLevelLibrary.get_pie_worlds(False)
if existing:
    pc = u.GameplayStatics.get_player_controller(existing[0], 0)
    if str(pc.get_editor_property("player_name")) != "VisualReview":
        raise RuntimeError("An unrelated PIE session is already running")
    STATE.update(world=existing[0], pc=pc, phase="creating")
else:
    u.EditorLoadingAndSavingUtils.load_map("/Game/IsekaiClass/World/L_TestMap")
STATE["handle"] = u.register_slate_post_tick_callback(tick)
if not existing:
    LEVELS.editor_request_begin_play()
write_status()
u.log("IC_LIVE_REVIEW_STARTING")
