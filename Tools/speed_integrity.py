"""Speed must return to baseline after casts overlap the jet's 2.6x boost."""
import unreal
S = {"h": None, "t0": None, "phase": "setup", "base": None}
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
    mv = pawn.get_movement_component()
    def spd(): return mv.get_editor_property("max_walk_speed")
    if S["t0"] is None:
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 1.5:
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        S["base"] = spd()
        unreal.log("SPD baseline=%.0f" % S["base"])
        S["phase"] = "jet"; S["t0"] = now
    elif S["phase"] == "jet" and age >= 0.4:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet)
        unreal.log("SPD during jet=%.0f (expect ~1612)" % spd())
        S["phase"] = "cast"; S["t0"] = now
    elif S["phase"] == "cast" and age >= 0.3:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab)
        unreal.log("SPD jet+cast=%.0f" % spd())
        S["phase"] = "cancel"; S["t0"] = now
    elif S["phase"] == "cancel" and age >= 0.5:
        asc.cancel_all_abilities()
        S["phase"] = "settle"; S["t0"] = now
    elif S["phase"] == "settle" and age >= 2.0:
        final = spd()
        unreal.log("SPD final=%.0f baseline=%.0f -> %s" % (
            final, S["base"], "OK" if abs(final - S["base"]) < 1.0 else "CORRUPTED"))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("SPD COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("SPD armed")
