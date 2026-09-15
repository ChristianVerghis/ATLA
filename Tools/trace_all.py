"""Same single-cast leg trace across all elements — is the collapse specific
to the Mixamo-driven ones, or does earth (reported fine) do it too?"""
import unreal
TESTS = [("WATER", unreal.ATLAElement.WATER, "ATLAAbility_WaterWhip"),
         ("EARTH", unreal.ATLAElement.EARTH, "ATLAAbility_RockJab"),
         ("FIRE",  unreal.ATLAElement.FIRE,  "ATLAAbility_FireJab"),
         ("AIR",   unreal.ATLAElement.AIR,   "ATLAAbility_AirBlast")]
S = {"h": None, "t0": None, "i": 0, "phase": "switch", "rows": []}
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
    if S["i"] >= len(TESTS):
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("TA COMPLETE"); return
    name, elem, cls = TESTS[S["i"]]
    mesh = pawn.get_editor_property("mesh")
    if S["phase"] == "switch" and age >= 0.5:
        pawn.set_element_loadout(elem); S["phase"] = "place"; S["t0"] = now
    elif S["phase"] == "place" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        S["phase"] = "idle"; S["t0"] = now
    elif S["phase"] == "idle" and age >= 0.6:
        # baseline while standing, no cast
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        S["base"] = min((fr-pel).length(), (fl-pel).length())
        asc = pawn.get_editor_property("ability_system_component")
        asc.try_activate_ability_by_class(getattr(unreal, cls))
        S["rows"] = []
        S["phase"] = "trace"; S["t0"] = now
    elif S["phase"] == "trace":
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        S["rows"].append(min((fr-pel).length(), (fl-pel).length()))
        if age >= 0.9:
            unreal.log("TA %-5s idle_legmin=%.0f during_cast min=%.0f max=%.0f -> %s" % (
                name, S["base"], min(S["rows"]), max(S["rows"]),
                "COLLAPSES" if min(S["rows"]) < 45 else "ok"))
            S["i"] += 1; S["phase"] = "switch"; S["t0"] = now
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("TA armed")
