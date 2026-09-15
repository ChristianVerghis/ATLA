"""Final verify: jab arms actually move (hand span during cast), lash kick
frame, glide stance + plumes. Screenshots + numeric hand-span check."""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)
S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0, "span": []}


def shot(name):
    unreal.AutomationLibrary.take_high_res_screenshot(1200, 700, os.path.join(SHOTS, name))


def hand_span(pawn):
    m = pawn.get_editor_property("mesh")
    return (m.get_socket_location("hand_r") - m.get_socket_location("hand_l")).length()


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
        pawn.set_actor_location(unreal.Vector(-120, 6200, 92), False, True)
        aim(-6, 0)
        goto("jab_cast")
    elif ph == "jab_cast" and age >= 0.5:
        S["span"] = [hand_span(pawn)]
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab)
        goto("jab_watch")
    elif ph == "jab_watch":
        S["span"].append(hand_span(pawn))
        if abs(age - 0.2) < 0.03:
            shot("v4_jab.png")
        if age >= 0.5:
            unreal.log("V4 jab hand span %.0f -> max %.0f cm (moves = anim plays)" % (S["span"][0], max(S["span"])))
            goto("lash_cast")
    elif ph == "lash_cast" and age >= 0.6:
        asc.try_activate_ability_by_class(unreal.ATLAAbility_FireLash)
        goto("lash_watch")
    elif ph == "lash_watch" and age >= 0.3:
        shot("v4_lash_kick.png")
        goto("glide_start")
    elif ph == "glide_start" and age >= 1.0:
        aim(-2, 0)
        unreal.log("V4 jet: %s" % asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJet))
        goto("glide_watch")
    elif ph == "glide_watch":
        pawn.do_move(0.0, 1.0)
        if age >= 1.0:
            m = pawn.get_editor_property("mesh")
            anim = m.get_anim_instance()
            mon = anim.get_current_active_montage()
            unreal.log("V4 glide: montage=%s speed=%.0f" % (mon.get_name() if mon else "NONE", pawn.get_velocity().length()))
            shot("v4_glide.png")
            goto("done")
    elif ph == "done" and age >= 0.3:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("V4 COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("V4 armed")
