"""Spam-cast while moving and screenshot the EXACT frame the legs collapse."""
import os
import unreal
SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
S = {"h": None, "t0": None, "phase": "setup", "caught": 0, "log": []}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        # PIE ended under us: release every reference and unregister, or the
        # held objects keep the game viewport alive and the editor asserts
        # (GameViewport.IsUnique) on the next PIE teardown.
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]
            S.clear()
            unreal.unregister_slate_post_tick_callback(h)
            unreal.log("probe auto-disarmed (PIE ended)")
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn: return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now
        pawn.set_element_loadout(unreal.ATLAElement.WATER)
        return
    age = now - S["t0"]
    mesh = pawn.get_editor_property("mesh")
    cap = pawn.get_editor_property("capsule_component")
    half = cap.get_editor_property("capsule_half_height")
    asc = pawn.get_editor_property("ability_system_component")
    if S["phase"] == "setup" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-10.0, yaw=0.0))
        S["phase"] = "hunt"; S["t0"] = now
    elif S["phase"] == "hunt":
        asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        pawn.do_move(0.0, 1.0)
        floor = pawn.get_actor_location().z - half
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        legmin = min((fr-pel).length(), (fl-pel).length())
        footmin = min(fr.z, fl.z) - floor
        if (legmin < 45 or footmin > 45) and S["caught"] < 3:
            unreal.AutomationLibrary.take_high_res_screenshot(
                900, 640, os.path.join(SHOTS, "glitch_%d.png" % S["caught"]))
            anim = mesh.get_anim_instance()
            mon = anim.get_current_active_montage()
            S["log"].append("caught#%d legmin=%.0f footZ=%+.0f montage=%s" % (
                S["caught"], legmin, footmin, mon.get_name() if mon else "none"))
            S["caught"] += 1
        if age >= 4.0:
            for l in S["log"]:
                unreal.log("CGS " + l)
            unreal.log("CGS caught=%d" % S["caught"])
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("CGS COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("CGS armed")
