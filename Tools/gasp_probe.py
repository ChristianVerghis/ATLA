"""Swap the player mesh onto GASP's retarget AnimBP at runtime and see whether
the graph animates on its own or needs pawn-side data."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "poses": []}
def snap(mesh):
    pel = mesh.get_socket_location("pelvis")
    return (mesh.get_socket_location("hand_r") - pel, mesh.get_socket_location("foot_r") - pel)
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
    if S["phase"] == "setup" and age >= 1.0:
        cls = unreal.load_class(None, '/Game/Blueprints/RetargetedCharacters/ABP_GenericRetarget.ABP_GenericRetarget_C')
        unreal.log("GSP anim class loaded: %s" % bool(cls))
        mesh.set_anim_instance_class(cls)
        S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        pawn.do_move(0.0, 1.0)
        S["poses"].append(snap(mesh))
        if age >= 2.0:
            hs = max((a[0]-S["poses"][0][0]).length() for a in S["poses"])
            fs = max((a[1]-S["poses"][0][1]).length() for a in S["poses"])
            ai = mesh.get_anim_instance()
            unreal.log("GSP after swap: animinstance=%s hand_var=%.0f foot_var=%.0f vel=%.0f -> %s" % (
                ai.get_class().get_name() if ai else 'NONE', hs, fs, pawn.get_velocity().length(),
                "ANIMATING" if fs > 10 else "STATIC (needs pawn data)"))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("GSP COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("GSP armed")
