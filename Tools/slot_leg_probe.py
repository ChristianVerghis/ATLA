import unreal
S = {"h": None, "t0": None, "feet": [], "hands": [], "played": False}
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
    mesh = pawn.get_editor_property("mesh")
    if S["t0"] is None:
        S["t0"] = now; return
    age = now - S["t0"]
    if not S["played"] and age >= 1.0:
        mesh.set_editor_property("disable_post_process_blueprint", True)
        seq = unreal.load_asset("/Game/Anims/Mixamo/MXQ_MX_chapa_2")
        anim = mesh.get_anim_instance()
        m = anim.play_slot_animation_as_dynamic_montage(seq, "DefaultSlot", 0.15, 0.35, 1.1, 1, -1.0, 0.35)
        unreal.log("SL montage played (postprocess OFF): %s" % (m is not None))
        S["played"] = True; S["t0"] = now; S["feet"] = []; S["hands"] = []
    elif S["played"]:
        if age < 1.1:
            S["feet"].append(mesh.get_socket_location("foot_r").z - pawn.get_actor_location().z)
            S["hands"].append(mesh.get_socket_location("hand_r").z - pawn.get_actor_location().z)
        else:
            unreal.log("SL DefaultSlot chapa_2: foot_r range=%.0f (kick should be >40) hand range=%.0f n=%d"
                % (max(S["feet"])-min(S["feet"]), max(S["hands"])-min(S["hands"]), len(S["feet"])))
            unreal.log("SL COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("SL armed")
