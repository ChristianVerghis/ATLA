"""High-frequency stream sampling: chi, tongue count/kinematics, dummy hp per tick."""
import unreal

S = {"h": None, "t0": None, "cast": False, "n": 0}


def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    dummies = [c for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if c != pawn]
    d = min(dummies, key=lambda c: (c.get_actor_location() - pawn.get_actor_location()).length())

    if S["t0"] is None:
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        return

    age = now - S["t0"]
    if not S["cast"] and age >= 0.6:
        # Open ground west of the volcanic dummy (the arctic spot at
        # dummy-520 sits inside an ice pillar at head height)
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        dummies2 = [c for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if c != pawn]
        d = min(dummies2, key=lambda c: (c.get_actor_location() - pawn.get_actor_location()).length())
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-4.0, yaw=0.0))
        asc = pawn.get_editor_property("ability_system_component")
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_FireStream)
        unreal.log("P2 activate=%s chi=%.1f walk=%.0f" % (
            ok, pawn.get_chi(), pawn.get_movement_component().get_editor_property("max_walk_speed")))
        S["cast"] = True
        S["t0"] = now
        return

    if S["cast"]:
        S["n"] += 1
        if S["n"] % 6 == 0:
            tongues = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireStreamBolt)
            desc = " ".join("(%.0f,%.0f,%.0f|v%.0f)" % (
                t.get_actor_location().x - d.get_actor_location().x,
                t.get_actor_location().y - d.get_actor_location().y,
                t.get_actor_location().z - d.get_actor_location().z,
                t.get_velocity().length()) for t in tongues[:4])
            unreal.log("P2 t=%.2f chi=%.1f walk=%.0f tongues=%d hp=%.0f %s" % (
                now - S["t0"], pawn.get_chi(),
                pawn.get_movement_component().get_editor_property("max_walk_speed"),
                len(tongues), d.get_health(), desc))
        if now - S["t0"] >= 1.5:
            unreal.unregister_slate_post_tick_callback(S["h"])
            S["h"] = None
            unreal.log("P2 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("P2 armed")
