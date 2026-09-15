"""Does a full-body frozen montage block walking? Push input for 1s with the
montage on, sample speed; stop montage, push 1s more, sample again."""
import unreal

S = {"h": None, "t0": None, "phase": "with_montage", "pt": 0.0}


def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_character(world, 0)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    anim = pawn.get_editor_property("mesh").get_anim_instance()
    if S["t0"] is None:
        S["t0"] = now
        S["pt"] = now
        return
    age = now - S["pt"]

    pawn.do_move(0.0, 1.0)
    if S["phase"] == "with_montage" and age >= 1.0:
        mon = anim.get_current_active_montage()
        unreal.log("MB with montage=%s speed=%.0f mode=%s" % (
            mon.get_name() if mon else "NONE", pawn.get_velocity().length(),
            pawn.get_movement_component().get_editor_property("movement_mode")))
        anim.montage_stop(0.1, None)
        S["phase"] = "without"
        S["pt"] = now
    elif S["phase"] == "without" and age >= 1.0:
        unreal.log("MB without montage speed=%.0f" % pawn.get_velocity().length())
        unreal.unregister_slate_post_tick_callback(S["h"])
        S["h"] = None
        unreal.log("MB COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("MB armed")
