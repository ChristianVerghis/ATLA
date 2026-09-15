import unreal
S = {"h": None, "t0": None, "phase": "setup"}
def others(world, pawn):
    return [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if a != pawn]
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
    pc = pawn.get_controller()
    if S["t0"] is None:
        S["t0"] = now
        for a in others(world, pawn):
            c = a.get_controller()
            if c: c.destroy_actor()
            a.destroy_actor()
        return
    age = now - S["t0"]
    if S["phase"] == "setup" and age >= 2.0:
        S["before"] = len(others(world, pawn))
        pc.toggle_sparring_partner()
        S["phase"] = "summoned"; S["t0"] = now
    elif S["phase"] == "summoned" and age >= 1.5:
        n = len(others(world, pawn))
        unreal.log("DW partner after toggle: %d (was %d, expect +1)" % (n, S["before"]))
        unreal.SystemLibrary.execute_console_command(world, "DebugSettleDuel")
        S["phase"] = "settled"; S["t0"] = now
    elif S["phase"] == "settled" and age >= 1.5:
        n = len(others(world, pawn))
        unreal.log("DW after settle: partners=%d (expect 0) my-health=%.0f (expect max)" % (n, pawn.get_health()))
        unreal.log("DW COMPLETE")
        h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("DW armed")
