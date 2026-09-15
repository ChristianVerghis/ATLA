"""Scripted playthrough for the portfolio gameplay reel (~50 s), captured frame-by-frame.
Runs PIE at a fixed 20 fps game step (t.OverrideFPS) and issues one `Shot` per frame (HUD included;
frames land in Saved/Screenshots/MacEditor as ScreenShot#####.png), so the capture is smooth even when the editor renders slowly
(occluded window). Assemble with Tools/reel_assemble.sh.
Water -> earth -> fire -> air -> coliseum duel. Drive with the shell:
start a screen recording of the PIE window, then run this via ue_run.py.
Self-disarms when PIE ends (never leaves a slate callback armed).
"""
import os
import unreal

REEL_FPS = 20
REEL_HOLD_FPS = 1000000  # ~zero delta while a frame is being written
REEL_SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Screenshots", "MacEditor")

E = unreal.ATLAElement
A = unreal
REEL_S = {"h": None, "t0": None, "done": set(), "move": None, "track": None, "aim": None, "auto": None, "frame": 0, "cap": "advance"}
LOG = "REEL"


def world_():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def pawn_(world):
    return unreal.GameplayStatics.get_player_character(world, 0)


def pc_(world):
    return unreal.GameplayStatics.get_player_controller(world, 0)


def asc_(pawn):
    return pawn.get_editor_property("ability_system_component")


def others(world, pawn):
    return [c for c in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLACharacter) if c != pawn]


def nearest(world, pawn):
    best, bd = None, 1e12
    for c in others(world, pawn):
        d = (c.get_actor_location() - pawn.get_actor_location()).length()
        if d < bd:
            best, bd = c, d
    return best


def aim_at(world, pawn, target_loc, pitch_bias=0.0):
    src = pawn.get_actor_location() + unreal.Vector(0, 0, 60)
    rot = unreal.MathLibrary.find_look_at_rotation(src, target_loc)
    pc_(world).set_control_rotation(unreal.Rotator(roll=0.0, pitch=float(rot.pitch + pitch_bias), yaw=float(rot.yaw)))


def aim(world, pitch, yaw):
    pc_(world).set_control_rotation(unreal.Rotator(roll=0.0, pitch=float(pitch), yaw=float(yaw)))


def cast(pawn, cls):
    ok = asc_(pawn).try_activate_ability_by_class(cls)
    unreal.log("%s cast %s -> %s" % (LOG, cls.__name__, ok))


def loadout(world, pawn, elem):
    pawn.set_element_loadout(elem)
    unreal.log("%s loadout %s at %s" % (LOG, elem, pawn.get_actor_location()))


def place(pawn, loc):
    pawn.set_actor_location(loc, False, True)


def nearest_pool(world, loc):
    best, bd = None, 1e12
    for p in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ATLAWaterSource):
        d = (p.get_actor_location() - loc).length()
        if d < bd:
            best, bd = p, d
    return best


# ---- timeline -------------------------------------------------------------
def water_start(w, p):
    loadout(w, p, E.WATER)
    anchor = unreal.Vector(0, 0, 320)
    pool = nearest_pool(w, anchor)
    spot = anchor
    if pool:
        # inside DrawRange (600) so the water meter pins at max (octopus needs 50),
        # but as close to the anchor as possible: the anchor side is open ground
        pl = pool.get_actor_location()
        rng = float(pool.get_editor_property("DrawRange")) if hasattr(pool, "get_editor_property") else 600.0
        try:
            rng = float(pool.get_editor_property("DrawRange"))
        except Exception:  # noqa
            rng = 600.0
        towards = anchor - pl
        towards.z = 0
        dist = towards.length()
        towards = towards.normal() if dist > 1 else unreal.Vector(1, 0, 0)
        d = min(rng - 70.0, dist)
        spot = unreal.Vector(pl.x + towards.x * d, pl.y + towards.y * d, anchor.z)
        unreal.log("%s pool at %s range %.0f -> standing %s" % (LOG, pl, rng, spot))
    place(p, spot)
    aim(w, -6, 45)
    REEL_S["move"] = (1.2, 0.0, 1.0)


def track_dummy(w, p):
    d = nearest(w, p)
    REEL_S["track"] = d
    REEL_S["move"] = None


def fire_scenery(w, p):
    REEL_S["track"] = None
    REEL_S["aim"] = (28, 60)


