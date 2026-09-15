import unreal
S = {"h": None, "t0": None, "phase": "watch"}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn: return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now; return
    age = now - S["t0"]
    if S["phase"] == "watch" and age >= 3.0:
        zones = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAHabitatZone)
        counts = {}
        for z in zones:
            n = len(z.get_components_by_class(unreal.StaticMeshComponent))
            counts[int(z.get_editor_property("element"))] = n
        unreal.log("H2 zones=%d piece-counts water/earth/fire/air=%s" % (len(zones), [counts.get(i) for i in range(4)]))
        creatures = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAHabitatCreature)
        for c in creatures:
            st = str(c.get_editor_property("style"))
            if "DRAGONS" in st:
                unreal.log("H2 dragon parts=%d (was ~90, now should be 130+)" % len(c.get_components_by_class(unreal.StaticMeshComponent)))
        # duel HUD path: summon partner and confirm getter is wired
        pc = pawn.get_controller()
        pc.toggle_sparring_partner()
        S["phase"] = "hud"; S["t0"] = now
    elif S["phase"] == "hud" and age >= 1.5:
        pc = pawn.get_controller()
        foe = pc.get_sparring_partner()
        unreal.log("H2 sparring partner getter: %s" % (foe.get_name() if foe else None))
        unreal.SystemLibrary.execute_console_command(world, "DebugSettleDuel")
        S["phase"] = "done"; S["t0"] = now
    elif S["phase"] == "done" and age >= 1.5:
        unreal.log("H2 COMPLETE")
        h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("H2 armed")
