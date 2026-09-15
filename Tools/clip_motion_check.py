"""Definitive clip test: play MNY_144_20 raw on the mesh, screenshot 3 poses,
and numerically compare hand socket positions from the live mesh."""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)
S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0, "poses": []}


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
    mesh = pawn.get_editor_property("mesh")

    def goto(p):
        S["phase"] = p
        S["pt"] = now

    if ph == "boot" and age >= 0.5:
        anim = unreal.load_asset("/Game/Anims/CMU_Manny/MNY_144_20.MNY_144_20")
        mesh.play_animation(anim, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-8.0, yaw=150.0))
        goto("s1")
    elif ph in ("s1", "s2", "s3", "s4") and age >= 1.2:
        h = mesh.get_socket_location("hand_r")
        p = pawn.get_actor_location()
        S["poses"].append(h - p)
        unreal.AutomationLibrary.take_high_res_screenshot(1000, 600, os.path.join(SHOTS, "clip_%s.png" % ph))
        unreal.log("CM %s hand_r rel=(%.0f,%.0f,%.0f)" % (ph, h.x - p.x, h.y - p.y, h.z - p.z))
        goto({"s1": "s2", "s2": "s3", "s3": "s4", "s4": "done"}[ph])
    elif ph == "done" and age >= 0.4:
        moved = max((a - b).length() for a in S["poses"] for b in S["poses"])
        unreal.log("CM max hand travel between samples: %.0f cm -> %s" % (
            moved, "CLIP ANIMATES" if moved > 25 else "CLIP IS STATIC"))
        mesh.set_anim_instance_class(unreal.load_asset("/Game/Characters/Mannequins/Animations/ABP_Unarmed.ABP_Unarmed_C") if False else mesh.get_anim_instance().get_class())
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("CM COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("CM armed")
