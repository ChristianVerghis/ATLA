import unreal
S = {"h": None, "t0": None, "phase": "watch", "pos": [], "seen": {}, "qdone": False}
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
    if S["phase"] == "watch":
        # find the AI bender: a pawn that isn't the player
        ai = None
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter):
            if a != pawn: ai = a
        if ai:
            ctrl = ai.get_controller()
            S["seen"]["ai"] = True
            S["seen"]["brain"] = ctrl.get_class().get_name() if ctrl else "none"
            S["seen"]["element"] = str(ai.get_element_loadout()) if hasattr(ai, 'get_element_loadout') else "?"
            S["pos"].append(ai.get_actor_location())
        if age >= 12.0:
            if not S["seen"].get("ai"):
                unreal.log("AV no AI bender found"); unreal.log("AV COMPLETE")
                unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None; return
            moved = 0.0
            for i in range(1, len(S["pos"])):
                moved += (S["pos"][i] - S["pos"][i-1]).length()
            unreal.log("AV brain=%s element=%s moved=%.0fcm over 12s (should be >400)" % (
                S["seen"]["brain"], S["seen"]["element"], moved))
            S["phase"] = "qtest"; S["t0"] = now
    elif S["phase"] == "qtest" and age >= 1.0 and not S["qdone"]:
        S["qdone"] = True
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        asc = pawn.get_editor_property("ability_system_component")
        S["phase"] = "qcast"; S["t0"] = now; S["asc"] = asc
    elif S["phase"] == "qcast" and age >= 2.0:
        ok = S["asc"].try_activate_ability_by_class(unreal.ATLAAbility_EarthSpikes)
        unreal.log("AV Q cast: %s" % ok)
        S["phase"] = "qwatch"; S["t0"] = now; S["legs"] = []
    elif S["phase"] == "qwatch":
        mesh = pawn.get_editor_property("mesh")
        if age < 3.0:
            az = pawn.get_actor_location().z
            fl = mesh.get_socket_location("foot_l").z - az
            fr = mesh.get_socket_location("foot_r").z - az
            S["legs"].append((age, round(min(fl,fr)), round(max(fl,fr))))
        else:
            # after-cast legs should settle to about -84 (standing) with no wild spikes
            tail = S["legs"][-5:]
            unreal.log("AV Q tail legs: %s" % tail)
            unreal.log("AV COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("AV armed")
