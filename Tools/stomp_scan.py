import unreal
def bp(seq, bone, t):
    opts = unreal.AnimPoseEvaluationOptions()
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
NAMES = ["pontera", "troca_1", "chapaeu_de_couro", "martelo_2", "meia_lua_de_frente", "Standing_2H_Magic_Area_Attack_01", "Standing_2H_Magic_Attack_01", "Standing_2H_Magic_Attack_02"]
for n in NAMES:
    seq = None
    for p in ("/Game/Anims/Mixamo/MXQ_MX_%s" % n, "/Game/Anims/Mixamo/MXQ_MX_%s_Anim_mixamo_com" % n):
        seq = unreal.load_asset(p)
        if seq: break
    if not seq: continue
    L = seq.get_editor_property("sequence_length")
    rows = []
    t = 0.0
    prev = None
    while t <= L:
        fr = bp(seq, "foot_r", t); fl = bp(seq, "foot_l", t)
        hr = bp(seq, "hand_r", t); hl = bp(seq, "hand_l", t); pv = bp(seq, "pelvis", t)
        f = fr if True else fl
        vz = 0 if prev is None else (fr.z - prev.z) / 0.1
        # horizontal travel of the striking foot this step
        vh = 0 if prev is None else ((fr.x-prev.x)**2 + (fr.y-prev.y)**2) ** 0.5 / 0.1
        rows.append("%.1f:fz%d,vz%d,vh%d,hz%d,pz%d" % (t, fr.z, vz, vh, min(hl.z,hr.z), pv.z))
        prev = fr
        t += 0.1
    unreal.log("SS %s len=%.2f %s" % (n, L, " ".join(rows)))
unreal.log("SS COMPLETE")
