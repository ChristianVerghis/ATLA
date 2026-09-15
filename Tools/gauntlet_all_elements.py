import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "GauntletShots")
os.makedirs(SHOTS, exist_ok=True)

S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0, "v": {}}


def shot(name):
    unreal.AutomationLibrary.take_high_res_screenshot(1400, 800, os.path.join(SHOTS, name))


def nearest_dummy(world, pawn):
    best, bd = None, 1e12
    for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter):
        if c == pawn:
            continue
        d = (c.get_actor_location() - pawn.get_actor_location()).length()
        if d < bd:
            best, bd = c, d
    return best


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

    def aim(pitch, yaw):
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=float(pitch), yaw=float(yaw)))

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if ph == "boot" and age >= 0.4:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("fire_setup")
    elif ph == "fire_setup" and age >= 0.8:
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-520, 0, 0), False, True)
        aim(-4, 0)
        unreal.log("E3 fire jab: %s chi=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab), pawn.get_chi()))
        goto("fire_jab_shot")
    elif ph == "fire_jab_shot" and age >= 0.12:
        shot("fire_jab.png")
        unreal.log("E3 jab bolts=%d" % cnt(unreal.ATLAFireBolt))
        goto("fire_wall_cast")
    elif ph == "fire_wall_cast" and age >= 0.6:
        unreal.log("E3 fire wall: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireWall))
        goto("fire_wall_shot")
    elif ph == "fire_wall_shot" and age >= 0.9:
        shot("fire_wall.png")
        unreal.log("E3 walls=%d" % cnt(unreal.ATLAFireWall))
        goto("fire_nova_cast")
    elif ph == "fire_nova_cast" and age >= 0.5:
        d = nearest_dummy(world, pawn)
        S["v"]["dhp"] = d.get_health()
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-260, 0, 0), False, True)
        unreal.log("E3 nova: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova))
        goto("fire_nova_shot")
    elif ph == "fire_nova_shot" and age >= 0.55:
        shot("fire_nova.png")
        goto("fire_nova_check")
    elif ph == "fire_nova_check" and age >= 0.8:
        d = nearest_dummy(world, pawn)
        unreal.log("E3 nova dmg %.0f->%.0f (expect -30)" % (S["v"]["dhp"], d.get_health()))
        aim(10, 0)
        unreal.log("E3 jet: %s chi=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet), pawn.get_chi()))
        goto("fire_jet_check")
    elif ph == "fire_jet_check" and age >= 0.3:
        unreal.log("E3 jet v=%.0f mode=%s kickbursts=%d" % (
            pawn.get_velocity().length(),
            pawn.get_movement_component().get_editor_property("movement_mode"),
            cnt(unreal.ATLAFireBurst)))
        goto("earth_switch")
    elif ph == "earth_switch" and age >= 1.4:
        pawn.set_element_loadout(unreal.ATLAElement.EARTH)
        goto("earth_setup")
    elif ph == "earth_setup" and age >= 0.9:
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-520, 0, 0), False, True)
        aim(-4, 0)
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_RockJab)
        unreal.log("E3 rock jab: %s drawstreams=%d earth=%.0f" % (ok, cnt(unreal.ATLADrawStream), pawn.get_earth()))
        shot("earth_draw.png")
        goto("earth_hoist_cast")
    elif ph == "earth_hoist_cast" and age >= 0.8:
        unreal.log("E3 hoist: %s earth=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_BoulderHoist), pawn.get_earth()))
        goto("earth_hoist_shot")
    elif ph == "earth_hoist_shot" and age >= 1.1:
        unreal.log("E3 hoisted=%d" % cnt(unreal.ATLAHoistedBoulder))
        shot("earth_hoist.png")
        goto("earth_throw_wait")
    elif ph == "earth_throw_wait" and age >= 3.3:
        # auto-throw fires 4s after activation
        unreal.log("E3 thrown boulders=%d hoisted=%d" % (cnt(unreal.ATLABoulderProjectile), cnt(unreal.ATLAHoistedBoulder)))
        shot("earth_throw.png")
        goto("water_switch")
    elif ph == "water_switch" and age >= 0.6:
        pawn.set_element_loadout(unreal.ATLAElement.WATER)
        goto("water_setup")
    elif ph == "water_setup" and age >= 0.9:
        aim(-6, 90)
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        unreal.log("E3 whip: %s drawstreams=%d water=%.0f" % (ok, cnt(unreal.ATLADrawStream), pawn.get_water()))
        shot("water_draw.png")
        goto("water_pool_shot")
    elif ph == "water_pool_shot" and age >= 0.7:
        aim(-32, 90)
        goto("water_pool_shot2")
    elif ph == "water_pool_shot2" and age >= 0.3:
        shot("water_pool.png")
        goto("air_switch")
    elif ph == "air_switch" and age >= 0.5:
        pawn.set_element_loadout(unreal.ATLAElement.AIR)
        goto("air_setup")
    elif ph == "air_setup" and age >= 0.9:
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-520, 0, 0), False, True)
        aim(-6, 0)
        unreal.log("E3 air blast: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_AirBlast))
        goto("air_blast_shot")
    elif ph == "air_blast_shot" and age >= 0.12:
        shot("air_blast.png")
        goto("air_scooter_cast")
    elif ph == "air_scooter_cast" and age >= 0.7:
        unreal.log("E3 scooter: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_AirScooter))
        goto("air_scooter_shot")
    elif ph == "air_scooter_shot" and age >= 0.6:
        meshz = pawn.get_editor_property("mesh").get_editor_property("relative_location").z
        unreal.log("E3 scooter balls=%d rider mesh z=%.0f (expect ~-6, normal -90)" % (cnt(unreal.ATLAAirScooterBall), meshz))
        shot("air_scooter.png")
        goto("air_dome_cast")
    elif ph == "air_dome_cast" and age >= 0.6:
        unreal.log("E3 dome: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_WindDome))
        goto("air_dome_shot")
    elif ph == "air_dome_shot" and age >= 0.7:
        shot("air_dome.png")
        unreal.log("E3 domes=%d" % cnt(unreal.ATLAWindDome))
        goto("done")
    elif ph == "done" and age >= 0.8:
        meshz = pawn.get_editor_property("mesh").get_editor_property("relative_location").z
        unreal.log("E3 mesh restored z=%.0f (expect -90 after scooter end... scooter lasts 4s, may still ride)" % meshz)
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("E3 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("E3 armed")
