"""Clean A/B: CMU clip vs Mixamo clip, both through DefaultSlot, identical
conditions, no animation-mode switching."""
import unreal
CMU = "/Game/Anims/CMU_Manny/MNY_135_09.MNY_135_09"
MIX = "/Game/Anims/Mixamo/MXQ_MX_Cross_Punch_Anim_mixamo_com.MXQ_MX_Cross_Punch_Anim_mixamo_com"
S = {"h": None, "t0": None, "phase": "setup", "a": [], "b": []}
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
    ai = mesh.get_anim_instance()
    if S["phase"] == "setup" and age >= 1.5:
        ai.play_slot_animation_as_dynamic_montage(unreal.load_asset(CMU), "DefaultSlot", 0.15, 0.35, 1.0, 1, -1.0, 2.0)
        S["phase"] = "cmu"; S["t0"] = now
    elif S["phase"] == "cmu":
        S["a"].append(legmin(mesh))
        if age >= 0.7:
            S["phase"] = "gap"; S["t0"] = now
    elif S["phase"] == "gap" and age >= 1.0:
        ai.play_slot_animation_as_dynamic_montage(unreal.load_asset(MIX), "DefaultSlot", 0.15, 0.35, 1.0, 1, -1.0, 1.0)
        S["phase"] = "mix"; S["t0"] = now
    elif S["phase"] == "mix":
        S["b"].append(legmin(mesh))
        if age >= 0.7:
            unreal.log("BS2 CMU-in-slot min=%.0f max=%.0f | MIXAMO-in-slot min=%.0f max=%.0f" % (
                min(S["a"]), max(S["a"]), min(S["b"]), max(S["b"])))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("BS2 COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("BS2 armed")
