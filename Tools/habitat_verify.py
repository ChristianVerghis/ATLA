import unreal
S = {"h": None, "t0": None, "phase": "watch", "koi": []}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
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
        if age < 3.0:
            koi = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAHabitatCreature)]
            if koi and len(S["koi"]) < 2:
                k = koi[0]
                comps = k.get_components_by_class(unreal.SceneComponent)
                S["koi"].append(len(comps))
        else:
            arenas = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAColiseum)
            creatures = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAHabitatCreature)
            styles = sorted(str(c.get_editor_property("style")) for c in creatures)
            unreal.log("HV coliseum=%d creatures=%d styles=%s" % (len(arenas), len(creatures), styles))
            if arenas:
                unreal.log("HV arena at %s" % arenas[0].get_actor_location())
            # travel test: teleport pawn to arena platform, check it lands and stays
            if arenas:
                pawn.set_actor_location(arenas[0].get_actor_location() + unreal.Vector(0,0,480), False, False)
            S["phase"] = "landed"; S["t0"] = now
    elif S["phase"] == "landed" and age >= 1.5:
        z = pawn.get_actor_location().z
        unreal.log("HV pawn z on platform after 1.5s = %.0f (deck ~480; falling through would be <100)" % z)
        unreal.log("HV COMPLETE")
        h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("HV armed")
