"""Verify pass-2 visuals: whip without red ribbon, nova without pancake."""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)
S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0}


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
        pawn.set_element_loadout(unreal.ATLAElement.WATER)
        goto("water_setup")
    elif ph == "water_setup" and age >= 0.8:
        pawn.set_actor_location(unreal.Vector(700, -20, 92), False, True)
        aim(-4, 90)
        goto("whip")
    elif ph == "whip" and age >= 0.4:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        goto("whip_shot")
    elif ph == "whip_shot" and age >= 0.25:
        shot("v2_whip.png")
        goto("fire_switch")
    elif ph == "fire_switch" and age >= 0.6:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("nova")
    elif ph == "nova" and age >= 1.0:
        unreal.log("V2 nova: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova))
        goto("nova_shot")
    elif ph == "nova_shot" and age >= 0.55:
        shot("v2_nova.png")
        goto("done")
    elif ph == "done" and age >= 0.5:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("V2 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("V2 armed")
