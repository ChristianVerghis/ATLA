import unreal
S = {"h": None, "t0": None}
def tick(dt):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        if S.get("t0") is not None and S.get("h") is not None:
            h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if S["t0"] is None:
        S["t0"] = now; return
    if now - S["t0"] < 2.0: return
    from collections import Counter
    zone = Counter(); c = Counter()
    anchors = {"water": (0,0), "earth": (6000,0), "fire": (0,6000), "air": (6000,6000)}
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        cls = a.get_class().get_name()
        c[cls] += 1
        loc = a.get_actor_location()
        for z, (x, y) in anchors.items():
            if abs(loc.x - x) < 2800 and abs(loc.y - y) < 2800:
                zone[z + ":" + cls] += 1
    unreal.log("ZC TOP %s" % c.most_common(20))
    unreal.log("ZC ZONES %s" % sorted(zone.items()))
    unreal.log("ZC COMPLETE")
    h = S["h"]; S.clear(); unreal.unregister_slate_post_tick_callback(h)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("ZC armed")
