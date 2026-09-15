"""Purpose-built move scans:
- THRUST: hand extends rapidly FORWARD (punch/lunge), not any-direction swing.
- RAISE: both hands rise together (earth wall lift).
Facing is pinned (+X forward in clip space after the pelvis pin).
"""
import unreal

OPTS = unreal.AnimPoseEvaluationOptions()
CLIPS = ["MNY_135_01", "MNY_135_02", "MNY_135_03", "MNY_135_04", "MNY_135_05",
         "MNY_135_06", "MNY_135_07", "MNY_135_09", "MNY_135_10", "MNY_135_11",
         "MNY_144_01", "MNY_144_02", "MNY_144_05", "MNY_144_09", "MNY_144_13",
         "MNY_144_20", "MNY_144_21"]


def pose_at(seq, t):
    return unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)


def bone(pose, b):
    return unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation


for clip in CLIPS:
    seq = unreal.load_asset("/Game/Anims/CMU_Manny/%s.%s" % (clip, clip))
    if not seq:
        continue
    L = min(seq.get_editor_property("sequence_length"), 45.0)
    # sample forward extension (x) and height (z) of both hands
    ts, rx, lx, rz, lz = [], [], [], [], []
    t = 0.1
    while t < L:
        p = pose_at(seq, t)
        pel = bone(p, "pelvis")
        hr = bone(p, "hand_r")
        hl = bone(p, "hand_l")
        ts.append(t)
        # clips face -X after the pelvis pin: forward extension = -(dx)
        rx.append(-(hr.x - pel.x))
        lx.append(-(hl.x - pel.x))
        rz.append(hr.z)
        lz.append(hl.z)
        t += 0.1
    thrusts, raises = [], []
    for i in range(3, len(ts) - 1):
        # THRUST: forward extension grows >35cm over 0.3s ending extended >50cm
        for arr in (rx, lx):
            gain = arr[i] - arr[i - 3]
            if gain > 35 and arr[i] > 50:
                thrusts.append((ts[i], arr[i], gain))
        # RAISE: both hands climb >30cm over 0.3s ending above chest (z>120)
        gr = rz[i] - rz[i - 3]
        gl = lz[i] - lz[i - 3]
        if gr > 30 and gl > 30 and rz[i] > 120 and lz[i] > 120:
            raises.append((ts[i], min(rz[i], lz[i])))
    # dedupe within 0.5s
    def dedupe(evs):
        out = []
        for e in sorted(evs, key=lambda x: -x[1] if len(x) < 3 else -x[2]):
            if all(abs(e[0] - o[0]) > 0.5 for o in out):
                out.append(e)
        return sorted(out)[:5]
    th = dedupe(thrusts)
    ra = dedupe(raises)
    if th or ra:
        unreal.log("MS %s | thrust: %s | raise: %s" % (
            clip,
            ", ".join("%.1fs(ext%.0f)" % (e[0], e[1]) for e in th) or "-",
            ", ".join("%.1fs(z%.0f)" % (e[0], e[1]) for e in ra) or "-"))
unreal.log("MS COMPLETE")
