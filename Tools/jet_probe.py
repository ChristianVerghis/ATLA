import unreal
S = {"h": None, "t0": None, "phase": "setup", "samples": []}
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
    move = pawn.get_editor_property("character_movement")
    if S["phase"] == "setup" and age >= 1.5:
        S["groundZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet)
        unreal.log("JP activate jet: %s" % ok)
        # push forward input by injecting velocity is jet's own kick; just observe
        S["phase"] = "jet"; S["t0"] = now
    elif S["phase"] == "jet":
        if age < 2.0:
            loc = pawn.get_actor_location()
            v = move.get_editor_property("velocity")
            S["samples"].append((loc.z - S["groundZ"], abs(v.x)+abs(v.y), str(move.get_editor_property("movement_mode"))))
        else:
            zs = [s[0] for s in S["samples"]]; sp = [s[1] for s in S["samples"]]
            unreal.log("JP hover dz min=%.0f max=%.0f | horiz speed max=%.0f | mode=%s | n=%d" % (min(zs), max(zs), max(sp), S["samples"][-1][2], len(zs)))
            # count niagara on pawn mesh
            n = sum(1 for c in pawn.get_components_by_class(unreal.NiagaraComponent) if c.is_visible())
            unreal.log("JP visible plumes+flames on pawn = %d" % n)
            unreal.log("JP COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("JP armed")
