"""Is the jab's dynamic montage actually playing? Sample the anim instance."""
import unreal

S = {"h": None, "t0": None, "cast": False}


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
        pawn.set_element_loadout(unreal.ATLAElement.FIRE)
        return
    if not S["cast"] and now - S["t0"] >= 1.0:
        asc = pawn.get_editor_property("ability_system_component")
        unreal.log("JA cast=%s vel=%.0f" % (
            asc.try_activate_ability_by_class(unreal.ATLAAbility_FireJab),
            pawn.get_velocity().length()))
        S["cast"] = True
        S["t0"] = now
        return
    if S["cast"]:
        t = now - S["t0"]
        anim = pawn.get_editor_property("mesh").get_anim_instance()
        mon = anim.get_current_active_montage() if anim else None
        if mon:
            pos = anim.montage_get_position(mon)
            unreal.log("JA t=%.2f montage=%s pos=%.2f playing=%s" % (
                t, mon.get_name(), pos, anim.montage_is_playing(mon)))
        else:
            unreal.log("JA t=%.2f NO ACTIVE MONTAGE" % t)
        if t >= 0.7:
            unreal.unregister_slate_post_tick_callback(S["h"])
            S["h"] = None
            unreal.log("JA COMPLETE")


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("JA armed")
