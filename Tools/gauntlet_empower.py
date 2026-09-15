import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "GauntletShots")
S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0, "v": {}}


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

    def aim(pitch, yaw):
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=float(pitch), yaw=float(yaw)))

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if ph == "boot" and age >= 0.5:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("setup")
    elif ph == "setup" and age >= 1.0:
        # Baseline jab BEFORE empowerment
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-420, 0, 0), False, True)
        aim(-8, 0)
        S["v"]["hp0"] = d.get_health()
        unreal.log("R5 baseline jab: %s hp=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab), d.get_health()))
        goto("baseline_check")
    elif ph == "baseline_check" and age >= 0.9:
        d = nearest_dummy(world, pawn)
        unreal.log("R5 baseline dmg: %.0f -> %.0f (expect -18)" % (S["v"]["hp0"], d.get_health()))
        S["v"]["hp1"] = d.get_health()
        unreal.log("R5 nova (empowers): %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireNova))
        goto("post_nova")
    elif ph == "post_nova" and age >= 1.4:
        d = nearest_dummy(world, pawn)
        unreal.log("R5 nova dmg: %.0f -> %.0f (expect -45 = 30 nova x1.5 empowered)" % (S["v"]["hp1"], d.get_health()))
        # scale check: fire a jab into open sky and measure the bolt
        aim(20, 180)
        unreal.log("R5 sky jab: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab))
        goto("scale_check")
    elif ph == "scale_check" and age >= 0.55:
        bolts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireBolt)
        if bolts:
            unreal.log("R5 empowered bolt scale: %.2f (expect 1.45, normal 1.0)" % bolts[0].get_actor_scale3d().x)
        else:
            unreal.log("R5 no bolt sampled (timing)")
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-420, 0, 0), False, True)
        aim(-8, 0)
        S["v"]["hp2"] = d.get_health()
        goto("emp_jab")
    elif ph == "emp_jab" and age >= 0.5:
        unreal.log("R5 empowered jab: %s hp=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab), nearest_dummy(world, pawn).get_health()))
        unreal.AutomationLibrary.take_high_res_screenshot(1400, 800, os.path.join(SHOTS, "empowered.png"))
        goto("emp_check")
    elif ph == "emp_check" and age >= 0.9:
        d = nearest_dummy(world, pawn)
        unreal.log("R5 empowered dmg: %.0f -> %.0f (expect -27 = 18 x1.5)" % (S["v"]["hp2"], d.get_health()))
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("R5 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("R5 armed")
