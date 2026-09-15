import unreal
CLIPS = {
    "stomp": ["bencao", "chapa_2", "martelo_do_chau", "Leg_Sweep"],
    "punch": ["Hook", "Punch_Combo", "Fist_Fight_A", "Illegal_Elbow_Punch"],
    "pull":  ["Standing_2H_Magic_Area_Attack_01", "Standing_2H_Cast_Spell_01",
              "Standing_2H_Magic_Attack_04", "Standing_2H_Magic_Attack_05"],
}
def bone_pos(seq, bone, t):
    opts = unreal.AnimPoseEvaluationOptions()
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
for role, names in CLIPS.items():
    for n in names:
        path = "/Game/Anims/Mixamo/MXQ_MX_%s_Anim_mixamo_com" % n
        seq = unreal.load_asset(path) or unreal.load_asset("/Game/Anims/Mixamo/MXQ_MX_%s" % n)
        if not seq:
            unreal.log("RS %s %s MISSING" % (role, n)); continue
        length = seq.get_editor_property("sequence_length")
        dt = 0.05
        prev = {}
        best = []
        t = 0.0
        while t <= length:
            fl = bone_pos(seq, "foot_l", t); fr = bone_pos(seq, "foot_r", t)
            hl = bone_pos(seq, "hand_l", t); hr = bone_pos(seq, "hand_r", t)
            if prev:
                if role == "stomp":
                    # downward foot velocity while foot is off the ground
                    for f, pf in (("fl", fl), ("fr", fr)):
                        vz = (pf.z - prev[f].z) / dt
                        if pf.z > 8 or vz < -150:
                            best.append((t, f, round(pf.z), round(vz)))
                elif role == "punch":
                    for h, ph in (("hl", hl), ("hr", hr)):
                        v = (ph - prev[h]) / dt
                        sp = (v.x**2 + v.y**2) ** 0.5
                        if sp > 350: best.append((t, h, round(ph.z), round(sp)))
                else:
                    # both hands low then rising together
                    avg = (hl.z + hr.z) / 2
                    vz = (avg - (prev["hl"].z + prev["hr"].z) / 2) / dt
                    if vz > 120 or avg < 75: best.append((t, "2h", round(avg), round(vz)))
            prev = {"fl": fl, "fr": fr, "hl": hl, "hr": hr}
            t += dt
        summary = " ".join("%.2f:%s z%d v%d" % b for b in best[:14])
        unreal.log("RS %s %s len=%.2f | %s" % (role, n, length, summary))
unreal.log("RS COMPLETE")
