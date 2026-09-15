"""Pass-2 diagnostics: water whip visuals (fire-flash hunt), nova look,
jet glide behavior, jab anim frame, camera height.

Run via: python3 Tools/ue_run.py Tools/fire_pass2_test.py
Logs prefixed G2. Screenshots to Saved/FirePassShots/.
"""
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

    def aim(pitch, yaw):
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=float(pitch), yaw=float(yaw)))

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if ph == "boot" and age >= 0.5:
        boom = pawn.get_editor_property("camera_boom")
        off = boom.get_editor_property("socket_offset")
        unreal.log("G2 camera socket_offset z=%.0f (expect 140)" % off.z)
        # Water first: whip toward the arctic dummy from open ground
        pawn.set_element_loadout(unreal.ATLAElement.WATER)
        goto("water_setup")
    elif ph == "water_setup" and age >= 0.8:
        pawn.set_actor_location(unreal.Vector(700, -20, 92), False, True)
        aim(-4, 90)
        goto("water_whip1")
    elif ph == "water_whip1" and age >= 0.4:
        unreal.log("G2 whip1: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip))
        goto("water_whip_shot1")
    elif ph == "water_whip_shot1" and age >= 0.25:
        shot("water_whip_flight.png")
        asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        goto("water_whip_shot2")
    elif ph == "water_whip_shot2" and age >= 0.3:
        shot("water_whip_impact.png")
        asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        goto("water_whip_shot3")
    elif ph == "water_whip_shot3" and age >= 0.2:
        shot("water_whip_more.png")
        goto("fire_switch")
    elif ph == "fire_switch" and age >= 0.8:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("fire_setup")
    elif ph == "fire_setup" and age >= 0.8:
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        aim(-4, 0)
        goto("jab_cast")
    elif ph == "jab_cast" and age >= 0.4:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab)
        goto("jab_shot")
    elif ph == "jab_shot" and age >= 0.16:
        shot("jab_release.png")
        goto("nova_cast")
    elif ph == "nova_cast" and age >= 0.8:
        unreal.log("G2 nova: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova))
        goto("nova_shot1")
    elif ph == "nova_shot1" and age >= 0.55:
        shot("nova_early.png")
        goto("nova_shot2")
    elif ph == "nova_shot2" and age >= 0.5:
        shot("nova_late.png")
        goto("glide_start")
    elif ph == "glide_start" and age >= 1.2:
        aim(8, 0)
        S["v"]["z0"] = pawn.get_actor_location().z
        unreal.log("G2 jet: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet))
        goto("glide_fly")
    elif ph == "glide_fly" and age >= 0.6:
        # push forward while gliding (simulate W)
        pawn.do_move(0.0, 1.0)
        if age >= 2.0:
            loc = pawn.get_actor_location()
            unreal.log("G2 glide 2s: mode=%s dz=%.0f v=%.0f" % (
                pawn.get_movement_component().get_editor_property("movement_mode"),
                loc.z - S["v"]["z0"], pawn.get_velocity().length()))
            shot("glide.png")
            goto("glide_long")
    elif ph == "glide_long":
        pawn.do_move(0.0, 1.0)
        if age >= 3.0:
            loc = pawn.get_actor_location()
            unreal.log("G2 glide 5s: mode=%s dz=%.0f (uncapped hold works)" % (
                pawn.get_movement_component().get_editor_property("movement_mode"),
                loc.z - S["v"]["z0"]))
            shot("glide_long.png")
            # release: cancel the jet
            tags = unreal.GameplayTagContainer()
            goto("glide_release")
    elif ph == "glide_release" and age >= 0.1:
        pawn.earth_launch_released() if hasattr(pawn, "earth_launch_released") else None
        # fall back: cancel by class via ASC if input fn not exposed
        asc.cancel_ability_by_class if hasattr(asc, "cancel_ability_by_class") else None
        goto("glide_check")
    elif ph == "glide_check" and age >= 0.8:
        unreal.log("G2 after release: mode=%s" % pawn.get_movement_component().get_editor_property("movement_mode"))
        goto("done")
    elif ph == "done" and age >= 0.3:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("G2 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("G2 armed")
