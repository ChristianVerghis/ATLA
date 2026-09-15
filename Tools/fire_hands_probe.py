import unreal
S = {"h": None, "t0": None, "phase": "setup"}
def count(world):
    n_total = n_active = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAElementAura):
        for c in a.get_components_by_class(unreal.NiagaraComponent):
            n_total += 1
            if c.is_active(): n_active += 1
    return n_total, n_active
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
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 2.0:
        t, a = count(world)
        unreal.log("FH idle: flames total=%d active=%d" % (t, a))
        unreal.log("FH casting nova: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova))
        S["phase"] = "ult"; S["t0"] = now
    elif S["phase"] == "ult" and age >= 2.5:
        t, a = count(world)
        tag = unreal.GameplayTagLibrary.request_gameplay_tag("State.Bending.Empowered") if hasattr(unreal, 'GameplayTagLibrary') else None
        unreal.log("FH during ultimate: flames total=%d active=%d" % (t, a))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("FH COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FH armed")