def air_scenery(w, p):
    REEL_S["track"] = None
    REEL_S["aim"] = (22, 120)


def to_coliseum(w, p):
    REEL_S["track"] = None
    arenas = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.ATLAColiseum)
    if arenas:
        top = arenas[0].get_platform_top()
        place(p, top + unreal.Vector(-500, 0, 100))
        unreal.log("%s coliseum top %s" % (LOG, top))
    aim(w, -4, 0)


def summon(w, p):
    pc_(w).toggle_sparring_partner()
    REEL_S["auto"] = 0.0
    unreal.log("%s summoned partner" % LOG)


def track_partner(w, p):
    REEL_S["track"] = nearest(w, p)


def settle(w, p):
    REEL_S["auto"] = None
    unreal.SystemLibrary.execute_console_command(w, "DebugSettleDuel")
    unreal.log("%s duel settled" % LOG)


def finish(w, p):
    unreal.SystemLibrary.execute_console_command(w, "t.OverrideFPS 0")
    unreal.log("%s END frames=%d" % (LOG, REEL_S["frame"]))
    h = REEL_S["h"]
    REEL_S.clear()
    REEL_S["h"] = None
    unreal.unregister_slate_post_tick_callback(h)


def step(cls):
    return lambda w, p: cast(p, cls)


def mover(secs, right, fwd):
    def f(w, p):
        REEL_S["move"] = (secs, right, fwd)
    return f


def aimer(pitch, yaw):
    def f(w, p):
        REEL_S["track"] = None
        REEL_S["aim"] = (pitch, yaw)
    return f


TIMELINE = [
    (0.0, water_start),
    (2.0, track_dummy),
    (2.3, step(A.ATLAAbility_WaterWhip)),
    (2.9, step(A.ATLAAbility_WaterWhip)),
    (3.5, step(A.ATLAAbility_WaterWhip)),
    (4.0, step(A.ATLAAbility_IceSpears)),
    (5.1, step(A.ATLAAbility_IceWall)),
    (6.3, step(A.ATLAAbility_Octopus)),
    (6.5, mover(1.4, 1.0, 0.3)),
    # earth
    (8.4, lambda w, p: (loadout(w, p, E.EARTH), aimer(-6, 0)(w, p))),
    (9.0, track_dummy),
    (9.2, step(A.ATLAAbility_RockJab)),
    (9.7, step(A.ATLAAbility_RockJab)),
    (10.4, step(A.ATLAAbility_EarthSpikes)),
    (11.6, step(A.ATLAAbility_EarthWall)),
    (12.7, step(A.ATLAAbility_EarthWall)),
    (13.7, step(A.ATLAAbility_EarthArmor)),
    (15.3, lambda w, p: (aimer(18, 0)(w, p), cast(p, A.ATLAAbility_EarthLaunch))),
    # fire
    (17.0, lambda w, p: (loadout(w, p, E.FIRE), fire_scenery(w, p))),
    (18.6, track_dummy),
    (18.8, step(A.ATLAAbility_FireJab)),
    (19.2, step(A.ATLAAbility_FireJab)),
    (19.6, step(A.ATLAAbility_FireJab)),
    (20.3, step(A.ATLAAbility_FireLash)),
    (21.3, step(A.ATLAAbility_FireWall)),
    (22.5, lambda w, p: (aimer(12, 0)(w, p), mover(1.0, 0.0, 1.0)(w, p), cast(p, A.ATLAAbility_FireJet))),
    (23.7, track_dummy),
    (23.9, step(A.ATLAAbility_FireNova)),
    (25.3, step(A.ATLAAbility_Lightning)),
    # air
    (27.0, lambda w, p: (loadout(w, p, E.AIR), air_scenery(w, p))),
    (28.5, track_dummy),
    (28.7, step(A.ATLAAbility_AirBlast)),
    (29.2, step(A.ATLAAbility_AirBlast)),
    (29.8, step(A.ATLAAbility_AirSwipe)),
    (30.6, step(A.ATLAAbility_WindDome)),
    (31.8, step(A.ATLAAbility_AirCyclone)),
    (33.2, lambda w, p: (aimer(-8, 90)(w, p), cast(p, A.ATLAAbility_AirScooter), mover(2.4, 0.0, 1.0)(w, p))),
    (35.8, lambda w, p: (aimer(20, 90)(w, p), cast(p, A.ATLAAbility_Updraft))),
    # coliseum duel
    (37.3, lambda w, p: loadout(w, p, E.FIRE)),
    (37.6, to_coliseum),
    (38.0, summon),
    (38.8, track_partner),
    (41.0, step(A.ATLAAbility_FireLash)),
    (42.2, step(A.ATLAAbility_Dodge)),
    (43.0, step(A.ATLAAbility_FireWall)),
    (44.4, step(A.ATLAAbility_FireBlast)),
    (46.0, settle),
    (49.5, finish),
]


