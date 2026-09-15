"""Round-3 verify: Q lash flies+damages, jab full-body anim frames,
boulder look, Azula booster speed/plumes."""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)
S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0, "v": {}}


def shot(name):
    unreal.AutomationLibrary.take_high_res_screenshot(1400, 800, os.path.join(SHOTS, name))


def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    asc = pawn.get_editor_property("ability_system_component")
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        S["pt"] = now
        return
    age = now - S["pt"]
    ph = S["phase"]

    def cnt(cls):
        return len(unreal.GameplayStatics.get_all_actors_of_class(world, cls))

    def dummy():
        ds = [c for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if c != pawn]
        return min(ds, key=lambda c: (c.get_actor_location() - pawn.get_actor_location()).length())

    def aim(pitch, yaw):
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=float(pitch), yaw=float(yaw)))

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if ph == "boot" and age >= 0.5:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("setup")
    elif ph == "setup" and age >= 0.8:
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        aim(-4, 0)
        S["v"]["dhp"] = dummy().get_health()
        unreal.log("P3 lash: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireLash))
        goto("lash_flight")
    elif ph == "lash_flight" and age >= 0.35:
        unreal.log("P3 lash bolts alive=%d" % cnt(unreal.ATLAFireLashBolt))
        shot("p3_lash.png")
        goto("lash_check")
    elif ph == "lash_check" and age >= 0.5:
        unreal.log("P3 lash dummy hp %.0f->%.0f (expect -22)" % (S["v"]["dhp"], dummy().get_health()))
        goto("jab_cast")
    elif ph == "jab_cast" and age >= 0.6:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab)
        goto("jab_frame1")
    elif ph == "jab_frame1" and age >= 0.12:
        shot("p3_jab_early.png")
        goto("jab_frame2")
    elif ph == "jab_frame2" and age >= 0.15:
        shot("p3_jab_late.png")
        goto("earth_switch")
    elif ph == "earth_switch" and age >= 0.8:
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        goto("earth_setup")
    elif ph == "earth_setup" and age >= 1.0:
        aim(-4, 0)
        asc.try_activate_ability_by_class(unreal.ATLAAbility_RockJab)
        goto("rock_shot")
    elif ph == "rock_shot" and age >= 0.35:
        shot("p3_rock.png")
        unreal.log("P3 hoist: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_BoulderHoist))
        goto("hoist_shot")
    elif ph == "hoist_shot" and age >= 1.2:
        shot("p3_boulder.png")
        goto("fire_back")
    elif ph == "fire_back" and age >= 0.6:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("boost_start")
    elif ph == "boost_start" and age >= 1.0:
        aim(-2, 0)
        S["v"]["p0"] = pawn.get_actor_location()
        unreal.log("P3 jet: %s walk=%.0f" % (
            asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet),
            pawn.get_movement_component().get_editor_property("max_walk_speed")))
        goto("boost_run")
    elif ph == "boost_run":
        pawn.do_move(0.0, 1.0)
        if age >= 1.2:
            shot("p3_boost.png")
            v = pawn.get_velocity()
            d = (pawn.get_actor_location() - S["v"]["p0"]).length()
            unreal.log("P3 boost 1.2s: speed=%.0f dist=%.0f walk=%.0f mode=%s" % (
                v.length(), d,
                pawn.get_movement_component().get_editor_property("max_walk_speed"),
                pawn.get_movement_component().get_editor_property("movement_mode")))
            goto("done")
    elif ph == "done" and age >= 0.3:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("P3 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("P3 armed")
