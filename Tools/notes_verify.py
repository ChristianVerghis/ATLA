"""Earth LMB = uppercut (rock rises) then punch (rock launches).
Fire hands = lit only while Empowered."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "rise": [], "launched": None}
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
    asc = pawn.get_editor_property("ability_system_component")
    def rocks():
        return unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLARockProjectile)
    if S["phase"] == "setup" and age >= 1.0:
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        S["phase"] = "place"; S["t0"] = now
    elif S["phase"] == "place" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(unreal.Rotator(roll=0.0, pitch=-5.0, yaw=0.0))
        S["phase"] = "cast"; S["t0"] = now
    elif S["phase"] == "cast" and age >= 0.5:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_RockJab)
        S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        for r in rocks():
            v = r.get_velocity()
            S["rise"].append((age, v.z, unreal.Vector(v.x, v.y, 0).length()))
        if age >= 1.4:
            rising = [r for r in S["rise"] if r[1] > 20 and r[2] < 50]
            flying = [r for r in S["rise"] if r[2] > 500]
            unreal.log("NV earth: rising-samples=%d (rock lifted) launched-samples=%d (rock thrown) -> %s" % (
                len(rising), len(flying), "TWO-BEAT OK" if rising and flying else "INCOMPLETE"))
            if rising: unreal.log("NV   lift began t=%.2f  launch began t=%.2f" % (rising[0][0], flying[0][0] if flying else -1))
            S["phase"] = "fire"; S["t0"] = now
    elif S["phase"] == "fire" and age >= 0.5:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        S["phase"] = "fireidle"; S["t0"] = now
    elif S["phase"] == "fireidle" and age >= 1.5:
        auras = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAElementAura)
        n = 0
        for a in auras:
            for c in a.get_components_by_class(unreal.NiagaraComponent):
                if c.is_active(): n += 1
        unreal.log("NV fire hands (no ultimate): active flame FX = %d -> %s" % (n, "OFF (correct)" if n == 0 else "STILL LIT"))
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova)
        S["phase"] = "fireult"; S["t0"] = now
    elif S["phase"] == "fireult" and age >= 2.0:
        auras = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAElementAura)
        n = 0
        for a in auras:
            for c in a.get_components_by_class(unreal.NiagaraComponent):
                if c.is_active(): n += 1
        unreal.log("NV fire hands (ultimate active): active flame FX = %d -> %s" % (n, "LIT (correct)" if n > 0 else "NOT LIT"))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("NV COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("NV armed")
