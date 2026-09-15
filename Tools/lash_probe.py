"""Track the lash bolt per tick: spawn, position, velocity, death."""
import unreal

S = {"h": None, "t0": None, "cast": False}


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
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-7.5, yaw=0.0))
        return
    if not S["cast"] and now - S["t0"] >= 0.4:
        asc = pawn.get_editor_property("ability_system_component")
        unreal.log("LP cast=%s pawn=%s" % (
            asc.try_activate_ability_by_class(unreal.ATLAAbility_FireLash),
            pawn.get_actor_location()))
        S["cast"] = True
        S["t0"] = now
        return
    if S["cast"]:
        t = now - S["t0"]
        pl = pawn.get_actor_location()
        bolts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireLashBolt)
        for b in bolts:
            loc = b.get_actor_location()
            unreal.log("LP t=%.2f lash rel (%.0f,%.0f,%.0f) v=%.0f" % (t, loc.x - pl.x, loc.y - pl.y, loc.z - pl.z, b.get_velocity().length()))
        for b in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireBurst):
            loc = b.get_actor_location()
            unreal.log("LP t=%.2f BURST rel (%.0f,%.0f,%.0f)" % (t, loc.x - pl.x, loc.y - pl.y, loc.z - pl.z))
        if t >= 0.8:
            unreal.unregister_slate_post_tick_callback(S["h"])
            S["h"] = None
            unreal.log("LP COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("LP armed")
