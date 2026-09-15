import unreal
S = {"h": None, "t0": None, "n": 0}
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
    pawn.do_move(0.0, 1.0)
    S["n"] += 1
    if age >= 1.5:
        mv = pawn.get_movement_component()
        unreal.log("MO ticks=%d vel=%.0f mode=%s maxwalk=%.0f animclass=%s" % (
            S["n"], pawn.get_velocity().length(),
            mv.get_editor_property("movement_mode"),
            mv.get_editor_property("max_walk_speed"),
            pawn.get_editor_property("mesh").get_anim_instance().get_class().get_name()))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("MO COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("MO armed")
