"""Contact sheet: freeze the character at candidate (clip, time) poses and
screenshot each — windows get picked by eye + metric, never blind again."""
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)

POSES = [
    ("thrust_14420_81", "MNY_144_20", 8.1),
    ("thrust_14420_77", "MNY_144_20", 7.7),
    ("thrust_14421_91", "MNY_144_21", 9.1),
    ("raise_14401_55", "MNY_144_01", 5.5),
    ("raise_14401_182", "MNY_144_01", 18.2),
    ("raise_14402_121", "MNY_144_02", 12.1),
    ("raise_14402_160", "MNY_144_02", 16.0),
    ("cur_jab_13506_151", "MNY_135_06", 15.1),
]

S = {"h": None, "t0": None, "i": 0, "pt": 0.0, "phase": "boot"}


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
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-12.0, yaw=140.0))
        return
    age = now - S["pt"]
    mesh = pawn.get_editor_property("mesh")

    if S["i"] >= len(POSES):
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("PS COMPLETE")
        return

    name, clip, t = POSES[S["i"]]
    if S["phase"] == "boot" and age >= 0.8:
        anim = unreal.load_asset("/Game/Anims/CMU_Manny/%s.%s" % (clip, clip))
        mesh.play_animation(anim, False)
        mesh.set_position(t, False)
        mesh.set_play_rate(0.0)
        S["phase"] = "shot"
        S["pt"] = now
    elif S["phase"] == "shot" and age >= 0.5:
        unreal.AutomationLibrary.take_high_res_screenshot(900, 620, os.path.join(SHOTS, "ps_%s.png" % name))
        unreal.log("PS shot %s" % name)
        S["i"] += 1
        S["phase"] = "boot"
        S["pt"] = now


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("PS armed")
