import unreal
S = {"h": None, "t0": None, "phase": "setup", "log": 0}
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
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        if age >= 0.8:
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
            S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        pawn.do_move(0.0, 1.0)
        S["log"] += 1
        if S["log"] % 4 == 0:
            floor = pawn.get_actor_location().z - half
            anim = mesh.get_anim_instance()
            mon = anim.get_current_active_montage()
            unreal.log("WSD t=%.2f meshZ=%.0f pelvisAboveFloor=%.0f footAboveFloor=%.0f actorZ=%.0f mode=%s mont=%s" % (
                age, mesh.get_editor_property("relative_location").z,
                mesh.get_socket_location("pelvis").z - floor,
                min(mesh.get_socket_location("foot_r").z, mesh.get_socket_location("foot_l").z) - floor,
                pawn.get_actor_location().z,
                pawn.get_movement_component().get_editor_property("movement_mode"),
                mon.get_name() if mon else "NONE"))
        if age >= 1.2:
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("WSD COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("WSD armed")
