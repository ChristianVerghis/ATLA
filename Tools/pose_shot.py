"""Freeze one (clip, time) on the player and screenshot it.
One pose per ue_run call — the stateful multi-pose harness stalls.
Env-free: edit CLIP/TIME/NAME below via string replace from the shell caller.
"""
import os
import sys
import unreal

CLIP = sys.argv[1] if len(sys.argv) > 1 else "/Game/Anims/Mixamo/MXQ_MX_Cross_Punch_Anim_mixamo_com"
TIME = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0
NAME = sys.argv[3] if len(sys.argv) > 3 else "pose"

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_character(world, 0)
mesh = pawn.get_editor_property("mesh")
anim = unreal.load_asset(CLIP + "." + CLIP.rsplit("/", 1)[1])
if not anim:
    unreal.log("PSH MISSING %s" % CLIP)
else:
    mesh.play_animation(anim, False)
    mesh.set_position(TIME, False)
    mesh.set_play_rate(0.0)
    unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
        unreal.Rotator(roll=0.0, pitch=-10.0, yaw=150.0))
    unreal.log("PSH posed %s @%.2f" % (CLIP, TIME))
