"""Does a real AnimMontage ASSET made from a Mixamo clip play correctly,
where the runtime-built dynamic montage collapses the legs?"""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "vals": []}
def legmin(mesh):
    pel = mesh.get_socket_location("pelvis")
    fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
    return min((fr-pel).length(), (fl-pel).length())
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
        m = unreal.load_asset('/Game/Anims/Mixamo/Montages/AM_TEST_CrossPunch.AM_TEST_CrossPunch')
        pawn.play_anim_montage(m, 1.0, "")
        S["phase"] = "measure"; S["t0"] = now
    elif S["phase"] == "measure":
        S["vals"].append(legmin(mesh))
        if age >= 1.2:
            unreal.log("MAT montage ASSET: legmin=%.0f legmax=%.0f -> %s" % (
                min(S["vals"]), max(S["vals"]),
                "WORKS" if min(S["vals"]) > 60 else "still collapsed"))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("MAT COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("MAT armed")
