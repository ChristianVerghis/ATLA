import unreal
S = {"h": None, "t0": None, "phase": "setup", "feet": [], "hands": []}
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
    mesh = pawn.get_editor_property("mesh")
    if S["t0"] is None:
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 1.5:
        S["baseZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthSpikes)
        unreal.log("LP spikes cast: %s" % ok)
        S["phase"] = "spikes"; S["t0"] = now; S["feet"] = []; S["hands"] = []
    elif S["phase"] == "spikes":
        if age < 1.6:
            fz = mesh.get_socket_location("foot_r").z - S["baseZ"]
            hz = mesh.get_socket_location("hand_r").z - S["baseZ"]
            S["feet"].append(fz); S["hands"].append(hz)
        else:
            unreal.log("LP spikes foot_r rel-z min=%.0f max=%.0f range=%.0f | hand range=%.0f (n=%d)"
                % (min(S["feet"]), max(S["feet"]), max(S["feet"])-min(S["feet"]),
                   max(S["hands"])-min(S["hands"]), len(S["feet"])))
            pawn.set_element_loadout(unreal.ATLAElement.WATER)
            S["phase"] = "waterprep"; S["t0"] = now
    elif S["phase"] == "waterprep" and age >= 1.0:
        S["phase"] = "waterrun"; S["t0"] = now; S["feet"] = []; S["hands"] = []
        unreal.log("LP water running-cast starting")
    elif S["phase"] == "waterrun":
        # keep pushing forward like a held W key
        pawn.add_movement_input(pawn.get_actor_forward_vector(), 1.0, False)
        if 0.4 < age and S.get("cast") is None:
            S["cast"] = asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
            unreal.log("LP whip cast while moving: %s vel=%.0f" % (S["cast"], pawn.get_velocity().length()))
        if age < 1.4:
            fz = mesh.get_socket_location("foot_r").z - pawn.get_actor_location().z
            S["feet"].append(fz)
        else:
            rng = max(S["feet"]) - min(S["feet"])
            unreal.log("LP water-moving foot_r range=%.0f (run cycle should be >8; frozen <3) n=%d" % (rng, len(S["feet"])))
            unreal.log("LP COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("LP armed")
