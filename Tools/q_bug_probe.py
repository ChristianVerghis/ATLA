import unreal
S = {"h": None, "t0": None, "phase": "setup", "rows": []}
def legs(mesh, pawn):
    az = pawn.get_actor_location().z
    fl = mesh.get_socket_location("foot_l"); fr = mesh.get_socket_location("foot_r")
    pv = mesh.get_socket_location("pelvis"); th = mesh.get_socket_location("thigh_r")
    leglen = (unreal.Vector(fr.x-th.x, fr.y-th.y, fr.z-th.z)).length()
    return (round(min(fl.z,fr.z)-az), round(pv.z-az), round(leglen))
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
    if S["phase"] == "setup" and age >= 2.0:
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_EarthSpikes)
        unreal.log("QB cast: %s" % ok)
        S["phase"] = "watch"; S["t0"] = now
    elif S["phase"] == "watch":
        if age < 3.5:
            f, p, L = legs(mesh, pawn)
            S["rows"].append("%.2f:f%d,p%d,L%d" % (age, f, p, L))
        else:
            unreal.log("QB trace " + " ".join(S["rows"]))
            unreal.log("QB COMPLETE")
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("QB armed")
