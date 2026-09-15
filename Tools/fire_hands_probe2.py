import unreal
S = {"h": None, "t0": None, "phase": "setup"}
def flames(pawn):
    n = a = 0
    for c in pawn.get_components_by_class(unreal.NiagaraComponent):
        n += 1
        if c.is_visible(): a += 1
    return n, a
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
        n, a = flames(pawn)
        unreal.log("FH2 idle fire: niagara on pawn total=%d ACTIVE=%d -> %s" % (n, a, "hands dark (correct)" if a == 0 else "hands lit (wrong)"))
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova)
        S["phase"] = "ult"; S["t0"] = now
    elif S["phase"] == "ult" and age >= 2.0:
        n, a = flames(pawn)
        unreal.log("FH2 ultimate: niagara on pawn total=%d ACTIVE=%d -> %s" % (n, a, "hands lit (correct)" if a > 0 else "hands dark (wrong)"))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("FH2 COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FH2 armed")
