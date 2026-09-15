"""Offline scan: per-clip limb-speed profiles from anim data.
Finds punch peaks (hand speed) and kick peaks (foot speed) so ability
windows can target actual strikes instead of guard bounce.
"""
import unreal

CLIPS = [
    "MNY_144_01", "MNY_144_02", "MNY_144_05", "MNY_144_06", "MNY_144_07",
    "MNY_144_08", "MNY_144_09", "MNY_144_10", "MNY_144_11", "MNY_144_12",
    "MNY_144_13", "MNY_144_14", "MNY_144_15", "MNY_144_16", "MNY_144_17",
    "MNY_144_18", "MNY_144_20", "MNY_144_21",
    "MNY_135_01", "MNY_135_02", "MNY_135_03", "MNY_135_04", "MNY_135_05",
    "MNY_135_06", "MNY_135_07", "MNY_135_09", "MNY_135_10", "MNY_135_11",
]
BONES = ["hand_r", "hand_l", "foot_r", "foot_l"]
STEP = 0.1


OPTS = unreal.AnimPoseEvaluationOptions()


def bone_pos(seq, bone, t):
    try:
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        tf = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
        return tf.translation
    except Exception:
        return None


for clip in CLIPS:
    seq = unreal.load_asset("/Game/Anims/CMU_Manny/%s.%s" % (clip, clip))
    if not seq:
        unreal.log("SCAN %s: MISSING" % clip)
        continue
    length = seq.get_editor_property("sequence_length")
    # sample positions RELATIVE TO PELVIS (root-motion teleports in the noisy
    # mocap otherwise swamp every limb with identical spikes)
    times, samples = [], {b: [] for b in BONES}
    t = 0.0
    ok = True
    while t < length:
        pelvis = bone_pos(seq, "pelvis", t)
        if pelvis is None:
            ok = False
            break
        for b in BONES:
            p = bone_pos(seq, b, t)
            if p is None:
                ok = False
                break
            samples[b].append(p - pelvis)
        if not ok:
            break
        times.append(t)
        t += STEP
    if not ok or len(times) < 3:
        unreal.log("SCAN %s: pose API failed" % clip)
        continue
    # speeds + peaks (ignore >12000 glitch spikes from noisy mocap)
    report = []
    for b in BONES:
        pts = samples[b]
        speeds = [(pts[i] - pts[i - 1]).length() / STEP for i in range(1, len(pts))]
        peaks = []
        for i in range(1, len(speeds) - 1):
            if 800 < speeds[i] < 12000 and speeds[i] >= speeds[i - 1] and speeds[i] >= speeds[i + 1]:
                peaks.append((times[i + 1], speeds[i]))
        peaks.sort(key=lambda x: -x[1])
        if peaks:
            report.append("%s: %s" % (b[:6], ", ".join("%.1f@%.0f" % (pt, ps) for pt, ps in peaks[:4])))
    unreal.log("SCAN %s len=%.1fs | %s" % (clip, length, " | ".join(report) if report else "quiet"))
unreal.log("SCAN COMPLETE")
