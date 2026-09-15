"""Probe stream tongue trajectories vs the dummy: where do they actually fly?"""
import unreal

S = {"h": None, "t0": None, "phase": "setup", "pt": 0.0}


def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        S["pt"] = now
        return
    age = now - S["pt"]
    ph = S["phase"]

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    dummies = [c for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if c != pawn]
    d = min(dummies, key=lambda c: (c.get_actor_location() - pawn.get_actor_location()).length())

    if ph == "setup" and age >= 0.3:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-520, 0, 0), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-4.0, yaw=0.0))
        goto("cast")
    elif ph == "cast" and age >= 0.5:
        asc = pawn.get_editor_property("ability_system_component")
        unreal.log("SP stream: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireStream))
        pl = pawn.get_actor_location()
        dl = d.get_actor_location()
        unreal.log("SP pawn=(%.0f,%.0f,%.0f) dummy=(%.0f,%.0f,%.0f)" % (pl.x, pl.y, pl.z, dl.x, dl.y, dl.z))
        goto("sample")
    elif ph == "sample" and age >= 0.45:
        dl = d.get_actor_location()
        for t in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireStreamBolt):
            loc = t.get_actor_location()
            vel = t.get_velocity()
            unreal.log("SP tongue loc=(%.0f,%.0f,%.0f) rel-dummy=(%.0f,%.0f,%.0f) speed=%.0f" % (
                loc.x, loc.y, loc.z, loc.x - dl.x, loc.y - dl.y, loc.z - dl.z, vel.length()))
        goto("sample2")
    elif ph == "sample2" and age >= 0.4:
        for t in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireStreamBolt):
            loc = t.get_actor_location()
            vel = t.get_velocity()
            unreal.log("SP2 tongue loc=(%.0f,%.0f,%.0f) speed=%.0f" % (loc.x, loc.y, loc.z, vel.length()))
        unreal.log("SP dummy hp=%.0f" % d.get_health())
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("SP COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("SP armed")
