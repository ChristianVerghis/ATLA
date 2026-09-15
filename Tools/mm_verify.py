"""Is motion matching live and animating? Check the anim instance class, the
trajectory component, and that the legs actually cycle while running."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "feet": []}
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
    if S["phase"] == "setup" and age >= 1.5:
        ai = mesh.get_anim_instance()
        traj = pawn.get_components_by_class(unreal.CharacterTrajectoryComponent)
        unreal.log("MMV animinstance=%s trajectory_components=%d" % (
            ai.get_class().get_name() if ai else "NONE", len(traj)))
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        pel = mesh.get_socket_location("pelvis")
        S["feet"].append((mesh.get_socket_location("foot_r") - pel).z)
        if age >= 2.0:
            lo, hi = min(S["feet"]), max(S["feet"])
            unreal.log("MMV running: foot height range %.0f..%.0f (stride=%.0f) vel=%.0f -> %s" % (
                lo, hi, hi-lo, pawn.get_velocity().length(),
                "LEGS CYCLING" if hi-lo > 12 else "NOT ANIMATING"))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("MMV COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("MMV armed")
