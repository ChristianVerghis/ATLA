"""Hold-LMB autofire: re-activate every frame like the real input does, and
watch for leg breakdown (the condition earth never hits — it casts on release)."""
import os
import unreal
SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
S = {"h": None, "t0": None, "phase": "setup", "worstfoot": 999.0, "worstleg": 0.0,
     "pelmin": 999.0, "shot": False}
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
        S["phase"] = "spam"; S["t0"] = now
    elif S["phase"] == "spam":
        # exactly what holding LMB does for water/air: fire every tick
        asc.try_activate_ability_by_class(unreal.ATLAAbility_WaterWhip)
        pawn.do_move(0.0, 1.0)
        floor = pawn.get_actor_location().z - half
        pel = mesh.get_socket_location("pelvis")
        fr = mesh.get_socket_location("foot_r"); fl = mesh.get_socket_location("foot_l")
        S["pelmin"] = min(S["pelmin"], pel.z - floor)
        S["worstfoot"] = min(S["worstfoot"], min(fr.z, fl.z) - floor)
        S["worstleg"] = max(S["worstleg"], max((fr-pel).length(), (fl-pel).length()))
        if age > 0.8 and not S["shot"]:
            unreal.AutomationLibrary.take_high_res_screenshot(900, 640, os.path.join(SHOTS, "spam_water.png"))
            S["shot"] = True
        if age >= 2.0:
            unreal.log("SPM held-LMB water: pelvisZ min=%.0f lowest foot=%+.0f leglen max=%.0f -> %s" % (
                S["pelmin"], S["worstfoot"], S["worstleg"],
                "BROKEN" if (S["worstfoot"] < -8 or S["worstleg"] > 110 or S["pelmin"] < 70) else "OK"))
            unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
            unreal.log("SPM COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("SPM armed")
