"""Same clip, two playback paths: direct play_animation vs dynamic montage in
DefaultSlot. Isolates whether the clip or the AnimBP slot path is at fault."""
import unreal
CLIP = "/Game/Anims/Mixamo/MXQ_MX_Cross_Punch_Anim_mixamo_com.MXQ_MX_Cross_Punch_Anim_mixamo_com"
S = {"h": None, "t0": None, "phase": "setup", "direct": [], "slot": []}
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
    anim = unreal.load_asset(CLIP)
    if S["phase"] == "setup" and age >= 1.0:
        mesh.play_animation(anim, False)
        mesh.set_position(1.0, False)
        mesh.set_play_rate(0.0)
        S["phase"] = "direct"; S["t0"] = now
    elif S["phase"] == "direct":
        S["direct"].append(legmin(mesh))
        if age >= 0.6:
            S["phase"] = "restore"; S["t0"] = now
    elif S["phase"] == "restore" and age >= 0.1:
        # back to the AnimBP, then play the same clip through the slot
        mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_BLUEPRINT)
        S["phase"] = "slotplay"; S["t0"] = now
    elif S["phase"] == "slotplay" and age >= 0.5:
        ai = mesh.get_anim_instance()
        ai.play_slot_animation_as_dynamic_montage(anim, "DefaultSlot", 0.15, 0.35, 1.0, 1, -1.0, 1.0)
        S["phase"] = "slotmeasure"; S["t0"] = now
    elif S["phase"] == "slotmeasure":
        S["slot"].append(legmin(mesh))
        if age >= 0.6:
            unreal.log("BIS direct play: min=%.0f max=%.0f | slot montage: min=%.0f max=%.0f" % (
                min(S["direct"]), max(S["direct"]), min(S["slot"]), max(S["slot"])))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("BIS COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("BIS armed")
