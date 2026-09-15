import unreal
S = {"h": None, "t0": None, "phase": "setup"}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
            unreal.log("probe auto-disarmed (PIE ended)")
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn: return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now; return
    age = now - S["t0"]
    pc = pawn.get_controller()
    if S["phase"] == "setup" and age >= 2.0:
        # airborne LMB: jump then cast earth jab mid-air
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        S["phase"] = "jump"; S["t0"] = now
    elif S["phase"] == "jump" and age >= 1.0:
        pawn.jump()
        S["phase"] = "aircast"; S["t0"] = now
    elif S["phase"] == "aircast" and age >= 0.3:
        move = pawn.get_editor_property("character_movement")
        airborne = not move.is_moving_on_ground()
        asc = pawn.get_editor_property("ability_system_component")
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_RockJab)
        unreal.log("DV airborne=%s earth-jab-in-air=%s (expect True/True)" % (airborne, ok))
        S["phase"] = "duelstart"; S["t0"] = now
    elif S["phase"] == "duelstart" and age >= 1.5:
        pc.toggle_sparring_partner() if hasattr(pc, 'toggle_sparring_partner') else None
        others = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if a != pawn]
        unreal.log("DV partner summoned via API: %d found" % len(others))
        S["phase"] = "kill"; S["t0"] = now
    elif S["phase"] == "kill" and age >= 2.0:
        others = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if a != pawn]
        if not others:
            unreal.log("DV no partner to kill (toggle unavailable from python?)"); unreal.log("DV COMPLETE")
            h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h); return
        foe = others[0]
        asc = foe.get_editor_property("ability_system_component")
        asc.apply_mod_to_attribute(unreal.ATLAAttributeSet.get_health_attribute() if hasattr(unreal.ATLAAttributeSet, 'get_health_attribute') else None, unreal.GameplayModOp.OVERRIDE, 0.0)
        unreal.log("DV foe health zeroed")
        S["phase"] = "settled"; S["t0"] = now
    elif S["phase"] == "settled" and age >= 1.5:
        others = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if a != pawn]
        unreal.log("DV after settle: partners remaining=%d (expect 0 - victory cleanup) my health=%.0f" % (len(others), pawn.get_health()))
        unreal.log("DV COMPLETE")
        h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("DV armed")
