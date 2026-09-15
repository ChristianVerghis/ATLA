"""PIE sanity-preview a retargeted anim on the player character: play it raw
on the mesh, screenshot mid-pose, end play. Usage: edit ANIM/SHOT below."""
import os
import unreal

ANIM = "/Game/Anims/CMU_Manny/MNY_12_04_Anim"
SHOT = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "GauntletShots", "retarget_taichi.png")

S = {"h": None, "t0": None, "phase": "boot", "pt": 0.0}
os.makedirs(os.path.dirname(SHOT), exist_ok=True)


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

    if ph == "boot" and age >= 0.6:
        anim = unreal.EditorAssetLibrary.load_asset(ANIM)
        pawn.get_editor_property("mesh").play_animation(anim, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-10.0, yaw=140.0))
        unreal.log("PRV playing %s" % ANIM)
        S["phase"] = "pose"
        S["pt"] = now
    elif ph == "pose" and age >= 2.5:
        unreal.AutomationLibrary.take_high_res_screenshot(1400, 800, SHOT)
        S["phase"] = "done"
        S["pt"] = now
    elif ph == "done" and age >= 0.8:
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("PRV COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("PRV armed")
