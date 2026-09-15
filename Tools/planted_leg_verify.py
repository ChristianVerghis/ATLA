import unreal
S = {"h": None, "t0": None, "phase": "setup", "feet": [], "pelvis": []}
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
    if S["phase"] == "setup" and age >= 2.5:
        S["baseZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthSpikes)
        unreal.log("PV spikes cast: %s" % ok)
        S["phase"] = "spikes"; S["t0"] = now; S["feet"] = []
    elif S["phase"] == "spikes":
        if age < 1.6:
            S["feet"].append(mesh.get_socket_location("foot_r").z - S["baseZ"])
        else:
            rng = max(S["feet"]) - min(S["feet"])
            unreal.log("PV spikes STOMP foot_r range=%.0f (need >40) -> %s" % (rng, "PASS" if rng > 40 else "FAIL"))
            S["phase"] = "armorwait"; S["t0"] = now
    elif S["phase"] == "armorwait" and age >= 2.5:
        S["baseZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthArmor)
        unreal.log("PV armor cast: %s" % ok)
        S["phase"] = "armor"; S["t0"] = now; S["pelvis"] = []
    elif S["phase"] == "armor":
        if age < 2.0:
            S["pelvis"].append(mesh.get_socket_location("pelvis").z - S["baseZ"])
        else:
            lo = min(S["pelvis"])
            unreal.log("PV armor ALL-FOURS pelvis min rel=%.0f (idle ~+95-baseZ~=-8; all-fours < -40) -> %s" % (lo, "PASS" if lo < -40 else "FAIL"))
            unreal.log("PV COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("PV armed")
