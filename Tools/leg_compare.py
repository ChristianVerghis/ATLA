"""Diff leg geometry during a RUNNING cast: Mixamo-driven (water/fire) vs
CMU-driven (earth, reported fine)."""
import unreal
TESTS = [("WATER", unreal.ATLAElement.WATER, "ATLAAbility_WaterWhip"),
         ("EARTH", unreal.ATLAElement.EARTH, "ATLAAbility_RockJab"),
         ("FIRE",  unreal.ATLAElement.FIRE,  "ATLAAbility_FireJab")]
S = {"h": None, "t0": None, "i": 0, "phase": "switch", "m": None}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        # PIE ended under us: release every reference and unregister, or the
        # held objects keep the game viewport alive and the editor asserts
        # (GameViewport.IsUnique) on the next PIE teardown.
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]
            S.clear()
            unreal.unregister_slate_post_tick_callback(h)
            unreal.log("probe auto-disarmed (PIE ended)")
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn: return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now; return
    age = now - S["t0"]
    if S["i"] >= len(TESTS):
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("LC COMPLETE"); return
    name, elem, cls = TESTS[S["i"]]
    mesh = pawn.get_editor_property("mesh")
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")
    if S["phase"] == "switch" and age >= 0.5:
        pawn.set_element_loadout(elem); S["phase"] = "place"; S["t0"] = now
    elif S["phase"] == "place" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        if age >= 0.8:
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(getattr(unreal, cls))
            S["m"] = {"pel": [], "foot": [], "leglen": [], "stride": []}
            S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        pawn.do_move(0.0, 1.0)
        floor = pawn.get_actor_location().z - half
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r")
        fl = mesh.get_socket_location("foot_l")
        S["m"]["pel"].append(pel.z - floor)
        S["m"]["foot"].append(min(fr.z, fl.z) - floor)
        S["m"]["leglen"].append(max((fr - pel).length(), (fl - pel).length()))
        S["m"]["stride"].append((fr - fl).length())
        if age >= 0.9:
            m = S["m"]
            unreal.log("LC %-5s pelvisZ %.0f..%.0f | lowest foot %+.0f | leglen max %.0f | stride max %.0f" % (
                name, min(m["pel"]), max(m["pel"]), min(m["foot"]),
                max(m["leglen"]), max(m["stride"])))
            S["i"] += 1; S["phase"] = "switch"; S["t0"] = now
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("LC armed")
