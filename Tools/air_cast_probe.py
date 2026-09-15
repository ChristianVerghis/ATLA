import unreal
S = {"h": None, "t0": None, "phase": "setup", "worst": 0.0, "maxlen": 0.0, "shots": 0}
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
    if S["phase"] == "setup" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        S["phase"] = "jump"; S["t0"] = now
    elif S["phase"] == "jump" and age >= 0.3:
        pawn.jump()
        S["phase"] = "cast"; S["t0"] = now
    elif S["phase"] == "cast" and age >= 0.25:
        asc = pawn.get_editor_property("ability_system_component")
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        unreal.log("ACP airborne cast=%s mode=%s" % (ok, pawn.get_movement_component().get_editor_property("movement_mode")))
        S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        pel = mesh.get_socket_location("pelvis")
        for b in ("foot_r","foot_l","calf_r","calf_l","thigh_r","thigh_l"):
            try:
                d = (mesh.get_socket_location(b) - pel).length()
                S["maxlen"] = max(S["maxlen"], d)
            except Exception: pass
        if age >= 1.0:
            unreal.log("ACP max leg-bone distance from pelvis = %.0f cm (normal <110)" % S["maxlen"])
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("ACP COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("ACP armed")
