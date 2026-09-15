import unreal
S = {"h": None, "t0": None, "phase": "setup", "worst": 999.0, "z0": None, "zdrop": 0.0}
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
        # flat volcanic plain
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        if age >= 0.7:
            S["z0"] = pawn.get_actor_location().z
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
            S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        pawn.do_move(0.0, 1.0)
        floor = pawn.get_actor_location().z - half
        S["zdrop"] = min(S["zdrop"], pawn.get_actor_location().z - S["z0"])
        for b in ("foot_r","foot_l","ball_r","ball_l"):
            try: S["worst"] = min(S["worst"], mesh.get_socket_location(b).z - floor)
            except Exception: pass
        if age >= 1.2:
            unreal.log("FLAT water moving-cast on FLAT ground: lowest foot=%+.1f cm, terrain drop=%.0f cm -> %s" % (
                S["worst"], S["zdrop"], "OK" if S["worst"] > -8 else "SINKING"))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("FLAT COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FLAT armed")
