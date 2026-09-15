"""Fire pass smoke test: breath-of-fire stream, limb spawn points, jet exhaust.

Run via: python3 Tools/ue_run.py Tools/fire_pass_test.py
Logs prefixed FP. Uses game-time phases + first-tick detection (backgrounded
PIE runs slow; wall-clock waits would misfire).
"""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
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

    if ph == "boot" and age >= 0.5:
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        goto("setup")
    elif ph == "setup" and age >= 0.8:
        d = nearest_dummy(world, pawn)
        pawn.set_actor_location(d.get_actor_location() + unreal.Vector(-520, 0, 0), False, True)
        aim(-4, 0)
        goto("jab_cast")
    elif ph == "jab_cast" and age >= 0.4:
        unreal.log("FP jab: %s chi=%.0f" % (asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab), pawn.get_chi()))
        goto("jab_check")
    elif ph == "jab_check" and age >= 0.2:
        # Bolt should be born at fist height, not the old fixed chest offset
        bolts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireBolt)
        for b in bolts:
            dz = b.get_actor_location().z - pawn.get_actor_location().z
            unreal.log("FP jab bolt dz=%.0f (fist ~ -20..+40)" % dz)
        shot("jab.png")
        goto("stream_start")
    elif ph == "stream_start" and age >= 0.8:
        d = nearest_dummy(world, pawn)
        S["v"]["chi"] = pawn.get_chi()
        S["v"]["dhp"] = d.get_health()
        ok = asc.try_activate_ability_by_class(unreal.ATLAAbility_FireStream)
        unreal.log("FP stream activate: %s chi=%.0f" % (ok, pawn.get_chi()))
        goto("stream_check")
    elif ph == "stream_check" and age >= 1.2:
        d = nearest_dummy(world, pawn)
        unreal.log("FP stream tongues=%d chi %.0f->%.0f (expect ~-15+) dummy hp %.0f->%.0f" % (
            cnt(unreal.ATLAFireStreamBolt), S["v"]["chi"], pawn.get_chi(), S["v"]["dhp"], d.get_health()))
        unreal.log("FP stream walkspeed=%.0f (expect ~341 while breathing)" %
                   pawn.get_movement_component().get_editor_property("max_walk_speed"))
        shot("stream.png")
        goto("blast_cast")
    elif ph == "blast_cast" and age >= 0.4:
        unreal.log("FP blast: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireBlast))
        goto("blast_check")
    elif ph == "blast_check" and age >= 0.45:
        for b in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAFireBlastBolt):
            dz = b.get_actor_location().z - pawn.get_actor_location().z
            unreal.log("FP blast bolt dz=%.0f (kicking foot: below chest, ~ -80..+20)" % dz)
        shot("blast.png")
        goto("jet_cast")
    elif ph == "jet_cast" and age >= 0.8:
        S["v"]["nc"] = len(pawn.get_components_by_class(unreal.NiagaraComponent))
        aim(10, 0)
        unreal.log("FP jet: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet))
        goto("jet_check")
    elif ph == "jet_check" and age >= 0.3:
        nc = len(pawn.get_components_by_class(unreal.NiagaraComponent))
        unreal.log("FP jet niagara comps %d->%d (expect +2 foot exhausts)" % (S["v"]["nc"], nc))
        shot("jet.png")
        goto("done")
    elif ph == "done" and age >= 0.6:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("FP COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("FP armed")
