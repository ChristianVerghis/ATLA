"""Reproduce the player's view: run forward, cast, screenshot mid-cast.
Water (reported broken) vs Earth (reported fine)."""
import os
import unreal
SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FirePassShots")
os.makedirs(SHOTS, exist_ok=True)
TESTS = [("water", unreal.ATLAElement.WATER, "ATLAAbility_WaterWhip"),
         ("earth", unreal.ATLAElement.EARTH, "ATLAAbility_RockJab"),
         ("fire",  unreal.ATLAElement.FIRE,  "ATLAAbility_FireJab")]
S = {"h": None, "t0": None, "i": 0, "phase": "switch", "n": 0}
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
        S["t0"] = now; return
    age = now - S["t0"]
    if S["i"] >= len(TESTS):
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("RCS COMPLETE"); return
    name, elem, cls = TESTS[S["i"]]
    if S["phase"] == "switch" and age >= 0.5:
        pawn.set_element_loadout(elem)
        S["phase"] = "place"; S["t0"] = now
    elif S["phase"] == "place" and age >= 1.2:
        pawn.set_actor_location(unreal.Vector(-400, 6200, 92), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(
            unreal.Rotator(roll=0.0, pitch=-12.0, yaw=0.0))
        S["phase"] = "run"; S["t0"] = now
    elif S["phase"] == "run":
        pawn.do_move(0.0, 1.0)
        if age >= 0.8:
            asc = pawn.get_editor_property("ability_system_component")
            asc.try_activate_ability_by_class(getattr(unreal, cls))
            S["n"] = 0
            S["phase"] = "shoot"; S["t0"] = now
    elif S["phase"] == "shoot":
        pawn.do_move(0.0, 1.0)
        want = [0.15, 0.35, 0.55]
        if S["n"] < len(want) and age >= want[S["n"]]:
            unreal.AutomationLibrary.take_high_res_screenshot(
                900, 640, os.path.join(SHOTS, "rc_%s_%d.png" % (name, S["n"])))
            S["n"] += 1
        if age >= 1.0:
            unreal.log("RCS %s shots done" % name)
            S["i"] += 1; S["phase"] = "switch"; S["t0"] = now
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("RCS armed")
