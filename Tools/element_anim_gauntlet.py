"""Per-element animation gauntlet: cast primary + secondary for each element,
verify (a) mesh stays with the capsule (no invisibility: pelvis offset), and
(b) the technique actually animates (limb travel). Screenshots per cast.
PASS: pelvis offset < 150cm AND limb travel > 12cm.
"""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)

TESTS = [
    ("WATER", unreal.ATLAElement.WATER, [("whip", "ATLAAbility_WaterWhip"), ("spears", "ATLAAbility_IceSpears")]),
    ("EARTH", unreal.ATLAElement.EARTH, [("rockjab", "ATLAAbility_RockJab"), ("spikes", "ATLAAbility_EarthSpikes")]),
    ("FIRE", unreal.ATLAElement.FIRE, [("jab", "ATLAAbility_FireJab"), ("lash", "ATLAAbility_FireLash")]),
    ("AIR", unreal.ATLAElement.AIR, [("blast", "ATLAAbility_AirBlast"), ("swipe", "ATLAAbility_AirSwipe")]),
]

S = {"h": None, "t0": None, "pt": 0.0, "ei": 0, "ai": 0, "phase": "switch",
     "pelvis_max": 0.0, "hand0": None, "hand_max": 0.0, "sampling": False}


def shot(name):
    unreal.AutomationLibrary.take_high_res_screenshot(1100, 650, os.path.join(SHOTS, name))


def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        S["pt"] = now
        return
    age = now - S["pt"]

    if S["ei"] >= len(TESTS):
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("EG COMPLETE")
        return

    ename, eval_, abilities = TESTS[S["ei"]]
    mesh = pawn.get_editor_property("mesh")

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if S["sampling"]:
        pl = pawn.get_actor_location()
        pv = mesh.get_socket_location("pelvis")
        off = unreal.Vector(pv.x - pl.x, pv.y - pl.y, 0).length()
        S["pelvis_max"] = max(S["pelvis_max"], off)
        h = mesh.get_socket_location("hand_r") - pl
        if S["hand0"] is None:
            S["hand0"] = h
            S["yaw0"] = mesh.get_socket_rotation("pelvis").yaw
        S["hand_max"] = max(S["hand_max"], (h - S["hand0"]).length())
        dy = abs(mesh.get_socket_rotation("pelvis").yaw - S["yaw0"])
        if dy > 180:
            dy = 360 - dy
        S["yaw_max"] = max(S.get("yaw_max", 0.0), dy)
        # Feet below the capsule floor = the "legs disappear" bug
        cap = pawn.get_editor_property("capsule_component")
        floor = pl.z - cap.get_editor_property("capsule_half_height")
        for b in ("foot_r", "foot_l", "ball_r", "ball_l"):
            try:
                S["sink"] = min(S.get("sink", 999.0), mesh.get_socket_location(b).z - floor)
            except Exception:
                pass

    if S["phase"] == "switch" and age >= 0.4:
        pawn.set_element_loadout(eval_)
        goto("settle")
    elif S["phase"] == "settle" and age >= 1.2:
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-5.0, yaw=0.0))
        goto("aimwait")
    elif S["phase"] == "aimwait" and age >= 0.5:
        aname, cls = abilities[S["ai"]]
        asc = pawn.get_editor_property("ability_system_component")
        ok = asc.try_activate_ability_by_class(getattr(unreal, cls))
        S["pelvis_max"], S["hand0"], S["hand_max"], S["yaw_max"] = 0.0, None, 0.0, 0.0
        S["sink"] = 999.0
        S["sampling"] = True
        S["v_name"] = "%s_%s" % (ename, aname)
        S["v_ok"] = ok
        goto("watch")
    elif S["phase"] == "watch":
        if abs(age - 0.3) < 0.05:
            shot("eg_%s.png" % S["v_name"])
        if age >= 0.7:
            S["sampling"] = False
            verdict = "PASS" if (S["pelvis_max"] < 150 and S["hand_max"] > 12
                                 and S["yaw_max"] < 60 and S["sink"] > -18) else "FAIL"
            unreal.log("EG %s cast=%s pelvis_off=%.0f hand_travel=%.0f yaw_dev=%.0f sink=%+.0f -> %s" % (
                S["v_name"], S["v_ok"], S["pelvis_max"], S["hand_max"], S["yaw_max"], S["sink"], verdict))
            S["ai"] += 1
            if S["ai"] >= len(abilities):
                S["ai"] = 0
                S["ei"] += 1
                goto("switch")
            else:
                goto("aimwait")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("EG armed")
