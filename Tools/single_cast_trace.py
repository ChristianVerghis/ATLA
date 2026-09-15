"""Trace leg geometry every tick through ONE cast (no spam)."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "rows": []}
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
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.WATER)
        return
    age = now - S["t0"]
    mesh = pawn.get_editor_property("mesh")
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")
    if S["phase"] == "setup" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        if age >= 0.8:
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
            S["phase"] = "trace"; S["t0"] = now
    elif S["phase"] == "trace":
        pawn.do_move(0.0, 1.0)
        floor = pawn.get_actor_location().z - half
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        S["rows"].append((age, min((fr-pel).length(), (fl-pel).length()),
                          min(fr.z, fl.z) - floor, pel.z - floor))
        if age >= 1.0:
            bad = [r for r in S["rows"] if r[1] < 45 or r[2] > 45]
            unreal.log("SCT samples=%d broken_frames=%d" % (len(S["rows"]), len(bad)))
            for r in S["rows"][::3][:12]:
                unreal.log("SCT t=%.2f legmin=%.0f footZ=%+.0f pelvisZ=%.0f" % r)
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("SCT COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("SCT armed")
