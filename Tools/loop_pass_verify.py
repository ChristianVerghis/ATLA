import unreal
# Verifies the loop pass: rideable air spout, metalbending unlock + metal jab,
# duel knockdown finish. Auto-disarms when PIE ends.
S = {"h": None, "t0": None, "phase": "setup"}

def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]
            S.clear()
            unreal.unregister_slate_post_tick_callback(h)
            unreal.log("loop probe auto-disarmed (PIE ended)")
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        # Clear the field so nothing interferes
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter):
            if a != pawn:
                c = a.get_controller()
                if c: c.destroy_actor()
                a.destroy_actor()
        pawn.set_element_loadout(unreal.ATLAElement.AIR)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")

    if S["phase"] == "setup" and age >= 2.0:
        S["baseZ"] = pawn.get_actor_location().z
        # Aim down at own feet: control rotation pitch steeply down
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        pc.set_control_rotation(unreal.Rotator(0.0, -80.0, 0.0))
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_AirCyclone)
        unreal.log("LP spout cast: %s" % ok)
        S["phase"] = "spout"; S["t0"] = now; S["maxRise"] = 0.0
    elif S["phase"] == "spout":
        S["maxRise"] = max(S["maxRise"], pawn.get_actor_location().z - S["baseZ"])
        if age >= 4.5:
            rise = S["maxRise"]
            unreal.log("LP RESULT spout_rise=%.0f %s" % (rise, "PASS" if rise > 400 else "FAIL"))
            # Metal mastery next
            pawn.set_element_loadout(unreal.ATLAElement.EARTH)
            S["phase"] = "metal"; S["t0"] = now
    elif S["phase"] == "metal" and age >= 1.5:
        for i in range(10):
            pawn.stoke_earth_mastery(0.12)
        unlocked = pawn.has_metalbending()
        unreal.log("LP RESULT metal_unlock=%s %s" % (unlocked, "PASS" if unlocked else "FAIL"))
        # Duel knockdown: summon partner, settle, watch the pause
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        pc.set_control_rotation(unreal.Rotator(0.0, 0.0, 0.0))
        pc.toggle_sparring_partner()
        S["phase"] = "duel_arm"; S["t0"] = now
    elif S["phase"] == "duel_arm" and age >= 1.0:
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        unreal.SystemLibrary.execute_console_command(world, "DebugSettleDuel", pc)
        S["phase"] = "duel_pause"; S["t0"] = now
    elif S["phase"] == "duel_pause" and age >= 1.2:
        # Mid-pause: partner should still exist (knocked down, not vanished)
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        partner = pc.get_sparring_partner()
        alive = partner is not None
        ragdoll = False
        if alive:
            m = partner.get_editor_property("mesh")
            ragdoll = m.is_simulating_physics()
        unreal.log("LP RESULT knockdown_pause alive=%s ragdoll=%s %s" % (alive, ragdoll, "PASS" if (alive and ragdoll) else "FAIL"))
        S["phase"] = "duel_clean"; S["t0"] = now
    elif S["phase"] == "duel_clean" and age >= 2.5:
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        partner = pc.get_sparring_partner()
        gone = partner is None
        unreal.log("LP RESULT cleanup partner_gone=%s %s" % (gone, "PASS" if gone else "FAIL"))
        unreal.log("LP DONE")
        h = S["h"]
        S.clear()
        S["t0"] = None; S["h"] = None
        unreal.unregister_slate_post_tick_callback(h)

S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("loop_pass_verify armed")
