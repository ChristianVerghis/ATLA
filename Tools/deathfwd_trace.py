import unreal
seq = unreal.load_asset("/Game/Anims/Mixamo/MXQ_MX_Standing_React_Death_Forward")
def bp(bone, t):
    opts = unreal.AnimPoseEvaluationOptions()
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
out = []
t = 0.0
while t <= seq.get_editor_property("sequence_length"):
    hl = bp("hand_l", t); hr = bp("hand_r", t); pv = bp("pelvis", t); hd = bp("head", t)
    out.append("%.1f: hands=%d,%d pelvis=%d head=%d" % (t, hl.z, hr.z, pv.z, hd.z))
    t += 0.2
unreal.log("DF " + " | ".join(out))
