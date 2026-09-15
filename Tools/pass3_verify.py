import unreal
S = {"h": None, "t0": None, "phase": "setup", "feet": [], "pelvis": [], "slabs": []}
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
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter):
            if a != pawn:
                c = a.get_controller()
                if c: c.destroy_actor()
                a.destroy_actor()
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        return
    age = now - S["t0"]
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 2.0:
        S["baseZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthSpikes)
        unreal.log("P3 Q cast: %s" % ok)
        S["phase"] = "q"; S["t0"] = now; S["feet"] = []; S["wide"] = 0.0
    elif S["phase"] == "q":
        if age < 1.4:
            fl = mesh.get_socket_location("foot_l"); fr = mesh.get_socket_location("foot_r")
            S["feet"].append(fr.z - S["baseZ"])
            d = unreal.Vector(fl.x-fr.x, fl.y-fr.y, 0).length()
            S["wide"] = max(S["wide"], d)
        else:
            rng = max(S["feet"]) - min(S["feet"])
            unreal.log("P3 Q stomp foot range=%.0f (>30 wanted) max-foot-spread=%.0f (chapa flared ~90+; bencao should be <75)" % (rng, S["wide"]))
            S["phase"] = "rwait"; S["t0"] = now
    elif S["phase"] == "rwait" and age >= 2.5:
        S["baseZ"] = pawn.get_actor_location().z
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthArmor)
        unreal.log("P3 R cast: %s" % ok)
        S["phase"] = "r"; S["t0"] = now; S["pelvis"] = []; S["slabs"] = []
    elif S["phase"] == "r":
        if age < 3.2:
            S["pelvis"].append((round(age,2), round(mesh.get_socket_location("pelvis").z - S["baseZ"])))
            n = len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAEarthCocoon))
            S["slabs"].append((round(age,2), n))
        else:
            lo = min(p[1] for p in S["pelvis"])
            lo_t = [p for p in S["pelvis"] if p[1] == lo][0][0]
            end = S["pelvis"][-1][1]
            coco = [s for s in S["slabs"] if s[1] > 0]
            unreal.log("P3 R pelvis: low=%d at %.1fs, end=%d (drop then rise). cocoon alive %.1f-%.1fs" % (
                lo, lo_t, end, coco[0][0] if coco else -1, coco[-1][0] if coco else -1))
            unreal.log("P3 COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("P3 armed")
