import unreal
S = {"h": None, "t0": None, "worst": 999.0, "phase": "run"}
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
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")
    pawn.do_move(0.0, 1.0)
    if age > 0.6:
        floor = pawn.get_actor_location().z - half
        for b in ("foot_r","foot_l","ball_r","ball_l"):
            try: S["worst"] = min(S["worst"], mesh.get_socket_location(b).z - floor)
            except Exception: pass
    if age >= 2.5:
        unreal.log("CTRL running-only lowest foot = %+.1f cm vel=%.0f mesh_rel_z=%.0f" % (
            S["worst"], pawn.get_velocity().length(),
            mesh.get_editor_property("relative_location").z))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("CTRL COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("CTRL armed")
