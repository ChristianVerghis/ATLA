import unreal
S = {"h": None, "t0": None, "phase": "setup", "samples": []}

def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]
            S.clear()
            unreal.unregister_slate_post_tick_callback(h)
            unreal.log("spout_debug auto-disarmed")
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.AIR)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 2.0:
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        pc.set_control_rotation(unreal.Rotator(0.0, -80.0, 0.0))
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_AirCyclone)
        unreal.log("SD cast=%s" % ok)
        S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        cy = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAAirCyclone)
        p = pawn.get_actor_location()
        mode = pawn.get_editor_property("character_movement").get_editor_property("movement_mode")
        if cy:
            c = cy[0].get_actor_location()
            d2 = unreal.Vector(p.x - c.x, p.y - c.y, 0).length()
            S["samples"].append((round(age,1), round(d2), round(p.z - c.z), str(mode)))
        else:
            S["samples"].append((round(age,1), None, round(p.z), str(mode)))
        if age >= 5.0:
            for s in S["samples"][::4]:
                unreal.log("SD %s" % (s,))
            unreal.log("SD DONE")
            h = S["h"]
            S.clear(); S["t0"] = None; S["h"] = None
            unreal.unregister_slate_post_tick_callback(h)

S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("spout_debug armed")
