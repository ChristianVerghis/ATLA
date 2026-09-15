"""How far do the feet sink below the capsule floor during each cast?
Negative = through the ground (the "legs disappear" bug)."""
import unreal

TESTS = [("FIRE", unreal.ATLAElement.FIRE, "ATLAAbility_FireJab"),
         ("FIRE2", unreal.ATLAElement.FIRE, "ATLAAbility_FireLash"),
         ("WATER", unreal.ATLAElement.WATER, "ATLAAbility_WaterWhip"),
         ("AIR", unreal.ATLAElement.AIR, "ATLAAbility_AirSwipe")]

S = {"h": None, "t0": None, "i": 0, "phase": "switch", "worst": 0.0}


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
        unreal.log("FLR COMPLETE")
        return

    name, elem, cls = TESTS[S["i"]]
    mesh = pawn.get_editor_property("mesh")
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")

    if S["phase"] == "switch" and age >= 0.5:
        pawn.set_element_loadout(elem)
        S["phase"] = "settle"
        S["t0"] = now
    elif S["phase"] == "settle" and age >= 1.2:
        asc = pawn.get_editor_property("ability_system_component")
        asc.try_activate_ability_by_class(getattr(unreal, cls))
        S["worst"] = 999.0
        S["phase"] = "watch"
        S["t0"] = now
    elif S["phase"] == "watch":
        floor = pawn.get_actor_location().z - half
        for b in ("foot_r", "foot_l", "ball_r", "ball_l"):
            try:
                S["worst"] = min(S["worst"], mesh.get_socket_location(b).z - floor)
            except Exception:
                pass
        if age >= 1.0:
            verdict = "OK" if S["worst"] > -8 else "SINKING"
            unreal.log("FLR %-6s lowest foot vs floor = %+.1f cm -> %s" % (name, S["worst"], verdict))
            S["i"] += 1
            S["phase"] = "switch"
            S["t0"] = now


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FLR armed")
