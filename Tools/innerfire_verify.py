import unreal
S = {"h": None, "t0": None, "phase": "setup"}
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
        # clear the arena: the AI opponent's hits would re-rattle the calm gate
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter):
            if a != pawn:
                c = a.get_controller()
                if c: c.destroy_actor()
                a.destroy_actor()
        pawn.set_element_loadout(unreal.ATLAElement.WATER); pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 2.0:
        # 1. drive starts 0, no tags
        d0 = pawn.get_inner_drive() if hasattr(pawn, 'get_inner_drive') else -1
        unreal.log("IF drive at rest=%.2f (expect 0)" % d0)
        # stoke it hard via the API (simulating landed hits)
        for _ in range(4): pawn.stoke_inner_drive(0.22)
        unreal.log("IF after 4 stokes: drive=%.2f (expect 0.88)" % pawn.get_inner_drive())
        S["phase"] = "calm"; S["t0"] = now
    elif S["phase"] == "calm" and age >= 1.0:
        # 2. lightning calm gate: fresh damage blocks it
        pawn.note_damage_taken()
        ok_rattled = asc.try_activate_ability_by_class(unreal.ATLAAbility_Lightning)
        unreal.log("IF lightning while rattled: %s (expect False)" % ok_rattled)
        S["phase"] = "calmwait"; S["t0"] = now
    elif S["phase"] == "calmwait" and age >= 4.5:
        ok_calm = asc.try_activate_ability_by_class(unreal.ATLAAbility_Lightning)
        unreal.log("IF lightning after 4.5s calm: %s (expect True)" % ok_calm)
        S["phase"] = "decay"; S["t0"] = now
    elif S["phase"] == "decay" and age >= 4.0:
        unreal.log("IF drive after 4s idle: %.2f (expect < 0.88, decaying)" % pawn.get_inner_drive())
        # 3. fire wall dies with maker: spawn wall then zero maker health
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireWall)
        S["phase"] = "wall"; S["t0"] = now
    elif S["phase"] == "wall" and age >= 2.0:
        walls = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireWall)
        mine = [w for w in walls if w.get_instigator() == pawn]
        unreal.log("IF wall standing while maker alive: %d (expect 1)" % len(mine))
        unreal.log("IF COMPLETE")
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("IF armed")
