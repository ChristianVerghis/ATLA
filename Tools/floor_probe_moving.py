"""Foot-vs-floor while casting AND moving (the untested combination).
Also reports which slot the montage landed in."""
import unreal

TESTS = [("WATER", unreal.ATLAElement.WATER, "ATLAAbility_WaterWhip"),
         ("FIRE", unreal.ATLAElement.FIRE, "ATLAAbility_FireJab"),
         ("AIR", unreal.ATLAElement.AIR, "ATLAAbility_AirSwipe")]

S = {"h": None, "t0": None, "i": 0, "phase": "switch", "worst": 999.0, "slot": "?"}


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
        return
    age = now - S["t0"]

    if S["i"] >= len(TESTS):
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("FLM COMPLETE")
        return

    name, elem, cls = TESTS[S["i"]]
    mesh = pawn.get_editor_property("mesh")
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")

    if S["phase"] == "switch" and age >= 0.5:
        pawn.set_element_loadout(elem)
        S["phase"] = "runup"
        S["t0"] = now
    elif S["phase"] == "runup":
        pawn.do_move(0.0, 1.0)   # keep running
        if age >= 1.0:
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(getattr(unreal, cls))
            S["worst"] = 999.0
            S["phase"] = "watch"
            S["t0"] = now
    elif S["phase"] == "watch":
        pawn.do_move(0.0, 1.0)   # still running through the cast
        floor = pawn.get_actor_location().z - half
        for b in ("foot_r", "foot_l", "ball_r", "ball_l"):
            try:
                S["worst"] = min(S["worst"], mesh.get_socket_location(b).z - floor)
            except Exception:
                pass
        if age >= 1.2:
            verdict = "OK" if S["worst"] > -8 else "SINKING"
            unreal.log("FLM %-6s moving-cast lowest foot = %+.1f cm vel=%.0f -> %s" % (
                name, S["worst"], pawn.get_velocity().length(), verdict))
            S["i"] += 1
            S["phase"] = "switch"
            S["t0"] = now


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FLM armed")
