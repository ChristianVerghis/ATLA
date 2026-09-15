"""At a known montage position, compare RUNTIME local bone transforms against
the clip's OFFLINE values. Pinpoints exactly which bones the slot path alters."""
import unreal
CLIP = "/Game/Anims/Mixamo/MXQ_MX_Cross_Punch_Anim_mixamo_com.MXQ_MX_Cross_Punch_Anim_mixamo_com"
AT = 1.0
BONES = ["pelvis", "thigh_r", "calf_r", "foot_r", "spine_01", "upperarm_r"]
S = {"h": None, "t0": None, "phase": "setup"}
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
    mesh = pawn.get_editor_property("mesh")
    if S["phase"] == "setup" and age >= 1.5:
        ai = mesh.get_anim_instance()
        # rate 0 so the pose holds exactly at AT
        ai.play_slot_animation_as_dynamic_montage(unreal.load_asset(CLIP), "DefaultSlot", 0.0, 0.0, 0.0, 1, -1.0, AT)
        S["phase"] = "measure"; S["t0"] = now
    elif S["phase"] == "measure" and age >= 0.4:
        OPTS = unreal.AnimPoseEvaluationOptions()
        seq = unreal.load_asset(CLIP)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, AT, OPTS)
        for b in BONES:
            off = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.LOCAL)
            rt_world = mesh.get_socket_transform(b, unreal.RelativeTransformSpace.RTS_COMPONENT)
            off_comp = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.COMPONENT)
            d = (rt_world.translation - off_comp.translation).length()
            unreal.log("BD %-11s componentDelta=%.1f cm  runtime=(%.0f,%.0f,%.0f) offline=(%.0f,%.0f,%.0f)" % (
                b, d, rt_world.translation.x, rt_world.translation.y, rt_world.translation.z,
                off_comp.translation.x, off_comp.translation.y, off_comp.translation.z))
        unreal.unregister_slate_post_tick_callback(S["h"]); S["h"] = None
        unreal.log("BD COMPLETE")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not les.is_in_play_in_editor(): les.editor_request_begin_play()
S["h"] = unreal.register_slate_post_tick_callback(tick)
unreal.log("BD armed")