def tick(dt):
    world = world_()
    if not world:
        if REEL_S.get("t0") is not None and REEL_S.get("h") is not None:
            h = REEL_S["h"]
            REEL_S.clear()
            unreal.unregister_slate_post_tick_callback(h)
            unreal.log("%s auto-disarmed (PIE ended)" % LOG)
        return
    if REEL_S.get("h") is None:
        return
    pawn = pawn_(world)
    if not pawn:
        return
    now = unreal.GameplayStatics.get_time_seconds(world)
    if REEL_S["t0"] is None:
        REEL_S["t0"] = now
        unreal.SystemLibrary.execute_console_command(world, "t.OverrideFPS %d" % REEL_FPS)
        unreal.log("%s START (fixed %d fps)" % (LOG, REEL_FPS))
        return
    age = now - REEL_S["t0"]
    step_dt = unreal.GameplayStatics.get_world_delta_seconds(world)
    # Paced capture: the world only advances (1/REEL_FPS) on "advance" frames; after each
    # advance we request a Shot and hold game time ~still until that PNG exists on disk.
    # Unpaced Shots queued faster than they were written and the editor ran out of memory.
    if REEL_S["cap"] == "capturing":
        f = os.path.join(REEL_SHOTS, "ScreenShot%05d.png" % (REEL_S["frame"] - 1))
        size = os.path.getsize(f) if os.path.exists(f) else 0
        if size > 0 and size == REEL_S.get("lastsize"):
            # written in full (two consecutive ticks saw the same non-zero size)
            unreal.SystemLibrary.execute_console_command(world, "t.OverrideFPS %d" % REEL_FPS)
            REEL_S["cap"] = "advance"
            REEL_S["lastsize"] = 0
        else:
            REEL_S["lastsize"] = size
        return  # nothing else moves while a frame is being written
    for i, (t, fn) in enumerate(TIMELINE):
        if i in REEL_S["done"] or age < t:
            continue
        REEL_S["done"].add(i)
        try:
            fn(world, pawn)
        except Exception as e:  # noqa
            unreal.log("%s step %d error: %s" % (LOG, i, e))
        if REEL_S.get("h") is None:
            return
    if REEL_S.get("aim") is not None:
        aim(world, *REEL_S["aim"])
        REEL_S["aim"] = None
    tr = REEL_S.get("track")
    if tr is not None:
        try:
            aim_at(world, pawn, tr.get_actor_location() + unreal.Vector(0, 0, 40))
        except Exception:  # noqa
            REEL_S["track"] = None
    mv = REEL_S.get("move")
    if mv is not None:
        secs, right, fwd = mv
        pawn.do_move(right, fwd)
        secs -= step_dt
        REEL_S["move"] = (secs, right, fwd) if secs > 0 else None
    if REEL_S.get("auto") is not None:
        REEL_S["auto"] += step_dt
        if REEL_S["auto"] >= 0.55:
            REEL_S["auto"] = 0.0
            try:
                asc_(pawn).try_activate_ability_by_class(A.ATLAAbility_FireJab)
            except Exception:  # noqa
                pass
    # capture this world frame, then hold game time until the PNG is on disk
    unreal.SystemLibrary.execute_console_command(world, "Shot")
    unreal.SystemLibrary.execute_console_command(world, "t.OverrideFPS %d" % REEL_HOLD_FPS)
    REEL_S["frame"] += 1
    REEL_S["cap"] = "capturing"
    if REEL_S["frame"] % (REEL_FPS * 5) == 0:
        unreal.log("%s frame %d age %.1f" % (LOG, REEL_S["frame"], age))


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor():
    les.editor_request_begin_play()
REEL_S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("%s armed" % LOG)
