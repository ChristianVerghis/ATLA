"""Locomotion diagnostics: foot sliding (feet should be planted while a foot
is in contact) and whether the foot IK control rig adapts on a slope."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "slide": [], "prev": None}
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
    mesh = pawn.get_editor_property("mesh")
    if S["phase"] == "setup" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        fr = mesh.get_socket_location("foot_r")
        fl = mesh.get_socket_location("foot_l")
        # the planted foot (lower one) should be still in world space
        planted = fr if fr.z < fl.z else fl
        if S["prev"] is not None and age > 0.4:
            moved = (planted - S["prev"]).length() / max(dt, 0.001)
            S["slide"].append(moved)
        S["prev"] = planted
        if age >= 2.5:
            sl = sorted(S["slide"])
            med = sl[len(sl)//2] if sl else 0
            unreal.log("LOC vel=%.0f  planted-foot world speed: median=%.0f cm/s (0=planted, ~vel=sliding)" % (
                pawn.get_velocity().length(), med))
            S["phase"] = "slope"; S["t0"] = now
    elif S["phase"] == "slope" and age >= 0.2:
        # drop onto the arctic slope area and compare foot heights (IK adapts per-foot)
        pawn.set_actor_location(unreal.Vector(700, -20, 200), False, True)
        S["phase"] = "slopewait"; S["t0"] = now
    elif S["phase"] == "slopewait" and age >= 1.5:
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        unreal.log("LOC on ground: footR.z=%.1f footL.z=%.1f delta=%.1f (IK adapts per foot if delta varies)" % (
            fr.z, fl.z, abs(fr.z-fl.z)))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("LOC COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("LOC armed")
