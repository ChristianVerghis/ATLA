import unreal
NAMES = ["rasteira_1","rasteira_2","macaco_side","au","au_to_role","esquiva_1","esquiva_2","esquiva_3","esquiva_4","esquiva_5","negativa","Standing_React_Death_Forward","Standing_Idle_To_Crouch","queshada_1","armada_to_esquiva","martelo_do_chau","martelo_do_chau_sem_mao"]
def bp(seq, bone, t):
    opts = unreal.AnimPoseEvaluationOptions()
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
for n in NAMES:
    seq = None
    for p in ("/Game/Anims/Mixamo/MXQ_MX_%s" % n, "/Game/Anims/Mixamo/MXQ_MX_%s_Anim_mixamo_com" % n):
        seq = unreal.load_asset(p)
        if seq: break
    if not seq: continue
    L = seq.get_editor_property("sequence_length")
    # find the moment both hands are lowest with pelvis low = on all fours
    best = None
    t = 0.0
    while t <= L:
        hl = bp(seq,"hand_l",t); hr = bp(seq,"hand_r",t); pv = bp(seq,"pelvis",t)
        hands = max(hl.z, hr.z)
        if best is None or hands < best[1]:
            best = (t, hands, round(pv.z))
        t += 0.1
    unreal.log("AF %s len=%.2f lowest-hands @%.2f handsZ=%d pelvisZ=%d" % (n, L, best[0], best[1], best[2]))
unreal.log("AF COMPLETE")
